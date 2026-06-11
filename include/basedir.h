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

        inline std::string_view check_for_absolute_path(std::string_view str, const char *var_name) {
            std::filesystem::path path {str};
            if (!path.is_absolute()) {
                throw mandatory_environment_variable_relative(var_name);
            }
            return str;
        }

        inline std::string_view get_home_dir() {
            auto env = std::getenv(HomeVariable);
            return env ? check_for_absolute_path(env, HomeVariable)
                       : throw mandatory_environment_variable_missing(HomeVariable);
        }

        inline std::string get_home_var(const char *name, std::string_view default_value) {
            auto env = std::getenv(name);
            if (env) {
                return std::string(check_for_absolute_path(env, name));
            } else {
                std::string res {get_home_dir()};
                res.append(default_value);
                return res;
            }
        }
    } // namespace detail

    inline std::string get_data_home() {
        return detail::get_home_var(XdgDataHomeVariable, "/.local/share");
    }

    inline std::string get_config_home() {
        return detail::get_home_var(XdgConfigHomeVariable, "/.config");
    }

    inline std::string get_state_home() {
        return detail::get_home_var(XdgStateHomeVariable, "/.local/state");
    }

    inline std::string get_cache_home() {
        return detail::get_home_var(XdgCacheHomeVariable, "/.cache");
    }

    inline std::string get_bin_home() {
        std::string res {detail::get_home_dir()};
        res.append("/.local/bin");
        return res;
    }

    inline std::string get_data_dirs() {
        auto env = std::getenv(XdgDataDirsVariable);
        return env ? env : "/usr/local/share/:/usr/share/";
    }

    inline std::string get_config_dirs() {
        auto env = std::getenv(XdgConfigDirsVariable);
        return env ? env : "/etc/xdg";
    }

    inline std::vector<std::string> get_data_dirs_as_list() {
        auto strs = get_data_dirs();
        std::vector<std::string> result;
        for (const auto &str : xdg::detail::utils::string_spliterator {strs, PathSeparator}) {
            result.emplace_back(detail::check_for_absolute_path(str, XdgDataDirsVariable));
        }
        return result;
    }

    inline std::vector<std::string> get_config_dirs_as_list() {
        auto strs = get_config_dirs();
        std::vector<std::string> result;
        for (const auto &str : xdg::detail::utils::string_spliterator {strs, PathSeparator}) {
            result.emplace_back(detail::check_for_absolute_path(str, XdgConfigDirsVariable));
        }
        return result;
    }

    inline std::string get_runtime_dir() {
        auto env = std::getenv(XdgRuntimeDirVariable);
        return env ? env : throw mandatory_environment_variable_missing(XdgRuntimeDirVariable);
    }
} // namespace xdg::basedir

#endif