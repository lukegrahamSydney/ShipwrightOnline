#ifndef ZELDAONLINEROOMWINDOWH
#define ZELDAONLINEROOMWINDOWH
#include <ship/window/gui/GuiWindow.h>
#include "soh/util.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <soh/cvar_prefixes.h>
#include <ship/window/gui/IconsFontAwesome4.h>

namespace ZeldaOnline
{
    class ZeldaOnlineRoomWindow : public Ship::GuiWindow {
  private:
        char m_inputBuffer[256] = "";
        bool m_focusNextFrame = false;

      public:
        using GuiWindow::GuiWindow;

        void InitElement() override {};
        void DrawElement() override {
            ImGui::SetNextItemWidth(-FLT_MIN);

            if (m_focusNextFrame) {
                ImGui::SetKeyboardFocusHere();
                m_focusNextFrame = false;
            }

            if (ImGui::InputTextWithHint("##zo_chat", "Press Enter to chat...", m_inputBuffer, sizeof(m_inputBuffer),
                                         ImGuiInputTextFlags_EnterReturnsTrue)) {
                if (m_inputBuffer[0] != '\0') {
                    m_inputBuffer[0] = '\0';
                }
                ImGui::SetKeyboardFocusHere(-1);
            }
        }

        void Draw() override {
            if (!IsVisible()) {
                return;
            }

            ImGui::PushStyleColor(ImGuiCol_WindowBg,
                                  ImVec4(0, 0, 0, CVarGetFloat(CVAR_SETTING("Notifications.BgOpacity"), 0.5f)));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 4.0f));

            auto vp = ImGui::GetMainViewport();
            ImGui::SetNextWindowViewport(vp->ID);

            const float barHeight = ImGui::GetFrameHeight() + 8.0f;
            ImGui::SetNextWindowPos(ImVec2(vp->Pos.x, vp->Pos.y + vp->Size.y - barHeight));
            ImGui::SetNextWindowSize(ImVec2(vp->Size.x, barHeight));

            ImGui::Begin("Zelda Online Chat", nullptr,
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

            DrawElement();

            ImGui::End();

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(2);
        }

        void UpdateElement() override {};
    };

}
#endif
