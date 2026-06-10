#include "desktop-entry.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <optional>
#include <string_view>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

using namespace std::string_view_literals;
using namespace xdg::desktop_entry_spec;

namespace {
    constexpr std::array well_known_keys_name {
        "Actions"sv,
        "Categories"sv,
        "Comment"sv,
        "DBusActivatable"sv,
        "Exec"sv,
        "GenericName"sv,
        "Hidden"sv,
        "Icon"sv,
        "Implements"sv,
        "Keywords"sv,
        "MimeType"sv,
        "Name"sv,
        "NoDisplay"sv,
        "NotShowIn"sv,
        "OnlyShowIn"sv,
        "Path"sv,
        "PrefersNonDefaultGPU"sv,
        "SingleMainWindow"sv,
        "StartupNotify"sv,
        "StartupWMClass"sv,
        "Terminal"sv,
        "TryExec"sv,
        "Type"sv,
        "URL"sv,
        "Version"sv
    };
    static_assert(std::ranges::is_sorted(well_known_keys_name));

    constexpr std::array entry_type_name {"Application"sv, "Link"sv, "Directory"sv};

    boost::regex IsCommentRe {"^[ \\t]*#|^[ \\t]*$", boost::regex_constants::flag_type_::optimize};
    boost::regex IsGroupHeadRe {"^[ \\t]*\\[([ -Z\\\\^-~]+)\\][ \\t]*$", boost::regex_constants::flag_type_::optimize};
    boost::regex IsKeyParsingRe {
        "^[ \\t]*(?<key>[A-Za-z0-9-]+)(?:\\[(?<locale>[ -Z\\\\^-~]+)\\])?[ \\t]*=[ "
        "\\t]*(?<value>.+)$",
        boost::regex_constants::flag_type_::optimize
    };

    bool is_comment_line(std::string_view line) {
        return boost::regex_match(line.begin(), line.end(), IsCommentRe);
    }

    std::string_view parse_group_head(std::string_view line) {
        boost::match_results<std::string_view::const_iterator> match;
        if (boost::regex_match(line.begin(), line.end(), match, IsGroupHeadRe)) {
            return std::string_view(match[1].begin(), match[1].end());
        }
        return {};
    }

    struct key_value_match {
        std::string_view key;
        std::string_view value;
        std::string_view locale;
    };

    key_value_match parse_key_value(std::string_view line) {
        boost::match_results<std::string_view::const_iterator> match;
        if (!boost::regex_match(line.begin(), line.end(), match, IsKeyParsingRe)) {
            throw std::runtime_error("Line did not match");
        }
        auto &key_match    = match["key"];
        auto &value_match  = match["value"];
        auto &locale_match = match["locale"];
        return {
            .key    = std::string_view(key_match.begin(), key_match.end()),
            .value  = std::string_view(value_match.begin(), value_match.end()),
            .locale = std::string_view(locale_match.begin(), locale_match.end())
        };
    }

    template<class>
    struct parse_as_helper;

    template<>
    struct parse_as_helper<entry_type> {
        static constexpr bool is_type(well_known_keys val) { return val == well_known_keys::Type; }

        static void parse(const key_value_match &val, std::any &out) {
            auto parsed = entry_type_from_string(val.value);
            if (!val.locale.empty()) {
                throw std::runtime_error("Type value cannot be localized");
            }
            if (!parsed) {
                throw std::runtime_error("Invalid type");
            }
            out = *parsed;
        }
    };

    template<>
    struct parse_as_helper<std::string> {
        static constexpr std::array string_types {
            well_known_keys::Exec,
            well_known_keys::Icon,
            well_known_keys::Path,
            well_known_keys::StartupWMClass,
            well_known_keys::TryExec,
            well_known_keys::URL,
            well_known_keys::Version
        };
        static_assert(std::ranges::is_sorted(string_types));

        static constexpr bool is_type(well_known_keys val) {
            return std::ranges::binary_search(string_types, val);
        }

        static void parse(const key_value_match &val, std::any &out) {
            if (!val.locale.empty()) {
                throw std::runtime_error("String values cannot be localized");
            }
            out = std::string(val.value);
        }
    };

