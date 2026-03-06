// main.cpp
// Rezn-seedr: interactive Step CA bootstrapper in a TUI using ImTui + Dear ImGui
//
// Build (assuming pkg‑config can locate ncurses):
//   g++ -std=c++23 -I/path/to/imtui -I/path/to/imgui -lstdc++fs \
//       main.cpp imtui-ncurses.cpp imgui.cpp imgui_draw.cpp imgui_widgets.cpp -lncursesw -lpthread -o rezn-seedr-tui
//
// Run:
//   ./rezn-seedr-tui
//
// NOTE: Error handling has been shortened for clarity; adapt as needed.

#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

#include "imtui/imtui.h"
#include "imtui/imtui-impl-ncurses.h"
#include "imtui/imtui-impl-opengl3.h"

using namespace std::literals;

// -------------------------------------------------------------
// Helpers
// -------------------------------------------------------------

static std::string randomPassword(std::size_t length = 14)
{
    static constexpr std::string_view alphabet =
        "ABCDEFGHJKLMNPQRSTUVWXYZ"
        "abcdefghijkmnopqrstuvwxyz"
        "0123456789@$*?#";
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<std::size_t> dist(0, alphabet.size() - 1);

    std::string result;
    result.reserve(length);
    for (std::size_t i = 0; i < length; ++i)
    {
        result.push_back(alphabet[dist(gen)]);
    }
    return result;
}

