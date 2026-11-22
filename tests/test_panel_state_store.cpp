/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include <catch2/catch_amalgamated.hpp>

#include <filesystem>
#include <fstream>
#include <string>

#include "adapters/storage/last_landing_recorder.hpp"
#include "adapters/storage/panel_state_store.hpp"
#include "fakes/fake_ports.hpp"

using fs24::adapters::storage::LastLandingRecorder;
using fs24::adapters::storage::PanelStateStore;
namespace storage = fs24::adapters::storage;

namespace
{

// A fresh directory per test, given with its trailing separator like "\work\" in the sim.
class TempDir
{
  public:
    TempDir()
        : path_(std::filesystem::temp_directory_path() /
                ("fs24_store_" + std::to_string(Catch::getSeed()) + "_" + std::to_string(++counter_)))
    {
        std::filesystem::remove_all(path_);
        std::filesystem::create_directories(path_);
    }
    ~TempDir() { std::filesystem::remove_all(path_); }

    std::string prefix() const { return path_.string() + "/"; }
    void write(const std::string &name, const std::string &content) const { std::ofstream(path_ / name) << content; }

  private:
    static inline int     counter_ = 0;
    std::filesystem::path path_;
};

} // namespace

TEST_CASE("panel state starts with defaults when no files exist")
{
    TempDir                dir;
    fs24::test::FakeLogger logger;
    PanelStateStore        store(dir.prefix(), logger);
    store.load();
    CHECK(store.ui_scale_percent() == storage::UI_SCALE_DEFAULT_PERCENT);
    CHECK_FALSE(store.last_landing_payload());
    CHECK(logger.lines.empty());
}

TEST_CASE("panel state survives a restart")
{
    TempDir                dir;
    fs24::test::FakeLogger logger;
    {
        PanelStateStore store(dir.prefix(), logger);
        store.set_ui_scale_percent(250);
        store.set_last_landing_payload(R"({"grade":"BUTTER!"})");
    }
    PanelStateStore restarted(dir.prefix(), logger);
    restarted.load();
    CHECK(restarted.ui_scale_percent() == 250);
    CHECK(restarted.last_landing_payload() == R"({"grade":"BUTTER!"})");
}

TEST_CASE("panel scale is clamped")
{
    TempDir                dir;
    fs24::test::FakeLogger logger;
    PanelStateStore        store(dir.prefix(), logger);
    store.set_ui_scale_percent(9000);
    CHECK(store.ui_scale_percent() == storage::UI_SCALE_MAX_PERCENT);
    store.set_ui_scale_percent(1);
    CHECK(store.ui_scale_percent() == storage::UI_SCALE_MIN_PERCENT);
}

TEST_CASE("corrupt panel state files fall back to defaults and are logged")
{
    TempDir dir;
    dir.write("settings.json", "garbage");
    dir.write("last_landing.json", "{\"grade\":");
    fs24::test::FakeLogger logger;
    PanelStateStore        store(dir.prefix(), logger);
    store.load();
    CHECK(store.ui_scale_percent() == storage::UI_SCALE_DEFAULT_PERCENT);
    CHECK_FALSE(store.last_landing_payload());
    CHECK(logger.lines.size() == 2);
}

TEST_CASE("a failed write is logged, the value still applies")
{
    fs24::test::FakeLogger logger;
    PanelStateStore        store("/nonexistent-fs24-dir/", logger);
    store.set_ui_scale_percent(150);
    CHECK(store.ui_scale_percent() == 150);
    REQUIRE(logger.lines.size() == 1);
    CHECK(logger.lines[0].find("could not write") == 0);
}

TEST_CASE("the recorder stores the landing payload and forwards the landing")
{
    TempDir                   dir;
    fs24::test::FakeLogger    logger;
    fs24::test::FakePublisher next;
    PanelStateStore           store(dir.prefix(), logger);
    LastLandingRecorder       recorder(store, next);
    fs24::domain::Landing     landing;
    landing.descent_fpm = -150.f;
    recorder.publish_landing(landing);
    CHECK(next.published.size() == 1);
    REQUIRE(store.last_landing_payload());
    CHECK(store.last_landing_payload()->find("\"descent_fpm\":-150") != std::string::npos);
}
