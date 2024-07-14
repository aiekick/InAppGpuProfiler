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

/* Simple mesh Vfx
 * FBO with one attachment
 * One vertex Shader quad
 * One fragment Shader
 * One tesselation control Shader
 * One tesselation eval Shader
 */

#include "glApi.hpp"

#include <array>
#include <memory>
#include <cassert>
#include <functional>

namespace glApi {

class MeshVfx;
typedef std::shared_ptr<MeshVfx> MeshVfxPtr;
typedef std::weak_ptr<MeshVfx> MeshVfxWeak;

class MeshVfx {
private:
    MeshVfxWeak m_This;
    std::string m_Name;
    ProcMeshWeak m_ProcMesh;
    FBOPipeLinePtr m_FBOPipeLinePtr = nullptr;
    ShaderPtr m_VertShaderPtr = nullptr;
    ShaderPtr m_FragShaderPtr = nullptr;
    ProgramPtr m_ProgramPtr = nullptr;
    std::array<GLuint, 2U> m_Size;
    bool m_UseMipMapping = false;
    bool m_MultiPass = false;
    GLuint m_RenderIterations = 1U;
    bool m_RenderingPause = false;

public:
    static MeshVfxPtr create(           //
        const std::string& vName,       //
        ProcMeshWeak vProcMesh,         //
        const std::string& vVertFile,   //
        const std::string& vFragFile,   //
        const GLsizei& vSx,             //
        const GLsizei& vSy,             //
        const uint32_t& vCountBuffers,  //
        const bool& vUseMipMapping,     //
        const bool& vMultiPass) {       //
        auto res = std::make_shared<MeshVfx>();
        res->m_This = res;
        if (!res->init(vName, vProcMesh, vVertFile, vFragFile, vSx, vSy, vCountBuffers, vUseMipMapping, vMultiPass)) {
            res.reset();
        }
        return res;
    }

public:
    MeshVfx() = default;
    ~MeshVfx() {
        unit();
    }

