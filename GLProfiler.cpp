// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2017-2023 Stephane Cuillerdier All rights reserved

// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "GLProfiler.h"
#include <ctools/Logger.h>

#include <Gui/CustomGuiWidgets.h>
#include <Res/CustomFont.h>

#include <stdarg.h>  // For va_start, etc.

#include <imgui_internal.h>

glp::AverageValue::AverageValue() {
    memset(prPerFrame, 0, sizeof(double) * 60U);
    prPerFrameIdx = 0;
    prPerFrameAccum = 0.0;
    prAverageValue = 0.0;
}

void glp::AverageValue::AddValue(double vValue) {
    if (vValue < prPerFrame[prPerFrameIdx]) {
        memset(prPerFrame, 0, sizeof(double) * 60U);
        prPerFrameIdx = 0;
        prPerFrameAccum = 0.0;
        prAverageValue = 0.0;
    }
    prPerFrameAccum += vValue - prPerFrame[prPerFrameIdx];
    prPerFrame[prPerFrameIdx] = vValue;
    prPerFrameIdx = (prPerFrameIdx + 1) % 60;
    if (prPerFrameAccum > 0.0)
        prAverageValue = prPerFrameAccum / 60.0;
}

double glp::AverageValue::GetAverage() {
    return prAverageValue;
}

////////////////////////////////////////////////////////////
/////////////////////// QUERY ZONE /////////////////////////
////////////////////////////////////////////////////////////

GLuint glp::QueryZone::sMaxDepthToOpen = 100U;  // the max by default
bool glp::QueryZone::sShowLeafMode = false;
float glp::QueryZone::sContrastRatio = 4.3f;
bool glp::QueryZone::sActivateLogger = false;

glp::QueryZone::QueryZone(const GuiBackend_Window& vThread, const std::string& vName, const std::string& vSectionName, const bool& vIsRoot)
    : prThread(vThread), puName(vName), prIsRoot(vIsRoot), prSectionName(vSectionName) {
    prStartFrameId = 0;
    prEndFrameId = 0;
    prStartTimeStamp = 0;
    prEndTimeStamp = 0;
    prElapsedTime = 0.0;
    puDepth = ScopedZone::sCurrentDepth;

    GuiBackend::MakeContextCurrent(prThread);
    glGenQueries(2, puIds);
}

glp::QueryZone::~QueryZone() {
    GuiBackend::MakeContextCurrent(prThread);
    glDeleteQueries(2, puIds);

    puName.clear();
    prStartFrameId = 0;
    prEndFrameId = 0;
    prStartTimeStamp = 0;
    prEndTimeStamp = 0;
    prElapsedTime = 0.0;
    prThread.clear();
    puZonesOrdered.clear();
    puZonesDico.clear();
}

void glp::QueryZone::Clear() {
    prStartFrameId = 0;
    prEndFrameId = 0;
    prStartTimeStamp = 0;
    prEndTimeStamp = 0;
    prElapsedTime = 0.0;
}

void glp::QueryZone::SetStartTimeStamp(const GLuint64& vValue) {
    prStartTimeStamp = vValue;
    prStartFrameId++;
}

void glp::QueryZone::SetEndTimeStamp(const GLuint64& vValue) {
    prEndTimeStamp = vValue;
    prEndFrameId++;

    // LogVarLightInfo("%*s end id retrieved : %u", puDepth, "", puIds[1]);

    // start computation of elapsed time
    // no needed after
    // will be used for Graph and labels
    // so DrawMetricGraph must be the first
    ComputeElapsedTime();

    if (glp::QueryZone::sActivateLogger && puZonesOrdered.empty())  // only the leafs
    {
        double v = (double)vValue / 1e9;
        LogVarLightInfo("<profiler section=\"%s\" epoch_time=\"%f\" name=\"%s\" render_time_ms=\"%f\">", prSectionName.c_str(), v, puName.c_str(),
                        prElapsedTime);
    }
}

void glp::QueryZone::ComputeElapsedTime() {
    // we take the last frame
    if (prStartFrameId == prEndFrameId) {
        prAverageStartValue.AddValue((double)(prStartTimeStamp * 1e-6));  // ns to ms
        prAverageEndValue.AddValue((double)(prEndTimeStamp * 1e-6));      // ns to ms
        prStartTime = prAverageStartValue.GetAverage();
        prEndTime = prAverageEndValue.GetAverage();
        prElapsedTime = prEndTime - prStartTime;
    }
}

