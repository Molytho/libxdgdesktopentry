#ifndef LIB_XDGDESKTOPENTRYPP_XDG_BASE_DIRECTORY_H
#define LIB_XDGDESKTOPENTRYPP_XDG_BASE_DIRECTORY_H

#include <filesystem>
#include <stdexcept>
#include <vector>

namespace xdg::desktop_entry_spec::detail::xdg::base_directory {
    class mandatory_envvar_missing : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    class mandatory_envvar_relative : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    std::filesystem::path get_data_home();
    std::vector<std::filesystem::path> get_data_dirs();
} // namespace xdg::base_directory

#endif