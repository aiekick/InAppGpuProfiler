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

/* 
* this class will manage 
* will aprse a code and filter uniforms code
* uniform widgets
* 
*/

#include <map>
#include <string>
#include <cassert>
#include <unordered_map>

namespace glApi {

class UniformParsingDatas {
public:
    std::string uniform_line;
    std::string uniform_type;
    std::string widget_type;
    std::string uniform_name;
    std::string uniform_comment;           // les \\n serait remaplce par des \n pour affichage
    std::string uniform_comment_original;  // les \\n reste des \\n
    std::string uniform_param_line;
    std::vector<std::string> params;
    bool valid = false;

public:
    UniformParsingDatas() = default;
    UniformParsingDatas(const std::string& vUniformString) {
        valid = m_ParseUniformString(vUniformString);
    }
    std::string getFinalUniformCode() const {
        if (uniform_comment_original.empty()) {
            return "uniform " + uniform_type + " " + uniform_name + ";";
        } else {
            return "uniform " + uniform_type + " " + uniform_name + "; // " + uniform_comment_original;
        }
    }
    const bool& isValid() {
        return valid;
    }

private:
    size_t m_IgnoreComments(const std::string& vUniformString, const size_t& vPos) {
        if (vPos != std::string::npos) {
            char c = vUniformString[vPos];
            if (c == '/' || c == ' ') {
                auto next_space = vUniformString.find_first_of(" /", vPos);
                auto start_pos = vUniformString.find("/*", vPos);
                if (next_space >= start_pos) {
                    auto start_not_pos = vUniformString.find("*/", vPos);
                    if (start_not_pos < start_pos) {
                        return vPos;
                    }
                    if (start_pos != std::string::npos) {
                        auto end_pos = vUniformString.find("*/", start_pos);
                        if (end_pos != std::string::npos) {
                            end_pos += 2;
                            return end_pos;
                        }
                    }
                }
            }
        }
        return vPos;
    }
    bool m_ParseUniformString(const std::string& vUniformString) {
        if (vUniformString.empty()) {
            return false;
        }

        uniform_line = vUniformString;

        // we wnat to get all theses
        // uniform type(widget:params) name; comment

        bool uniform_found = false;
        bool type_found = false;
        bool name_found = false;

        auto first_comment = vUniformString.find("//");
        auto pos = vUniformString.find("uniform");
        if (first_comment != std::string::npos && first_comment < pos) {
            return false;
        }

        if (first_comment > pos && pos != std::string::npos) {
            uniform_found = true;
            pos += std::string("uniform").size();

            pos = m_IgnoreComments(vUniformString, pos);

            // type and params
            auto type_pos = vUniformString.find_first_of("abcdefghijklmnopqrstuvwxyz(", pos);
            if (type_pos != std::string::npos) {
                if (vUniformString[type_pos] == '(') {
                    return (uniform_found && type_found && name_found);
                }
                // pos = type_pos;
                size_t first_parenthesis_pos = vUniformString.find('(', type_pos + 1);
                if (first_parenthesis_pos != std::string::npos) {
                    first_parenthesis_pos += 1;
                    uniform_type = vUniformString.substr(type_pos, first_parenthesis_pos - (type_pos + 1));
                    if (!uniform_type.empty()) {
                        type_found = true;
                    }
                    first_parenthesis_pos = m_IgnoreComments(vUniformString, first_parenthesis_pos);
                    size_t last_parenthesis_pos = vUniformString.find_first_of("()", first_parenthesis_pos);
                    if (last_parenthesis_pos != std::string::npos) {
                        if (vUniformString[last_parenthesis_pos] == ')') {
                            uniform_param_line = vUniformString.substr(first_parenthesis_pos, last_parenthesis_pos - first_parenthesis_pos);
                            pos = last_parenthesis_pos + 1;
                        } else {
                            return (uniform_found && type_found && name_found);
                        }
                    } else {
                        return (uniform_found && type_found && name_found);
                    }
                } else {
                    size_t nextSpace = vUniformString.find_first_of(" /", type_pos + 1);
                    if (nextSpace != std::string::npos) {
                        uniform_type = vUniformString.substr(type_pos, nextSpace - type_pos);
                        if (!uniform_type.empty()) {
                            type_found = true;
                        }
                        if (vUniformString[nextSpace] == ' ') {
                            pos = nextSpace + 1;
                        }
                        if (vUniformString[nextSpace] == '/') {
                            pos = nextSpace;
                        }
                    }
                }
            }
        }

        // name
        pos = m_IgnoreComments(vUniformString, pos);
        size_t coma_pos = vUniformString.find_first_of(";\n\r/[", pos);
        if (coma_pos != std::string::npos) {
            size_t namePos = vUniformString.find_first_of("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", pos);
            if (namePos != std::string::npos) {
                if (namePos < coma_pos) {
                    uniform_name = vUniformString.substr(namePos, coma_pos - namePos);
                    m_replace_string(uniform_name, " ", "_");
                    if (!uniform_type.empty()) {
                        name_found = true;
                    }
                }
            }

            if (vUniformString[coma_pos] != ';' && vUniformString[coma_pos] != '[' && vUniformString[coma_pos] != '/') {
                return (uniform_found && type_found && name_found);
            }
        } else {
            return (uniform_found && type_found && name_found);
        }

        if (uniform_name.empty()) {
            return (uniform_found && type_found && name_found);
        }

        // array
        /*size_t arrayPos = vUniformString.find_first_of('[');
        if (arrayPos != std::string::npos) {
            arrayPos += 1;
            size_t endArray = vUniformString.find_first_of("[]", arrayPos);
            if (endArray != std::string::npos) {
                if (vUniformString[endArray] == ']') {
                    array = vUniformString.substr(arrayPos, endArray - arrayPos);
                    pos = endArray + 1;
                } else {
                    return (uniform_found && type_found && name_found);
                }
            } else {
                return (uniform_found && type_found && name_found);
            }
        }*/

        // comment
        coma_pos = vUniformString.find_first_of(";\n\r");
        if (coma_pos != std::string::npos) {
            if (vUniformString[coma_pos] == ';') {
                coma_pos += 1;
                std::string afterComaStr = vUniformString.substr(coma_pos);
                if (!afterComaStr.empty()) {
                    size_t uniform_comment_pos = afterComaStr.find("//");
                    if (uniform_comment_pos == std::string::npos) {
                        uniform_comment_pos = afterComaStr.find("/*");
                    }
                    if (uniform_comment_pos != std::string::npos) {
                        uniform_comment_pos += 2;
                        uniform_comment_original = afterComaStr.substr(uniform_comment_pos);
                        m_replace_string(uniform_comment_original, "*/", "");  // win
                        uniform_comment = uniform_comment_original;
                        m_replace_string(uniform_comment, "\\n", "\n");  // win
                        m_replace_string(uniform_comment, "\\r", "\r");  // unix
                    }
                }
            } else {
                return (uniform_found && type_found && name_found);
            }
        }

        m_replace_string(uniform_type, " ", "");
        m_replace_string(widget_type, " ", "");
        m_replace_string(uniform_name, " ", "");
        m_replace_string(uniform_param_line, " ", "");

        m_replace_string(uniform_type, "\t", "");
        m_replace_string(widget_type, "\t", "");
        m_replace_string(uniform_name, "\t", "");
        m_replace_string(uniform_param_line, "\t", "");

        m_replace_string(uniform_type, "\r", "");
        m_replace_string(widget_type, "\r", "");
        m_replace_string(uniform_name, "\r", "");
        m_replace_string(uniform_param_line, "\r", "");

        /// parse params

        std::string word;
        bool first_word = true;
        for (auto c : uniform_param_line) {
            if (c != ':') {
                word += c;
            } else {
                if (first_word) {
                    if (m_is_content_string(word)) {
                        widget_type = word;
                        first_word = false;
                    } else {
                        widget_type = "slider";
                        params.push_back(word);
                        first_word = false;
                    }
                } else {
                    params.push_back(word);
                }
                word.clear();
            }
        }
        if (!word.empty()) {
            if (first_word) {
                if (m_is_content_string(word)) {
                    widget_type = word;
                }
            } else {
                params.push_back(word);
            }
        }

        return (uniform_found && type_found && name_found);
    }
    bool m_replace_string(::std::string& vStr, const ::std::string& vOldStr, const ::std::string& vNewStr) {
        bool found = false;
        size_t pos = 0;
        while ((pos = vStr.find(vOldStr, pos)) != ::std::string::npos) {
            found = true;
            vStr.replace(pos, vOldStr.length(), vNewStr);
            pos += vNewStr.length();
        }
        return found;
    }
    bool m_is_content_string(const std::string& vStr) {
        auto num_pos = vStr.find_first_of("0123456789.");
        auto str_pos = vStr.find_first_of("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
        return (str_pos < num_pos);
    }
};

/*
 * class used for expsoe internal function of Uniforms class for test
 */
class UniformStrings {
public:
    // uniform pos, uniform line, uniform end
    typedef std::unordered_map<size_t, UniformParsingDatas> UniformStringsContainer;

private:
    UniformStringsContainer m_uniform_strings;

public:
    std::string parse_and_filter_code(const std::string& vCode) {
        std::string final_code = vCode;
        auto _uniform_strings = m_get_uniform_strings(final_code);
        if (!_uniform_strings.empty()) {
            final_code = m_filter_code(final_code, _uniform_strings);
        }
        return final_code;
    }
    const UniformStringsContainer& get_uniform_strings() {
        return m_uniform_strings;
    }
    UniformStringsContainer& get_uniforms_strings_ref() {
        return m_uniform_strings;
    }

protected:
    UniformStringsContainer m_get_uniform_strings(const std::string& vCode) {
        UniformStringsContainer res;
        size_t uniform_pos = 0U;
        while (uniform_pos != std::string::npos) {
            uniform_pos = vCode.find("uniform", uniform_pos);
            if (uniform_pos != std::string::npos) {
                size_t end_line_pos = vCode.find_first_of("\n\r", uniform_pos);
                if (end_line_pos != std::string::npos) {
                    auto uniform_line = vCode.substr(uniform_pos, end_line_pos - uniform_pos);
                    res[uniform_pos] = UniformParsingDatas(uniform_line);
                    uniform_pos = end_line_pos;
                }
                ++uniform_pos;
            }
        }
        return res;
    }