void glp::QueryZone::DrawMetricLabels() {
    if (prStartFrameId) {
        bool res = false;

        ImGuiTreeNodeFlags flags = 0;
        if (puZonesOrdered.empty())
            flags = ImGuiTreeNodeFlags_Leaf;

        if (prHighlighted)
            flags |= ImGuiTreeNodeFlags_Framed;

        if (prIsRoot) {
            res = ImGui::TreeNodeEx(this, flags, "(%u) %s %u : GPU %.2f ms", puDepth, puName.c_str(), prStartFrameId - 1U, prElapsedTime);
        } else {
            res = ImGui::TreeNodeEx(this, flags, "(%u) %s => GPU %.2f ms", puDepth, puName.c_str(), prElapsedTime);
        }

        if (ImGui::IsItemHovered())
            prHighlighted = true;

        if (res) {
            prExpanded = true;

            ImGui::Indent();

            for (const auto zone : puZonesOrdered) {
                if (zone.use_count()) {
                    zone->DrawMetricLabels();
                }
            }

            ImGui::Unindent();

            ImGui::TreePop();
        } else {
            prExpanded = false;
        }
    }
}

bool glp::QueryZone::DrawMetricGraph(std::shared_ptr<QueryZone> vParent, uint32_t vDepth) {
    bool pressed = false;

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems /* || !prStartFrameId*/)
        return pressed;

    const ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const float aw = ImGui::GetContentRegionAvail().x - style.FramePadding.x;

    if (puDepth > QueryZone::sMaxDepthToOpen)
        return pressed;

    if (!vParent.use_count()) {
        vParent = puThis;
        puTopQuery = puThis;
    }

    if (prElapsedTime > 0.0) {
        if (puDepth == 0) {
            prBarWidth = aw;
            prBarPos = 0.0f;
        } else if (vParent->prElapsedTime > 0.0) {
            puTopQuery = vParent->puTopQuery;
            const float startRatio = (float)((prStartTime - vParent->prStartTime) / vParent->prElapsedTime);
            const float elapsedRatio = (float)(prElapsedTime / vParent->prElapsedTime);
            prBarWidth = vParent->prBarWidth * elapsedRatio;
            prBarPos = vParent->prBarPos + vParent->prBarWidth * startRatio;
        }

        if ((puZonesOrdered.empty() && QueryZone::sShowLeafMode) || !QueryZone::sShowLeafMode) {
            ImGui::PushID(this);
            prBarLabel = ct::toStr("%s (%.1f ms | %.1f f/s)", puName.c_str(), prElapsedTime, 1000.0f / prElapsedTime);
            const char* label = prBarLabel.c_str();
            const ImGuiID id = window->GetID(label);
            ImGui::PopID();

            const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
            const float height = label_size.y + style.FramePadding.y * 2.0f;
            const ImVec2 bPos = ImVec2(prBarPos + style.FramePadding.x, vDepth * height + style.FramePadding.y);
            const ImVec2 bSize = ImVec2(prBarWidth - style.FramePadding.x, 0.0f);

            const ImVec2 pos = window->DC.CursorPos + bPos;
            const ImVec2 size = ImVec2(prBarWidth, height);

            const ImRect bb(pos, pos + size);
            bool hovered, held;
            pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick);

            prHighlighted = false;
            if (hovered) {
                ImGui::SetTooltip("section %s : %s\nElapsed time : %.1f ms\nElapsed FPS : %.1f f/s", prSectionName.c_str(), puName.c_str(),
                                  prElapsedTime, 1000.0f / prElapsedTime);
                prHighlighted = true;  // to highlight label graph by this button
            } else if (prHighlighted)
                hovered = true;  // highlight this button by the label graph

            // Render
            // const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
            ImVec4 cv4, hsv = ImVec4((float)(0.5 - prElapsedTime * 0.5 / puTopQuery->prElapsedTime), 0.5f, 1.0f, 1.0f);
            ImGui::ColorConvertHSVtoRGB(hsv.x, hsv.y, hsv.z, cv4.x, cv4.y, cv4.z);
            cv4.w = 1.0f;
            ImGui::RenderNavHighlight(bb, id);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
            ImGui::RenderFrame(bb.Min, bb.Max, ImGui::ColorConvertFloat4ToU32(cv4), true, 2.0f);
            ImGui::PopStyleVar();
            const bool pushed =
                ImGui::PushStyleColorWithContrast(ImGui::ColorConvertFloat4ToU32(cv4), ImGuiCol_Text, ImVec4(0, 0, 0, 1), QueryZone::sContrastRatio);
            ImGui::RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, nullptr, &label_size,
                                     ImVec2(0.5f, 0.5f) /*style.ButtonTextAlign*/, &bb);
            if (pushed)
                ImGui::PopStyleColor();

            vDepth++;
        }

        // childs
        for (const auto zone : puZonesOrdered) {
            if (zone.use_count()) {
                pressed |= zone->DrawMetricGraph(puThis, vDepth);
            }
        }
    }

    if (puDepth == 0 && ((puZonesOrdered.empty() && QueryZone::sShowLeafMode) || !QueryZone::sShowLeafMode)) {
        const ImVec2 pos = window->DC.CursorPos;
        const ImVec2 size = ImVec2(aw, ImGui::GetFrameHeight() * (ScopedZone::sMaxDepth + 1U));
        ImGui::ItemSize(size);

        const ImRect bb(pos, pos + size);
        const ImGuiID id = window->GetID((puName + "##canvas").c_str());
        if (!ImGui::ItemAdd(bb, id))
            return pressed;
    }

    return pressed;
}

