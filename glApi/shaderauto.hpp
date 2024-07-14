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
#include <map>
#include <memory>
#include <string>
#include <cassert>

namespace glApi {

class ShaderAuto;
typedef std::shared_ptr<ShaderAuto> ShaderAutoPtr;
typedef std::weak_ptr<ShaderAuto> ShaderAutoWeak;

class ShaderAuto {
private:
    ShaderAutoWeak m_This;
    GLuint m_ShaderId = 0U;
    std::string m_ShaderName;
    GLenum m_ShaderType = 0;
    UniformStrings m_UniformStrings;

public:
    static ShaderAutoPtr createFromFile(const std::string& vShaderName, const GLenum& vShaderType, const std::string& vFile) {
        auto res = std::make_shared<ShaderAuto>();
        res->m_This = res;
        if (!res->initFromFile(vShaderName, vShaderType, vFile)) {
            res.reset();
        }
        return res;
    }
    static ShaderAutoPtr createFromCode(const std::string& vShaderName, const GLenum& vShaderType, const std::string& vCode) {
        auto res = std::make_shared<ShaderAuto>();
        res->m_This = res;
        if (!res->initFromCode(vShaderName, vShaderType, vCode)) {
            res.reset();
        }
        return res;
    }

public:
    ShaderAuto() = default;
    ~ShaderAuto() {
        unit();
    }

    bool initFromFile(const std::string& vShaderName, const GLenum& vShaderType, const std::string& vFile) {
        assert(!vShaderName.empty());
        assert(!vFile.empty());
        assert(vShaderType > 0);
        const auto& code = getCodeFromFile(vFile);
        return initFromCode(vShaderName, vShaderType, code);
    }

    bool initFromCode(const std::string& vShaderName, const GLenum& vShaderType, const std::string& vCode) {
        bool res = false;
        assert(!vShaderName.empty());
        assert(!vCode.empty());
        assert(vShaderType > 0);
        m_ShaderName = vShaderName;
        m_ShaderType = vShaderType;
        m_ShaderId = glCreateShader((GLenum)vShaderType);
        CheckGLErrors;
        if (m_ShaderId > 0U) {
            const auto code = m_UniformStrings.parse_and_filter_code(vCode);
            const GLchar* sources = code.c_str();
            glShaderSource(m_ShaderId, 1U, &sources, nullptr);
            CheckGLErrors;
            glCompileShader(m_ShaderId);
            CheckGLErrors;
            glFinish();
            GLint compiled = 0;
            glGetShaderiv(m_ShaderId, GL_COMPILE_STATUS, &compiled);
            CheckGLErrors;
            if (!compiled) {
                printShaderLogs(vShaderName, "Errors");
                res = false;
            } else {
                printShaderLogs(vShaderName, "Warnings");
                res = true;
            }
        }
        return res;
    }
    void unit() {
        if (m_ShaderId > 0U) {
            glDeleteShader(m_ShaderId);
            CheckGLErrors;
            m_ShaderId = 0U;
        }
    }
    const std::string& getName() {
        return m_ShaderName;
    }
    GLuint getShaderId() {
        return m_ShaderId;
    }

private:
    std::string getCodeFromFile(const std::string& vFile) {
        assert(!vFile.empty());
        std::string res;
        std::ifstream docFile(vFile, std::ios::in);
        if (docFile.is_open()) {
            std::stringstream strStream;
            strStream << docFile.rdbuf();  // read the file
            res = strStream.str();
            docFile.close();
        }
        return res;
    }
    void printShaderLogs(const std::string& vShaderName, const std::string& vLogTypes) {
        assert(!vShaderName.empty());
        assert(!vLogTypes.empty());
        if (m_ShaderId > 0U) {
            GLint infoLen = 0;
            glGetShaderiv(m_ShaderId, GL_INFO_LOG_LENGTH, &infoLen);
            CheckGLErrors;
            if (infoLen > 1) {
                char* infoLog = new char[infoLen];
                glGetShaderInfoLog(m_ShaderId, infoLen, nullptr, infoLog);
                CheckGLErrors;
                printf("#### SHADER %s ####\n", vShaderName.c_str());
                printf("%s : %s", vLogTypes.c_str(), infoLog);
                delete[] infoLog;
            }
        }
    }
};

}  // namespace glApi
