/*
MIT License

Copyright (c) 2023-2023 Stephane Cuillerdier (aka aiekick)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include "glApi.hpp"
#include <memory>

namespace glApi {

class Texture;
typedef std::shared_ptr<Texture> TexturePtr;
typedef std::weak_ptr<Texture> TextureWeak;

class Texture {
private:
    TextureWeak m_This;
    GLuint m_TexId = 0U;
    bool m_EnableMipMap = false;

public:
    static TexturePtr createEmpty(const GLsizei& vSx, const GLsizei& vSy, const bool& vEnableMipMap) {
        auto res = std::make_shared<Texture>();
        res->m_This = res;
        if (!res->initEmpty(vSx, vSy, vEnableMipMap)) {
            res.reset();
        }
        return res;
    }

public:
    Texture() = default;
    ~Texture() {
        unit();
    }

    bool initEmpty(const GLsizei& vSx, const GLsizei& vSy, const bool& vEnableMipMap) {
        assert(vSx > 0);
        assert(vSy > 0);
        m_EnableMipMap = vEnableMipMap;
        glGenTextures(1, &m_TexId);
        CheckGLErrors;
        glBindTexture(GL_TEXTURE_2D, m_TexId);
        CheckGLErrors;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, vSx, vSy, 0, GL_RGBA, GL_FLOAT, nullptr);
        CheckGLErrors;
        glFinish();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        CheckGLErrors;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        CheckGLErrors;
        if (m_EnableMipMap) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            CheckGLErrors;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 8U);
            CheckGLErrors;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            CheckGLErrors;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            CheckGLErrors;
            updateMipMaping();
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            CheckGLErrors;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            CheckGLErrors;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            CheckGLErrors;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            CheckGLErrors;
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        CheckGLErrors;
        return check();
    }

    void updateMipMaping() {
        if (m_EnableMipMap) {
            AIGPScoped("Opengl", "glGenerateMipmap %u", m_TexId);
            glGenerateMipmap(GL_TEXTURE_2D);
            CheckGLErrors;
            glFinish();
        }
    }

    bool resize(const GLsizei& vSx, const GLsizei& vSy) {
        if (m_TexId > 0U) {
            glBindTexture(GL_TEXTURE_2D, m_TexId);
            CheckGLErrors;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, vSx, vSy, 0, GL_RGBA, GL_FLOAT, nullptr);
            CheckGLErrors;
            glFinish();
            glBindTexture(GL_TEXTURE_2D, 0);
            CheckGLErrors;
            return true;
        }
        return false;
    }
    
    void unit() {
        glDeleteTextures(1, &m_TexId);
        CheckGLErrors;
    }

    bool check() {
        return (glIsTexture(m_TexId) == GL_TRUE);
    }

    GLuint getTexId() {
        return m_TexId;
    }
};

}  // namespace glApi