    template<>
    struct parse_as_helper<bool> {
        static constexpr std::array bool_types {
            well_known_keys::DBusActivatable,
            well_known_keys::Hidden,
            well_known_keys::NoDisplay,
            well_known_keys::PrefersNonDefaultGPU,
            well_known_keys::SingleMainWindow,
            well_known_keys::StartupNotify,
            well_known_keys::Terminal
        };
        static_assert(std::ranges::is_sorted(bool_types));

        static constexpr bool is_type(well_known_keys val) {
            return std::ranges::binary_search(bool_types, val);
        }

        static void parse(const key_value_match &val, std::any &out) {
            if (!val.locale.empty()) {
                throw std::runtime_error("bool values cant be localized");
            }

            // TODO: Rework this
            if (val.value == "true") {
                out = true;
            } else if (val.value == "false") {
                out = false;
            } else {
                throw std::runtime_error("Invalid bool value");
            }
        }
    };

    template<>
    struct parse_as_helper<std::vector<std::string>> {
        static constexpr std::array string_list_types {
            well_known_keys::Actions,
            well_known_keys::Categories,
            well_known_keys::Implements,
            well_known_keys::MimeType,
            well_known_keys::NotShowIn,
            well_known_keys::OnlyShowIn
        };
        static_assert(std::ranges::is_sorted(string_list_types));

        static constexpr bool is_type(well_known_keys val) {
            return std::ranges::binary_search(string_list_types, val);
        }

        static std::vector<std::string> parse_as_vector(std::string_view value) {
            std::vector<std::string> values;
            if (!value.empty()) {
                size_t begin_pos, end_pos = -1;
                do {
                    begin_pos = end_pos + 1;
                    end_pos   = value.find(';', begin_pos);

                    auto sub_str = value.substr(begin_pos, end_pos - begin_pos);
                    if (!sub_str.empty()) {
                        values.emplace_back(sub_str);
                    }
                } while (end_pos < value.size());
            }
            return values;
        }

        static void parse(const key_value_match &val, std::any &out) {
            if (!val.locale.empty()) {
                throw std::runtime_error("string list values cant be localized");
            }

            out = parse_as_vector(val.value);
        }
    };

    template<>
    struct parse_as_helper<localized_string> {
        static constexpr std::array localized_string_type {well_known_keys::Comment, well_known_keys::GenericName, well_known_keys::Name};
        static_assert(std::ranges::is_sorted(localized_string_type));

        static constexpr bool is_type(well_known_keys val) {
            return std::ranges::binary_search(localized_string_type, val);
        }

        static void parse(const key_value_match &val, std::any &out) {
            if (!out.has_value()) {
                out = localized_string();
            }
            auto &buf = std::any_cast<localized_string &>(out);
            buf.add(val.locale, std::string(val.value));
        }
    };

    template<>
    struct parse_as_helper<localized_string_list> {
        static constexpr bool is_type(well_known_keys val) {
            return val == well_known_keys::Keywords;
        }

        static void parse(const key_value_match &val, std::any &out) {
            if (!out.has_value()) {
                out = localized_string_list();
            }
            auto &buf = std::any_cast<localized_string_list &>(out);

            auto parsed = parse_as_helper<std::vector<std::string>>::parse_as_vector(val.value);
            buf.add(val.locale, std::move(parsed));
        }
    };

    template<class T, class... Args>
    void parse_into_impl(well_known_keys key, const key_value_match &val, std::any &out) {
        if (parse_as_helper<T>::is_type(key)) {
            parse_as_helper<T>::parse(val, out);
        } else {
            if constexpr (sizeof...(Args) > 0) {
                parse_into_impl<Args...>(key, val, out);
            } else {
                // throw std::runtime_error("Could not parse key");
            }
        }
    }

    void parse_well_know_key_into(well_known_keys key, const key_value_match &val, std::any &out) {
        parse_into_impl<entry_type, std::string, bool, std::vector<std::string>, localized_string, localized_string_list>(key,
            val,
            out);
    }

    struct parsed_language {
        std::string_view lang;
        std::string_view country;
        std::string_view encoding;
        std::string_view modifier;
    };

