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

struct ProcDatas {
    std::array<float, 3U> p = {};
    std::array<float, 2U> t = {};
    ProcDatas(){};
    ProcDatas(const float& px, const float& py, const float& pz, const float& tx, const float& ty) : p{px, py, pz}, t{tx, ty} {};
};

class ProcMesh;
typedef std::shared_ptr<ProcMesh> ProcMeshPtr;
typedef std::weak_ptr<ProcMesh> ProcMeshWeak;

class ProcMesh : public Mesh<ProcDatas> {
public:
    static ProcMeshPtr createUVSphere(const double& vRadius, const uint32_t& vUSubdivs, const uint32_t& vVSubdivs) {
        auto res = std::make_shared<ProcMesh>();
        res->m_This = res;
        assert(vRadius > 0.0 && vUSubdivs > 2U && vVSubdivs > 2U);

        std::vector<ProcDatas> vertices;
        std::vector<uint32_t> indices;

        ProcDatas p0, p1, p2, p3;
        const double _PI_ = 3.1415926535897932384626433832795;
        const double _2PI_ = _PI_ * 2.0;
        const double s_step = 1.0 / vUSubdivs;
        const double t_step = 1.0 / vVSubdivs;
        const double u_step = _2PI_ * s_step;
        const double v_step = _2PI_ * t_step;

        for (double u = 0.0, s = 0.0; u < _2PI_; u += u_step, s += s_step) {
            double xy = vRadius * cos(u);
            double z = vRadius * sin(u);
            for (double v = -_PI_, t = 0.0; v < _PI_; v += v_step, t += t_step) {
                double x = xy * cos(v);
                double y = xy = sin(v);
                vertices.push_back(ProcDatas((float)x, (float)y, (float)z, (float)s, (float)t));
            }
        }

        for (uint32_t i = 0U; i < vUSubdivs; ++i) {
            uint32_t k1 = i * (vVSubdivs + 1);
            uint32_t k2 = k1 + vVSubdivs + 1;

            for (uint32_t j = 0U; j < vVSubdivs; ++j) {
                if (i != 0U) {
                    indices.push_back(k1);
                    indices.push_back(k2);
                    indices.push_back(k1 + 1U);
                }

                if (i != (vUSubdivs - 1U)) {
                    indices.push_back(k1 + 1U);
                    indices.push_back(k2);
                    indices.push_back(k2 + 1U);
                }

                ++k1;
                ++k2;
            }
        }

        if (!res->init(vertices, indices, {3, 2})) {
            res.reset();
        }
        return res;
    }

private:
    ProcMeshWeak m_This;
};

}  // namespace glApi
