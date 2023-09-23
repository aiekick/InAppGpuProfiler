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
#include <vector>
#include <memory>

namespace glApi {

class FBO;
typedef std::shared_ptr<FBO> FBOPtr;
typedef std::weak_ptr<FBO> FBOWeak;

class FBO {
private:
    FBOWeak m_This;
    GLuint m_FBOId = 0U;
    GLsizei m_SizeX = 0;
    GLsizei m_SizeY = 0;
    GLuint m_CountBuffers = 0U;
    bool m_UseMipMapping = false;
    std::vector<TexturePtr> m_Textures;
    GLenum* m_ColorDrawBuffers = nullptr;

public:
    static FBOPtr create(const GLsizei& vSX, const GLsizei& vSY, const GLuint& vCountBuffers, const bool& vUseMipMapping) {
        auto res = std::make_shared<FBO>();
        res->m_This = res;
        if (!res->init(vSX, vSY, vCountBuffers, vUseMipMapping)) {
            res.reset();
        }
        return res;
    }

public:
    FBO() = default;
    virtual ~FBO() {
        unit();
    }

    bool init(const GLsizei& vSX, const GLsizei& vSY, const GLuint& vCountBuffers, const bool& vUseMipMapping) {
        bool res = false;
        m_SizeX = vSX;
        m_SizeY = vSY;
        m_CountBuffers = vCountBuffers;
        m_UseMipMapping = vUseMipMapping;
        if (m_CountBuffers > 0U) {
            glGenFramebuffers(1, &m_FBOId);
            CheckGLErrors;
            glBindFramebuffer(GL_FRAMEBUFFER, m_FBOId);
            CheckGLErrors;
            m_Textures.resize(m_CountBuffers);
            m_ColorDrawBuffers = new GLenum[m_CountBuffers];
            for (GLuint idx = 0U; idx < vCountBuffers; ++idx) {
                m_Textures[idx] = Texture::createEmpty(vSX, vSY, vUseMipMapping);
                if (m_Textures[idx] != nullptr) {
                    m_ColorDrawBuffers[idx] = GL_COLOR_ATTACHMENT0 + (GLenum)idx;
                    glFramebufferTexture2D(GL_FRAMEBUFFER, m_ColorDrawBuffers[idx], GL_TEXTURE_2D, m_Textures[idx]->getTexId(), 0);
                    CheckGLErrors;
                }
            }
            glFinish();
            res = check();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            CheckGLErrors;
        }
        return res;
    }

    bool bind() {
        AIGPScoped("FBO", "bind");
        if (m_FBOId > 0) {
            glBindFramebuffer(GL_FRAMEBUFFER, m_FBOId);
            CheckGLErrors;
            return true;
        }
        return false;
    }

    void clearBuffer(const std::array<float, 4U>& vColor) {
        AIGPScoped("FBO", "clearBuffer");
        if (bind()) {
            glClearColor(vColor[0], vColor[1], vColor[2], vColor[3]); 
            glClear(GL_COLOR_BUFFER_BIT);
            unbind();        
        }
    }

    void updateMipMaping() {
        if (m_UseMipMapping) {
            AIGPScoped("FBO", "updateMipMaping %u", m_FBOId);
            for (auto& tex_ptr : m_Textures) {
                if (tex_ptr != nullptr) {
                    tex_ptr->updateMipMaping();
                }
            }         
        }
    }
    
    void selectBuffers() {
        AIGPScoped("FBO", "glDrawBuffers");
        glDrawBuffers(m_CountBuffers, m_ColorDrawBuffers);
        CheckGLErrors;
    }

    void unbind() {
        AIGPScoped("FBO", "unbind");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        CheckGLErrors;
        glBindTexture(GL_TEXTURE_2D, 0);
        CheckGLErrors;
    }

    GLuint getTextureId(const size_t& vBufferIdx = 0U) {
        if (m_Textures.size() > vBufferIdx) {
            return m_Textures[vBufferIdx]->getTexId();
        }
        return 0U;
    }
    
    bool resize(const GLsizei& vNewSx, const GLsizei& vNewSy) {
        bool res = false;
        if (m_FBOId > 0) {
            glBindFramebuffer(GL_FRAMEBUFFER, m_FBOId);
            CheckGLErrors;
            for (GLuint idx = 0U; idx < m_CountBuffers; ++idx) {
                if (m_Textures[idx] != nullptr) {
                    if (m_Textures[idx]->resize(vNewSx, vNewSy)) {
                        if (m_Textures[idx] != nullptr) {
                            m_ColorDrawBuffers[idx] = GL_COLOR_ATTACHMENT0 + (GLenum)idx;
                            glFramebufferTexture2D(GL_FRAMEBUFFER, m_ColorDrawBuffers[idx], GL_TEXTURE_2D, m_Textures[idx]->getTexId(), 0);
                            CheckGLErrors;
                        }
                    }
                }
            }
            glFinish();
            res = check();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            CheckGLErrors;
        }
        return res;
    }

