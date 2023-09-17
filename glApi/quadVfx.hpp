#pragma once

/* Simple quad Vfx
 * Quad Mesh
 * FBO with one attachment
 * One vertex Shader quad
 * One fragment Shader
 */

#include <glad/glad.h>
#include "glApi.hpp"
#include <imgui.h>

#include <memory>
#include <cassert>

namespace glApi {

class QuadVfx;
typedef std::shared_ptr<QuadVfx> QuadVfxPtr;
typedef std::weak_ptr<QuadVfx> QuadVfxWeak;

class QuadVfx {
private:
    QuadVfxWeak m_This;
    QuadMeshWeak m_QuadMesh;
    ShaderWeak m_VertShader;
    FBOPtr m_FboPtr = nullptr;
    ShaderPtr m_FragShaderPtr = nullptr;
    ProgramPtr m_ProgramPtr = nullptr;
    GLsizei m_SizeX = 0;
    GLsizei m_SizeY = 0;

public:
    static QuadVfxPtr create(             //
        const std::string& vName,         //
        ShaderWeak vVertShader,           //
        QuadMeshWeak vQuadMesh,           //
        const std::string& vFragFile,     //
        const GLsizei& vSx,               //
        const GLsizei& vSy,               //
        const uint32_t& vCountBuffers) {  //
        auto res = std::make_shared<QuadVfx>();
        res->m_This = res;
        if (!res->init(vName, vVertShader, vQuadMesh, vFragFile, vSx, vSy, vCountBuffers)) {
            res.reset();
        }
        return res;
    }

public:
    QuadVfx() = default;
    ~QuadVfx() {
        unit();
    }

    bool init(                            //
        const std::string& vName,         //
        ShaderWeak vVertShader,           //
        QuadMeshWeak vQuadMesh,           //
        const std::string& vFragFile,     //
        const GLsizei& vSx,               //
        const GLsizei& vSy,               //
        const uint32_t& vCountBuffers) {  //
        assert(!vVertShader.expired());
        assert(!vQuadMesh.expired());
        assert(!vFragFile.empty());
        assert(vSx > 0U);
        assert(vSy > 0U);
        assert(vCountBuffers > 0U);
        m_VertShader = vVertShader;
        m_QuadMesh = vQuadMesh;
        m_SizeX = vSx;
        m_SizeY = vSy;
        m_FboPtr = FBO::create(vSx, vSy, vCountBuffers);
        if (m_FboPtr != nullptr) {
            m_FragShaderPtr = glApi::Shader::createFromFile(vName, GL_FRAGMENT_SHADER, vFragFile);
            if (m_FragShaderPtr != nullptr) {
                m_ProgramPtr = glApi::Program::create(vName);
                if (m_ProgramPtr != nullptr) {
                    if (m_ProgramPtr->addShader(m_VertShader)) {
                        if (m_ProgramPtr->addShader(m_FragShaderPtr)) {
                            return m_ProgramPtr->link();
                        }
                    }
                }
            }
        }
        return false;
    }

    bool resize(const float& vSx, const float vSy) {
        assert(m_FboPtr != nullptr);
        return m_FboPtr->resize(vSx, vSy);
    }

    void render() {
        auto quad_ptr = m_QuadMesh.lock();
        if (quad_ptr != nullptr) {
            if (m_FboPtr != nullptr) {
                if (m_ProgramPtr != nullptr) {
                    if (m_FboPtr->bind()) {
                        if (m_ProgramPtr->use()) {
                            m_FboPtr->selectBuffers();
                            quad_ptr->render(GL_TRIANGLES);
                            m_FboPtr->updateMipMaping();
                            m_ProgramPtr->unuse();
                        }
                        m_FboPtr->unbind();
                    }
                }
            }
        }
    }

    void drawImGuiThumbnail() {
        if (m_FboPtr != nullptr) {
            const auto texId = m_FboPtr->getTextureId();
            if (texId > 0U) {
                ImGui::Image((ImTextureID)texId, ImVec2((float)m_SizeX, (float)m_SizeY));
            }
        }
    }

    void unit() {
        m_ProgramPtr.reset();
        m_FragShaderPtr.reset();
        m_FboPtr.reset();
    }
};

}  // namespace glApi
