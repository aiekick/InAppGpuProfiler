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

template <typename T>
class Mesh {
private:
    std::weak_ptr<Mesh<T>> m_This;
    GLuint m_VboId = 0U;
    GLuint m_IboId = 0U;
    GLuint m_VaoId = 0U;

    std::vector<T> m_Vertices;
    std::vector<uint32_t> m_Indices;
    std::vector<uint32_t> m_Format;

    const GLsizei m_VerticeSize = sizeof(T);
    const GLsizei m_IndiceSize = sizeof(uint32_t);

public:
    static std::shared_ptr<Mesh<T>> create(std::vector<T> vVertices, std::vector<uint32_t> vIndices) {
        auto res = std::make_shared<Mesh<T>>();
        res->m_This = res;
        if (!res->init(vVertices, vIndices)) {
            res.reset();
        }
        return res;
    }

public:
    Mesh() = default;
    virtual ~Mesh() {
        unit();
    }
    uint32_t GetVaoID() {
        return m_VboId;
    }
    uint32_t GetVboID() {
        return m_IboId;
    }
    uint32_t GetIboID() {
        return m_VaoId;
    }
    bool init(std::vector<T> vVertices, std::vector<uint32_t> vIndices, std::vector<uint32_t> vFormat) {
        assert(!vVertices.empty());
        assert(!vIndices.empty());
        assert(!vFormat.empty());
        m_Vertices = vVertices;
        m_Indices = vIndices;
        m_Format = vFormat;
        glGenVertexArrays(1, &m_VaoId);
        CheckGLErrors;
        glGenBuffers(1, &m_VboId);
        CheckGLErrors;
        glGenBuffers(1, &m_IboId);
        CheckGLErrors;

        // bind
        glBindVertexArray(m_VaoId);
        CheckGLErrors;
        glBindBuffer(GL_ARRAY_BUFFER, m_VboId);
        CheckGLErrors;
        glBufferData(GL_ARRAY_BUFFER, m_VerticeSize * m_Vertices.size(), m_Vertices.data(), GL_STATIC_DRAW);
        CheckGLErrors;

        // vertices

        GLuint idx = 0U;
        GLuint offset = 0U;
        for (const auto& format : m_Format) {
            assert(format > 0U);
            glEnableVertexAttribArray(idx);
            CheckGLErrors;
            if (idx == 0U) {
                glVertexAttribPointer(idx, format, GL_FLOAT, GL_FALSE, m_VerticeSize, (void*)nullptr);
            } else {
                glVertexAttribPointer(idx, format, GL_FLOAT, GL_FALSE, m_VerticeSize, (void*)(sizeof(float) * offset));
            }
            CheckGLErrors;
            glDisableVertexAttribArray(idx);
            CheckGLErrors;
            offset += format;
            ++idx;
        }

        // indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IboId);
        CheckGLErrors;
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_IndiceSize * m_Indices.size(), m_Indices.data(), GL_STATIC_DRAW);
        CheckGLErrors;

        // unbind
        glBindVertexArray(0);
        CheckGLErrors;
        // if i not unbind the VBOs and IBOs after the unbind of the VAO, it seem the VAO is corrupted..
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        CheckGLErrors;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        CheckGLErrors;

        return (glIsVertexArray(m_VaoId) == GL_TRUE);
    }

    bool bind() {
        if (glIsVertexArray(m_VaoId) == GL_TRUE) {
            CheckGLErrors;
            glBindVertexArray(m_VaoId);
            CheckGLErrors;
            for (size_t idx = 0U; idx < m_Format.size(); ++idx) {
                glEnableVertexAttribArray((GLuint)idx);
                CheckGLErrors;
            }
            return true;
        }
        return false;
    }

    void unbind() {
        for (size_t idx = 0U; idx < m_Format.size(); ++idx) {
            glDisableVertexAttribArray((GLuint)idx);
            CheckGLErrors;
        }
        glBindVertexArray(0);
    }

    void render(GLenum vRenderMode) {
        AIGPScoped("Mesh", "render");
        if (bind()) {
            {
                AIGPScoped("Opengl", "glDrawElements");
                glDrawElements(vRenderMode, (GLsizei)m_Indices.size(), GL_UNSIGNED_INT, nullptr);
            }
            CheckGLErrors;
            unbind();
        }
    }

    void unit() {
        if (m_VaoId > 0) {
            glDeleteVertexArrays(1, &m_VaoId);
            CheckGLErrors;
            m_VaoId = 0U;
        }
        if (m_VboId > 0) {
            glDeleteBuffers(1, &m_VboId);
            CheckGLErrors;
            m_VboId = 0U;
        }
        if (m_IboId > 0) {
            glDeleteBuffers(1, &m_IboId);
            CheckGLErrors;
            m_IboId = 0U;
        }
    }
};

}  // namespace glApi
