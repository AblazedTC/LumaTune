#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>

namespace luma::test {

// RAII temp directory, removed on destruction. AGENTS.md §9: "use
// tests/support/TempDir" for tests that need real file IO.
class TempDir {
public:
    TempDir() : m_path(std::filesystem::temp_directory_path() / uniqueName()) {
        std::filesystem::create_directories(m_path);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(m_path, ec);
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    const std::filesystem::path& path() const { return m_path; }

private:
    static std::string uniqueName() {
        static std::atomic<unsigned> counter{0};
        const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        return "luma_test_" + std::to_string(now) + "_" + std::to_string(counter++);
    }

    std::filesystem::path m_path;
};

} // namespace luma::test
