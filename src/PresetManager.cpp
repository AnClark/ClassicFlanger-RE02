#include "PresetManager.h"
#include "UI.h"

#include "../3rdparty/json.hpp"

#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstring>

#if defined(_WIN32)
#  include <windows.h>
#  include <shlobj.h>
#elif defined(__APPLE__)
#  include <unistd.h>
#  include <pwd.h>
#else
// Linux / other POSIX
#  include <unistd.h>
#  include <pwd.h>
#endif

using json = nlohmann::json;
namespace fs = std::filesystem;

// ── Factory presets ────────────────────────────────────────────────────────
// Preset fields (struct order): Name, DelayMs, Rate, Depth, Feedback, Mix, StereoPhase, Level, Spread, BalancedFeedback
static const Preset kFactoryPresets[] = {
    {"Elec. Guitar",   1.6f,  1.17f, 0.5f,  0.5f,  0.5f,  90.0f,  0.0f, 0.0f, 1.0f},
    {"Subtle Flange",  5.0f,  0.50f, 0.25f, 0.3f,  0.35f, 90.0f,  0.0f, 0.0f, 0.0f},
    {"Deep Flange",    3.0f,  0.30f, 0.75f, 0.5f,  0.5f,  90.0f,  0.0f, 0.0f, 0.0f},
    {"Jet Flange",     1.5f,  0.15f, 0.90f, 0.75f, 0.6f,  90.0f,  0.0f, 0.0f, 1.0f},
    {"Vibrato",        3.0f,  2.00f, 1.00f, 0.0f,  1.0f,  0.0f,   0.0f, 0.0f, 0.0f},
    {"Chorus-ish",     8.0f,  1.20f, 0.50f, 0.2f,  0.4f,  90.0f,  0.0f, 1.0f, 1.0f},
    {"Saw Madness",    2.5f,  1.50f, 0.60f, 0.5f,  0.5f,  90.0f,  0.0f, 0.0f, 0.0f},
    {"Wide Stereo",    4.0f,  0.70f, 0.40f, 0.4f,  0.45f, 180.0f, 0.0f, 1.0f, 1.0f},
    {"Resonant",       2.0f,  0.40f, 0.50f, 0.85f, 0.5f,  90.0f,  0.0f, 0.0f, 0.0f},
    {"Punchy Up",      3.5f,  0.90f, 0.45f, 0.2f,  0.6f,  90.0f,  3.0f, 1.0f, 1.0f},
};
static constexpr int kFactoryPresetsCount = (int)(sizeof(kFactoryPresets) / sizeof(kFactoryPresets[0]));

// ── Default preset ─────────────────────────────────────────────────────────
// This is used as a fallback when loading an imported preset from a file that doesn't contain all the expected fields.
// Since the Preset struct has default member initializers, we can just use the field values from the default-constructed Preset object.
static const Preset kDefaultPreset;

// ── Constructor ────────────────────────────────────────────────────────────
PresetManager::PresetManager(ClassicFlangerUI* ui)
    : fUI(ui)
{
}

// ── Factory preset accessors ───────────────────────────────────────────────
int PresetManager::factoryPresetCount() const
{
    return kFactoryPresetsCount;
}

const Preset& PresetManager::factoryPreset(int index) const
{
    DISTRHO_SAFE_ASSERT_RETURN(index >= 0 && index < kFactoryPresetsCount, kFactoryPresets[0])
    return kFactoryPresets[index];
}

// ── User preset accessors ──────────────────────────────────────────────────
int PresetManager::userPresetCount() const
{
    return (int)fUserPresets.size();
}

const Preset& PresetManager::userPreset(int index) const
{
    DISTRHO_SAFE_ASSERT_RETURN(index >= 0 && index < (int)fUserPresets.size(), fImportedPreset)
    return fUserPresets[index];
}

// ── Preset selection ───────────────────────────────────────────────────────
void PresetManager::loadDefaultPreset()
{
    DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr, )
    for (uint32_t i = 0; i < NUM_PARAMS; ++i)
        _triggerParamUpdate(i, paramInfo[i].defaultVal);
    syncPluginState(PresetType::Factory, -1, false);
}

void PresetManager::selectFactoryPreset(int index)
{
    DISTRHO_SAFE_ASSERT_RETURN(index >= 0 && index < kFactoryPresetsCount, )
    _applyPreset(kFactoryPresets[index]);
    syncPluginState(PresetType::Factory, index, false);
}

void PresetManager::selectUserPreset(int index)
{
    DISTRHO_SAFE_ASSERT_RETURN(index >= 0 && index < (int)fUserPresets.size(), )
    _applyPreset(fUserPresets[index]);
    syncPluginState(PresetType::User, index, false);
}

