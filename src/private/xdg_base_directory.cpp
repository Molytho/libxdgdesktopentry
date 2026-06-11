#include "private/xdg_base_directory.h"

#include <cstdlib>

#include "private/string_helper.h"

using namespace xdg::desktop_entry_spec;

namespace {
    constexpr std::string_view XdgDataDirsDefault = "/usr/local/share/:/usr/share/";

    std::string get_home_var() {
        auto var = getenv("HOME");
        if (!var) {
            throw detail::xdg::base_directory::mandatory_envvar_missing("HOME");
        }
        return var;
    }

    std::string get_string_var(const char *name) {
        auto var = getenv(name);
        return var ? std::string(var) : std::string();
    }

    std::filesystem::path get_var_or_default(const char *name, const char *default_value) {
        std::filesystem::path path = [&]() {
            std::string res = get_string_var(name);
            if (res.empty()) {
                res = get_home_var().append(default_value);
            }
            return res;
        }();
        if (path.is_relative()) {
            std::stringstream strstr;
            strstr << name << " or HOME";
            throw detail::xdg::base_directory::mandatory_envvar_relative(std::move(strstr.str()));
        }
        return path;
    }
} // namespace

namespace xdg::desktop_entry_spec::detail::xdg::base_directory {
    std::filesystem::path get_data_home() {
        return get_var_or_default("XDG_DATA_HOME", "/.local/share");
    }

    std::vector<std::filesystem::path> get_data_dirs() {
        auto env = get_string_var("XDG_DATA_DIRS");
        if (env.empty()) {
            env = XdgDataDirsDefault;
        }
        std::vector<std::filesystem::path> res;
        for (const auto &str : ::xdg::desktop_entry_spec::detail::utils::string_spliterator(env, ':')) {
            res.push_back(str);
        }
        return res;
    }

} // namespace xdg::desktop_entry_spec::detail::xdg::base_directory