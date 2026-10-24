// C++ translation of nijimi/source/render/vulkan/vulkan_backend.d.
// Shader assets and blend/pipeline selection follow that backend; SDL/ImGui
// presentation and native nicxlive packet/resource access are host adaptations.
#include "backend.hpp"
#include "render/window_mode.hpp"
#include "render/settings.hpp"
#ifdef _WIN32
#include "windows_spout.hpp"
#endif
#include "vulkan_shaders.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <core/render/backend_queue.hpp>
#include <core/render/shared_deform_buffer.hpp>
#include <core/runtime_state.hpp>
#include <cstring>
#include <stdexcept>

namespace nijiexpose::vulkan {
namespace core = nicxlive::core;
using namespace nicxlive::core::nodes;
namespace {
constexpr VkImageUsageFlags colorUsage =
    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
struct Vertex {
  float x, y, u, v;
};
struct Material {
  float tint[4]{1, 1, 1, 1};
  float screen[4]{};
};
Vec3 transformClip(const Mat4 &m, const Vec3 &v) {
  float in[4] = {v.x, v.y, v.z, 1}, out[4]{};
  for (int row = 0; row < 4; ++row)
    for (int col = 0; col < 4; ++col)
      out[row] += m[row][col] * in[col];
  float inv = out[3] == 0 || std::isnan(out[3]) ? 1.f : 1.f / out[3];
  return {out[0] * inv, out[1] * inv, out[2] * inv};
}
bool advanced(BlendMode m) {
  auto n = int(m);
  return (n >= 1 && n <= 6) || (n >= 9 && n <= 13);
}
VkPipelineColorBlendAttachmentState blendState(BlendMode mode) {
  VkPipelineColorBlendAttachmentState s{};
  s.blendEnable = VK_TRUE;
  s.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  s.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  s.colorBlendOp = VK_BLEND_OP_ADD;
  s.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  s.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  s.alphaBlendOp = VK_BLEND_OP_ADD;

  auto setBlendFunc = [&](VkBlendFactor src, VkBlendFactor dst) {
    s.srcColorBlendFactor = src;
    s.dstColorBlendFactor = dst;
    s.srcAlphaBlendFactor = src;
    s.dstAlphaBlendFactor = dst;
  };
  auto setBlendFuncSeparate =
      [&](VkBlendFactor srcColor, VkBlendFactor dstColor,
          VkBlendFactor srcAlpha, VkBlendFactor dstAlpha) {
        s.srcColorBlendFactor = srcColor;
        s.dstColorBlendFactor = dstColor;
        s.srcAlphaBlendFactor = srcAlpha;
        s.dstAlphaBlendFactor = dstAlpha;
      };

  switch (mode) {
  case BlendMode::Normal:
    s.colorBlendOp = VK_BLEND_OP_ADD;
    s.alphaBlendOp = VK_BLEND_OP_ADD;
    setBlendFunc(VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    break;
  case BlendMode::Multiply:
    s.colorBlendOp = VK_BLEND_OP_ADD;
    s.alphaBlendOp = VK_BLEND_OP_ADD;
    setBlendFunc(VK_BLEND_FACTOR_DST_COLOR,
                 VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    break;
  case BlendMode::Screen:
    s.colorBlendOp = VK_BLEND_OP_ADD;
    s.alphaBlendOp = VK_BLEND_OP_ADD;
    setBlendFunc(VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR);
    break;
  case BlendMode::Overlay:
  case BlendMode::ColorBurn:
  case BlendMode::HardLight:
  case BlendMode::SoftLight:
  case BlendMode::Difference:
    // Legacy fallback path: approximate unsupported equations.
    s.colorBlendOp = VK_BLEND_OP_ADD;
    s.alphaBlendOp = VK_BLEND_OP_ADD;
    setBlendFunc(VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    break;
  case BlendMode::Darken:
    s.colorBlendOp = VK_BLEND_OP_MIN;
    s.alphaBlendOp = VK_BLEND_OP_ADD;
    setBlendFunc(VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE);
    break;
  case BlendMode::Lighten:
    s.colorBlendOp = VK_BLEND_OP_MAX;
    s.alphaBlendOp = VK_BLEND_OP_ADD;
    setBlendFunc(VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE);
    break;
  case BlendMode::ColorDodge:
    s.colorBlendOp = VK_BLEND_OP_ADD;
    s.alphaBlendOp = VK_BLEND_OP_ADD;
    setBlendFunc(VK_BLEND_FACTOR_DST_COLOR, VK_BLEND_FACTOR_ONE);
    break;
  case BlendMode::LinearDodge:
    s.colorBlendOp = VK_BLEND_OP_ADD;
    s.alphaBlendOp = VK_BLEND_OP_ADD;
    setBlendFuncSeparate(
        VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
        VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    break;
  case BlendMode::AddGlow:
    s.colorBlendOp = VK_BLEND_OP_ADD;
    s.alphaBlendOp = VK_BLEND_OP_ADD;
    setBlendFuncSeparate(VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE,
                         VK_BLEND_FACTOR_ONE,
                         VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    break;
  case BlendMode::Exclusion:
    s.colorBlendOp = VK_BLEND_OP_ADD;
    s.alphaBlendOp = VK_BLEND_OP_ADD;
    setBlendFuncSeparate(VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
                         VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
                         VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE);
    break;
  case BlendMode::Subtract:
    s.colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
    s.alphaBlendOp = VK_BLEND_OP_ADD;
    setBlendFunc(VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR, VK_BLEND_FACTOR_ONE);
    break;
  case BlendMode::Inverse:
    s.colorBlendOp = VK_BLEND_OP_ADD;
    s.alphaBlendOp = VK_BLEND_OP_ADD;
    setBlendFunc(VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
                 VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    break;
  case BlendMode::DestinationIn:
    s.colorBlendOp = VK_BLEND_OP_ADD;
    s.alphaBlendOp = VK_BLEND_OP_ADD;
    setBlendFunc(VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_SRC_ALPHA);
    break;
  case BlendMode::ClipToLower:
    s.colorBlendOp = VK_BLEND_OP_ADD;
    s.alphaBlendOp = VK_BLEND_OP_ADD;
    setBlendFunc(VK_BLEND_FACTOR_DST_ALPHA,
                 VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    break;
  case BlendMode::SliceFromLower:
    s.colorBlendOp = VK_BLEND_OP_ADD;
    s.alphaBlendOp = VK_BLEND_OP_ADD;
    setBlendFunc(VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    break;
  }
  return s;
}

} // namespace
VkBlendOp advancedBlendOpFor(BlendMode mode) {
  switch (mode) {
  case BlendMode::Multiply:
    return VK_BLEND_OP_MULTIPLY_EXT;
  case BlendMode::Screen:
    return VK_BLEND_OP_SCREEN_EXT;
  case BlendMode::Overlay:
    return VK_BLEND_OP_OVERLAY_EXT;
  case BlendMode::Darken:
    return VK_BLEND_OP_DARKEN_EXT;
  case BlendMode::Lighten:
    return VK_BLEND_OP_LIGHTEN_EXT;
  case BlendMode::ColorDodge:
    return VK_BLEND_OP_COLORDODGE_EXT;
  case BlendMode::ColorBurn:
    return VK_BLEND_OP_COLORBURN_EXT;
  case BlendMode::HardLight:
    return VK_BLEND_OP_HARDLIGHT_EXT;
  case BlendMode::SoftLight:
    return VK_BLEND_OP_SOFTLIGHT_EXT;
  case BlendMode::Difference:
    return VK_BLEND_OP_DIFFERENCE_EXT;
  case BlendMode::Exclusion:
    return VK_BLEND_OP_EXCLUSION_EXT;
  case BlendMode::Normal:
  case BlendMode::LinearDodge:
  case BlendMode::AddGlow:
  case BlendMode::Subtract:
  case BlendMode::Inverse:
  case BlendMode::DestinationIn:
  case BlendMode::ClipToLower:
  case BlendMode::SliceFromLower:
    return VK_BLEND_OP_ADD;
  }
  throw std::invalid_argument("Unknown blend mode");
}

struct Backend::Impl {
  Context &c = context();
  struct Texture {
    Image image;
    VkSampler sampler{};
    int channels = 4;
    bool stencil = false;
  };
  struct Target {
    std::array<uint32_t, 3> colors{};
    Image stencil;
    VkFramebuffer framebuffer{};
    uint32_t width{}, height{};
  };
  struct Batch {
    Target *target{};
    uint32_t first{}, count{};
    int32_t base{};
    std::array<uint32_t, 3> textures{};
    Material material;
    BlendMode blend = BlendMode::Normal;
    int mask = 0;
    int kind = 0;
    uint32_t reference = 1;
    int clearStencil = -1;
    bool clear = false;
  };
  bool spoutTarget = false;
  std::map<uint32_t, Texture> textures;
  std::map<uint32_t, std::vector<uint16_t>> indices;
  uint32_t nextTexture = 1, nextIndex = 1;
  std::unique_ptr<Target> root;
  struct Composite {
    std::weak_ptr<core::DynamicCompositeSurface> owner;
    std::unique_ptr<Target> target;
  };
  std::map<const core::DynamicCompositeSurface *, Composite> composites;
  std::vector<Target *> stack;
  Target *target{};
  std::vector<Vertex> vertices;
  std::vector<uint16_t> elements;
  std::vector<Batch> batches;
  Buffer vertexBuffer, indexBuffer;
  VkRenderPass pass{}, offscreenPass{};
  VkDescriptorSetLayout setLayout{};
  VkPipelineLayout layout{};
  VkDescriptorPool pool{};
  std::map<int, VkPipeline> pipelines;
  VkShaderModule vert{}, maskVert{}, frag{}, stage1{}, stage2{}, partMask{},
      maskFrag{}, blurFrag{};
  uint32_t white{}, black{};
  int mask = 0;
  uint32_t reference = 1;
  bool lighting = false;
  bool maskWritten = false;
  bool blurRequested = false;
  std::unique_ptr<Target> blurA, blurB;
  void destroyTarget(Target &t) {
    if (t.framebuffer)
      vkDestroyFramebuffer(c.device, t.framebuffer, nullptr);
    c.destroy(t.stencil);
    t = {};
  }
  std::unique_ptr<Target> makeTarget(const std::array<uint32_t, 3> &ids,
                                     bool rootTarget = false) {
    auto t = std::make_unique<Target>();
    t->colors = ids;
    auto &first = textures.at(ids[0]).image;
    t->width = first.width;
    t->height = first.height;
    t->stencil = c.createImage(
          t->width, t->height, VK_FORMAT_D24_UNORM_S8_UINT,
          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
          VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    VkImageView views[2] = {textures.at(ids[0]).image.view, t->stencil.view};
    VkFramebufferCreateInfo f{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    f.renderPass = rootTarget ? pass : offscreenPass;
    f.attachmentCount = 2;
    f.pAttachments = views;
    f.width = t->width;
    f.height = t->height;
    f.layers = 1;
    check(vkCreateFramebuffer(c.device, &f, nullptr, &t->framebuffer));
    return t;
  }
  void initialize() {
    VkAttachmentDescription attachments[2]{};
    for (int i = 0; i < 2; ++i) {
      auto &a = attachments[i];
      a.format =
          i == 1 ? VK_FORMAT_D24_UNORM_S8_UINT : VK_FORMAT_R8G8B8A8_UNORM;
      a.samples = VK_SAMPLE_COUNT_1_BIT;
      a.loadOp = a.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      a.storeOp = a.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
      a.initialLayout = a.finalLayout =
          i == 1 ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                 : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    VkAttachmentReference
        colors[] = {{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}},
        depth{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub{};
    sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount = 1;
    sub.pColorAttachments = colors;
    sub.pDepthStencilAttachment = &depth;
    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = dep.dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    dep.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    if(c.advancedBlendSupported) dep.dstAccessMask|=VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT;
    VkSubpassDependency dependencies[2]{dep,{}};
    auto& blendDependency=dependencies[1];
    blendDependency.srcSubpass=blendDependency.dstSubpass=0;
    blendDependency.srcStageMask=blendDependency.dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    blendDependency.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    blendDependency.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT;
    blendDependency.dependencyFlags=VK_DEPENDENCY_BY_REGION_BIT;
    VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rp.attachmentCount = 2;
    rp.pAttachments = attachments;
    rp.subpassCount = 1;
    rp.pSubpasses = &sub;
    rp.dependencyCount = c.advancedBlendSupported ? 2 : 1;
    rp.pDependencies = dependencies;
    check(vkCreateRenderPass(c.device, &rp, nullptr, &pass));
    check(vkCreateRenderPass(c.device, &rp, nullptr, &offscreenPass));
    VkDescriptorSetLayoutBinding bindings[3]{};
    for (int i = 0; i < 3; ++i) {
      bindings[i].binding = i;
      bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      bindings[i].descriptorCount = 1;
      bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    VkDescriptorSetLayoutCreateInfo d{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    d.bindingCount = 3;
    d.pBindings = bindings;
    check(vkCreateDescriptorSetLayout(c.device, &d, nullptr, &setLayout));
    VkPushConstantRange push{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Material)};
    VkPipelineLayoutCreateInfo l{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    l.setLayoutCount = 1;
    l.pSetLayouts = &setLayout;
    l.pushConstantRangeCount = 1;
    l.pPushConstantRanges = &push;
    check(vkCreatePipelineLayout(c.device, &l, nullptr, &layout));
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32768};
    VkDescriptorPoolCreateInfo poolInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.maxSets = 8192;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &size;
    check(vkCreateDescriptorPool(c.device, &poolInfo, nullptr, &pool));
    auto shader = [&](const auto &data) {
      VkShaderModuleCreateInfo s{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
      s.codeSize = sizeof(data);
      s.pCode = data;
      VkShaderModule m;
      check(vkCreateShaderModule(c.device, &s, nullptr, &m));
      return m;
    };
    vert = shader(part_vert);
    maskVert = shader(mask_vert);
    frag = shader(part_frag);
    stage1 = shader(part_stage1_frag);
    stage2 = shader(part_stage2_frag);
    partMask = shader(part_mask_frag);
    maskFrag = shader(mask_frag);
    blurFrag = shader(overlay_blur_frag);
  }
  VkPipeline pipeline(const Batch &b) {
    bool offscreen = b.target != root.get();
    int key = int(b.blend) * 32 + b.kind + (offscreen ? 16 : 0);
    if (auto it = pipelines.find(key); it != pipelines.end())
      return it->second;
    VkPipelineShaderStageCreateInfo stages[2]{};
    for (int i = 0; i < 2; ++i) {
      stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      stages[i].stage =
          i == 0 ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
      stages[i].module = i == 0
                             ? ((b.kind == 4 || b.kind == 8) ? maskVert : vert)
                             : (b.kind == 1                    ? stage1
                                : b.kind == 2                  ? stage2
                                : b.kind == 9                  ? blurFrag
                                : (b.kind == 3 || b.kind == 6 || b.kind == 7) ? partMask
                                : (b.kind == 4 || b.kind == 8) ? maskFrag
                                                               : frag);
      stages[i].pName = "main";
    }
    VkVertexInputBindingDescription binding{0, sizeof(Vertex),
                                            VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attrs[] = {
        {0, 0, VK_FORMAT_R32G32_SFLOAT, 0}, {1, 0, VK_FORMAT_R32G32_SFLOAT, 8}};
    VkPipelineVertexInputStateCreateInfo vi{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &binding;
    vi.vertexAttributeDescriptionCount = 2;
    vi.pVertexAttributeDescriptions = attrs;
    VkPipelineInputAssemblyStateCreateInfo ia{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo vp{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = vp.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.lineWidth = 1;
    VkPipelineMultisampleStateCreateInfo ms{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo ds{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    ds.stencilTestEnable = b.mask != 0;
    ds.front.compareOp =
        b.mask == 1 ? VK_COMPARE_OP_ALWAYS : VK_COMPARE_OP_EQUAL;
    ds.front.passOp = b.mask == 1 ? VK_STENCIL_OP_REPLACE : VK_STENCIL_OP_KEEP;
    ds.front.failOp = ds.front.depthFailOp = VK_STENCIL_OP_KEEP;
    ds.front.compareMask = 255;
    ds.front.writeMask = b.mask == 1 ? 255 : 0;
    ds.back = ds.front;
    auto blend = blendState(b.blend);
    bool useAdvanced = c.advancedBlendSupported && advanced(b.blend);
    VkPipelineColorBlendAdvancedStateCreateInfoEXT advancedInfo{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_ADVANCED_STATE_CREATE_INFO_EXT};
    if (useAdvanced) {
      blend.colorBlendOp = blend.alphaBlendOp = advancedBlendOpFor(b.blend);
      blend.srcColorBlendFactor = blend.dstColorBlendFactor =
          blend.srcAlphaBlendFactor = blend.dstAlphaBlendFactor =
              VK_BLEND_FACTOR_ONE;
      advancedInfo.srcPremultiplied = advancedInfo.dstPremultiplied = VK_TRUE;
      advancedInfo.blendOverlap = VK_BLEND_OVERLAP_UNCORRELATED_EXT;
    }
    blend.colorWriteMask = b.mask == 1 ? 0 : 15;
    if (b.kind == 9)
      blend.blendEnable = VK_FALSE;
    if (b.kind == 10) {
      blend.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
      blend.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
      blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    }
    VkPipelineColorBlendStateCreateInfo cb{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount = 1;
    cb.pAttachments = &blend;
    if (useAdvanced)
      cb.pNext = &advancedInfo;
    VkDynamicState dynamic[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                VK_DYNAMIC_STATE_SCISSOR,
                                VK_DYNAMIC_STATE_STENCIL_REFERENCE};
    VkPipelineDynamicStateCreateInfo dy{
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dy.dynamicStateCount = 3;
    dy.pDynamicStates = dynamic;
    VkGraphicsPipelineCreateInfo info{
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    info.stageCount = 2;
    info.pStages = stages;
    info.pVertexInputState = &vi;
    info.pInputAssemblyState = &ia;
    info.pViewportState = &vp;
    info.pRasterizationState = &rs;
    info.pMultisampleState = &ms;
    info.pDepthStencilState = &ds;
    info.pColorBlendState = &cb;
    info.pDynamicState = &dy;
    info.layout = layout;
    info.renderPass = offscreen ? offscreenPass : pass;
    VkPipeline result;
    check(vkCreateGraphicsPipelines(c.device, VK_NULL_HANDLE, 1, &info, nullptr,
                                    &result));
    pipelines.emplace(key, result);
    return result;
  }
  void record() {
    auto cmd = c.command;
    check(vkResetDescriptorPool(c.device, pool, 0));
    c.ensureBuffer(vertexBuffer, vertices.size() * sizeof(Vertex),
                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    c.ensureBuffer(indexBuffer, elements.size() * sizeof(uint16_t),
                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    if (!vertices.empty())
      std::memcpy(vertexBuffer.mapped, vertices.data(),
                  vertices.size() * sizeof(Vertex));
    if (!elements.empty())
      std::memcpy(indexBuffer.mapped, elements.data(),
                  elements.size() * sizeof(uint16_t));
    Target *active = nullptr;
    auto end = [&] {
      if (active) {
        vkCmdEndRenderPass(cmd);
        c.transition(cmd, textures.at(active->colors[0]).image,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        active = nullptr;
      }
    };
    for (const auto &b : batches) {
      auto &t = *b.target;
      if (active != &t) {
        end();
        c.transition(cmd, textures.at(t.colors[0]).image,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        if (t.stencil.handle)
          c.transition(cmd, t.stencil,
                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                       VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
        VkRenderPassBeginInfo r{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        r.renderPass = &t == root.get() ? pass : offscreenPass;
        r.framebuffer = t.framebuffer;
        r.renderArea.extent = {t.width, t.height};
        vkCmdBeginRenderPass(cmd, &r, VK_SUBPASS_CONTENTS_INLINE);
        active = &t;
        bool offscreen = &t != root.get();
        VkViewport vp{0,
                      offscreen ? 0.f : float(t.height),
                      float(t.width),
                      offscreen ? float(t.height) : -float(t.height),
                      0,
                      1};
        VkRect2D sc{{0, 0}, {t.width, t.height}};
        vkCmdSetViewport(cmd, 0, 1, &vp);
        vkCmdSetScissor(cmd, 0, 1, &sc);
      }
      if (b.clear || b.clearStencil >= 0) {
        std::vector<VkClearAttachment> clears;
        if (b.clear) {
          for (uint32_t i = 0; i < 1; ++i) {
            VkClearAttachment a{};
            a.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            a.colorAttachment = i;
            a.clearValue.color = {{0, 0, 0, 0}};
            if (&t == root.get() && !nijiexpose::render::transparentWindow)
              core::inGetClearColor(
                  a.clearValue.color.float32[0], a.clearValue.color.float32[1],
                  a.clearValue.color.float32[2], a.clearValue.color.float32[3]);
            clears.push_back(a);
          }
        }
        VkClearAttachment stencil{};
        stencil.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
        stencil.clearValue.depthStencil = {
            1, uint32_t(std::max(0, b.clearStencil))};
        if (t.stencil.handle)
          clears.push_back(stencil);
        VkClearRect rect{{{0, 0}, {t.width, t.height}}, 0, 1};
        vkCmdClearAttachments(cmd, uint32_t(clears.size()), clears.data(), 1,
                              &rect);
      }
      if (!b.count)
        continue;
      VkDescriptorSet set;
      VkDescriptorSetAllocateInfo ai{
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
      ai.descriptorPool = pool;
      ai.descriptorSetCount = 1;
      ai.pSetLayouts = &setLayout;
      check(vkAllocateDescriptorSets(c.device, &ai, &set));
      VkDescriptorImageInfo images[3]{};
      VkWriteDescriptorSet writes[3]{};
      for (int i = 0; i < 3; ++i) {
        auto id = b.textures[i];
        if (!id || !textures.contains(id))
          id = i == 0 ? white : black;
        auto &tex = textures.at(id);
        images[i] = {tex.sampler ? tex.sampler : c.sampler, tex.image.view,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = set;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].pImageInfo = &images[i];
      }
      vkUpdateDescriptorSets(c.device, 3, writes, 0, nullptr);
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline(b));
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0,
                              1, &set, 0, nullptr);
      vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                         sizeof(Material), &b.material);
      vkCmdSetStencilReference(cmd, VK_STENCIL_FACE_FRONT_AND_BACK,
                               b.reference);
      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer.handle, &offset);
      vkCmdBindIndexBuffer(cmd, indexBuffer.handle, 0, VK_INDEX_TYPE_UINT16);
      if (c.advancedBlendSupported && !c.advancedBlendCoherent && advanced(b.blend)) {
        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        barrier.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT;
        // Noncoherent hardware also needs ordering for overlapping triangles
        // within a part. Coherent devices keep the single-draw fast path.
        for(uint32_t offset=0;offset<b.count;offset+=3) {
          vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_DEPENDENCY_BY_REGION_BIT,
              1,&barrier,0,nullptr,0,nullptr);
          vkCmdDrawIndexed(cmd,std::min(3u,b.count-offset),1,b.first+offset,b.base,0);
        }
      } else vkCmdDrawIndexed(cmd, b.count, 1, b.first, b.base, 0);
    }
    end();
  }
};
Backend &backend() {
  auto b = std::dynamic_pointer_cast<Backend>(core::getCurrentRenderBackend());
  if (!b)
    throw std::logic_error("Vulkan model backend is not active");
  return *b;
}
ImTextureID uiTexture(uint32_t id) { return id ? backend().uiTexture(id) : 0; }
Backend::Backend() : p(std::make_unique<Impl>()) { p->c.renderBackend_ = this; }
Backend::~Backend() { shutdown(); }
void Backend::shutdown() {
  if (!p)
    return;
  p->c.idle();
  if (p->root)
    p->destroyTarget(*p->root);
  if (p->blurA)
    p->destroyTarget(*p->blurA);
  if (p->blurB)
    p->destroyTarget(*p->blurB);
  for (auto &[_, v] : p->composites)
    p->destroyTarget(*v.target);
  for (auto &[_, v] : p->textures) {
    if (v.sampler)
      vkDestroySampler(p->c.device, v.sampler, nullptr);
    p->c.destroy(v.image);
  }
  p->c.destroy(p->vertexBuffer);
  p->c.destroy(p->indexBuffer);
  for (auto &[_, v] : p->pipelines)
    vkDestroyPipeline(p->c.device, v, nullptr);
  vkDestroyShaderModule(p->c.device, p->vert, nullptr);
  vkDestroyShaderModule(p->c.device, p->frag, nullptr);
  for (auto shader : {p->maskVert, p->stage1, p->stage2, p->partMask,
                      p->maskFrag, p->blurFrag})
    vkDestroyShaderModule(p->c.device, shader, nullptr);
  vkDestroyDescriptorPool(p->c.device, p->pool, nullptr);
  vkDestroyPipelineLayout(p->c.device, p->layout, nullptr);
  vkDestroyDescriptorSetLayout(p->c.device, p->setLayout, nullptr);
  vkDestroyRenderPass(p->c.device, p->pass, nullptr);
  vkDestroyRenderPass(p->c.device, p->offscreenPass, nullptr);
  p->c.renderBackend_ = nullptr;
  p.reset();
}
void Backend::initializeRenderer() {
  p->initialize();
  p->white = createTexture({255, 255, 255, 255}, 1, 1, 4, false);
  p->black = createTexture({0, 0, 0, 0}, 1, 1, 4, false);
  resizeViewportTargets(p->c.width(), p->c.height());
}
void Backend::createDrawableBuffers(uint32_t &id) {
  if (!id)
    id = p->nextIndex++;
}
void Backend::disposeDrawableBuffers(uint32_t id) {
  if (p)
    p->indices.erase(id);
}
void Backend::uploadDrawableIndices(uint32_t id,
                                    const std::vector<uint16_t> &values) {
  p->indices[id] = values;
}
uint32_t Backend::createTexture(const std::vector<uint8_t> &data, int w, int h,
                                int channels, bool stencil) {
  if (w == 0 && h == 0 && data.empty())
    return createTexture({0,0,0,0},1,1,4,false);
  if (w <= 0 || h <= 0 || channels < 1 || channels > 4)
    throw std::runtime_error("Invalid texture dimensions");
  auto id = p->nextTexture++;
  Impl::Texture t;
  t.channels = channels;
  t.stencil = stencil;
  t.image = p->c.createImage(w, h, VK_FORMAT_R8G8B8A8_UNORM, colorUsage);
  p->textures.emplace(id, std::move(t));
  updateTexture(id, data, w, h, channels);
  return id;
}
void Backend::updateTexture(uint32_t id, const std::vector<uint8_t> &data,
                            int w, int h, int channels) {
  auto &t = p->textures.at(id);
  if (w != int(t.image.width) || h != int(t.image.height))
    throw std::runtime_error("Texture resize requires recreation");
  std::vector<uint8_t> rgba(size_t(w) * h * 4, 0);
  if (!data.empty()) {
    if (data.size() < size_t(w) * h * channels)
      throw std::runtime_error("Truncated texture pixels");
    for (size_t i = 0; i < size_t(w) * h; ++i) {
      rgba[i * 4] = data[i * channels];
      rgba[i * 4 + 1] = channels > 1 ? data[i * channels + 1] : rgba[i * 4];
      rgba[i * 4 + 2] = channels > 2 ? data[i * channels + 2] : rgba[i * 4];
      rgba[i * 4 + 3] = channels > 3 ? data[i * channels + 3] : 255;
    }
  }
  p->c.upload(t.image, rgba);
}
void Backend::setTextureParams(uint32_t id, core::Filtering filtering,
                               core::Wrapping wrapping, float) {
  auto &t = p->textures.at(id);
  p->c.idle();
  if (t.sampler)
    vkDestroySampler(p->c.device, t.sampler, nullptr);
  VkSamplerCreateInfo s{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  s.magFilter = s.minFilter = filtering == core::Filtering::Linear
                                  ? VK_FILTER_LINEAR
                                  : VK_FILTER_NEAREST;
  s.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  s.addressModeU = s.addressModeV = s.addressModeW =
      wrapping == core::Wrapping::Clamp ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
                                        : VK_SAMPLER_ADDRESS_MODE_REPEAT;
  check(vkCreateSampler(p->c.device, &s, nullptr, &t.sampler));
}
void Backend::disposeTexture(uint32_t id) {
  if (!p)
    return;
  auto it = p->textures.find(id);
  if (it == p->textures.end())
    return;
  p->c.idle();
  for (auto c = p->composites.begin(); c != p->composites.end();) {
    auto &ids = c->second.target->colors;
    if (std::find(ids.begin(), ids.end(), id) != ids.end()) {
      p->destroyTarget(*c->second.target);
      c = p->composites.erase(c);
    } else
      ++c;
  }
  if (it->second.sampler)
    vkDestroySampler(p->c.device, it->second.sampler, nullptr);
  p->c.destroy(it->second.image);
  p->textures.erase(it);
}
std::vector<uint8_t> Backend::readTexture(uint32_t id) {
  return p->c.read(p->textures.at(id).image);
}
ImTextureID Backend::uiTexture(uint32_t id) {
  return p->c.texture(p->textures.at(id).image);
}
void Backend::resizeViewportTargets(int w, int h) {
  if (w <= 0 || h <= 0)
    return;
  bool sharedOutput = false;
#ifdef _WIN32
  sharedOutput = p->c.spoutBridge_ != nullptr;
#endif
  if (p->spoutTarget == sharedOutput && p->root && p->root->width == uint32_t(w) &&
      p->root->height == uint32_t(h))
    return;
  p->c.idle();
  if (p->root) {
    auto ids = p->root->colors;
    p->destroyTarget(*p->root);
    p->root.reset();
    for (auto id : ids)
      disposeTexture(id);
  }
  std::array<uint32_t, 3> ids;
  for (auto &id : ids) {
#ifdef _WIN32
    if (&id == &ids[0] && sharedOutput) {
      id = p->nextTexture++;
      Impl::Texture texture;
      texture.image = p->c.spoutBridge_->createImage(w,h);
      p->textures.emplace(id,std::move(texture));
      continue;
    }
#endif
    id = createTexture({}, w, h, 4, false);
  }
  p->spoutTarget = sharedOutput;
  p->root = p->makeTarget(ids, true);
  p->target = p->root.get();
}
void Backend::beginScene() {
  p->vertices.clear();
  p->elements.clear();
  p->batches.clear();
  p->stack.clear();
  p->target = p->root.get();
  p->mask = 0;
  Impl::Batch clear;
  clear.target = p->target;
  clear.clear = true;
  p->batches.push_back(clear);
  for (auto it = p->composites.begin(); it != p->composites.end();)
    if (it->second.owner.expired()) {
      p->destroyTarget(*it->second.target);
      it = p->composites.erase(it);
    } else
      ++it;
}
void Backend::endScene() {}
void Backend::addBasicLightingPostProcess() { p->lighting = true; }
void Backend::postProcessScene() {
  if (!p->lighting) return;
  const auto light = nijiexpose::render::inSceneAmbientLight;
  Impl::Batch b;
  b.target = p->root.get();
  b.base = int32_t(p->vertices.size());
  b.first = uint32_t(p->elements.size());
  b.count = 6;
  b.kind = 10;
  b.material.tint[0] = light.x;
  b.material.tint[1] = light.y;
  b.material.tint[2] = light.z;
  for (auto v : std::array<Vertex,4>{{{-1,-1,0,0},{1,-1,1,0},{1,1,1,1},{-1,1,0,1}}})
    p->vertices.push_back(v);
  for (uint16_t i : {0,1,2,0,2,3}) p->elements.push_back(i);
  p->batches.push_back(b);
}
void Backend::render() {
  if (p->blurRequested) {
    auto append = [&](Impl::Target *target, uint32_t source, float x, float y) {
      Impl::Batch b;
      b.target = target;
      b.base = int32_t(p->vertices.size());
      b.first = uint32_t(p->elements.size());
      b.count = 6;
      b.kind = 9;
      b.textures[0] = source;
      b.material.screen[0] = x;
      b.material.screen[1] = y;
      for (auto v : std::array<Vertex, 4>{
               {{-1, -1, 0, 0}, {1, -1, 1, 0}, {1, 1, 1, 1}, {-1, 1, 0, 1}}})
        p->vertices.push_back(v);
      for (uint16_t i : {0, 1, 2, 0, 2, 3})
        p->elements.push_back(i);
      p->batches.push_back(b);
    };
    auto a = p->blurA.get(), b = p->blurB.get();
    append(a, renderImageHandle(), 0, 0);
    append(b, a->colors[0], 2.2f / a->width, 0);
    append(a, b->colors[0], 0, 2.2f / a->height);
    append(b, a->colors[0], 3.6f / a->width, 0);
    append(a, b->colors[0], 0, 3.6f / a->height);
  }
  p->record();
  p->blurRequested = false;
}
ImTextureID Backend::overlayBlurTexture() {
  auto w = std::max(1u, p->root->width / 4),
       h = std::max(1u, p->root->height / 4);
  if (!p->blurA || p->blurA->width != w || p->blurA->height != h) {
    p->c.idle();
    for (auto ptr : {&p->blurA, &p->blurB}) {
      if (*ptr) {
        auto id = (*ptr)->colors[0];
        p->destroyTarget(**ptr);
        ptr->reset();
        disposeTexture(id);
      }
      auto id = createTexture({}, w, h, 4, false);
      *ptr = p->makeTarget({id, id, id});
    }
  }
  p->blurRequested = true;
  return uiTexture(p->blurA->colors[0]);
}
Image &Backend::sceneImage() {
  return p->textures.at(renderImageHandle()).image;
}
uint32_t Backend::renderImageHandle() { return p->root->colors[0]; }
uint32_t Backend::mainAlbedoHandle() { return p->root->colors[0]; }
uint32_t Backend::mainEmissiveHandle() { return p->root->colors[1]; }
uint32_t Backend::mainBumpHandle() { return p->root->colors[2]; }
void Backend::dumpViewport(std::vector<uint8_t> &out, int w, int h) {
  if (w != int(p->root->width) || h != int(p->root->height))
    throw std::runtime_error("Viewport readback dimensions mismatch");
  out = readTexture(renderImageHandle());
}
void Backend::drawPuppet(core::Puppet &puppet) {
  using namespace core::render;
  auto host = core::getCurrentRenderBackend();
  auto queue = std::make_shared<QueueRenderBackend>();
  puppet.setRenderBackend(queue);
  core::setCurrentRenderBackend(host);
  puppet.draw();
  for (const auto &cmd : queue->queue) {
    switch (cmd.kind) {
    case RenderCommandKind::DrawPart:
      drawPartPacket(cmd.partPacket);
      break;
    case RenderCommandKind::BeginMask:
      beginMask(cmd.usesStencil);
      break;
    case RenderCommandKind::ApplyMask:
      applyMask(cmd.maskApplyPacket);
      break;
    case RenderCommandKind::BeginMaskContent:
      beginMaskContent();
      break;
    case RenderCommandKind::EndMask:
      endMask();
      break;
    case RenderCommandKind::BeginDynamicComposite:
      beginDynamicComposite(cmd.dynamicPass);
      break;
    case RenderCommandKind::EndDynamicComposite:
      endDynamicComposite(cmd.dynamicPass);
      break;
    }
  }
  queue->clear();
}
void Backend::drawPartPacket(const PartDrawPacket &packet) {
  if (!packet.renderable || !packet.vertexCount || !packet.indexCount)
    return;
  auto ix = p->indices.find(packet.indexBuffer);
  if (ix == p->indices.end())
    throw std::runtime_error("Missing Vulkan drawable indices");
  auto v = core::render::sharedVertexBufferData().rawStorage(),
       u = core::render::sharedUvBufferData().rawStorage(),
       d = core::render::sharedDeformBufferData().rawStorage();
  auto count = packet.vertexCount;
  if (packet.vertexOffset + packet.vertexAtlasStride + count > v.length ||
      packet.uvOffset + packet.uvAtlasStride + count > u.length ||
      packet.deformOffset + packet.deformAtlasStride + count > d.length)
    throw std::runtime_error("Drawable atlas bounds exceeded");
  Impl::Batch b;
  b.target = p->target;
  b.base = int32_t(p->vertices.size());
  b.first = uint32_t(p->elements.size());
  b.count = std::min<uint32_t>(packet.indexCount, uint32_t(ix->second.size()));
  b.mask = p->mask;
  b.reference = p->reference;
  b.blend = p->mask == 1 ? BlendMode::Normal : packet.blendMode;
  auto matrix = packet.renderMatrix * packet.modelMatrix;
  for (uint32_t i = 0; i < count; ++i) {
    float x = v.ptr[packet.vertexOffset + i] + d.ptr[packet.deformOffset + i] -
              packet.origin.x;
    float y = v.ptr[packet.vertexOffset + packet.vertexAtlasStride + i] +
              d.ptr[packet.deformOffset + packet.deformAtlasStride + i] -
              packet.origin.y;
    auto clip = transformClip(matrix, {x, y, 0});
    p->vertices.push_back({clip.x, clip.y, u.ptr[packet.uvOffset + i],
                           u.ptr[packet.uvOffset + packet.uvAtlasStride + i]});
  }
  p->elements.insert(p->elements.end(), ix->second.begin(),
                     ix->second.begin() + b.count);
  std::copy_n(packet.textureBackendIds, 3, b.textures.begin());
  if (auto node = packet.node.lock())
    for (int i = 0; i < 3; ++i)
      if (node->textures[i])
        b.textures[i] = node->textures[i]->backendId();
  b.material.tint[0] = packet.clampedTint.x;
  b.material.tint[1] = packet.clampedTint.y;
  b.material.tint[2] = packet.clampedTint.z;
  b.material.tint[3] = packet.opacity;
  b.material.screen[0] = packet.clampedScreen.x;
  b.material.screen[1] = packet.clampedScreen.y;
  b.material.screen[2] = packet.clampedScreen.z;
  b.material.screen[3] = packet.emissionStrength;
  b.kind = p->mask == 1 ? 6 : packet.isMask ? 3 : p->mask == 2 ? 5 : 0;
  if (packet.isMask || p->mask == 1)
    b.material.screen[3] = packet.maskThreshold;
  if (p->mask == 1)
    p->maskWritten = true;
  p->batches.push_back(b);
}
void Backend::beginMask(bool normal) {
  p->maskWritten = false;
  p->mask = 1;
  Impl::Batch b;
  b.target = p->target;
  b.clearStencil = normal ? 0 : 1;
  p->batches.push_back(b);
}
void Backend::beginMaskContent() {
  p->mask = p->maskWritten ? 2 : 0;
  p->reference = 1;
}
void Backend::endMask() { p->mask = 0; }
void Backend::applyMask(const MaskApplyPacket &m) {
  if (p->mask != 1)
    return;
  p->reference = m.isDodge ? 0 : 1;
  if (m.kind == MaskDrawableKind::Part) {
    drawPartPacket(m.partPacket);
    return;
  }
  const auto &mask = m.maskPacket;
  auto v = core::render::sharedVertexBufferData().rawStorage(),
       d = core::render::sharedDeformBufferData().rawStorage();
  auto it = p->indices.find(mask.indexBuffer);
  if (it == p->indices.end() || !mask.vertexCount)
    return;
  Impl::Batch b;
  b.target = p->target;
  b.base = int32_t(p->vertices.size());
  b.first = uint32_t(p->elements.size());
  b.count = uint32_t(it->second.size());
  b.mask = 1;
  b.reference = p->reference;
  b.kind = 8;
  p->maskWritten = true;
  for (uint32_t i = 0; i < mask.vertexCount; ++i) {
    auto clip = transformClip(
        mask.mvp, {v.ptr[mask.vertexOffset + i] + d.ptr[mask.deformOffset + i] -
                       mask.origin.x,
                   v.ptr[mask.vertexOffset + mask.vertexAtlasStride + i] +
                       d.ptr[mask.deformOffset + mask.deformAtlasStride + i] -
                       mask.origin.y,
                   0});
    p->vertices.push_back({clip.x, clip.y, 0, 0});
  }
  p->elements.insert(p->elements.end(), it->second.begin(), it->second.end());
  p->batches.push_back(b);
}
void Backend::beginDynamicComposite(const core::DynamicCompositePass &pass) {
  if (!pass.surface)
    throw std::runtime_error("Dynamic composite missing surface");
  std::array<uint32_t, 3> ids{};
  for (int i = 0; i < 3; ++i)
    if (pass.surface->textures[i])
      ids[i] = pass.surface->textures[i]->backendId();
  if (!ids[0])
    throw std::runtime_error("Dynamic composite missing color target");
  auto &entry = p->composites[pass.surface.get()];
  if (!entry.target || entry.target->colors != ids) {
    if (entry.target)
      p->destroyTarget(*entry.target);
    entry.owner = pass.surface;
    entry.target = p->makeTarget(ids);
  }
  p->stack.push_back(p->target);
  p->target = entry.target.get();
  Impl::Batch b;
  b.target = p->target;
  b.clear = true;
  p->batches.push_back(b);
}
void Backend::endDynamicComposite(const core::DynamicCompositePass &) {
  if (p->stack.empty())
    throw std::runtime_error("Unbalanced dynamic composite");
  p->target = p->stack.back();
  p->stack.pop_back();
}
void Backend::destroyDynamicComposite(
    const std::shared_ptr<core::DynamicCompositeSurface> &surface) {
  if (!p)
    return;
  auto it = p->composites.find(surface.get());
  if (it != p->composites.end()) {
    p->c.idle();
    p->destroyTarget(*it->second.target);
    p->composites.erase(it);
  }
}
void Backend::drawSprites(uint32_t texture, const float *data, size_t count,
                          const Mat4 &matrix) {
  if (!count)
    return;
  Impl::Batch b;
  b.target = p->target;
  b.base = int32_t(p->vertices.size());
  b.first = uint32_t(p->elements.size());
  b.count = uint32_t(count);
  b.textures[0] = texture;
  auto &image = p->textures.at(texture).image;
  for (size_t i = 0; i < count; ++i) {
    auto v = matrix.transformPoint({data[i * 8], data[i * 8 + 1], 0});
    p->vertices.push_back({v.x, v.y, data[i * 8 + 2] / image.width,
                           data[i * 8 + 3] / image.height});
    p->elements.push_back(uint16_t(i));
  }
  std::copy_n(data + 4, 4, b.material.tint);
  p->batches.push_back(b);
}
} // namespace nijiexpose::vulkan
