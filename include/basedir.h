#ifndef LIB_XDGPP_BASEDIR_H
#define LIB_XDGPP_BASEDIR_H

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "private/string_helper.h"

namespace xdg::basedir {
    constexpr char XdgDataHomeVariable[]   = "XDG_DATA_HOME";
    constexpr char XdgConfigHomeVariable[] = "XDG_CONFIG_HOME";
    constexpr char XdgStateHomeVariable[]  = "XDG_STATE_HOME";
    constexpr char XdgCacheHomeVariable[]  = "XDG_CACHE_HOME";
    constexpr char XdgDataDirsVariable[]   = "XDG_DATA_DIRS";
    constexpr char XdgConfigDirsVariable[] = "XDG_CONFIG_DIRS";
    constexpr char XdgRuntimeDirVariable[] = "XDG_RUNTIME_DIR";
    constexpr char PathSeparator           = ':';

    struct mandatory_environment_variable_missing : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    struct mandatory_environment_variable_relative : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    namespace detail {
        constexpr char HomeVariable[] = "HOME";

        inline std::filesystem::path check_for_absolute_path(std::filesystem::path path, const char *var_name) {
            if (!path.is_absolute()) {
                throw mandatory_environment_variable_relative(var_name);
            }
            return path;
        }

        inline std::filesystem::path get_home_dir() {
            auto env = std::getenv(HomeVariable);
            return env ? check_for_absolute_path(env, HomeVariable)
                       : throw mandatory_environment_variable_missing(HomeVariable);
        }

        inline std::filesystem::path get_home_var(const char *name, std::string_view default_value) {
            auto env = std::getenv(name);
            if (env) {
                return check_for_absolute_path(env, name);
            } else {
                return get_home_dir() / default_value;
            }
        }
    } // namespace detail

    inline std::filesystem::path get_data_home() {
        return detail::get_home_var(XdgDataHomeVariable, ".local/share");
    }

    inline std::filesystem::path get_config_home() {
        return detail::get_home_var(XdgConfigHomeVariable, ".config");
    }

    inline std::filesystem::path get_state_home() {
        return detail::get_home_var(XdgStateHomeVariable, ".local/state");
    }

    inline std::filesystem::path get_cache_home() {
        return detail::get_home_var(XdgCacheHomeVariable, ".cache");
    }

    inline std::filesystem::path get_bin_home() {
        return detail::get_home_dir() / ".local/bin";
    }

    inline std::string get_data_dirs_raw() {
        auto env = std::getenv(XdgDataDirsVariable);
        return env ? env : "/usr/local/share/:/usr/share/";
    }

    inline std::string get_config_dirs_raw() {
        auto env = std::getenv(XdgConfigDirsVariable);
        return env ? env : "/etc/xdg";
    }

    inline std::vector<std::filesystem::path> get_data_dirs() {
        auto strs = get_data_dirs_raw();
        std::vector<std::filesystem::path> result;
        for (const auto &str : xdg::detail::utils::string_spliterator {strs, PathSeparator}) {
            result.emplace_back(detail::check_for_absolute_path(str, XdgDataDirsVariable));
        }
        return result;
    }

    inline std::vector<std::filesystem::path> get_config_dirs() {
        auto strs = get_config_dirs_raw();
        std::vector<std::filesystem::path> result;
        for (const auto &str : xdg::detail::utils::string_spliterator {strs, PathSeparator}) {
            result.emplace_back(detail::check_for_absolute_path(str, XdgConfigDirsVariable));
        }
        return result;
    }

    inline std::filesystem::path get_runtime_dir() {
        auto env = std::getenv(XdgRuntimeDirVariable);
        return env ? detail::check_for_absolute_path(env, XdgRuntimeDirVariable)
                   : throw mandatory_environment_variable_missing(XdgRuntimeDirVariable);
    }
} // namespace xdg::basedir

#endif