void PresetManager::selectImportedPreset()
{
    _applyPreset(fImportedPreset);
    syncPluginState(PresetType::Imported, -1, false);
}

// ── User preset CRUD ──────────────────────────────────────────────────────
bool PresetManager::nameExists(const std::string& name) const
{
    for (const auto& p : fUserPresets)
        if (p.name == name) return true;
    return false;
}

bool PresetManager::saveAsNew(const std::string& name)
{
    if (nameExists(name))
        return false;

    Preset p  = snapshotFromUI();
    p.name    = name;
    fUserPresets.push_back(p);

    const bool ok = saveUserPresetsToDisk();
    syncPluginState(PresetType::User, (int)fUserPresets.size() - 1, false);
    return ok;
}

bool PresetManager::overwriteCurrent()
{
    if (fCurrentType != PresetType::User)
        return false;
    DISTRHO_SAFE_ASSERT_RETURN(fCurrentIndex >= 0 && fCurrentIndex < (int)fUserPresets.size(), false)

    const std::string name = fUserPresets[fCurrentIndex].name;
    Preset p = snapshotFromUI();
    p.name   = name;
    fUserPresets[fCurrentIndex] = p;

    const bool ok = saveUserPresetsToDisk();
    syncPluginState(false);
    return ok;
}

bool PresetManager::deleteCurrent()
{
    if (fCurrentType != PresetType::User)
        return false;
    DISTRHO_SAFE_ASSERT_RETURN(fCurrentIndex >= 0 && fCurrentIndex < (int)fUserPresets.size(), false)

    fUserPresets.erase(fUserPresets.begin() + fCurrentIndex);
    PresetType newType;
    int        newIndex;
    if (fUserPresets.empty()) {
        newType  = PresetType::Factory;
        newIndex = 0;
    } else {
        newType  = PresetType::User;
        newIndex = std::min(fCurrentIndex, (int)fUserPresets.size() - 1);
    }

    const bool ok = saveUserPresetsToDisk();
    syncPluginState(newType, newIndex, false);
    return ok;
}

bool PresetManager::renameCurrent(const std::string& newName)
{
    if (fCurrentType != PresetType::User)
        return false;
    DISTRHO_SAFE_ASSERT_RETURN(fCurrentIndex >= 0 && fCurrentIndex < (int)fUserPresets.size(), false)

    // No-op: preset already has this name.
    if (fUserPresets[fCurrentIndex].name == newName)
        return true;
    // Reject: another preset already uses this name.
    if (nameExists(newName))
        return false;

    fUserPresets[fCurrentIndex].name = newName;

    const bool ok = saveUserPresetsToDisk();
    syncPluginState(fModified);
    return ok;
}

// ── Import / Export ────────────────────────────────────────────────────────
bool PresetManager::hasImported() const
{
    return !fImportedPreset.name.empty();
}

const Preset* PresetManager::importedPreset() const
{
    return hasImported() ? &fImportedPreset : nullptr;
}

bool PresetManager::importFromFile(const std::string& filePath)
{
    try {
        std::ifstream f(filePath);
        if (!f.is_open()) return false;
        json j = json::parse(f);

        fImportedPreset.name        = j.value("name",         "Imported");
        fImportedPreset.rate        = j.value("rate",         kDefaultPreset.rate);
        fImportedPreset.depth       = j.value("depth",        kDefaultPreset.depth);
        fImportedPreset.feedback    = j.value("feedback",     kDefaultPreset.feedback);
        fImportedPreset.delayMs     = j.value("delay_ms",     kDefaultPreset.delayMs);
        fImportedPreset.mix         = j.value("mix",          kDefaultPreset.mix);
        fImportedPreset.stereoPhase = j.value("stereo_phase", kDefaultPreset.stereoPhase);
        fImportedPreset.level       = j.value("level",        kDefaultPreset.level);
        fImportedPreset.spread      = j.value("spread",       kDefaultPreset.spread);
        fImportedPreset.balancedFeedback = j.value("balanced_feedback", kDefaultPreset.balancedFeedback);
        selectImportedPreset();
        return true;
    } catch (...) {
        return false;
    }
}

bool PresetManager::exportCurrentToFile(const std::string& filePath)
{
    const Preset* p = currentPreset();
    if (!p)
        return false;

    try {
        json j;
        j["name"]          = p->name;
        j["rate"]          = p->rate;
        j["depth"]         = p->depth;
        j["feedback"]      = p->feedback;
        j["delay_ms"]      = p->delayMs;
        j["mix"]           = p->mix;
        j["stereo_phase"]  = p->stereoPhase;
        j["level"]         = p->level;
        j["spread"]        = p->spread;
        j["balanced_feedback"] = p->balancedFeedback;
        std::ofstream f(filePath);
        if (!f.is_open()) return false;
        f << j.dump(4);
        return true;
    } catch (...) {
        return false;
    }
}

