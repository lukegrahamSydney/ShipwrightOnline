#ifndef ZELDAONLINEROOMWINDOWH
#define ZELDAONLINEROOMWINDOWH
#include <ship/window/gui/GuiWindow.h>
#include <ship/window/Window.h>
#include <ship/Context.h>
#include "soh/util.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <soh/cvar_prefixes.h>
#include <ship/window/gui/IconsFontAwesome4.h>
#include <string>
#include <vector>
#include <functional>

const char* ResolveSceneID(int sceneID, int roomID);

namespace ZeldaOnline {
struct SkinOption {
    std::string displayName;
    std::string referenceName;
};

struct PlayerEntry {
    std::string name;
    uint32_t networkID = 0;
    bool isInParty = false;
    uint32_t pendingInvitePartyID = 0;
    int16_t sceneNum = 0;
    uint8_t roomIndex = 0;
    uint8_t age = 0;
};

enum class PlayerListAction {
    None,
    Invite,
    AcceptInvite,
    DeclineInvite,
    CreateParty,
    LeaveParty,
    SetPartyScenes,
    TeleportTo,
    ChangeSkin,
    ChangeName,
    Connect,
    Disconnect,
    UnstuckMe
};

struct PlayerListActionEvent {
    PlayerListAction action = PlayerListAction::None;
};

struct PlayerListActionEventInvite {
    PlayerListActionEvent actionEvent;
    uint32_t networkID = 0;
    uint32_t partyID = 0;
};

struct PlayerListActionEventText {
    PlayerListActionEvent actionEvent;
    std::string text;
};

struct PlayerListActionEventToggle {
    PlayerListActionEvent actionEvent;
    bool enabled = false;
};

struct PlayerListActionEventConnect {
    PlayerListActionEvent actionEvent;
    std::string host;
    uint16_t port = 0;
    std::string nickname;
    std::string fileServer;
};

class ZeldaOnlineRoomWindow : public Ship::GuiWindow {
  private:
    std::vector<PlayerEntry> m_players;
    std::vector<SkinOption> m_availableSkins;
    std::string m_currentSkin;
    std::function<void(const PlayerListActionEvent*)> m_onAction;
    bool m_hasParty = false;
    bool m_partyScenes = false;
    std::string m_statusText;
    ImVec4 m_statusColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    char m_nameBuffer[100] = "";
    bool m_recenterNextFrame = false;

    bool m_beginAutoConnect = false;
    bool m_connected = false;
    bool m_connecting = false;
    bool m_fieldsLoaded = false;
    bool m_autoConnect = false;
    char m_hostBuffer[128] = "";
    int m_portValue = 0;
    char m_fileServerBuffer[256] = "";

    PlayerEntry* FindPlayerEntry(uint32_t networkID) {
        for (auto& entry : m_players) {
            if (entry.networkID == networkID)
                return &entry;
        }
        return nullptr;
    }

    int PartyMemberCount() const {
        int count = 0;
        for (const auto& entry : m_players) {
            if (entry.isInParty)
                count++;
        }
        return count;
    }

    void DrawMenuBar(PlayerListActionEventText& textEvent, PlayerListActionEventToggle& toggleEvent,
                     PlayerListActionEvent*& pending) {
        if (!ImGui::BeginMenuBar())
            return;

        if (!m_hasParty) {
            if (ImGui::Button(ICON_FA_USER_PLUS " Create Party")) {
                textEvent.actionEvent.action = PlayerListAction::CreateParty;
                pending = &textEvent.actionEvent;
            }
        } else {
            if (ImGui::Button(ICON_FA_SIGN_OUT " Leave Party")) {
                textEvent.actionEvent.action = PlayerListAction::LeaveParty;
                pending = &textEvent.actionEvent;
            }

            ImGui::SameLine();

            if (ImGui::Checkbox("Private Dungeons", &m_partyScenes)) {
                toggleEvent.actionEvent.action = PlayerListAction::SetPartyScenes;
                toggleEvent.enabled = m_partyScenes;
                pending = &toggleEvent.actionEvent;
            }

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Create a separate dungeon instance for your party.\n"
                                  "Only members of your party can enter the instance.");
        }

        ImGui::EndMenuBar();
    }