////////////////////////////////////////////////////////////
/////////////////////// GL CONTEXT /////////////////////////
////////////////////////////////////////////////////////////

glp::GLContext::GLContext(const GuiBackend_Window& vThread) : prThread(vThread) {
}

void glp::GLContext::Clear() {
    prRootZone.reset();
    prPendingUpdate.clear();
    prQueryIDToZone.clear();
    prDepthToLastZone.clear();
}

void glp::GLContext::Init() {
}

void glp::GLContext::Unit() {
    Clear();
}

void glp::GLContext::Collect() {
    // LogVarLightInfo("------ Collect Trhead (%i) -----", (intptr_t)prThread);

    auto it = prPendingUpdate.begin();
    while (!prPendingUpdate.empty() && it != prPendingUpdate.end()) {
        GLuint id = *it;
        GLuint value = 0;
        glGetQueryObjectuiv(id, GL_QUERY_RESULT_AVAILABLE, &value);
        const auto it_to_erase_eventually = it;

        it++;

        if (value == GL_TRUE /* || id == prRootZone->puIds[0] || id == prRootZone->puIds[1]*/) {
            GLuint64 value64 = 0;
            glGetQueryObjectui64v(id, GL_QUERY_RESULT, &value64);
            if (prQueryIDToZone.find(id) != prQueryIDToZone.end()) {
                auto ptr = prQueryIDToZone[id];
                if (ptr.use_count()) {
                    if (id == ptr->puIds[0])
                        ptr->SetStartTimeStamp(value64);
                    else if (id == ptr->puIds[1])
                        ptr->SetEndTimeStamp(value64);
                    else
                        CTOOL_DEBUG_BREAK;
                }
            }
            prPendingUpdate.erase(it_to_erase_eventually);
        } else {
            auto ptr = prQueryIDToZone[id];
            if (ptr.use_count()) {
                LogVarError("%*s id not retrieved : %u", ptr->puDepth, "", id);
            }
        }
    }

    // LogVarLightInfo("------ End Frame -----");
}

void glp::GLContext::Draw() {
    if (prRootZone.use_count()) {
        prRootZone->DrawMetricGraph();
        /*
        if (!QueryZone::sShowLeafMode)
            prRootZone->DrawMetricLabels();
        */
    }
}

std::shared_ptr<glp::QueryZone> glp::GLContext::GetQueryZoneForName(const std::string& vName, const std::string& vSection, const bool& vIsRoot) {
    std::shared_ptr<QueryZone> res = nullptr;

    /////////////////////////////////////////////
    //////////////// CREATION ///////////////////
    /////////////////////////////////////////////

    ScopedZone::sMaxDepth = ct::maxi(ScopedZone::sMaxDepth, ScopedZone::sCurrentDepth);

    if (ScopedZone::sCurrentDepth == 0) {
        // LogVarLightInfo("------ Start Frame -----");

        prDepthToLastZone.clear();
        if (!prRootZone.use_count()) {
            res = std::make_shared<glp::QueryZone>(prThread, vName, vSection, vIsRoot);
            if (res.use_count()) {
                res->puThis = res;
                res->puDepth = ScopedZone::sCurrentDepth;
                prQueryIDToZone[res->puIds[0]] = res;
                prQueryIDToZone[res->puIds[1]] = res;
                prRootZone = res;
                // LogVarDebug("Profile : add zone %s at puDepth %u", vName.c_str(), ScopedZone::sCurrentDepth);
            }
        } else {
            res = prRootZone;
        }
    } else {  // else child zone
        auto root = GetQueryZoneFromDepth(ScopedZone::sCurrentDepth - 1U);
        if (root.use_count()) {
            if (root->puZonesDico.find(vName) == root->puZonesDico.end()) {  // not found
                res = std::make_shared<glp::QueryZone>(prThread, vName, vSection, vIsRoot);
                if (res.use_count()) {
                    res->puThis = res;
                    res->puDepth = ScopedZone::sCurrentDepth;
                    prQueryIDToZone[res->puIds[0]] = res;
                    prQueryIDToZone[res->puIds[1]] = res;
                    root->puZonesDico[vName] = res;
                    root->puZonesOrdered.push_back(res);
                    // LogVarDebug("Profile : add zone %s at puDepth %u", vName.c_str(), ScopedZone::sCurrentDepth);
                } else {
                    CTOOL_DEBUG_BREAK;
                }
            } else {
                res = root->puZonesDico[vName];
            }
        } else {
            return res;  // happen when profiling is activated inside a profiling zone
        }
    }

    /////////////////////////////////////////////
    //////////////// UTILISATION ////////////////
    /////////////////////////////////////////////

    if (res.use_count()) {
        SetQueryZoneForDepth(res, ScopedZone::sCurrentDepth);

        if (res->puName != vName) {
            // at puDepth 0 there is only one frame
            LogVarDebugError("was registerd at depth %u %s. but we got %s\nwe clear the profiler",  //
                             ScopedZone::sCurrentDepth, res->puName.c_str(), vName.c_str());
            // c'est pas normal, dans le doute on efface le profiler, ca va forcer a le re remplir
            Clear();
        }

        prPendingUpdate.emplace(res->puIds[0]);
        prPendingUpdate.emplace(res->puIds[1]);
    }

    return res;
}

