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

#include "mesh.hpp"
#include <vector>
#include <memory>

namespace glApi {

struct QuadDatas {
    float p[2] = {};
    float t[2] = {};
    QuadDatas(const float& px, const float& py, const float& tx, const float& ty) : p{px, py}, t{tx, ty} {};
};

class QuadMesh;
typedef std::shared_ptr<QuadMesh> QuadMeshPtr;
typedef std::weak_ptr<QuadMesh> QuadMeshWeak;

class QuadMesh : public Mesh<QuadDatas> {
public:
    static QuadMeshPtr create() {
        auto res = std::make_shared<QuadMesh>();
        res->m_This = res;

        std::vector<QuadDatas> vertices = {
            QuadDatas(-1.0f, -1.0f, 0.0f, 0.0f),
            QuadDatas(1.0f, -1.0f, 1.0f, 0.0f),
            QuadDatas(1.0f, 1.0f, 1.0f, 1.0f),
            QuadDatas(-1.0f, 1.0f, 0.0f, 1.0f),
        };

        std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};

        if (!res->init(vertices, indices, {2, 2})) {
            res.reset();
        }
        return res;
    }

private:
    QuadMeshWeak m_This;
};

}  // namespace glApi
