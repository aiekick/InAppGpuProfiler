#pragma once

#include <glad/glad.h>
#include <memory>

namespace glApi {

class Texture;
typedef std::shared_ptr<Texture> TexturePtr;

class Texture {
private:
    GLuint m_TexId = 0U;

public:
    static TexturePtr createEmpty(const GLsizei& vSx, const GLsizei& vSy, const bool& vEnableMipMap);
    static TexturePtr createDepth(const GLsizei& vSx, const GLsizei& vSy, const bool& vEnableMipMap);

public:
    Texture() = default;
    ~Texture() {
        unit();
    }

    bool initEmpty(const GLsizei& vSx, const GLsizei& vSy, const bool& vEnableMipMap) {
        glGenTextures(1, &m_TexId);
        glBindTexture(GL_TEXTURE_2D, m_TexId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, vSx, vSy, 0, GL_RGBA, GL_FLOAT, nullptr);
        glFinish();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        if (vEnableMipMap) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 8U);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            updateMipMaping();
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        return check();
    }
    
    void updateMipMaping() {
        glGenerateMipmap(GL_TEXTURE_2D);
        glFinish();
    }

    bool resize(const GLsizei& vSx, const GLsizei& vSy) {
        if (m_TexId > 0U) {
            glBindTexture(GL_TEXTURE_2D, m_TexId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, vSx, vSy, 0, GL_RGBA, GL_FLOAT, nullptr);
            glFinish();
            glBindTexture(GL_TEXTURE_2D, 0);
            return true;
        }
        return false;
    }
    
    void unit() {
        glDeleteTextures(1, &m_TexId);
    }

    bool check() {
        return (glIsTexture(m_TexId) == GL_TRUE);
    }

    GLuint getTexId() {
        return m_TexId;
    }
};

}  // namespace glApi