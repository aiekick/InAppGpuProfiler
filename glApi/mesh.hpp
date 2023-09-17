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

#include <glad/glad.h>
#include <vector>
#include <memory>

template<typename T>
class Mesh {
private:
    std::weak_ptr<Mesh<T>> m_This;
    GLuint m_VboId = 0U;
    GLuint m_IboId = 0U;
    GLuint m_VaoId = 0U;

    std::vector<T> m_Vertices;
    std::vector<uint32_t> m_Indices;

    const GLsizei verticeSize = sizeof(T);
    const GLsizei indiceSize = sizeof(uint32_t);

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
    uint32_t GetVaoID() {
        return m_VboId;
    }
    uint32_t GetVboID() {
        return m_IboId;
    }
    uint32_t GetIboID() {
        return m_VaoId;
    }
    bool init(std::vector<T> vVertices, std::vector<uint32_t> vIndices) {
        assert(!vVertices.empty());
        assert(!vIndices.empty());
        glGenVertexArrays(1, &m_VaoId);
        glGenBuffers(1, &m_VboId);
        glGenBuffers(1, &m_IboId);

        glBindVertexArray(m_VaoId);
        glBindBuffer(GL_ARRAY_BUFFER, m_VboId);
        glBufferData(GL_ARRAY_BUFFER, verticeSize * vVertices.size(), vVertices.data(), GL_STATIC_DRAW);

        // pos
        glEnableVertexAttribArray(0);
        LogGlError();
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, m_MeshDatas.verticeSize, (void*)nullptr);
        LogGlError();
        glDisableVertexAttribArray(0);
        LogGlError();

        // tex
        glEnableVertexAttribArray(1);
        LogGlError();
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, m_MeshDatas.verticeSize, (void*)(sizeof(float) * 2));
        LogGlError();
        glDisableVertexAttribArray(1);
        LogGlError();

        if (!m_MeshDatas.m_Indices.empty()) {
            SetIndicesCount(m_MeshDatas.m_Indices.size());
            SetIndicesCountToShow(m_MeshDatas.m_Indices.size());

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_MeshDatas.m_Ibo);
            LogGlError();

            glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_MeshDatas.indiceSize * m_MeshDatas.m_Indices.size(), m_MeshDatas.m_Indices.data(),
                         GL_DYNAMIC_DRAW);
            LogGlError();
        }

        // unbind
        glBindVertexArray(0);
        LogGlError();
        glBindBuffer(GL_ARRAY_BUFFER, 0);  // bien unbind les buffer apres le vao sinon le contexte est v�rol�
        LogGlError();
        if (!m_MeshDatas.m_Indices.empty()) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            LogGlError();
        }

        return false;
    }
    void unit() {
        if(m_VaoId > 0) {
            glDeleteVertexArrays(1, &m_VaoId);
            m_VaoId = 0U;
        }
        if (m_VboId > 0) {
            glDeleteBuffers(1, &m_VboId);
            m_VboId = 0U;
        }
        if (m_IboId > 0) {
            glDeleteBuffers(1, &m_IboId);
            m_IboId = 0U;
        }
    }
};