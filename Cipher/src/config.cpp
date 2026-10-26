#include "win_pre.h"
#include "config.h"
#include <fstream>
#include <string>

std::wstring Config::DefaultPath() {
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring p = buf;
    size_t s = p.find_last_of(L"\\/");
    if (s != std::wstring::npos) p = p.substr(0, s + 1);
    return p + L"cipher_config.ini";
}

static void WriteKV(std::ofstream& f, const char* k, bool v) {
    f << k << '=' << (v ? 1 : 0) << '\n';
}
static void WriteKV(std::ofstream& f, const char* k, float v) {
    f << k << '=' << v << '\n';
}
static void WriteKV(std::ofstream& f, const char* k, int v) {
    f << k << '=' << v << '\n';
}

bool Config::Save(const EspConfig& e, const AimConfig& a, const TriggerConfig& t, const std::wstring& path) {
    std::ofstream f(path);
    if (!f) return false;

    f << "[esp]\n";
    WriteKV(f, "enabled", e.enabled);
    WriteKV(f, "box", e.box);
    WriteKV(f, "skeleton", e.skeleton);
    WriteKV(f, "name", e.name);
    WriteKV(f, "distance", e.distance);
    WriteKV(f, "headDot", e.headDot);
    WriteKV(f, "snaplines", e.snaplines);
    WriteKV(f, "teamCheck", e.teamCheck);
    WriteKV(f, "radar", e.radar);
    WriteKV(f, "outline", e.outline);
    WriteKV(f, "healthBar", e.healthBar);
    WriteKV(f, "chams", e.chams);
    WriteKV(f, "boxStyle", (int)e.boxStyle);
    WriteKV(f, "maxDistance", e.maxDistance);
    WriteKV(f, "boxThickness", e.boxThickness);
    WriteKV(f, "skelThickness", e.skelThickness);

    f << "[aim]\n";
    WriteKV(f, "enabled", a.enabled);
    WriteKV(f, "softAim", a.softAim);
    WriteKV(f, "silent", a.silent);
    WriteKV(f, "fovDeg", a.fovDeg);
    WriteKV(f, "softSmooth", a.softSmooth);
    WriteKV(f, "hardSmooth", a.hardSmooth);
    WriteKV(f, "bone", a.bone);
    WriteKV(f, "aimKey", a.aimKey);
    WriteKV(f, "drawFov", a.drawFov);
    WriteKV(f, "fovRadar", a.fovRadar);
    WriteKV(f, "visibleCheck", a.visibleCheck);
    WriteKV(f, "aimKeyToggle", a.aimKeyToggle);

    f << "[trigger]\n";
    WriteKV(f, "enabled", t.enabled);
    WriteKV(f, "delayMs", t.delayMs);
    WriteKV(f, "maxDist", t.maxDist);
    WriteKV(f, "visibleOnly", t.visibleOnly);
    WriteKV(f, "requireKey", t.requireKey);
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
        std::string k = line.substr(0, eq);
        std::string v = line.substr(eq + 1);

        if (section == "[esp]") {
            if (k == "enabled") setb(e.enabled, v);
            else if (k == "box") setb(e.box, v);
            else if (k == "skeleton") setb(e.skeleton, v);
            else if (k == "name") setb(e.name, v);
            else if (k == "distance") setb(e.distance, v);
            else if (k == "radar") setb(e.radar, v);
            else if (k == "chams") setb(e.chams, v);
            else if (k == "maxDistance") e.maxDistance = std::stof(v);
            else if (k == "boxStyle") e.boxStyle = (BoxStyle)std::stoi(v);
        } else if (section == "[aim]") {
            if (k == "enabled") setb(a.enabled, v);
            else if (k == "softAim") setb(a.softAim, v);
            else if (k == "silent") setb(a.silent, v);
            else if (k == "fovDeg") a.fovDeg = std::stof(v);
            else if (k == "softSmooth") a.softSmooth = std::stof(v);
            else if (k == "hardSmooth") a.hardSmooth = std::stof(v);
            else if (k == "smooth") a.softSmooth = std::stof(v); // legacy
            else if (k == "bone") a.bone = std::stoi(v);
            else if (k == "aimKey") a.aimKey = std::stoi(v);
            else if (k == "drawFov") setb(a.drawFov, v);
            else if (k == "fovRadar") setb(a.fovRadar, v);
            else if (k == "visibleCheck") setb(a.visibleCheck, v);
            else if (k == "aimKeyToggle") setb(a.aimKeyToggle, v);
        } else if (section == "[trigger]") {
            if (k == "enabled") setb(t.enabled, v);
            else if (k == "delayMs") t.delayMs = std::stoi(v);
            else if (k == "maxDist") t.maxDist = std::stof(v);
            else if (k == "visibleOnly") setb(t.visibleOnly, v);
            else if (k == "requireKey") setb(t.requireKey, v);
        }
    }
    return true;
}
