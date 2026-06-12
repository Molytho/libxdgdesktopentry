#include "private/desktop-entry-parser.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <optional>
#include <string_view>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include "private/string_helper.h"

using namespace std::string_view_literals;
using namespace xdg::desktop_entry_spec;
using namespace std::filesystem;

namespace {
    boost::regex IsCommentRe {"^[ \\t]*(?:#.*)?$", boost::regex_constants::flag_type_::optimize};
    boost::regex IsGroupHeadRe {"^[ \\t]*\\[([ -Z\\\\^-~]+)\\][ \\t]*$", boost::regex_constants::flag_type_::optimize};
    boost::regex IsKeyParsingRe {
        "^[ \\t]*(?<key>[A-Za-z0-9-]+)(?:\\[(?<locale>[ -Z\\\\^-~]+)\\])?[ \\t]*=[ "
        "\\t]*(?<value>.*)$",
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
            if (value.empty()) {
                return {};
            }

            std::vector<std::string> values;
            for (const auto &str : xdg::detail::utils::string_spliterator(value, ';')) {
                if (!str.empty()) {
                    values.emplace_back(str);
                }
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
        static constexpr std::array localized_string_type {
            well_known_keys::Comment,
            well_known_keys::GenericName,
            well_known_keys::Icon,
            well_known_keys::Name
        };
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
                throw std::runtime_error("Could not parse key");
            }
        }
    }

    void parse_well_know_key_into(well_known_keys key, const key_value_match &val, std::any &out) {
        parse_into_impl<entry_type, std::string, bool, std::vector<std::string>, localized_string, localized_string_list>(key,
            val,
            out);
    }
} // namespace

namespace xdg::desktop_entry_spec::detail {
    void desktop_entry_parser::update_current_section(std::string_view section) {
        m_current_section = section;
        if (section == "Desktop Entry") {
            m_is_main_section   = true;
            m_is_action_section = false;
        } else {
            m_is_main_section   = false;
            m_is_action_section = false;
        }
    }

    void desktop_entry_parser::parse_key_value_line(std::string line) {
        auto parse_result = parse_key_value(line);
        if (m_is_main_section) {
            if (auto well_known_key = well_known_keys_from_string(parse_result.key); well_known_key) {
                parse_well_know_key_into(*well_known_key, parse_result, m_target.get_well_known_value(*well_known_key));
            } else {
                // TODO
            }
        } else {
            // TODO
        }
    }

    bool desktop_entry_parser::entry_has_action(std::string_view str) {
        auto actions = m_target.get_actions();
        if (!actions) {
            return false;
        }
        return std::ranges::find(*m_target.get_actions(), str) != actions->end();
    }

    desktop_entry_parser::desktop_entry_parser(desktop_entry &target) : m_target(target) { }

    void desktop_entry_parser::parse(std::istream &is) try {
        for (std::string line; std::getline(is, line);) {
            if (is_comment_line(line)) {
                continue;
            } else if (std::string_view section_parsed = parse_group_head(line); !section_parsed.empty()) {
                update_current_section(section_parsed);
            } else {
                parse_key_value_line(std::move(line));
            }
        }
        if (is.eof()) {
            is.clear(std::ios::eofbit);
        }
    } catch (const std::runtime_error &ex) {
        is.setstate(std::ios::failbit);
    }
} // namespace xdg::desktop_entry_spec::detail