    bool check() {
        if (glIsFramebuffer(m_FBOId) == GL_TRUE) {
            CheckGLErrors;
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
                CheckGLErrors;
                return true;
            }
        }
        return false;
    }

    void unit() {
        glDeleteFramebuffers(1, &m_FBOId);
        CheckGLErrors;
    }
};

/*
* add front and back FBO
* and switch between front and back after rendering
*/

class FBOPipeLine;
typedef std::shared_ptr<FBOPipeLine> FBOPipeLinePtr;
typedef std::weak_ptr<FBOPipeLine> FBOPipeLineWeak;
class FBOPipeLine {
private:
    FBOPipeLineWeak m_This;
    FBOPtr m_FrontFBOPtr = nullptr;
    FBOPtr m_BackFBOPtr = nullptr;
    bool m_MultiPass = false;

public:
    static FBOPipeLinePtr create(const GLsizei& vSX, const GLsizei& vSY, const GLuint& vCountBuffers, const bool& vUseMipMapping, const bool& vMultiPass) {
        auto res = std::make_shared<FBOPipeLine>();
        res->m_This = res;
        if (!res->init(vSX, vSY, vCountBuffers, vUseMipMapping, vMultiPass)) {
            res.reset();
        }
        return res;
    }

public:
    FBOPipeLine() = default;
    virtual ~FBOPipeLine() {
        unit();
    }
    bool init(const GLsizei& vSX, const GLsizei& vSY, const GLuint& vCountBuffers, const bool& vUseMipMapping, const bool& vMultiPass) {
        bool res = true;
        m_MultiPass = vMultiPass;
        m_FrontFBOPtr = FBO::create(vSX, vSY, vCountBuffers, vUseMipMapping);
        if (m_FrontFBOPtr != nullptr) {
            if (m_MultiPass) {
                m_BackFBOPtr = FBO::create(vSX, vSY, vCountBuffers, vUseMipMapping);
                if (m_BackFBOPtr != nullptr) {
                    res = true;
                }
            } else {
                res = true;
            }
        }
        return res;
    }
    bool resize(const GLsizei& vNewSx, const GLsizei& vNewSy) {
        bool res = false;
        assert(m_FrontFBOPtr != nullptr);
        res = m_FrontFBOPtr->resize(vNewSx, vNewSy);
        if (m_MultiPass) {
            assert(m_BackFBOPtr != nullptr);
            res &= m_BackFBOPtr->resize(vNewSx, vNewSy);
        }
        return res;
    }
    void unit() {
        m_FrontFBOPtr.reset();
        m_BackFBOPtr.reset();
    }
    bool bind() {
        assert(m_FrontFBOPtr != nullptr);
        return m_FrontFBOPtr->bind();
    }
    void clearBuffer(const std::array<float, 4U>& vColor) {
        assert(m_FrontFBOPtr != nullptr);
        m_FrontFBOPtr->clearBuffer(vColor);
        if (m_MultiPass) {
            assert(m_BackFBOPtr != nullptr);
            m_BackFBOPtr->clearBuffer(vColor);
        }
    }
    void updateMipMaping() {
        assert(m_FrontFBOPtr != nullptr);
        m_FrontFBOPtr->updateMipMaping();
    }
    void selectBuffers() {
        assert(m_FrontFBOPtr != nullptr);
        m_FrontFBOPtr->selectBuffers();
    }
    void unbind() {
        assert(m_FrontFBOPtr != nullptr);
        m_FrontFBOPtr->unbind();
        if (m_MultiPass) {
            swapFBOs();
        }

    }
    GLuint getFrontTextureId(const size_t& vBufferIdx = 0U) {
        assert(m_FrontFBOPtr != nullptr);
        return m_FrontFBOPtr->getTextureId(vBufferIdx);
    }
    GLuint getBackTextureId(const size_t& vBufferIdx = 0U) {
        assert(m_MultiPass);
        assert(m_BackFBOPtr != nullptr);
        return m_BackFBOPtr->getTextureId(vBufferIdx);
    }
    FBOWeak getFrontFBO() {
        return m_FrontFBOPtr;
    }
    FBOWeak getBackFBO() {
        assert(m_MultiPass);
        return m_BackFBOPtr;
    }
    void swapFBOs() {
        assert(m_MultiPass);
        FBOPtr tmp = m_BackFBOPtr;
        m_BackFBOPtr = m_FrontFBOPtr;
        m_FrontFBOPtr = tmp;
    }
};
}  // namespace glApi