    bool init(                          //
        const std::string& vName,       //
        ProcMeshWeak vProcMesh,         //
        const std::string& vVertFile,   //
        const std::string& vFragFile,   //
        const GLsizei& vSx,             //
        const GLsizei& vSy,             //
        const uint32_t& vCountBuffers,  //
        const bool& vUseMipMapping,     //
        const bool& vMultiPass) {       //
        assert(!vProcMesh.expired());
        assert(!vVertFile.empty());
        assert(!vFragFile.empty());
        assert(vSx > 0U);
        assert(vSy > 0U);
        assert(vCountBuffers > 0U);
        m_Name = vName;
        m_ProcMesh = vProcMesh;
        m_Size[0] = vSx;
        m_Size[1] = vSy;
        m_UseMipMapping = vUseMipMapping;
        m_MultiPass = vMultiPass;
        m_FBOPipeLinePtr = FBOPipeLine::create(vSx, vSy, vCountBuffers, vUseMipMapping, m_MultiPass);
        if (m_FBOPipeLinePtr != nullptr) {
            m_VertShaderPtr = glApi::Shader::createFromFile(vName, GL_VERTEX_SHADER, vVertFile);
            if (m_VertShaderPtr != nullptr) {
                m_FragShaderPtr = glApi::Shader::createFromFile(vName, GL_FRAGMENT_SHADER, vFragFile);
                if (m_FragShaderPtr != nullptr) {
                    m_ProgramPtr = glApi::Program::create(vName);
                    if (m_ProgramPtr != nullptr) {
                        if (m_ProgramPtr->addShader(m_VertShaderPtr)) {
                            if (m_ProgramPtr->addShader(m_FragShaderPtr)) {
                                return m_ProgramPtr->link();
                            }
                        }
                    }
                }
            }
        }
        return false;
    }
    const char* getLabelName() {
        return m_Name.c_str();
    }
    const std::array<GLuint, 2U>& getSize() {
        return m_Size;
    }
    void setRenderingIterations(const GLuint& vRenderingIterations) {
        m_RenderIterations = vRenderingIterations;
    }
    GLuint& getRenderingIterationsRef() {
        return m_RenderIterations;
    }
    void setRenderingPause(const bool& vRenderingPause) {
        m_RenderingPause = vRenderingPause;
    }
    bool& getRenderingPauseRef() {
        return m_RenderingPause;
    }
    void setUniformPreUploadFunctor(Program::UniformPreUploadFunctor vUniformPreUploadFunctor) {
        assert(m_ProgramPtr != nullptr);
        m_ProgramPtr->setUniformPreUploadFunctor(vUniformPreUploadFunctor);
    }
    void addUniformFloat(                 //
        const GLenum& vShaderType,        //
        const std::string& vUniformName,  //
        float* vUniformPtr,               //
        const GLuint& vCountChannels,     //
        const bool& vShowWidget,          //
        const Program::UniformWidgetFunctor& vWidgetFunctor) {
        assert(m_ProgramPtr != nullptr);
        m_ProgramPtr->addUniformFloat(vShaderType, vUniformName, vUniformPtr, vCountChannels, vShowWidget, vWidgetFunctor);
    }
    void addUniformInt(                   //
        const GLenum& vShaderType,        //
        const std::string& vUniformName,  //
        int32_t* vUniformPtr,             //
        const GLuint& vCountChannels,     //
        const bool& vShowWidget,          //
        const Program::UniformWidgetFunctor& vWidgetFunctor) {
        assert(m_ProgramPtr != nullptr);
        m_ProgramPtr->addUniformInt(vShaderType, vUniformName, vUniformPtr, vCountChannels, vShowWidget, vWidgetFunctor);
    }
    void addUniformSampler2D(             //
        const GLenum& vShaderType,        //
        const std::string& vUniformName,  //
        int32_t vSampler2D,               //
        const bool& vShowWidget) {
        assert(m_ProgramPtr != nullptr);
        m_ProgramPtr->addUniformSampler2D(vShaderType, vUniformName, vSampler2D);
    }
    void finalizeBeforeRendering() {
        assert(m_ProgramPtr != nullptr);
        m_ProgramPtr->locateUniforms();
    }
    bool resize(const GLsizei& vSx, const GLsizei vSy) {
        assert(m_FBOPipeLinePtr != nullptr);
        if (m_FBOPipeLinePtr->resize(vSx, vSy)) {
            m_Size[0] = vSx;
            m_Size[1] = vSy;
            return true;
        }
        return false;
    }
    void clearBuffers(const std::array<float, 4U>& vColor) {
        assert(m_FBOPipeLinePtr != nullptr);
        m_FBOPipeLinePtr->clearBuffer(vColor);
    }
    void render() {
        if (m_RenderingPause) {
            return;
        }
        AIGPScoped("Mesh VFX", "Render %s", m_Name.c_str());
        auto quad_ptr = m_ProcMesh.lock();
        assert(quad_ptr != nullptr);
        assert(m_FBOPipeLinePtr != nullptr);
        assert(m_ProgramPtr != nullptr);
        for (GLuint idx = 0; idx < m_RenderIterations; ++idx) {
            AIGPScoped("Mesh VFX", "Iter %i", idx);
            if (m_FBOPipeLinePtr->bind()) {
                if (m_ProgramPtr->use()) {
                    m_ProgramPtr->uploadUniforms(m_FBOPipeLinePtr);
                    m_FBOPipeLinePtr->selectBuffers();
                    {
                        AIGPScoped("Opengl", "glViewport");
                        glViewport(0, 0, m_Size[0], m_Size[1]);
                    }
                    quad_ptr->render(GL_TRIANGLES);
                    m_FBOPipeLinePtr->updateMipMaping();
                    m_ProgramPtr->unuse();
                }
                m_FBOPipeLinePtr->unbind();
            }
        }
    }
    GLuint getTextureId() {
        assert(m_FBOPipeLinePtr != nullptr);
        auto front_fbo_ptr = m_FBOPipeLinePtr->getFrontFBO().lock();
        if (front_fbo_ptr != nullptr) {
            return front_fbo_ptr->getTextureId();
        }    
        return 0U;
    }
    void drawImGuiThumbnail(const float& vSx, const float& vSy, const float& vScaleInv) {
        assert(m_FBOPipeLinePtr != nullptr);
        auto front_fbo_ptr = m_FBOPipeLinePtr->getFrontFBO().lock();
        if (front_fbo_ptr != nullptr) {
            const auto texId = front_fbo_ptr->getTextureId();
            if (texId > 0U) {
                ImGui::ImageButton(m_Name.c_str(), (ImTextureID)(size_t)texId, ImVec2(vSx, vSy), ImVec2(0, vScaleInv), ImVec2(vScaleInv, 0));
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
        m_FBOPipeLinePtr.reset();
    }
};

}  // namespace glApi
