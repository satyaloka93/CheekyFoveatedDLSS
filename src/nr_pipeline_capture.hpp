#pragma once
#include <filesystem>
#include <string>
#include <cstdint>
namespace cheeky::foveated_dlss {
bool request_nr_pipeline_capture(const std::filesystem::path& directory) noexcept;
std::string nr_pipeline_capture_status();
bool nr_pipeline_capture_busy();
unsigned nr_pipeline_capture_saved();
void cancel_nr_pipeline_capture();
}