    static std::string LocationLabel(const PlayerEntry& entry) {
        const char* scene = ResolveSceneID(entry.sceneNum, entry.roomIndex);

        if (scene == nullptr)
            return "";

        std::string label = scene;
        size_t paren = label.find('(');

        if (paren != std::string::npos)
            label = label.substr(0, paren);

        while (!label.empty() && label.back() == ' ')
            label.pop_back();

        if (label.empty())
            return "";

        label += (entry.age == 0) ? " (Adult)" : " (Child)";
        return label;
    }

    void DrawPlayerRow(const PlayerEntry& entry, PlayerListActionEventInvite& inviteEvent,
                       PlayerListActionEvent*& pending) {
        ImGui::PushID((int)(entry.networkID));

        if (entry.isInParty)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.95f, 0.55f, 1.00f));
        else if (entry.pendingInvitePartyID != 0)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 0.85f, 0.40f, 1.00f));
        else
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_Text));

        std::string label = entry.name;

        if (entry.pendingInvitePartyID != 0)
            label = ICON_FA_ENVELOPE " " + label;

        ImGui::Selectable(label.c_str());
        ImGui::PopStyleColor();

        if (entry.sceneNum >= 0) {
            std::string location = LocationLabel(entry);

            if (!location.empty()) {
                const float padding = ImGui::GetStyle().FramePadding.x;
                const float spacing = ImGui::GetStyle().ItemSpacing.x;
                const ImVec2 rowStart = ImGui::GetItemRectMin();
                const float rowRight = ImGui::GetItemRectMax().x - padding;
                const float nameEnd = rowStart.x + ImGui::CalcTextSize(label.c_str()).x + spacing;

                const float textWidth = ImGui::CalcTextSize(location.c_str()).x;
                const float available = rowRight - nameEnd;

                if (available > 0.0f) {
                    const float drawX = (textWidth > available) ? nameEnd : (rowRight - textWidth);
                    const ImVec2 drawPos = ImVec2(drawX, rowStart.y + 2);

                    ImGui::PushClipRect(
                        drawPos, ImVec2(rowRight + 1.0f, rowStart.y + ImGui::GetTextLineHeightWithSpacing()), true);
                    ImGui::GetWindowDrawList()->AddText(drawPos, ImGui::GetColorU32(ImGuiCol_TextDisabled),
                                                        location.c_str());
                    ImGui::PopClipRect();
                }
            }
        }

        if (ImGui::BeginPopupContextItem()) {
            ImGui::TextDisabled("%s", entry.name.c_str());
            ImGui::Separator();

            if (entry.pendingInvitePartyID != 0) {
                if (ImGui::MenuItem(ICON_FA_CHECK " Accept party invite")) {
                    inviteEvent.actionEvent.action = PlayerListAction::AcceptInvite;
                    inviteEvent.networkID = entry.networkID;
                    inviteEvent.partyID = entry.pendingInvitePartyID;
                    pending = &inviteEvent.actionEvent;
                }
                if (ImGui::MenuItem(ICON_FA_TIMES " Decline")) {
                    inviteEvent.actionEvent.action = PlayerListAction::DeclineInvite;
                    inviteEvent.networkID = entry.networkID;
                    inviteEvent.partyID = entry.pendingInvitePartyID;
                    pending = &inviteEvent.actionEvent;
                }
            } else if (entry.isInParty) {
                if (ImGui::MenuItem(ICON_FA_MAP_MARKER " Teleport to")) {
                    inviteEvent.actionEvent.action = PlayerListAction::TeleportTo;
                    inviteEvent.networkID = entry.networkID;
                    inviteEvent.partyID = 0;
                    pending = &inviteEvent.actionEvent;
                }
            } else if (m_hasParty) {
                if (ImGui::MenuItem(ICON_FA_USER_PLUS " Invite to party")) {
                    inviteEvent.actionEvent.action = PlayerListAction::Invite;
                    inviteEvent.networkID = entry.networkID;
                    pending = &inviteEvent.actionEvent;
                }
            } else {
                ImGui::TextDisabled("No actions");
            }

            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

  public:
    using GuiWindow::GuiWindow;

    inline static ZeldaOnlineRoomWindow* Instance;

    ~ZeldaOnlineRoomWindow() {
        if (Instance == this)
            Instance = nullptr;
    }

    void SetDisplayName(const std::string& name) {
        snprintf(m_nameBuffer, sizeof(m_nameBuffer), "%s", name.c_str());
    }

    void SetActionHandler(std::function<void(const PlayerListActionEvent*)> handler) {
        m_onAction = std::move(handler);
    }

    void SetAvailableSkins(const std::vector<SkinOption>& skins) {
        m_availableSkins = skins;
    }

    void SetCurrentSkin(const std::string& skin) {
        m_currentSkin = skin;
        CVarSetString("gZeldaOnline.Skin", m_currentSkin.c_str());
        Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }

    void SetHasParty(bool hasParty) {
        m_hasParty = hasParty;

        if (!hasParty) {
            for (auto& entry : m_players)
                entry.isInParty = false;
        }
    }

    bool HasParty() const {
        return m_hasParty;
    }

    void SetPartyScenes(bool enabled) {
        m_partyScenes = enabled;
    }

    bool PartyScenes() const {
        return m_partyScenes;
    }

    void AddPlayer(const PlayerEntry& player) {

        if (PlayerEntry* existing = FindPlayerEntry(player.networkID)) {
            existing->name = player.name;
            return;
        }

        m_players.push_back(player);
    }

    void RemovePlayer(uint32_t networkID) {
        for (size_t i = 0; i < m_players.size(); i++) {
            if (m_players[i].networkID == networkID) {
                m_players.erase(m_players.begin() + i);
                return;
            }
        }
    }

    std::string NameOf(uint32_t networkID) {

        if (PlayerEntry* entry = FindPlayerEntry(networkID))
            return entry->name;

        return "";
    }

    void UpdateEntry(uint32_t networkID, const std::string& name, int sceneIndex, int roomIndex, uint8_t age) {
        if (PlayerEntry* entry = FindPlayerEntry(networkID)) {
            entry->name = name;
            entry->sceneNum = sceneIndex;
            entry->roomIndex = roomIndex;
            entry->age = age;
        }
    }

    void SetInParty(uint32_t networkID, bool inParty) {

        if (PlayerEntry* entry = FindPlayerEntry(networkID)) {
            entry->isInParty = inParty;
            if (inParty)
                entry->pendingInvitePartyID = 0;
        }
    }

    void SetPendingPartyInvite(uint32_t networkID, uint32_t partyID) {
        if (PlayerEntry* entry = FindPlayerEntry(networkID))
            entry->pendingInvitePartyID = partyID;
    }

    uint32_t PendingInvitePartyID(uint32_t networkID) {
        if (PlayerEntry* entry = FindPlayerEntry(networkID))
            return entry->pendingInvitePartyID;

        return 0;
    }

    void ClearPendingInvites() {
        for (auto& entry : m_players)
            entry.pendingInvitePartyID = 0;
    }

    void ClearPartyFlags() {

        for (auto& entry : m_players) {
            entry.isInParty = false;
            entry.pendingInvitePartyID = 0;
        }
    }

    void ClearPlayers() {
        m_players.clear();
    }

    void SetConnected(bool connected) {
        m_connected = connected;
        m_connecting = false;

        if (!connected) {
            m_players.clear();
            m_hasParty = false;
            m_fieldsLoaded = false;
        } else {
            SetDisplayName("");
        }
    }

    void SetConnecting(bool connecting) {
        m_connecting = connecting;
    }

    bool IsConnected() const {
        return m_connected;
    }

    void CenterAndExpand() {
        m_recenterNextFrame = true;
    }

    void SetStatus(const std::string& text, const ImVec4& color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f)) {
        m_statusText = text;
        m_statusColor = color;
    }

    void ClearStatus() {
        m_statusText.clear();
    }

    void Reset() {
        m_players.clear();
        m_hasParty = false;
        m_partyScenes = false;
        m_connected = false;
        m_connecting = false;
        m_statusText.clear();
        m_statusColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        m_fieldsLoaded = false;
    }

    void BeginAutoConnect() {
        m_beginAutoConnect = true;
    }

    void InitElement() override {};

    void LoadConnectionFields() {
        if (m_fieldsLoaded)
            return;

        snprintf(m_hostBuffer, sizeof(m_hostBuffer), "%s", CVarGetString("gZeldaOnline.Host", "107.175.79.45"));
        m_portValue = CVarGetInteger("gZeldaOnline.Port", 21050);
        snprintf(m_nameBuffer, sizeof(m_nameBuffer), "%s", CVarGetString("gZeldaOnline.Nickname", "Player"));
        snprintf(m_fileServerBuffer, sizeof(m_fileServerBuffer), "%s", CVarGetString("gZeldaOnline.FileServer", ""));
        m_autoConnect = CVarGetInteger("gZeldaOnline.AutoConnect", 0) != 0;

        m_fieldsLoaded = true;
    }

    void SaveConnectionFields() {
        CVarSetString("gZeldaOnline.Host", m_hostBuffer);
        CVarSetInteger("gZeldaOnline.Port", m_portValue);
        CVarSetString("gZeldaOnline.Nickname", m_nameBuffer);
        CVarSetString("gZeldaOnline.FileServer", m_fileServerBuffer);
        CVarSetInteger("gZeldaOnline.AutoConnect", m_autoConnect ? 1 : 0);
    }

    void DrawConnectSetup(PlayerListActionEventConnect& connectEvent, PlayerListActionEvent*& pending) {
        LoadConnectionFields();

        ImGui::TextUnformatted("Host");
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputTextWithHint("##zo_host", "127.0.0.1", m_hostBuffer, sizeof(m_hostBuffer));

        ImGui::TextUnformatted("Port");
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputInt("##zo_port", &m_portValue, 0, 0);

        if (m_portValue < 0)
            m_portValue = 0;
        if (m_portValue > 65535)
            m_portValue = 65535;

        ImGui::TextUnformatted("Nickname");
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputTextWithHint("##zo_nick", "Player", m_nameBuffer, sizeof(m_nameBuffer));

        ImGui::TextUnformatted("Resource Server");
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputTextWithHint("##zo_fileserver", "http://...", m_fileServerBuffer, sizeof(m_fileServerBuffer));

        ImGui::Spacing();

        bool valid = m_hostBuffer[0] != '\0' && m_nameBuffer[0] != '\0' && m_portValue > 1024 && m_portValue < 65535;

        ImGui::BeginDisabled(!valid || m_connecting);

        if (ImGui::Button(m_connecting ? "Connecting..." : "Connect", ImVec2(-FLT_MIN, 0.0f)) || m_beginAutoConnect) {
            connectEvent.actionEvent.action = PlayerListAction::Connect;
            connectEvent.host = m_hostBuffer;
            connectEvent.port = (uint16_t)(m_portValue);
            connectEvent.nickname = m_nameBuffer;
            connectEvent.fileServer = m_fileServerBuffer;
            pending = &connectEvent.actionEvent;
            m_connecting = true;
            m_beginAutoConnect = false;
            SaveConnectionFields();
        }

        ImGui::EndDisabled();

        if (ImGui::Checkbox("Auto-connect", &m_autoConnect)) {
            CVarSetInteger("gZeldaOnline.AutoConnect", m_autoConnect ? 1 : 0);
            Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
        }

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Connect automatically when the game starts");

        if (m_connecting) {
            if (ImGui::Button(ICON_FA_TIMES " Cancel", ImVec2(-FLT_MIN, 0.0f))) {
                connectEvent.actionEvent.action = PlayerListAction::Disconnect;
                pending = &connectEvent.actionEvent;
                m_connecting = false;
            }
        }
    }

    void DrawElement() override {
        PlayerListActionEventInvite inviteEvent;
        PlayerListActionEventText textEvent;
        PlayerListActionEventToggle toggleEvent;
        PlayerListActionEventConnect connectEvent;
        PlayerListActionEvent actionEvent;
        PlayerListActionEvent* pending = nullptr;

        if (!m_connected) {
            DrawConnectSetup(connectEvent, pending);

            if (!m_statusText.empty()) {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, m_statusColor);
                ImGui::TextWrapped("%s", m_statusText.c_str());
                ImGui::PopStyleColor();
            }

            if (pending != nullptr && m_onAction)
                m_onAction(pending);

            return;
        }

        {
            DrawMenuBar(textEvent, toggleEvent, pending);

            float footerHeight = (ImGui::GetFrameHeightWithSpacing() * 3.0f) + 12.0f;

            if (!m_statusText.empty())
                footerHeight += ImGui::GetTextLineHeightWithSpacing();

            if (m_hasParty) {
                ImGui::Text(ICON_FA_USERS " Party: %d others", PartyMemberCount());
                ImGui::Separator();

                for (const auto& entry : m_players) {
                    if (entry.isInParty)
                        DrawPlayerRow(entry, inviteEvent, pending);
                }

                ImGui::Spacing();
            }

            int otherCount = (int)(m_players.size()) - PartyMemberCount();

            ImGui::Text(ICON_FA_USER " Players: %d others", otherCount);
            ImGui::Separator();

            if (ImGui::BeginChild("##zo_player_list", ImVec2(0, -footerHeight), false)) {
                if (otherCount == 0)
                    ImGui::TextDisabled("No other players connected");

                for (const auto& entry : m_players) {
                    if (!entry.isInParty)
                        DrawPlayerRow(entry, inviteEvent, pending);
                }
            }
            ImGui::EndChild();

            ImGui::Separator();

            ImGui::TextUnformatted("Skin");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-FLT_MIN);

            const std::string currentKey = m_currentSkin.empty() ? std::string("link") : m_currentSkin;
            const char* preview = currentKey.c_str();

            for (const auto& skin : m_availableSkins) {
                if (skin.referenceName == currentKey) {
                    preview = skin.displayName.c_str();
                    break;
                }
            }

            if (ImGui::BeginCombo("##zo_skin", preview)) {
                if (m_availableSkins.empty())
                    ImGui::TextDisabled("No skins found");

                for (const auto& skin : m_availableSkins) {
                    bool selected = skin.referenceName == currentKey;
                    ImGui::PushID(skin.referenceName.c_str());

                    if (ImGui::Selectable(skin.displayName.c_str(), selected)) {
                        textEvent.actionEvent.action = PlayerListAction::ChangeSkin;
                        textEvent.text = skin.referenceName;
                        pending = &textEvent.actionEvent;
                    }

                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", skin.referenceName.c_str());

                    if (selected)
                        ImGui::SetItemDefaultFocus();

                    ImGui::PopID();
                }

                ImGui::EndCombo();
            }

            ImGui::TextUnformatted("Name");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-FLT_MIN);

            if (ImGui::InputTextWithHint("##zo_name", "Display name...", m_nameBuffer, sizeof(m_nameBuffer),
                                         ImGuiInputTextFlags_EnterReturnsTrue)) {
                if (m_nameBuffer[0] != '\0') {
                    textEvent.actionEvent.action = PlayerListAction::ChangeName;
                    textEvent.text = m_nameBuffer;
                    pending = &textEvent.actionEvent;
                }
            }

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));

            if (ImGui::Button(ICON_FA_SIGN_OUT "##zo_disconnect")) {
                actionEvent.action = PlayerListAction::Disconnect;
                pending = &actionEvent;
            }

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Disconnect");

            ImGui::SameLine();

            if (ImGui::Button(ICON_FA_REFRESH "##zo_unstuck")) {
                actionEvent.action = PlayerListAction::UnstuckMe;
                pending = &actionEvent;
            }

            ImGui::PopStyleVar();

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Unstuck me");

            if (!m_statusText.empty()) {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, m_statusColor);
                ImGui::TextWrapped("%s", m_statusText.c_str());
                ImGui::PopStyleColor();
            }
        }

        if (pending != nullptr && m_onAction)
            m_onAction(pending);
    }

    void Draw() override {
        if (!IsVisible()) {
            return;
        }

        ImGui::PushStyleColor(ImGuiCol_WindowBg,
                              ImVec4(0, 0, 0, CVarGetFloat(CVAR_SETTING("Notifications.BgOpacity"), 0.5f)));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));

        const ImVec2 defaultSize = ImVec2(240.0f, 380.0f);
        auto vp = ImGui::GetMainViewport();

        ImGui::SetNextWindowSize(defaultSize, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(180.0f, 200.0f), ImVec2(FLT_MAX, FLT_MAX));

        const ImGuiCond placementCond = m_recenterNextFrame ? ImGuiCond_Always : ImGuiCond_FirstUseEver;

        ImGui::SetNextWindowPos(
            ImVec2(vp->Pos.x + (vp->Size.x - defaultSize.x) * 0.5f, vp->Pos.y + (vp->Size.y - defaultSize.y) * 0.5f),
            placementCond, ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowCollapsed(false, placementCond);

        if (m_recenterNextFrame) {
            ImGui::SetNextWindowSize(defaultSize, ImGuiCond_Always);
            ImGui::SetNextWindowFocus();
            m_recenterNextFrame = false;
        }

        if (ImGui::Begin("Zelda Online", &mIsVisible,
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_MenuBar)) {
            DrawElement();
        }

        ImGui::End();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(1);
    }

    void UpdateElement() override {};
};

} // namespace ZeldaOnline
#endif