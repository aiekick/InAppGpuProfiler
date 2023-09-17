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

/* Simple quad Vfx
 * Quad Mesh
 * FBO with one attachment
 * One vertex Shader quad
 * One fragment Shader
 */

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
    std::string m_Name;
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
        m_Name = vName;
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
    void addUniform(const GLenum& vShaderType, const std::string& vUniformName, float* vUniformPtr, const bool& vShowWidget) {
        m_ProgramPtr->addUniform(vShaderType, vUniformName, vUniformPtr, vShowWidget);
    }
    void finalizeBeforeRendering() {
        assert(m_ProgramPtr != nullptr);
        m_ProgramPtr->locateUniforms();
    }
    bool resize(const float& vSx, const float vSy) {
        assert(m_FboPtr != nullptr);
        return m_FboPtr->resize(vSx, vSy);
    }

    void declareViewPort() {
        AIGPScoped(m_Name, "Viewport");
        glViewport(0, 0, m_SizeX, m_SizeY);
    }

    void render() {
        AIGPScoped(m_Name, "Render quad %s", m_Name.c_str());
        auto quad_ptr = m_QuadMesh.lock();
        assert(quad_ptr != nullptr);
        assert(m_FboPtr != nullptr);
        assert(m_ProgramPtr != nullptr);
        if (m_FboPtr->bind()) {
            if (m_ProgramPtr->use()) {
                m_ProgramPtr->uploadUniforms();
                m_FboPtr->selectBuffers();
                m_FboPtr->clearColorAttachments();
                declareViewPort();
                quad_ptr->render(GL_TRIANGLES);
                m_FboPtr->updateMipMaping();
                m_ProgramPtr->unuse();
            }
            m_FboPtr->unbind();
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
    void drawUniformWidgets() {
        assert(m_ProgramPtr != nullptr);
        m_ProgramPtr->drawUniformWidgets();
    }
    void unit() {
        m_ProgramPtr.reset();
        m_FragShaderPtr.reset();
        m_FboPtr.reset();
    }
};

}  // namespace glApi
