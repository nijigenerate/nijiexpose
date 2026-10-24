// RenderingBackend resource operations from nijimi/source/render/opengl/opengl_backend.d.
#include "drawable.hpp"
#include <unordered_map>
#include <vector>
namespace nijiexpose::ogl {
namespace {
GLuint drawableVAO;
bool drawableBuffersInitialized = false;
GLuint sharedDeformBuffer, sharedVertexBuffer, sharedUvBuffer, sharedIndexBuffer;
size_t sharedIndexCapacity, sharedIndexOffset;
size_t vertexBytes{},uvBytes{},deformBytes{};
bool indexCollectionPending=false;
GLuint nextIndexHandle = 1;
struct IndexRange {
    size_t offset{}, count{}, capacity{};
    std::vector<uint16_t> data;
};
std::unordered_map<GLuint, IndexRange> sharedIndexRanges;
void ensureSharedIndexBuffer(size_t bytes) {
    if (sharedIndexBuffer == 0) {
        glGenBuffers(1, &sharedIndexBuffer);
        sharedIndexCapacity = 0;
        sharedIndexOffset = 0;
    }
    if (bytes > sharedIndexCapacity) {
        size_t newCap = sharedIndexCapacity == 0 ? 1024 : sharedIndexCapacity;
        while (newCap < bytes) newCap *= 2;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sharedIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, newCap, nullptr, GL_DYNAMIC_DRAW);
        sharedIndexCapacity = newCap;
        sharedIndexOffset = 0;
        for (auto& [key, entry] : sharedIndexRanges) {
            if (entry.data.empty()) continue;
            auto entryBytes = entry.data.size() * sizeof(uint16_t);
            entry.offset = sharedIndexOffset;
            entry.count = entry.data.size();
            entry.capacity = entryBytes;
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, entry.offset, entryBytes, entry.data.data());
            sharedIndexOffset += entryBytes;
        }
    }
}
void glUploadFloatVecArray(GLuint buffer, const Vec2Array& data, GLenum usage) {
    if (buffer == 0 || data.size() == 0) return;
    auto raw = data.rawStorage();
    if (raw.length == 0) return;
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, raw.length * sizeof(float), raw.ptr, usage);
}
}
void oglInitDrawableBackend() {
    if (drawableBuffersInitialized) return;
    drawableBuffersInitialized = true;
    glGenVertexArrays(1, &drawableVAO);
}
void oglBindDrawableVao() {
    oglInitDrawableBackend();
    glBindVertexArray(drawableVAO);
}
void oglCreateDrawableBuffers(GLuint& ibo) {
    oglInitDrawableBackend();
    if (ibo == 0) ibo = nextIndexHandle++;
}
static void compactIndexBuffer();
void oglDisposeDrawableBuffers(GLuint id) {
    if(sharedIndexRanges.erase(id))indexCollectionPending=true;
    // Release the last GPU allocation immediately; survivor compaction is batched
    // until the next upload/draw, entirely inside this backend.
    if(sharedIndexRanges.empty())compactIndexBuffer();
}
static void compactIndexBuffer() {
    if(!indexCollectionPending)return;
    indexCollectionPending=false;
    GLint previousVao=0;glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&previousVao);
    oglBindDrawableVao();
    size_t bytes=0;for(auto& [id,range]:sharedIndexRanges)bytes+=range.data.size()*sizeof(uint16_t);
    sharedIndexOffset=0;sharedIndexCapacity=bytes;
    if(!bytes) {
        if(sharedIndexBuffer)glDeleteBuffers(1,&sharedIndexBuffer);
        sharedIndexBuffer=0;
        decltype(sharedIndexRanges)().swap(sharedIndexRanges);
    } else {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,sharedIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,bytes,nullptr,GL_DYNAMIC_DRAW);
        for(auto& [id,range]:sharedIndexRanges) {
            range.offset=sharedIndexOffset;range.count=range.data.size();range.capacity=range.count*sizeof(uint16_t);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,range.offset,range.capacity,range.data.data());
            sharedIndexOffset+=range.capacity;
        }
    }
    glBindVertexArray(static_cast<GLuint>(previousVao));
}
DrawableResourceUsage oglDrawableResourceUsage() {
    size_t cpu=0;for(auto& [id,range]:sharedIndexRanges)cpu+=range.data.capacity()*sizeof(uint16_t);
    return {sharedIndexRanges.size(),cpu,sharedIndexCapacity,vertexBytes,uvBytes,deformBytes};
}
void uploadOrRelease(GLuint& buffer,const Vec2Array& data,size_t& bytes) {
    bytes=data.rawStorage().length*sizeof(float);
    if(!bytes) {
        GLint previousVao=0;glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&previousVao);
        oglBindDrawableVao();
        if(buffer)glDeleteBuffers(1,&buffer);buffer=0;
        glBindVertexArray(static_cast<GLuint>(previousVao));
        return;
    }
    if(!buffer)glGenBuffers(1,&buffer);
    glUploadFloatVecArray(buffer,data,GL_DYNAMIC_DRAW);
}
void oglUploadDrawableIndices(GLuint ibo, std::span<const uint16_t> indices) {
    if (ibo == 0 || indices.empty()) return;
    auto bytes = indices.size() * sizeof(uint16_t);
    ensureSharedIndexBuffer(bytes + sharedIndexOffset);
    IndexRange range;
    auto existing = sharedIndexRanges.find(ibo);
    if (existing != sharedIndexRanges.end()) range = existing->second;
    if (existing == sharedIndexRanges.end() || bytes > range.capacity) {
        range.offset = sharedIndexOffset;
        range.count = indices.size();
        range.capacity = bytes;
        sharedIndexOffset += bytes;
    } else {
        range.count = indices.size();
    }
    range.data.assign(indices.begin(), indices.end());
    sharedIndexRanges[ibo] = range;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sharedIndexBuffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, range.offset, bytes, indices.data());
}
void oglUploadSharedVertexBuffer(const Vec2Array& vertices) {
    compactIndexBuffer();
    uploadOrRelease(sharedVertexBuffer,vertices,vertexBytes);
}
void oglUploadSharedUvBuffer(const Vec2Array& uvs) {
    uploadOrRelease(sharedUvBuffer,uvs,uvBytes);
}
void oglUploadSharedDeformBuffer(const Vec2Array& deformation) {
    uploadOrRelease(sharedDeformBuffer,deformation,deformBytes);
}
GLuint oglGetSharedVertexBuffer() {
    if (sharedVertexBuffer == 0) glGenBuffers(1, &sharedVertexBuffer);
    return sharedVertexBuffer;
}
GLuint oglGetSharedUvBuffer() {
    if (sharedUvBuffer == 0) glGenBuffers(1, &sharedUvBuffer);
    return sharedUvBuffer;
}
GLuint oglGetSharedDeformBuffer() {
    if (sharedDeformBuffer == 0) glGenBuffers(1, &sharedDeformBuffer);
    return sharedDeformBuffer;
}
void oglDrawDrawableElements(GLuint ibo, size_t indexCount) {
    compactIndexBuffer();
    if (ibo == 0 || indexCount == 0) return;
    auto range = sharedIndexRanges.find(ibo);
    if (range == sharedIndexRanges.end() || sharedIndexBuffer == 0) return;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sharedIndexBuffer);
    auto offset = range->second.offset;
    glDrawElements(GL_TRIANGLES, static_cast<int>(indexCount), GL_UNSIGNED_SHORT, reinterpret_cast<void*>(offset));
}
}
