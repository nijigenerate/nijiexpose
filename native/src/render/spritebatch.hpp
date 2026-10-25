#pragma once
#include "nijilive/opengl/shader.hpp"
#include <core/texture.hpp>
#include <limits>
#include <vector>
namespace nijiexpose {
using ogl::vec2; using ogl::vec4; using ogl::mat4;
struct SpriteRect {
    float x=std::numeric_limits<float>::quiet_NaN();
    float y=std::numeric_limits<float>::quiet_NaN();
    float width=std::numeric_limits<float>::quiet_NaN();
    float height=std::numeric_limits<float>::quiet_NaN();
};
enum class SpriteFlip { None=0, Horizontal=1, Vertical=2 };
class SpriteBatch {
    static constexpr size_t EntryCount=10000,VecSize=2,UVSize=2,ColorSize=4,VertsCount=6;
    static constexpr size_t DataLength=VecSize+UVSize+ColorSize,DataSize=DataLength*VertsCount;
    std::vector<float> data;
    size_t dataOffset{},tris{};
    GLuint vao{},buffer{};
    GLint vp{};
    std::shared_ptr<nicxlive::core::Texture> currentTexture;
    mat4 viewProjection;
    void addVertexData(vec2 position,vec2 uvs,vec4 color);
    void updateVP();
public:
    SpriteBatch();
    void draw(const std::shared_ptr<nicxlive::core::Texture>& texture,SpriteRect position,
        SpriteRect cutout={},vec2 origin={0,0},float rotation=0,SpriteFlip flip=SpriteFlip::None,vec4 color={1,1,1,1});
    void flush();
};
extern thread_local std::shared_ptr<SpriteBatch> AppBatch;
}