    // will replace bad glsl uniform code (code with declarative uniform syntax) to good uniform code
    std::string m_filter_code(const std::string& vCode, const UniformStringsContainer& vContainer) {
        std::string final_code = vCode;
        size_t offset = 0;
        for (auto _uniform_datas : vContainer) {
            const auto& datas = _uniform_datas.second;
            if (!datas.widget_type.empty()) {
                const auto& uniform_code = datas.getFinalUniformCode();
                size_t pos = _uniform_datas.first - offset;
                final_code.replace(pos, datas.uniform_line.size(), uniform_code);
                offset += datas.uniform_line.size() - uniform_code.size();  // le nouveau code sera frocemment plus court
                assert(offset != std::string::npos);
            }
        }
#ifdef _DEBUG
        m_save_string_to_file(final_code, "debug/shader.glsl");
#endif
        return final_code;
    }

#ifdef _DEBUG
    void m_save_string_to_file(const std::string& vString, const std::string& vFilePathName) {
        std::ofstream fileWriter(vFilePathName, std::ios::out);
        if (!fileWriter.bad()) {
            fileWriter << vString;
            fileWriter.close();
        }
    }
#endif
};

class IUniform {
public:
    typedef std::function<void(IUniform*)> IUniformDrawWidgetFunctor;

protected:
    std::string m_name;
    GLint m_loc = -1;
    GLuint m_channels = 1U;
    bool m_used = false;
    bool m_showed = false;
    std::string m_type;
    std::string m_widget;
    IUniformDrawWidgetFunctor m_draw_widget_functor = nullptr;
    UniformParsingDatas m_uniform_parsing_datas;

public:
    void set_uniform_parsing_datas(const UniformParsingDatas& vUniformParsingDatas) {
        m_uniform_parsing_datas = vUniformParsingDatas;
    }
    void set_draw_widget_functor(const IUniformDrawWidgetFunctor& vUniformDrawWidgetFunctor) {
        m_draw_widget_functor = vUniformDrawWidgetFunctor;
    }
    virtual bool draw_widget() {
        if (m_draw_widget_functor != nullptr) {
            m_draw_widget_functor(this);
        }
    };
    const char* get_general_help() {
        return u8R"(
general syntax is : 
- uniform type(widget:params) name; // simple or multiline comment
)";
    }
    virtual const char* get_help(){};
    virtual bool upload_sampler(int32_t& texture_slot_id){};
    virtual bool upload_scalar(){};
};

class UniformTime : public IUniform {
private:
    bool m_play = false;
    float m_time = 0.0f;

public:
    bool upload_scalar() override {
        if (m_loc > 0) {
            glUniform1fv(m_loc, 1, &m_time); 
            CheckGLErrors;
            return true;
        }
        return false;
    }
    const char* get_help() override {
        return u8R"(
buffer uniform syantax :(default is optional)
- uniform float(time:default) name;
)";
    }
};