    parsed_language parse_language(std::string_view lang) {
        parsed_language res {};

        auto delimiter = lang.find_first_of("_.@");
        res.lang       = lang.substr(0, delimiter);

        if (delimiter != std::string_view::npos && lang.at(delimiter) == '_') {
            auto start_pos = delimiter + 1;
            delimiter      = lang.find_first_of(".@", start_pos);
            res.country    = lang.substr(start_pos, delimiter - start_pos);
        }

        if (delimiter != std::string_view::npos && lang.at(delimiter) == '.') {
            auto start_pos = delimiter + 1;
            delimiter      = lang.find_first_of("@", start_pos);
            res.encoding   = lang.substr(start_pos, delimiter - start_pos);
        }

        if (delimiter != std::string_view::npos) {
            assert(lang.at(delimiter) == '@');
            res.modifier = lang.substr(delimiter + 1);
        }

        return res;
    }
} // namespace

namespace xdg::desktop_entry_spec {
    std::string_view to_string(well_known_keys val) noexcept {
        return well_known_keys_name.at(size_t(val));
    }

    std::optional<well_known_keys> well_known_keys_from_string(std::string_view str) noexcept {
        auto it = std::ranges::lower_bound(well_known_keys_name, str);
        if (it != well_known_keys_name.end() && *it == str) {
            return {static_cast<well_known_keys>(std::distance(well_known_keys_name.begin(), it))};
        } else {
            return {};
        }
    }

    std::ostream &operator<<(std::ostream &os, well_known_keys val) {
        return os << to_string(val);
    }

    std::istream &operator>>(std::istream &is, well_known_keys &out) {
        std::string str;
        is >> str;
        auto res = well_known_keys_from_string(str);
        if (res) {
            out = *res;
        } else {
            is.setstate(std::ios_base::failbit);
        }
        return is;
    }

    std::string_view to_string(entry_type val) noexcept {
        return entry_type_name.at(size_t(val));
    }

    std::optional<entry_type> entry_type_from_string(std::string_view str) noexcept {
        for (size_t i = 0; i < entry_type_name.size(); i++) {
            if (str == entry_type_name.at(i)) {
                return static_cast<entry_type>(i);
            }
        }
        return {};
    }

    std::ostream &operator<<(std::ostream &os, entry_type type) {
        return os << to_string(type);
    }

    std::istream &operator>>(std::istream &is, entry_type &out) {
        std::string str;
        is >> str;
        auto res = entry_type_from_string(str);
        if (res) {
            out = *res;
        } else {
            is.setstate(std::ios_base::failbit);
        }
        return is;
    }

    namespace detail {
        alternative_locales::alternative_locales_iter &alternative_locales::alternative_locales_iter::operator++() {
            auto parsed_orig    = parse_language(m_orig_str);
            auto parsed_current = parse_language(m_str);

            assert(!parsed_current.lang.empty());
            if (parsed_current.modifier.empty() && parsed_current.country.empty()) {
                m_str = {};
            } else if (!parsed_current.modifier.empty()) {
                std::string new_str {parsed_current.lang};
                if (!parsed_current.country.empty()) {
                    new_str.push_back('_');
                    new_str.append(parsed_current.country);
                }
                m_str = std::move(new_str);
            } else if (!parsed_current.country.empty()) {
                std::string new_str {parsed_current.lang};
                if (!parsed_orig.modifier.empty()) {
                    new_str.push_back('@');
                    new_str.append(parsed_orig.modifier);
                }
                m_str = std::move(new_str);
            }

            return *this;
        }
    } // namespace detail

    desktop_entry::desktop_entry(std::istream &is) {
        std::string section;
        for (std::string line; std::getline(is, line);) {
            if (is_comment_line(line)) {
                continue;
            } else if (std::string_view section_parsed = parse_group_head(line); !section_parsed.empty()) {
                section = section_parsed;
            } else {
                auto parse_result = parse_key_value(line);
                if (section == "Desktop Entry") {
                    if (auto well_known_key = well_known_keys_from_string(parse_result.key); well_known_key) {
                        parse_well_know_key_into(*well_known_key, parse_result, get_well_known_value(*well_known_key));
                    }
                }
            }
        }
    }

} // namespace xdg::desktop_entry_spec