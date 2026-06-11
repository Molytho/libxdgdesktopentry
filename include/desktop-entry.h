#ifndef LIB_XDGDESKTOPENTRYPP_DESKTOP_ENTRY_H
#define LIB_XDGDESKTOPENTRYPP_DESKTOP_ENTRY_H

#include <any>
#include <array>
#include <clocale>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "private/helper.h"

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
    API_PUBLIC std::string_view to_string(well_known_keys val) noexcept;
    API_PUBLIC std::optional<well_known_keys> well_known_keys_from_string(std::string_view str) noexcept;
    API_PUBLIC std::ostream &operator<<(std::ostream &os, well_known_keys val);
    API_PUBLIC std::istream &operator>>(std::istream &is, well_known_keys &out);

    enum class entry_type : uint8_t { Application, Link, Directory };
    API_PUBLIC std::string_view to_string(entry_type val) noexcept;
    API_PUBLIC std::optional<entry_type> entry_type_from_string(std::string_view str) noexcept;
    API_PUBLIC std::ostream &operator<<(std::ostream &os, entry_type val);
    API_PUBLIC std::istream &operator>>(std::istream &is, entry_type &out);

    struct parsed_locale {
        std::string_view lang;
        std::string_view country;
        std::string_view encoding;
        std::string_view modifier;
    };

    class API_PUBLIC locale {
        std::string m_str;

    public:
        constexpr locale() : m_str() { }

        constexpr explicit locale(std::string_view str) : m_str(str) { }

        constexpr explicit locale(std::string str) : m_str(str) { }

        constexpr locale(const locale &other) : m_str(other.m_str) { }

        constexpr locale &operator=(const locale &other) {
            m_str = other.m_str;
            return *this;
        }

        constexpr locale(locale &&other) noexcept : m_str(std::move(other.m_str)) { }

        constexpr locale &operator=(locale &&other) noexcept {
            m_str = std::move(other.m_str);
            return *this;
        }

        constexpr std::string_view str() const noexcept { return m_str; }

        parsed_locale parse() const noexcept;

        void strip_encoding();

        constexpr bool operator==(const locale &other) const noexcept {
            return m_str == other.m_str;
        }
    };

    namespace detail {
        class API_PUBLIC alternative_locales_iterator {
            std::string_view m_modifier;
            locale m_locale;

        public:
            alternative_locales_iterator(locale locale);

            alternative_locales_iterator(std::string_view str) :
                    alternative_locales_iterator(locale(str)) { }

            const locale &operator*() const noexcept;

            alternative_locales_iterator &operator++();

            bool operator==(std::default_sentinel_t) const noexcept;
        };

        inline alternative_locales_iterator begin(alternative_locales_iterator it) {
            return it;
        }

        constexpr std::default_sentinel_t end(const alternative_locales_iterator &) {
            return {};
        }

        template<class T>
        class API_PUBLIC localized_data {
            T m_generic {};
            std::unordered_map<locale, T> m_translations {};

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

            const T &get() const {
                auto locale = std::setlocale(LC_MESSAGES, "");
                if (!locale) {
                    throw std::runtime_error("Failed to get locale");
                }
                return get(locale);
            }

            const T &get(std::string_view locale_str) const {
                if (!locale_str.empty()) {
                    for (const auto &locale : alternative_locales_iterator(locale_str)) {
                        if (auto it = m_translations.find(locale); it != m_translations.end()) {
                            return it->second;
                        }
                    }
                }
                return m_generic;
            }
        };
    } // namespace detail

    using localized_string      = detail::localized_data<std::string>;
    using localized_string_list = detail::localized_data<std::vector<std::string>>;

    class API_PUBLIC desktop_entry {
    public:
        desktop_entry(std::istream &is);
        desktop_entry(std::istream &&is);
        desktop_entry(std::filesystem::path store, std::filesystem::path relative_path);

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

        const localized_string *get_icon() const noexcept {
            return get_well_known_typed<localized_string>(well_known_keys::Icon);
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

        std::string get_id() const;

        bool should_show() const;

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

        std::filesystem::path m_store {};
        std::filesystem::path m_relative_path {};
        std::array<std::any, size_t(well_known_keys::Last) + 1> m_well_known_keys {};
    };

    API_PUBLIC std::vector<std::unique_ptr<desktop_entry>> get_all_desktop_entries();
} // namespace xdg::desktop_entry_spec

namespace std {
    template<>
    struct hash<xdg::desktop_entry_spec::locale> {
        hash<std::string_view> m_str_hash {};

        size_t operator()(const xdg::desktop_entry_spec::locale &locale) const {
            return m_str_hash(locale.str());
        }
    };
} // namespace std
#endif