#ifndef LIB_XDGDESKTOPENTRYPP_DESKTOP_ENTRY_H
#define LIB_XDGDESKTOPENTRYPP_DESKTOP_ENTRY_H

#include <any>
#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "helper.h"

namespace xdg::desktop_entry_spec {
    enum class well_known_keys : uint8_t {
        Actions,
        Categories,
        Comment,
        DBusActivatable,
        Exec,
        GenericName,
        Hidden,
        Icon,
        Implements,
        Keywords,
        MimeType,
        Name,
        NoDisplay,
        NotShowIn,
        OnlyShowIn,
        Path,
        PrefersNonDefaultGPU,
        SingleMainWindow,
        StartupNotify,
        StartupWMClass,
        Terminal,
        TryExec,
        Type,
        URL,
        Version,
        Last = Version
    };
    std::string_view to_string(well_known_keys val) noexcept;
    std::optional<well_known_keys> well_known_keys_from_string(std::string_view str) noexcept;
    std::ostream &operator<<(std::ostream &os, well_known_keys val);
    std::istream &operator>>(std::istream &is, well_known_keys &out);

    enum class entry_type : uint8_t { Application, Link, Directory };
    std::string_view to_string(entry_type val) noexcept;
    std::optional<entry_type> entry_type_from_string(std::string_view str) noexcept;
    std::ostream &operator<<(std::ostream &os, entry_type val);
    std::istream &operator>>(std::istream &is, entry_type &out);

    namespace detail {
        class LIBXDGDESKTOPENTRY_PUBLIC alternative_locales {
            struct end_tag { };

            class iter {
                std::string_view m_modifier;
                std::string m_str;

            public:
                using difference_type = std::ptrdiff_t;
                using value_type      = std::string;

                iter(std::string_view orig);
                iter(const iter &other);

                const std::string &operator*() const noexcept;

                iter &operator++();
                iter operator++(int);

                bool operator==(end_tag) const noexcept { return m_str.empty(); }
            };

            std::string_view m_str;

        public:
            alternative_locales(std::string_view str);

            iter begin() const { return iter(m_str); }

            end_tag end() const noexcept { return {}; }
        };

        template<class T>
        class LIBXDGDESKTOPENTRY_PUBLIC localized_data {
            T m_generic {};
            std::unordered_map<std::string, T> m_translations {};

        public:
            localized_data() = default;

            void add(std::string_view lang, T val) {
                if (lang.empty()) {
                    set_generic(std::move(val));
                } else {
                    add_translated(lang, std::move(val));
                }
            }

            void set_generic(T val) { m_generic = std::move(val); }

            void add_translated(std::string_view lang, T val) {
                m_translations.emplace(lang, std::move(val));
            }

            const T *get(std::string_view locale) {
                if (!locale.empty()) {
                    for (const auto &str : alternative_locales(locale)) {
                        if (auto it = m_translations.find(str); it != m_translations.end()) {
                            return std::addressof(it->second);
                        }
                    }
                }
                return std::addressof(m_generic);
            }
        };
    } // namespace detail

    using localized_string      = detail::localized_data<std::string>;
    using localized_string_list = detail::localized_data<std::vector<std::string>>;

    class LIBXDGDESKTOPENTRY_PUBLIC desktop_entry {
    public:
        desktop_entry(std::istream &is);

        entry_type get_type() const noexcept {
            return std::any_cast<entry_type>(get_well_known_value(well_known_keys::Type));
        }

        std::string_view get_version() const {
            return get_well_known_string(well_known_keys::Version);
        }

        const localized_string &get_name() const noexcept {
            return get_well_known_typed<localized_string, true>(well_known_keys::Name);
        }

        const localized_string *get_generic_name() const noexcept {
            return get_well_known_typed<localized_string>(well_known_keys::GenericName);
        }

        bool get_no_display() const noexcept {
            return get_well_known_bool<false>(well_known_keys::NoDisplay);
        }

        const localized_string *get_comment() const noexcept {
            return get_well_known_typed<localized_string>(well_known_keys::Comment);
        }

