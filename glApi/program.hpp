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
#include <string>
#include <cassert>

namespace glApi {

class Program;
typedef std::shared_ptr<Program> ProgramPtr;

class Program {
private:
    GLuint m_ProgramId = 0U;
    std::string m_ProgramName;
    std::map<uintptr_t, ShaderWeak> m_Shaders; // a same shader object can be added two times
public:
    static ProgramPtr create(const std::string& vProgramName) {
        auto res = std::make_shared<Program>();
        if (!res->init(vProgramName)) {
            res.reset();
        }
        return res;
    }

public:
    Program() = default;
    ~Program() {
        unit();
    }
    bool init(const std::string& vProgramName) {
        bool res = false;
        assert(!vProgramName.empty());
        m_ProgramName = vProgramName;
        m_ProgramId = glCreateProgram();
        CheckGLErrors;
        if (m_ProgramId > 0U) {
            return true;
        }
        return false;
    }
    bool addShader(ShaderWeak vShader) {
        if (!vShader.expired()) {
            m_Shaders[(uintptr_t)vShader.lock().get()] = vShader;
            return true;
        }
        return false;
    }    
    bool link() {
        bool res = false;
        if (m_ProgramId > 0U) {
            bool one_shader_at_least = false;
            for (auto& shader : m_Shaders) {
                auto ptr = shader.second.lock();
                if (ptr != nullptr) {
                    one_shader_at_least = true;
                    glAttachShader(m_ProgramId, ptr->getShaderId());
                    CheckGLErrors;
                    // we could delete shader id after linking,
                    // but we dont since we can have many shader for the same program
                }
            }
            if (one_shader_at_least) {
                glLinkProgram(m_ProgramId);
                CheckGLErrors;
                glFinish();
                GLint linked = 0;
                glGetProgramiv(m_ProgramId, GL_LINK_STATUS, &linked);
                CheckGLErrors;
                if (!linked) {
                    if (!printProgramLogs(m_ProgramName, "Link Errors")) {
                        printf("Program \"%s\" linking fail for unknown reason\n", m_ProgramName.c_str());
                    }
                    res = false;
                } else {
                    printProgramLogs(m_ProgramName, "Link Warnings");
                    res = true;
                }
            }
        }
        return res;    
    }
    void unit() {
        if (m_ProgramId > 0U) {
            glDeleteProgram(m_ProgramId);
            CheckGLErrors;
            m_ProgramId = 0U;
        }
    }

    bool use() {
        if (m_ProgramId > 0U) {
            AIGPScoped("", "Porgram::Use");
            glUseProgram(m_ProgramId);
            CheckGLErrors;
            return true;
        }
        return false;
    }

    void unuse() {
        glUseProgram(0);
    }

private:
    bool printProgramLogs(const std::string& vProgramName, const std::string& vLogTypes) {
        assert(!vProgramName.empty());
        assert(!vLogTypes.empty());
        if (m_ProgramId > 0U) {
            GLint infoLen = 0;
            glGetProgramiv(m_ProgramId, GL_INFO_LOG_LENGTH, &infoLen);
            CheckGLErrors;
            if (infoLen > 1) {
                char* infoLog = new char[infoLen];
                glGetProgramInfoLog(m_ProgramId, infoLen, nullptr, infoLog);
                CheckGLErrors;
                printf("#### PROGRAM %s ####", vProgramName.c_str());
                printf("%s : %s", vLogTypes.c_str(), infoLog);
                delete[] infoLog;
                return true;
            }
        }
        return false;
    }
};

}  // namespace glApi