static bool secureDelete(const std::filesystem::path &p)
{
    try
    {
        auto sz = std::filesystem::file_size(p);
        if (sz > 0)
        {
            std::fstream f(p, std::ios::in | std::ios::out | std::ios::binary);
            std::vector<char> zeros(4096, '\0');
            while (sz > 0)
            {
                auto chunk = std::min<std::uintmax_t>(zeros.size(), sz);
                f.write(zeros.data(), chunk);
                sz -= chunk;
            }
        }
        std::filesystem::remove(p);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

// Very small 'which' clone
static std::string findExe(std::string_view exe)
{
    auto path = std::getenv("PATH");
    if (!path)
        return {};
    std::string_view sv{path};
    std::size_t beg = 0;
    while (beg < sv.size())
    {
        auto end = sv.find(':', beg);
        if (end == std::string_view::npos)
            end = sv.size();
        auto dir = sv.substr(beg, end - beg);
        std::filesystem::path p = std::string(dir);
        p /= exe;
        if (std::filesystem::exists(p) && (p.status().permissions() & std::filesystem::perms::owner_exec) != std::filesystem::perms::none)
        {
            return p.string();
        }
        beg = end + 1;
    }
    return {};
}

// Wrap popen, capture stdout/stderr combined
static std::string execCapture(const std::string &cmd, int &exitCode)
{
    std::array<char, 256> buf;
    std::string output;
#ifdef _WIN32
    FILE *pipe = _popen((cmd + " 2>&1").c_str(), "r");
#else
    FILE *pipe = popen((cmd + " 2>&1").c_str(), "r");
#endif
    if (!pipe)
    {
        exitCode = -1;
        return "Failed to spawn process\n";
    }
    while (std::fgets(buf.data(), buf.size(), pipe) != nullptr)
    {
        output += buf.data();
    }
#ifdef _WIN32
    exitCode = _pclose(pipe);
#else
    exitCode = pclose(pipe);
#endif
    return output;
}

// -------------------------------------------------------------
// State container
// -------------------------------------------------------------

struct AppState
{
    char caName[64] = "rezn-seedr";
    char dnsName[64] = "localhost";
    char addr[64] = "127.0.0.1:9000";
    char provName[64] = "admin";
    bool addAcme = false;
    bool enableSsh = true;
    bool disableDb = false;
    char caPass[32] = "";
    char provPass[32] = "";
    bool caPassRandom = false;
    bool provPassRandom = false;

    std::string log;
    bool initialized = false;
};

// -------------------------------------------------------------
// UI
// -------------------------------------------------------------

static void drawUi(AppState &st)
{
    ImGui::Begin("rezn-seedr – Step CA bootstrapper (TUI)");

    ImGui::InputText("CA name", st.caName, IM_ARRAYSIZE(st.caName));
    ImGui::InputText("DNS name", st.dnsName, IM_ARRAYSIZE(st.dnsName));
    ImGui::InputText("Listen addr", st.addr, IM_ARRAYSIZE(st.addr));
    ImGui::InputText("Provisioner", st.provName, IM_ARRAYSIZE(st.provName));

    ImGui::Checkbox("Add ACME provisioner", &st.addAcme);
    ImGui::Checkbox("Enable SSH CA", &st.enableSsh);
    ImGui::Checkbox("Disable DB", &st.disableDb);

    ImGui::Separator();
    ImGui::Checkbox("Randomize CA password", &st.caPassRandom);
    if (!st.caPassRandom)
    {
        ImGui::InputText("CA password", st.caPass, IM_ARRAYSIZE(st.caPass), ImGuiInputTextFlags_Password);
    }

    ImGui::Checkbox("Randomize provisioner password", &st.provPassRandom);
    if (!st.provPassRandom)
    {
        ImGui::InputText("Provisioner password", st.provPass, IM_ARRAYSIZE(st.provPass), ImGuiInputTextFlags_Password);
    }

    if (ImGui::Button("Initialize CA"))
    {
        st.log.clear();
        st.initialized = false;

        // Pre‑flight
        auto stepExe = findExe("step");
        if (stepExe.empty())
        {
            st.log = "'step' binary not found in PATH\n";
            goto done;
        }

        // Maybe generate passwords
        std::string caPw = st.caPassRandom ? randomPassword() : st.caPass;
        std::string provPw = st.provPassRandom ? randomPassword() : st.provPass;

        // Temp files
        std::filesystem::path caPwFile;
        std::filesystem::path provPwFile;

        auto createTemp = [&](const std::string &hint, const std::string &content, std::filesystem::path &out) -> bool
        {
            if (content.empty())
                return true;
            char tmpl[] = "/tmp/XXXXXX";
            int fd = mkstemp(tmpl);
            if (fd == -1)
                return false;
            out = tmpl;
            ::write(fd, content.data(), content.size());
            ::close(fd);
            chmod(out.c_str(), S_IRUSR | S_IWUSR);
            return true;
        };

        if (!createTemp("step-pw", caPw, caPwFile))
        {
            st.log = "Failed to create temp file for CA password\n";
            goto done;
        }
        if (!createTemp("step-provpw", provPw, provPwFile))
        {
            st.log = "Failed to create temp file for provisioner password\n";
            goto done;
        }

        // Build arguments
        std::ostringstream cmd;
        cmd << '"' << stepExe << "" ca init "
            << " --name \"" << st.caName << "\""
            << " --dns \"" << st.dnsName << "\""
            << " --address \"" << st.addr << "\""
            << " --provisioner \"" << st.provName << "\""
            << " --deployment-type standalone";

        if (st.addAcme)
            cmd << " --acme";
        if (st.enableSsh)
            cmd << " --ssh";
        if (!caPwFile.empty())
            cmd << " --password-file \"" << caPwFile.string() << "\"";
        if (!provPwFile.empty())
            cmd << " --provisioner-password-file \"" << provPwFile.string() << "\"";
        if (st.disableDb)
            cmd << " --no-db";

        st.log += "Running: " + cmd.str() + "\n\n";

        int ret = 0;
        st.log += execCapture(cmd.str(), ret);

        if (ret == 0)
        {
            st.log += "\n[OK] CA initialized successfully!\n";
            st.initialized = true;
        }
        else
        {
            st.log += "\n[ERR] step-ca init failed with exit code: " + std::to_string(ret) + "\n";
        }

    done:
        if (!st.initialized)
        {
            if (!caPwFile.empty())
                secureDelete(caPwFile);
            if (!provPwFile.empty())
                secureDelete(provPwFile);
        }
    }

    ImGui::Separator();
    ImGui::Text("Log:");
    ImGui::BeginChild("log", ImVec2(0, 12 * ImGui::GetTextLineHeightWithSpacing()), true);
    ImGui::TextUnformatted(st.log.c_str());
    ImGui::EndChild();

    ImGui::End();
}

// -------------------------------------------------------------
// main
// -------------------------------------------------------------

int main()
{
    ImTui::TuiApplication app;
    app.setGlobalDrawCallback(drawUi);
    app.run(80, 45); // width, height in chars
    return 0;
}
