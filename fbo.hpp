#pragma once

#include <glad/glad.h>
#include <vector>
#include <memory>

#include <texture.hpp>

namespace glApi {

class FBO;
typedef std::shared_ptr<FBO> FBOPtr;

class FBO {
private:
    GLuint m_FBOId = 0U;
    GLsizei m_SizeX = 0;
    GLsizei m_SizeY = 0;
    GLuint m_CountBuffers = 0U;
    std::vector<TexturePtr> m_Textures;
    GLenum* m_ColorDrawBuffers = nullptr;

public:
    static FBOPtr create(const GLsizei& vSX, const GLsizei& vSY, const GLuint& vCountBuffers) {

    }

public:
    FBO() = default;
    ~FBO() {
        unit();
    }

    bool init(const GLsizei& vSX, const GLsizei& vSY, const GLuint& vCountBuffers) {
        bool res = false;
        m_SizeX = vSX;
        m_SizeY = vSY;
        m_CountBuffers = vCountBuffers;
        if (m_CountBuffers > 0U) {
            glGenFramebuffers(1, &m_FBOId);
            glBindFramebuffer(GL_FRAMEBUFFER, m_FBOId);
            m_Textures.resize(m_CountBuffers);
            m_ColorDrawBuffers = new GLenum[m_CountBuffers];
            for (GLuint idx = 0U; idx < vCountBuffers; ++idx) {
                m_Textures[idx] = Texture::createEmpty(vSX, vSY, true);
                if (m_Textures[idx] != nullptr) {
                    m_ColorDrawBuffers[idx] = GL_COLOR_ATTACHMENT0 + (GLenum)idx;
                    glFramebufferTexture2D(GL_FRAMEBUFFER, m_ColorDrawBuffers[idx], GL_TEXTURE_2D, m_Textures[idx]->getTexId(), 0);
                }
            }
            glFinish();
            res = check();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        return res;
    }

    bool bind() {
        if (m_FBOId > 0) {
            glBindFramebuffer(GL_FRAMEBUFFER, m_FBOId);
            return true;
        }
        return false;
    }

    void clearColorAttachments() {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void updateMipMaping() {
        for (auto tex_ptr : m_Textures) {
            if (tex_ptr != nullptr) {
                tex_ptr->updateMipMaping();
            }
        }         
    }
    
    void selectBuffers() {
        glDrawBuffers(m_CountBuffers, m_ColorDrawBuffers);
    }

    bool resize(const GLsizei& vNewSx, const GLsizei& vNewSy) {
        bool res = false;
        if (m_FBOId > 0) {
            glBindFramebuffer(GL_FRAMEBUFFER, m_FBOId);
            for (GLuint idx = 0U; idx < m_CountBuffers; ++idx) {
                if (m_Textures[idx] != nullptr) {
                    if (m_Textures[idx]->resize(vNewSx, vNewSy)) {
                        if (m_Textures[idx] != nullptr) {
                            m_ColorDrawBuffers[idx] = GL_COLOR_ATTACHMENT0 + (GLenum)idx;
                            glFramebufferTexture2D(GL_FRAMEBUFFER, m_ColorDrawBuffers[idx], GL_TEXTURE_2D, m_Textures[idx]->getTexId(), 0);
                        }
                    }
                }
            }
            glFinish();
            res = check();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        return res;
    }

    bool check() {
        if (glIsFramebuffer(m_FBOId) == GL_TRUE) {
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
                return true;
            }
        }
        return false;
    }

    void unbind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void unit() {
        glDeleteFramebuffers(1, &m_FBOId);
    }
};

}  // namespace glApi