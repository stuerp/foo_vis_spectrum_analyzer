
/** $VER: PresetManager.h (2024.03.09) P. Stuer - Represents a preset of the component. **/

#pragma once

#include "State.h"

#include <Path.h>

#define MakeFOURCC(ch0, ch1, ch2, ch3) ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) | ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24))

class PresetManager
{
public:
    PresetManager() = delete;

    PresetManager(const PresetManager &) = delete;
    PresetManager & operator=(const PresetManager &) = delete;
    PresetManager(PresetManager &&) = delete;
    PresetManager & operator=(PresetManager &&) = delete;

    ~PresetManager() = delete;

    static bool Load(const Path & rootPath, const std::wstring & presetName, state_t * state) noexcept;
    static bool Save(const Path & rootPath, const std::wstring & presetName, const state_t * state) noexcept;
    static bool Delete(const Path & rootPath, const std::wstring & presetName) noexcept;

    static bool GetPresetNames(const Path & rootPath, std::vector<std::wstring> & FileNames) noexcept;

private:
    static const uint32_t Magic = MakeFOURCC('F','V','S','A');
    static const uint32_t Version = 1;
};
