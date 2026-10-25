// Port of nijilive/core/render/backends/opengl/debug_renderer.d.
#include "debug.hpp"
#include "assets.hpp"
namespace nijiexpose::ogl {
thread_local Shader lineShader;
thread_local Shader pointShader;


thread_local GLuint vao;
thread_local GLuint vbo;
thread_local GLuint ibo;
thread_local GLuint currentVbo;
thread_local int indexCount;
thread_local int lineMvpLocation = -1;
thread_local int lineColorLocation = -1;
thread_local int pointMvpLocation = -1;
thread_local int pointColorLocation = -1;
int pointCount;
bool bufferIsSoA;

void ensureInitialized() {
    if (bool(lineShader)) return;

    lineShader = Shader(LineShaderSource);
    pointShader = Shader(PointShaderSource);

    lineMvpLocation = lineShader.getUniformLocation("mvp");
    lineColorLocation = lineShader.getUniformLocation("color");
    pointMvpLocation = pointShader.getUniformLocation("mvp");
    pointColorLocation = pointShader.getUniformLocation("color");

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);
    currentVbo = vbo;
    indexCount = 0;
}

 void oglInitDebugRenderer() {
    ensureInitialized();
}

 void oglSetDebugPointSize(float size) {
    glPointSize(size);
}

 void oglSetDebugLineWidth(float size) {
    glLineWidth(size);
}

 void oglUploadDebugBuffer(const std::vector<vec3>& points, std::span<const uint16_t> indices) {
    ensureInitialized();
    if (points.size() == 0 || indices.size() == 0) {
        indexCount = 0;
        pointCount = 0;
        bufferIsSoA = false;
        return;
    }

    glBindVertexArray(vao);
    std::vector<float> raw(points.size() * 3);
    for (size_t i = 0; i < points.size(); ++i) {
        raw[i] = points[i].x;
        raw[points.size() + i] = points[i].y;
        raw[points.size() * 2 + i] = points[i].z;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, raw.size() * sizeof(float), raw.data(), GL_DYNAMIC_DRAW);
    currentVbo = vbo;
    pointCount = static_cast<int>(points.size());
    bufferIsSoA = true;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint16_t), indices.data(), GL_DYNAMIC_DRAW);
    indexCount = static_cast<int>(indices.size());
}

 void oglSetDebugExternalBuffer(GLuint vertexBuffer, GLuint indexBuffer, int count) {
    ensureInitialized();

    auto vertexHandle = static_cast<GLuint>(vertexBuffer);
    auto indexHandle = static_cast<GLuint>(indexBuffer);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vertexHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexHandle);
    currentVbo = vertexHandle;
    indexCount = count;
    bufferIsSoA = false;
    pointCount = 0;
}

void prepareDraw(Shader shader, int mvpLocation, int colorLocation, mat4 mvp, vec4 color) {
    if (!shader || indexCount <= 0) return;

    shader.use();
    shader.setUniform(mvpLocation, mvp);
    shader.setUniform(colorLocation, color);

    glBindVertexArray(vao);
    if (bufferIsSoA && pointCount > 0) {
        auto laneBytes = static_cast<ptrdiff_t>(pointCount) * sizeof(float);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, currentVbo);
        glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, currentVbo);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(laneBytes));

        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, currentVbo);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(laneBytes * 2));
    } else {
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, currentVbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
    }
}

void finishDraw() {
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindVertexArray(0);
}

 void oglDrawDebugPoints(vec4 color, mat4 mvp) {
    ensureInitialized();
    if (indexCount <= 0) return;

    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

    prepareDraw(pointShader, pointMvpLocation, pointColorLocation, mvp, color);
    glDrawElements(GL_POINTS, indexCount, GL_UNSIGNED_SHORT, nullptr);
    finishDraw();

    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
}

 void oglDrawDebugLines(vec4 color, mat4 mvp) {
    ensureInitialized();
    if (indexCount <= 0) return;

    glEnable(GL_LINE_SMOOTH);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

    prepareDraw(lineShader, lineMvpLocation, lineColorLocation, mvp, color);
    glDrawElements(GL_LINES, indexCount, GL_UNSIGNED_SHORT, nullptr);
    finishDraw();

    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
    glDisable(GL_LINE_SMOOTH);
}

}