bool PresetManager::commitImported(const std::string& name)
{
    Preset p = fImportedPreset;
    p.name   = name;
    fUserPresets.push_back(p);

    const bool ok = saveUserPresetsToDisk();
    syncPluginState(PresetType::User, (int)fUserPresets.size() - 1, false);
    return ok;
}

// ── Disk I/O ───────────────────────────────────────────────────────────────
bool PresetManager::loadUserPresetsFromDisk()
{
    const std::string path = _getUserPresetsFilePath();
    try {
        std::ifstream f(path);
        if (!f.is_open()) return true; // File not found yet — OK
        json j = json::parse(f);
        fUserPresets.clear();
        for (auto& entry : j["presets"]) {
            Preset p;
            p.name          = entry.value("name",           "");
            p.rate          = entry.value("rate",           kDefaultPreset.rate);
            p.depth         = entry.value("depth",          kDefaultPreset.depth);
            p.feedback      = entry.value("feedback",       kDefaultPreset.feedback);
            p.delayMs       = entry.value("delay_ms",       kDefaultPreset.delayMs);
            p.mix           = entry.value("mix",            kDefaultPreset.mix);
            p.stereoPhase   = entry.value("stereo_phase",   kDefaultPreset.stereoPhase);
            p.level         = entry.value("level",          kDefaultPreset.level);
            p.spread        = entry.value("spread",         kDefaultPreset.spread);
            p.balancedFeedback = entry.value("balanced_feedback", kDefaultPreset.balancedFeedback);
            if (!p.name.empty())
                fUserPresets.push_back(std::move(p));
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool PresetManager::saveUserPresetsToDisk()
{
    if (!_ensureDataDirExists())
        return false;

    const std::string path = _getUserPresetsFilePath();
    try {
        json arr = json::array();
        for (const auto& p : fUserPresets) {
            json entry;
            entry["name"]          = p.name;
            entry["rate"]          = p.rate;
            entry["depth"]         = p.depth;
            entry["feedback"]      = p.feedback;
            entry["delay_ms"]      = p.delayMs;
            entry["mix"]           = p.mix;
            entry["stereo_phase"]  = p.stereoPhase;
            entry["level"]         = p.level;
            entry["spread"]        = p.spread;
            entry["balanced_feedback"] = p.balancedFeedback;
            arr.push_back(entry);
        }
        json j;
        j["version"] = 1;
        j["presets"] = arr;
        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << j.dump(4);
        return true;
    } catch (...) {
        return false;
    }
}

// ── State accessors ────────────────────────────────────────────────────────
const Preset* PresetManager::currentPreset() const
{
    switch (fCurrentType) {
    case PresetType::Factory:
        if (fCurrentIndex >= 0 && fCurrentIndex < kFactoryPresetsCount)
            return &kFactoryPresets[fCurrentIndex];
        break;
    case PresetType::User:
        if (fCurrentIndex >= 0 && fCurrentIndex < (int)fUserPresets.size())
            return &fUserPresets[fCurrentIndex];
        break;
    case PresetType::Imported:
        return &fImportedPreset;
    }
    return nullptr;
}

void PresetManager::markModified()
{
    if (!fModified)
        syncPluginState(true);
}

void PresetManager::clearModified()
{
    if (fModified)
        syncPluginState(false);
}

void PresetManager::syncPluginState(PresetType type, int index, bool modified)
{
    fCurrentType  = type;
    fCurrentIndex = index;
    syncPluginState(modified);
}

void PresetManager::syncPluginState(bool modified)
{
    fModified = modified;
    DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr, )

    const Preset* p      = currentPreset();
    const char*   name   = p ? p->name.c_str() : "";
    const char*   mod    = fModified ? "true" : "false";
    const char*   typeStr;

    switch (fCurrentType) {
    case PresetType::Factory:  typeStr = "Factory";  break;
    case PresetType::User:     typeStr = "User";     break;
    case PresetType::Imported: typeStr = "Imported"; break;
    default:                   typeStr = "Factory";  break;
    }

    fUI->setState(STATE_PRESET_NAME,     name);
    fUI->setState(STATE_PRESET_MODIFIED, mod);
    fUI->setState(STATE_PRESET_TYPE,     typeStr);
}

Preset PresetManager::snapshotFromUI() const
{
    Preset p;
    p.rate        = fUI->fParams[pParamRate];
    p.depth       = fUI->fParams[pParamDepth];
    p.feedback    = fUI->fParams[pParamFeedback];
    p.delayMs     = fUI->fParams[pParamDelay];
    p.mix         = fUI->fParams[pParamMix];
    p.stereoPhase = fUI->fParams[pParamStereoPhase];
    p.level       = fUI->fParams[pParamLevel];
    p.spread      = fUI->fParams[pParamSpread];
    p.balancedFeedback = fUI->fParams[pParamBalancedFeedback];
    return p;
}

void PresetManager::restoreFromState(const std::string& typeStr,
                                     const std::string& nameStr,
                                     bool modified)
{
    // Restore type
    if (typeStr == "User")          fCurrentType = PresetType::User;
    else if (typeStr == "Imported") fCurrentType = PresetType::Imported;
    else                            fCurrentType = PresetType::Factory;

    // Restore index by searching for the name in the appropriate list.
    // Factory fallback is -1 ("Default") — not index 0 — so that an empty
    // preset_name on first launch resolves to Default, not the first factory preset.
    fCurrentIndex = -1;
    if (fCurrentType == PresetType::Factory) {
        for (int i = 0; i < kFactoryPresetsCount; ++i) {
            if (kFactoryPresets[i].name == nameStr) { fCurrentIndex = i; break; }
        }
    } else if (fCurrentType == PresetType::User) {
        fCurrentIndex = -1;
        for (int i = 0; i < (int)fUserPresets.size(); ++i) {
            if (fUserPresets[i].name == nameStr) { fCurrentIndex = i; break; }
        }
        // If the user preset no longer exists (e.g. deleted after state was saved),
        // fall back to Factory / -1 (= Default) so the plugin is in a defined state.
        if (fCurrentIndex == -1) {
            fCurrentType  = PresetType::Factory;
            fModified     = false;
        }
    }
    // For Imported, index stays -1 (the name is in fImportedPreset which we can't restore)

    fModified = modified;
}

// ── Private: parameter application ────────────────────────────────────────
void PresetManager::_applyPreset(const Preset& preset)
{
    DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr, )
    _triggerParamUpdate(pParamRate,        preset.rate);
    _triggerParamUpdate(pParamDepth,       preset.depth);
    _triggerParamUpdate(pParamFeedback,    preset.feedback);
    _triggerParamUpdate(pParamDelay,       preset.delayMs);
    _triggerParamUpdate(pParamMix,         preset.mix);
    _triggerParamUpdate(pParamStereoPhase, preset.stereoPhase);
    _triggerParamUpdate(pParamLevel,       preset.level);
    _triggerParamUpdate(pParamSpread,      preset.spread);
    _triggerParamUpdate(pParamBalancedFeedback, preset.balancedFeedback);
}

void PresetManager::_triggerParamUpdate(uint32_t index, float value)
{
    DISTRHO_SAFE_ASSERT_RETURN(index < NUM_PARAMS, )
    fUI->setParameterValue(index, value);
    fUI->parameterChanged(index, value);
}

// ── Private: platform-specific data directory ─────────────────────────────
std::string PresetManager::_getUserDataDir() const
{
#if defined(_WIN32)
    PWSTR wpath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &wpath))) {
        int len = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, nullptr, 0, nullptr, nullptr);
        std::string result(static_cast<size_t>(len - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wpath, -1, &result[0], len, nullptr, nullptr);
        CoTaskMemFree(wpath);
        return result + "\\" CLASSIC_FLANGER_APPDATA_DIR_NAME;
    }
    // Fallback
    const char* appdata = getenv("APPDATA");
    return std::string(appdata ? appdata : ".") + "\\" CLASSIC_FLANGER_APPDATA_DIR_NAME;
#elif defined(__APPLE__)
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : nullptr;
    }
    return std::string(home ? home : ".") + "/Library/Application Support/" CLASSIC_FLANGER_APPDATA_DIR_NAME;
#else
    // Linux / other POSIX
    const char* xdgData = getenv("XDG_DATA_HOME");
    if (xdgData && xdgData[0] != '\0')
        return std::string(xdgData) + "/" CLASSIC_FLANGER_APPDATA_DIR_NAME;
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : nullptr;
    }
    return std::string(home ? home : ".") + "/.local/share/" CLASSIC_FLANGER_APPDATA_DIR_NAME;
#endif
}

std::string PresetManager::_getUserPresetsFilePath() const
{
#if defined(_WIN32)
    return _getUserDataDir() + "\\" CLASSIC_FLANGER_PRESET_FILE_NAME;
#else
    return _getUserDataDir() + "/" CLASSIC_FLANGER_PRESET_FILE_NAME;
#endif
}

bool PresetManager::_ensureDataDirExists() const
{
    try {
        fs::create_directories(_getUserDataDir());
        return true;
    } catch (...) {
        return false;
    }
}
