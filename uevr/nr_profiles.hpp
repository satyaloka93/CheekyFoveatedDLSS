#pragma once
#include "settings_io.hpp"

namespace cheeky::foveated_dlss {
inline std::filesystem::path nr_profile_path(const std::filesystem::path& directory, bool previous) {
    return directory / (previous ? L"CheekyNR-previous.ini" : L"CheekyNR-current.ini");
}
inline void copy_nr_settings(Settings& destination, const Settings& source) {
#define CHEEKY_SETTING(name, field) if (std::string_view(name).starts_with("Nr")) destination.field = source.field;
#include "settings_fields.inc"
#undef CHEEKY_SETTING
}
// Profile files use the existing schema, but only NR keys are applied.
inline bool read_nr_profile(const std::filesystem::path& directory, bool previous,
    Settings& settings, std::string& error) {
    const auto path = nr_profile_path(directory, previous);
    Settings profile;
    if (std::filesystem::exists(path)) {
        if (!read_settings_file(path, profile, error)) return false;
    } else if (!previous) {
        return true; // First current launch retains the user's existing tuning.
    }
    copy_nr_settings(settings, profile); // First previous launch uses safe NR defaults (off).
    return true;
}
}