void glp::GLContext::SetQueryZoneForDepth(std::shared_ptr<QueryZone> vQueryZone, GLuint vDepth) {
    prDepthToLastZone[vDepth] = vQueryZone;
}

std::shared_ptr<glp::QueryZone> glp::GLContext::GetQueryZoneFromDepth(GLuint vDepth) {
    std::shared_ptr<glp::QueryZone> res = nullptr;

    if (prDepthToLastZone.find(vDepth) != prDepthToLastZone.end()) {  // found
        res = prDepthToLastZone[vDepth];
    }

    return res;
}

////////////////////////////////////////////////////////////
/////////////////////// GL PROFILER ////////////////////////
////////////////////////////////////////////////////////////

void glp::GLProfiler::Clear() {
    prContexts.clear();
}

void glp::GLProfiler::Init() {
}

void glp::GLProfiler::Unit() {
    Clear();
}

void glp::GLProfiler::Collect() {
    if (!puIsActive || puIsPaused) {
        return;
    }

    glFinish();

    for (const auto& con : prContexts) {
        if (con.second.use_count()) {
            con.second->Collect();
        }
    }
}

void glp::GLProfiler::Draw() {
    if (!puIsActive)
        return;

    if (ImGui::BeginMenuBar()) {
        if (ScopedZone::sMaxDepth) {
            QueryZone::sMaxDepthToOpen = ScopedZone::sMaxDepth;
        }
        ImGui::Text("%s", "Logging");
        ImGui::Checkbox("##logging", &glp::QueryZone::sActivateLogger);
        ImGui::ToggleContrastedButton(ICON_NDP_PLAY, ICON_NDP_PAUSE, &puIsPaused, "Play/Pause Profiling");

        ImGui::EndMenuBar();
    }

    for (const auto& con : prContexts) {
        if (con.second.use_count()) {
            con.second->Draw();
        }
    }
}

std::shared_ptr<glp::GLContext> glp::GLProfiler::GetContext(const GuiBackend_Window& vThread) {
    if (!puIsActive)
        return nullptr;

    if (vThread.win) {
        if (prContexts.find((intptr_t)vThread.win) == prContexts.end()) {
            prContexts[(intptr_t)vThread.win] = std::make_shared<GLContext>(vThread);
        }

        return prContexts[(intptr_t)vThread.win];
    }

    LogVarError("GuiBackend_Window thread is NULL");

    return nullptr;
}

////////////////////////////////////////////////////////////
/////////////////////// SCOPED ZONE ////////////////////////
////////////////////////////////////////////////////////////

// STATIC
uint32_t glp::ScopedZone::sCurrentDepth = 0U;
uint32_t glp::ScopedZone::sMaxDepth = 0U;

// SCOPED ZONE
glp::ScopedZone::ScopedZone(bool vIsRoot, std::string vSection, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    static char TempBuffer[256];
    const int w = vsnprintf(TempBuffer, 256, fmt, args);
    va_end(args);
    if (w) {
        auto context = GLProfiler::Instance()->GetContext(GuiBackend::Instance()->GetCurrentContext());
        if (context.use_count()) {
            query = context->GetQueryZoneForName(std::string(TempBuffer, (size_t)w), vSection, vIsRoot);
            if (query.use_count()) {
                glQueryCounter(query->puIds[0], GL_TIMESTAMP);
                // LogVarLightInfo("%*s begin : %u", query->puDepth, "", query->puIds[0]);
                sCurrentDepth++;
            }
        }
    }
}

glp::ScopedZone::~ScopedZone() {
    if (query.use_count()) {
        glQueryCounter(query->puIds[1], GL_TIMESTAMP);
        if (!sCurrentDepth) {
            CTOOL_DEBUG_BREAK;
        }
        sCurrentDepth--;
    }
}
