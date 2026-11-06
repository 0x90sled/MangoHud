#include "file_utils.h"
#include <filesystem.h>
#include <string>
#include <array>
#include <spdlog/spdlog.h>
#include "hud_elements.h"
#include "overlay.h"

namespace fs = ghc::filesystem;
class WineSync {
    private:
        inline static const std::unordered_map<std::string, std::string> methods {
            {"NONE", "NONE"},
            {"winesync", "Wserver"},
            {"esync", "Esync"},
            {"fsync", "Fsync"},
            {"ntsync", "NTsync"},
        };

        // what to look for, best guess first. ntsync/winesync have real
        // /dev nodes so they're a sure thing, esync/fsync are just "library
        // is loaded" hints. NONE is for display only, don't search for it.
        inline static const std::array<const char*, 4> sync_order {{"ntsync", "winesync", "fsync", "esync"}};

        pid_t pid = 0; // 0 = haven't checked yet. gamescope gives us -1 or a real pid, never 0
        std::string method = "NONE";
        bool inside_wine = true;
    public:
        void determine_sync_variant() {
#ifdef __linux__
            // start fresh every time, otherwise stale results stick around
            // when the game changes
            method = "NONE";
            inside_wine = false;

            // whose exe do we check? our own in layer mode (pid <= 0), but in
            // mangoapp we're not wine — the game is, so look at its exe instead
            std::string wineProcess = pid > 0
                ? read_symlink((fs::path("/proc") / std::to_string(pid) / "exe").string())
                : get_exe_path();
            auto n = wineProcess.find_last_of('/');
            std::string preloader = wineProcess.substr(n + 1);
            if (preloader != "wine-preloader" && preloader != "wine64-preloader")
                return;

            inside_wine = true;

            for (auto key : sync_order) {
                if (lib_loaded(key, pid)) {
                    method = key;
                    break;
                }
            }

            SPDLOG_DEBUG("Wine sync method: {}", methods.at(method));
#endif
        }

        bool valid() {
            return inside_wine;
        }

        // return sync method as display name
        const char* get_method() {
            return methods.at(method).c_str();
        }

        void set_pid(pid_t _pid) {
            if (_pid != pid) {
                pid = _pid;
                determine_sync_variant();
            }
        }
};

extern std::unique_ptr<WineSync> winesync_ptr;
