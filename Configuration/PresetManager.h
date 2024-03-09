
/** $VER: PresetManager.h (2024.03.09) P. Stuer - Represents a preset of the component. **/

#pragma once

#include "State.h"

#include <Path.h>

class PresetManager
{
public:
    PresetManager() = delete;

    PresetManager(const PresetManager &) = delete;
    PresetManager & operator=(const PresetManager &) = delete;
    PresetManager(PresetManager &&) = delete;
    PresetManager & operator=(PresetManager &&) = delete;

    ~PresetManager() = delete;

    static bool Load(const Path & rootPath, const std::wstring & presetName, State * state) noexcept;
    static bool Save(const Path & rootPath, const std::wstring & presetName, const State * state) noexcept;
    static bool Delete(const Path & rootPath, const std::wstring & presetName) noexcept;

    static bool GetPresetNames(const Path & rootPath, std::vector<std::wstring> & FileNames) noexcept;

private:
    static const DWORD Magic = mmioFOURCC('F','V','S','A');
    static const DWORD Version = 1;
};