class UniformBuffer : public IUniform {
private:
    std::array<float, 3U> m_size = {};
    int32_t m_sampler2D = -1;  // sampler2D

public:
    UniformBuffer() {
        m_widget = "buffer";
    }
    bool upload_scalar() override {
        if (m_loc > 0) {
            switch (m_channels) {
                case 2U: glUniform2fv(m_loc, 1, m_size.data()); break;
                case 3U: glUniform3fv(m_loc, 1, m_size.data()); break;
            }
            CheckGLErrors;
            return true;
        }
        return false;
    }
    bool upload_sampler(int32_t& texture_slot_id) override {
        if (m_loc > 0) {
            glActiveTexture(GL_TEXTURE0 + texture_slot_id);
            CheckGLErrors;
            glBindTexture(GL_TEXTURE_2D, m_sampler2D);
            CheckGLErrors;
            glUniform1i(m_loc, texture_slot_id);
            CheckGLErrors;
            ++texture_slot_id;
            return true;
        }
        return false;    
    }
    const char* get_help() override {
        return u8R"(
buffer uniform syantax :
- resolution :
  - vec2 resolution => uniform vec2(buffer) name;
  - vec3 resolution => uniform vec2(buffer) name; (like shadertoy resolution)
- sampler2D back buffer : (target is optional. n is the fbo attachment id frrm 0 to 7)
  - uniform sampler2D(buffer) name; => for target == 0
  - uniform sampler2D(buffer:target=n) name; => for target == n
)";
    }
};

