// Port of nijilive/core/render/backends/opengl/diff_collect_impl.d.
#include "difference.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace nijiexpose::ogl {
using std::clamp; using std::min; using std::max; using std::floor;
constexpr int TileColumns = 16;
constexpr size_t TileCount = TileColumns * TileColumns;
static GLuint compileShader(GLenum,const std::string&);
static GLuint linkProgram(GLuint,GLuint);
struct ReductionLevel {
    GLuint fbo;
    GLuint texture;
    int width;
    int height;
};

struct DifferenceEvaluator {
    bool enabled;
    bool useRegion;
    DifferenceEvaluationRegion region;

    bool resultReady;
    uint32_t pendingSampleCount;

    GLuint collectProgram;
    GLuint reduceProgram;
    GLuint fullscreenVao;

    GLuint collectFbo;
    GLuint collectTexture;

    std::vector<ReductionLevel> reductionLevels;
    GLuint lastTexture;

    int resourceWidth;
    int resourceHeight;
    int finalWidth;
    int finalHeight;

    GLint uCollectViewport;
    GLint uCollectRect;
    GLint uCollectUseRect;
    GLint uCollectSource;

    GLint uReduceSourceSize;
    GLint uReduceSource;

    void setEnabled(bool value) {
        enabled = value;
        if (!enabled) {
            resultReady = false;
            pendingSampleCount = 0;
        }
    }

    void setRegion(DifferenceEvaluationRegion region) {
        this->region = region;
        useRegion = region.width > 0 && region.height > 0;
    }

    DifferenceEvaluationRegion getRegion() const {
        return region;
    }

    bool isEnabled() const {
        return enabled;
    }

    void ensurePrograms() {
        if (collectProgram != 0 && reduceProgram != 0 && fullscreenVao != 0) {
            return;
        }

        GLuint vertShader = compileShader(GL_VERTEX_SHADER, R"NIJIGLSL(#version 150 core

out vec2 vUv;

const vec2 POSITIONS[4] = vec2[4](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0)
);

const vec2 UVS[4] = vec2[4](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

void main() {
    gl_Position = vec4(POSITIONS[gl_VertexID], 0.0, 1.0);
    vUv = UVS[gl_VertexID];
}
)NIJIGLSL");
        GLuint fragCollect = compileShader(GL_FRAGMENT_SHADER, R"NIJIGLSL(#version 150 core

in vec2 vUv;
out vec4 fragColor;

uniform sampler2D uSource;
uniform ivec2 uViewportSize;
uniform ivec4 uRect;
uniform bool uUseRect;

void main() {
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    if (uUseRect) {
        pixel += uRect.xy;
    }
    pixel = clamp(pixel, ivec2(0), uViewportSize - ivec2(1));

    vec3 sample = clamp(texelFetch(uSource, pixel, 0).rgb, 0.0, 1.0);
    fragColor = vec4(sample, 1.0);
}
)NIJIGLSL");
        GLuint fragReduce = compileShader(GL_FRAGMENT_SHADER, R"NIJIGLSL(#version 150 core

out vec4 fragColor;

uniform sampler2D uSource;
uniform ivec2 uSourceSize;

vec4 fetchPixel(ivec2 coord) {
    if (coord.x < 0 || coord.y < 0 || coord.x >= uSourceSize.x || coord.y >= uSourceSize.y) {
        return vec4(0.0);
    }
    return texelFetch(uSource, coord, 0);
}

void main() {
    ivec2 dst = ivec2(gl_FragCoord.xy);
    ivec2 base = dst * 2;

    vec4 sum = vec4(0.0);
    sum += fetchPixel(base);
    sum += fetchPixel(base + ivec2(1, 0));
    sum += fetchPixel(base + ivec2(0, 1));
    sum += fetchPixel(base + ivec2(1, 1));

    fragColor = sum;
}
)NIJIGLSL");

        collectProgram = linkProgram(vertShader, fragCollect);
        reduceProgram = linkProgram(vertShader, fragReduce);

        glDeleteShader(vertShader);
        glDeleteShader(fragCollect);
        glDeleteShader(fragReduce);

        uCollectViewport = glGetUniformLocation(collectProgram, "uViewportSize");
        uCollectRect = glGetUniformLocation(collectProgram, "uRect");
        uCollectUseRect = glGetUniformLocation(collectProgram, "uUseRect");
        uCollectSource = glGetUniformLocation(collectProgram, "uSource");

        uReduceSourceSize = glGetUniformLocation(reduceProgram, "uSourceSize");
        uReduceSource = glGetUniformLocation(reduceProgram, "uSource");

        glGenVertexArrays(1, &fullscreenVao);
    }