        std::string_view get_icon() const noexcept {
            return get_well_known_string(well_known_keys::Icon);
        }

        bool get_hidden() const noexcept {
            return get_well_known_bool<false>(well_known_keys::Hidden);
        }

        const std::vector<std::string> *get_only_show_in() const noexcept {
            return get_well_known_typed<std::vector<std::string>>(well_known_keys::OnlyShowIn);
        }

        const std::vector<std::string> *get_not_show_in() const noexcept {
            return get_well_known_typed<std::vector<std::string>>(well_known_keys::NotShowIn);
        }

        bool get_dbus_activatable() const noexcept {
            return get_well_known_bool<false>(well_known_keys::DBusActivatable);
        }

        std::string_view get_try_exec() const noexcept {
            return get_well_known_string(well_known_keys::TryExec);
        }

        std::string_view get_exec() const noexcept {
            return get_well_known_string(well_known_keys::Exec);
        }

        std::string_view get_path() const noexcept {
            return get_well_known_string(well_known_keys::Path);
        }

        bool get_terminal() const noexcept {
            return get_well_known_bool<false>(well_known_keys::Terminal);
        }

        const std::vector<std::string> *get_actions() const noexcept {
            return get_well_known_typed<std::vector<std::string>>(well_known_keys::Actions);
        }

        const std::vector<std::string> *get_mime_types() const noexcept {
            return get_well_known_typed<std::vector<std::string>>(well_known_keys::MimeType);
        }

        const std::vector<std::string> *get_categories() const noexcept {
            return get_well_known_typed<std::vector<std::string>>(well_known_keys::Categories);
        }

        const std::vector<std::string> *get_implements() const noexcept {
            return get_well_known_typed<std::vector<std::string>>(well_known_keys::Implements);
        }

        const localized_string_list *get_keywords() const noexcept {
            return get_well_known_typed<localized_string_list>(well_known_keys::Keywords);
        }

        bool get_startup_notify() const noexcept {
            return get_well_known_bool<false>(well_known_keys::StartupNotify);
        }

        std::string_view get_startup_wm_class() const noexcept {
            return get_well_known_string(well_known_keys::StartupWMClass);
        }

        std::string_view get_url() const {
            return get_well_known_string<true>(well_known_keys::URL);
        }

        bool get_prefers_non_default_gpu() const noexcept {
            return get_well_known_bool<false>(well_known_keys::PrefersNonDefaultGPU);
        }

        bool get_single_main_window() const noexcept {
            return get_well_known_bool<false>(well_known_keys::SingleMainWindow);
        }

    private:
        bool check_required_keys() const noexcept;

        constexpr const std::any &get_well_known_value(well_known_keys val) const noexcept {
            return m_well_known_keys.at(size_t(val));
        }

        constexpr std::any &get_well_known_value(well_known_keys val) noexcept {
            return m_well_known_keys.at(size_t(val));
        }

        template<bool required = false>
        constexpr std::string_view get_well_known_string(well_known_keys key) const noexcept {
            auto &val = get_well_known_value(key);
            if (required || val.has_value()) {
                return std::any_cast<const std::string &>(val);
            } else {
                return {};
            }
        }

        template<bool Default>
        constexpr bool get_well_known_bool(well_known_keys key) const noexcept {
            auto &val = get_well_known_value(key);
            if (val.has_value()) {
                return std::any_cast<bool>(val);
            } else {
                return Default;
            }
        }

        template<class T, bool required = false>
        std::conditional_t<required, const T &, const T *> get_well_known_typed(well_known_keys key) const {
            auto &val = get_well_known_value(key);
            if constexpr (required) {
                return std::any_cast<const T &>(val);
            } else {
                return val.has_value() ? std::addressof(std::any_cast<const T &>(val)) : nullptr;
            }
        }

        std::array<std::any, size_t(well_known_keys::Last) + 1> m_well_known_keys {};
    };

} // namespace xdg::desktop_entry_spec

#endif