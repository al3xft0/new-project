#include "win_pre.h"
#include "config.h"
#include <fstream>
#include <sstream>
#include <cstdio>

std::wstring Config::DefaultPath() {
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring p = buf;
    size_t s = p.find_last_of(L"\\/");
    if (s != std::wstring::npos) p = p.substr(0, s + 1);
    return p + L"cipher_config.ini";
}

static void W(std::ofstream& f, const char* k, bool v) { f << k << '=' << (v ? 1 : 0) << '\n'; }
static void W(std::ofstream& f, const char* k, float v) { f << k << '=' << v << '\n'; }
static void W(std::ofstream& f, const char* k, int v) { f << k << '=' << v << '\n'; }

bool Config::Save(const EspConfig& e, const AimConfig& a, const TriggerConfig& t, const std::wstring& path) {
    std::ofstream f(path);
    if (!f) return false;
    f << "[esp]\n";
    W(f, "enabled", e.enabled); W(f, "box", e.box); W(f, "skeleton", e.skeleton);
    W(f, "name", e.name); W(f, "distance", e.distance); W(f, "headDot", e.headDot);
    W(f, "snaplines", e.snaplines); W(f, "teamCheck", e.teamCheck); W(f, "radar", e.radar);
    W(f, "outline", e.outline); W(f, "healthBar", e.healthBar);
    W(f, "boxStyle", (int)e.boxStyle); W(f, "maxDistance", e.maxDistance);
    W(f, "boxThickness", e.boxThickness); W(f, "skelThickness", e.skelThickness);
    f << "[aim]\n";
    W(f, "enabled", a.enabled); W(f, "silent", a.silent); W(f, "fovDeg", a.fovDeg);
    W(f, "smooth", a.smooth); W(f, "bone", a.bone); W(f, "aimKey", a.aimKey);
    W(f, "drawFov", a.drawFov); W(f, "visibleCheck", a.visibleCheck);
    f << "[trigger]\n";
    W(f, "enabled", t.enabled); W(f, "delayMs", t.delayMs); W(f, "maxDist", t.maxDist);
    W(f, "visibleOnly", t.visibleOnly); W(f, "requireKey", t.requireKey);
    return true;
}

bool Config::Load(EspConfig& e, AimConfig& a, TriggerConfig& t, const std::wstring& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::string line, section;
    auto setb = [](bool& b, const std::string& v) { b = (v == "1" || v == "true"); };
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        if (line.front() == '[') { section = line; continue; }
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq), v = line.substr(eq + 1);
        if (section == "[esp]") {
            if (k == "enabled") setb(e.enabled, v);
            else if (k == "box") setb(e.box, v);
            else if (k == "skeleton") setb(e.skeleton, v);
            else if (k == "name") setb(e.name, v);
            else if (k == "distance") setb(e.distance, v);
            else if (k == "radar") setb(e.radar, v);
            else if (k == "maxDistance") e.maxDistance = std::stof(v);
            else if (k == "boxStyle") e.boxStyle = (BoxStyle)std::stoi(v);
        } else if (section == "[aim]") {
            if (k == "enabled") setb(a.enabled, v);
            else if (k == "silent") setb(a.silent, v);
            else if (k == "fovDeg") a.fovDeg = std::stof(v);
            else if (k == "smooth") a.smooth = std::stof(v);
            else if (k == "bone") a.bone = std::stoi(v);
            else if (k == "aimKey") a.aimKey = std::stoi(v);
        } else if (section == "[trigger]") {
            if (k == "enabled") setb(t.enabled, v);
            else if (k == "delayMs") t.delayMs = std::stoi(v);
            else if (k == "maxDist") t.maxDist = std::stof(v);
        }
    }
    return true;
}