    void ensureResources(int width, int height) {
        if (resourceWidth == width && resourceHeight == height) {
            return;
        }

        destroyResources();

        resourceWidth = width;
        resourceHeight = height;
        finalWidth = width;
        finalHeight = height;

        collectTexture = createTexture(width, height);
        collectFbo = createFramebuffer(collectTexture);

        int currentWidth = width;
        int currentHeight = height;

        while (currentWidth > 16 || currentHeight > 16) {
            currentWidth = (currentWidth + 1) / 2;
            currentHeight = (currentHeight + 1) / 2;

            ReductionLevel level;
            level.width = currentWidth;
            level.height = currentHeight;
            level.texture = createTexture(level.width, level.height);
            level.fbo = createFramebuffer(level.texture);
            reductionLevels.push_back(level);
        }
    }

    GLuint createTexture(int width, int height) {
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

    GLuint createFramebuffer(GLuint texture) {
        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) throw std::runtime_error("Failed to create framebuffer for difference aggregation");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return fbo;
    }

    void destroyResources() {
        for (auto& level : reductionLevels) {
            if (level.fbo) glDeleteFramebuffers(1, &level.fbo);
            if (level.texture) glDeleteTextures(1, &level.texture);
        }
        reductionLevels.clear();

        if (collectFbo) glDeleteFramebuffers(1, &collectFbo);
        if (collectTexture) glDeleteTextures(1, &collectTexture);

        collectFbo = 0;
        collectTexture = 0;
        lastTexture = 0;
        resourceWidth = resourceHeight = 0;
        finalWidth = finalHeight = 0;
    }

    void destroyPrograms() {
        if (collectProgram) glDeleteProgram(collectProgram);
        if (reduceProgram) glDeleteProgram(reduceProgram);
        if (fullscreenVao) glDeleteVertexArrays(1, &fullscreenVao);

        collectProgram = 0;
        reduceProgram = 0;
        fullscreenVao = 0;
        uCollectViewport = -1;
        uCollectRect = -1;
        uCollectUseRect = -1;
        uCollectSource = -1;
        uReduceSourceSize = -1;
        uReduceSource = -1;
    }

    bool evaluate(GLuint sourceTexture, int viewportWidth, int viewportHeight) {
        if (!enabled) {
            resultReady = false;
            pendingSampleCount = 0;
            return false;
        }
        if (sourceTexture == 0 || viewportWidth <= 0 || viewportHeight <= 0) {
            resultReady = false;
            pendingSampleCount = 0;
            return false;
        }

        ensurePrograms();

        DifferenceEvaluationRegion clampedRegion = region;
        if (useRegion) {
            clampedRegion.x = clamp(clampedRegion.x, 0, viewportWidth);
            clampedRegion.y = clamp(clampedRegion.y, 0, viewportHeight);
            clampedRegion.width = clamp(clampedRegion.width, 0, viewportWidth - clampedRegion.x);
            clampedRegion.height = clamp(clampedRegion.height, 0, viewportHeight - clampedRegion.y);
        }

        int targetWidth = useRegion ? clampedRegion.width : viewportWidth;
        int targetHeight = useRegion ? clampedRegion.height : viewportHeight;
        ensureResources(targetWidth, targetHeight);
        if (targetWidth <= 0 || targetHeight <= 0) {
            resultReady = false;
            pendingSampleCount = 0;
            return false;
        }

        pendingSampleCount = static_cast<uint32_t>(targetWidth) * static_cast<uint32_t>(targetHeight);

        GLint prevFbo;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);

        std::array<GLint,4> prevViewport;
        glGetIntegerv(GL_VIEWPORT, prevViewport.data());

        GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);

        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        glBindFramebuffer(GL_FRAMEBUFFER, collectFbo);
        glDrawBuffers(1, std::array<GLenum, 1>{GL_COLOR_ATTACHMENT0}.data());
        glViewport(0, 0, targetWidth, targetHeight);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(collectProgram);
        if (uCollectViewport != -1) glUniform2i(uCollectViewport, viewportWidth, viewportHeight);
        if (uCollectRect != -1) glUniform4i(uCollectRect,
            clampedRegion.x,
            clampedRegion.y,
            targetWidth,
            targetHeight
        );
        if (uCollectUseRect != -1) glUniform1i(uCollectUseRect, useRegion ? 1 : 0);
        if (uCollectSource != -1) glUniform1i(uCollectSource, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sourceTexture);

        glBindVertexArray(fullscreenVao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        GLuint currentTexture = collectTexture;
        int currentWidth = targetWidth;
        int currentHeight = targetHeight;

        for (auto& level : reductionLevels) {
            if (level.width < TileColumns || level.height < TileColumns) {
                break;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, level.fbo);
            glDrawBuffers(1, std::array<GLenum, 1>{GL_COLOR_ATTACHMENT0}.data());
            glViewport(0, 0, level.width, level.height);
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(reduceProgram);
            if (uReduceSourceSize != -1) glUniform2i(uReduceSourceSize, currentWidth, currentHeight);
            if (uReduceSource != -1) glUniform1i(uReduceSource, 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, currentTexture);

            glBindVertexArray(fullscreenVao);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            currentTexture = level.texture;
            currentWidth = level.width;
            currentHeight = level.height;
        }

        lastTexture = currentTexture;
        finalWidth = currentWidth;
        finalHeight = currentHeight;
        resultReady = true;

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
        glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

        if (blendEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        if (depthEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if (cullEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);

        return true;
    }

    bool fetch(DifferenceEvaluationResult& result) {
        if (!resultReady || lastTexture == 0) {
            result = DifferenceEvaluationResult{};
            return false;
        }

        int width = finalWidth > 0 ? finalWidth : 1;
        int height = finalHeight > 0 ? finalHeight : 1;
        size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);

        std::vector<float> data(pixelCount * 4);
        glBindTexture(GL_TEXTURE_2D, lastTexture);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, data.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        double totalRed = 0;
        double totalGreen = 0;
        double totalBlue = 0;
        double totalAlpha = 0;

        std::array<double,TileCount> tileTotals{};
        std::array<double,TileCount> tileCounts{};

        double regionWidth = resourceWidth > 0 ? static_cast<double>(resourceWidth) : static_cast<double>(width);
        double regionHeight = resourceHeight > 0 ? static_cast<double>(resourceHeight) : static_cast<double>(height);
        if (regionWidth <= 0 || regionHeight <= 0) {
            result = DifferenceEvaluationResult{};
            return false;
        }

        double tileWidth = regionWidth / TileColumns;
        double tileHeight = regionHeight / TileColumns;
        if (tileWidth <= 0 || tileHeight <= 0) {
            result = DifferenceEvaluationResult{};
            return false;
        }

        double pixelWidth = regionWidth / static_cast<double>(width);
        double pixelHeight = regionHeight / static_cast<double>(height);
        if (pixelWidth <= 0 || pixelHeight <= 0) {
            result = DifferenceEvaluationResult{};
            return false;
        }
        double pixelArea = pixelWidth * pixelHeight;
        if (pixelArea <= 0) {
            result = DifferenceEvaluationResult{};
            return false;
        }
        const double epsilon = 1e-9;

        for (int y = 0; y < height; ++y) {
            double srcTop = y * pixelHeight;
            double srcBottom = (y + 1) * pixelHeight;

            for (int x = 0; x < width; ++x) {
                size_t idx = (static_cast<size_t>(y) * static_cast<size_t>(width) + x) * 4;

                double r = data[idx + 0];
                double g = data[idx + 1];
                double b = data[idx + 2];
                double a = data[idx + 3];

                totalRed += r;
                totalGreen += g;
                totalBlue += b;
                double weight = a > 0 ? a : 1.0;
                totalAlpha += weight;

                double srcLeft = x * pixelWidth;
                double srcRight = (x + 1) * pixelWidth;

                int tileXStart = static_cast<int>(floor(srcLeft / tileWidth));
                int tileXEnd = static_cast<int>(floor((srcRight - epsilon) / tileWidth));
                int tileYStart = static_cast<int>(floor(srcTop / tileHeight));
                int tileYEnd = static_cast<int>(floor((srcBottom - epsilon) / tileHeight));

                tileXStart = clamp(tileXStart, 0, TileColumns - 1);
                tileXEnd = clamp(tileXEnd, 0, TileColumns - 1);
                tileYStart = clamp(tileYStart, 0, TileColumns - 1);
                tileYEnd = clamp(tileYEnd, 0, TileColumns - 1);

                double sampleTotal = r + g + b;
                for (int tileY = tileYStart; tileY < tileYEnd + 1; ++tileY) {
                    double tileTop = tileY * tileHeight;
                    double tileBottom = tileTop + tileHeight;
                    double overlapY = min(srcBottom, tileBottom) - max(srcTop, tileTop);
                    if (overlapY <= 0) continue;

                    for (int tileX = tileXStart; tileX < tileXEnd + 1; ++tileX) {
                        double tileLeft = tileX * tileWidth;
                        double tileRight = tileLeft + tileWidth;
                        double overlapX = min(srcRight, tileRight) - max(srcLeft, tileLeft);
                        if (overlapX <= 0) continue;

                        double overlapArea = overlapX * overlapY;
                        if (overlapArea <= 0) continue;

                        double fraction = overlapArea / pixelArea;
                        if (fraction <= 0) continue;

                        size_t tileIndex = static_cast<size_t>(tileY) * TileColumns + tileX;
                        tileTotals[tileIndex] += sampleTotal * fraction;
                        tileCounts[tileIndex] += weight * fraction;
                    }
                }
            }
        }

        if (totalAlpha <= 0) {
            totalAlpha = static_cast<double>(pendingSampleCount);
            for (auto& count : tileCounts) if (count == 0) count = 1.0;
        }

        result.red = totalRed;
        result.green = totalGreen;
        result.blue = totalBlue;
        result.alpha = totalAlpha;
        result.sampleCount = static_cast<uint32_t>(totalAlpha);
        result.tileTotals = tileTotals;
        result.tileCounts = tileCounts;

        resultReady = false;
        return true;
    }

    ~DifferenceEvaluator() {
        destroyResources();
        destroyPrograms();
    }
};

