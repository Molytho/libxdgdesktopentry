#ifndef STRING_HELPER_H
#define STRING_HELPER_H

#include <stdexcept>
#include <string_view>

namespace utils {
    class string_spliterator {
        char m_delimiter;
        bool m_ended;
        std::string_view m_str;
        size_t m_end_pos;

    public:
        constexpr string_spliterator(std::string_view str, char delimiter) :
                m_delimiter(delimiter), m_ended(false), m_str(str), m_end_pos(m_str.find(delimiter)) { }

        constexpr std::string_view operator*() const noexcept { return m_str.substr(0, m_end_pos); }

        constexpr string_spliterator &operator++() {
            if (m_ended) {
                throw std::logic_error("Tried to increment iterator while it is at the end");
            }

            if (m_end_pos == std::string_view::npos) {
                m_ended = true;
            } else {
                m_str     = m_str.substr(m_end_pos + 1);
                m_end_pos = m_str.find(m_delimiter);
            }
            return *this;
        }

        constexpr bool operator==(std::default_sentinel_t) const noexcept {
            return m_ended;
        }
    };

    constexpr string_spliterator begin(string_spliterator it) noexcept {
        return it;
    }

    constexpr std::default_sentinel_t end(const string_spliterator &) noexcept {
        return {};
    }
} // namespace utils

#endif