class UniformSlider : public IUniform {
public:
    const char* get_help() override {
        return u8R"(
float slider uniform syntax : (default and step are otionals)
- uniform float(inf:sup:default:step) name;
- uniform vec2(inf:sup:default:step) name;
- uniform vec3(inf:sup:default:step) name;
- uniform vec4(inf:sup:default:step) name;
- uniform int(inf:sup:default:step) name;
- uniform ivec2(inf:sup:default:step) name;
- uniform ivec3(inf:sup:default:step) name;
- uniform ivec4(inf:sup:default:step) name;
)";
    }
};
class UniformFloatSlider : public UniformSlider {
private:
    std::array<float, 4U> m_datas_f;

public:
    bool upload_scalar() override {
        if (m_loc > 0) {
            switch (m_channels) {
                case 1U: glUniform2fv(m_loc, 1, m_datas_f.data()); break;
                case 2U: glUniform2fv(m_loc, 1, m_datas_f.data()); break;
                case 3U: glUniform3fv(m_loc, 1, m_datas_f.data()); break;
                case 4U: glUniform2fv(m_loc, 1, m_datas_f.data()); break;
            }
            CheckGLErrors;
            return true;
        }
        return false;
    }
};

class UniformIntSlider : public UniformSlider {
private:
    std::array<int32_t, 4U> m_datas_i;

public:
    bool upload_scalar() override {
        if (m_loc > 0) {
            switch (m_channels) {
                case 1U: glUniform2iv(m_loc, 1, m_datas_i.data()); break;
                case 2U: glUniform2iv(m_loc, 1, m_datas_i.data()); break;
                case 3U: glUniform3iv(m_loc, 1, m_datas_i.data()); break;
                case 4U: glUniform2iv(m_loc, 1, m_datas_i.data()); break;
            }
            CheckGLErrors;
            return true;
        }
        return false;
    }
};


class UniformsManager : public UniformStrings {
public:
    typedef std::map<std::string, IUniform*> UniformsContainer;

private:
    UniformsContainer m_uniforms;

public:
    const UniformsContainer& get_uniforms() {
        return m_uniforms;
    }
    UniformsContainer& get_uniforms_ref() {
        return m_uniforms;
    }
};

}  // namespace glApi