thread_local DifferenceEvaluator gDifferenceEvaluator;

 void oglSetDifferenceAggregationEnabled(bool enabled) {
    gDifferenceEvaluator.setEnabled(enabled);
}

 bool oglIsDifferenceAggregationEnabled() {
    return gDifferenceEvaluator.isEnabled();
}

 void oglSetDifferenceAggregationRegion(DifferenceEvaluationRegion region) {
    gDifferenceEvaluator.setRegion(region);
}

 DifferenceEvaluationRegion oglGetDifferenceAggregationRegion() {
    return gDifferenceEvaluator.getRegion();
}

 bool oglEvaluateDifferenceAggregation(GLuint texture, int viewportWidth, int viewportHeight) {
    return gDifferenceEvaluator.evaluate(static_cast<GLuint>(texture), viewportWidth, viewportHeight);
}

 bool oglFetchDifferenceAggregationResult(DifferenceEvaluationResult& result) {
    return gDifferenceEvaluator.fetch(result);
}

GLuint compileShader(GLenum stage, const std::string& source) {
    GLuint shader = glCreateShader(stage);
    auto ptr = source.c_str();
    glShaderSource(shader, 1, &ptr, nullptr);
    glCompileShader(shader);

    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_TRUE) {
        return shader;
    }

    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string message = "unknown";
    if (length > 0) {
        std::string buffer(length, '\0');
        glGetShaderInfoLog(shader, length, nullptr, buffer.data());
        message = buffer;
    }

    glDeleteShader(shader);
    throw std::runtime_error("Failed to compile shader: " + message);
}

GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_TRUE) {
        return program;
    }

    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    std::string message = "unknown";
    if (length > 0) {
        std::string buffer(length, '\0');
        glGetProgramInfoLog(program, length, nullptr, buffer.data());
        message = buffer;
    }
    glDeleteProgram(program);
    throw std::runtime_error("Failed to link shader program: " + message);
}

}
