#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <charconv>

#include "types.hpp"

namespace stdx::details {

namespace common {
inline std::string_view ltrim(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.remove_prefix(1);
    return s;
}

inline std::string_view rtrim(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.remove_suffix(1);
    return s;
}

constexpr std::string_view trim(std::string_view s) {
    return rtrim(ltrim(s));
}

}
namespace parse_value {
template <std::integral Int>
std::expected<Int, scan_error> parse_int(std::string_view s, int base) {
    if (s.empty())
        return std::unexpected(scan_error{"empty input for integer"});

    if constexpr (std::is_unsigned_v<Int>) {
        if (!s.empty() && s.front() == '-')
            return std::unexpected(scan_error{"negative value for unsigned type"});
    }

    if (s.front() == '+')
        s.remove_prefix(1);

    Int value{};
    auto* first = s.data();
    auto* last  = s.data() + s.size();
    auto res = std::from_chars(first, last, value, base);
    if (res.ec != std::errc{} || res.ptr != last)
        return std::unexpected(scan_error{"invalid integer format"});
    return value;
}

template <std::floating_point Float>
std::expected<Float, scan_error> parse_float(std::string_view s) {
    if (s.empty())
        return std::unexpected(scan_error{"empty input for floating"});

    Float value{};
    auto* first = s.data();
    auto* last  = s.data() + s.size();
    auto res = std::from_chars(first, last, value, std::chars_format::general);
    if (res.ec != std::errc{} || res.ptr != last)
        return std::unexpected(scan_error{"invalid floating format"});
    return value;
}
}

namespace concepts {
template <class T>
concept StringLike =
    std::same_as<std::remove_cvref_t<T>, std::string> ||
    std::same_as<std::remove_cvref_t<T>, std::string_view>;

template <class T>
concept HasParseFloat =
    std::floating_point<T> &&
    requires(std::string_view s) {
        { parse_value::parse_float<T>(s) } -> std::same_as<std::expected<T, scan_error>>;
    };

template <class T>
concept HasParseIntSigned =
    std::signed_integral<T> &&
    requires(std::string_view s) {
        { parse_value::parse_int<T>(s, 10) } -> std::same_as<std::expected<T, scan_error>>;
    };

template <class T>
concept HasParseIntUnsigned =
    std::unsigned_integral<T> &&
    requires(std::string_view s) {
        { parse_value::parse_int<T>(s, 10) } -> std::same_as<std::expected<T, scan_error>>;
    };

template <class T>
concept ParsableWithFormat =
       StringLike<T>
    || HasParseFloat<T>
    || HasParseIntSigned<T>
    || HasParseIntUnsigned<T>;
}

enum class conv {
    int_, uint_, float_, string_
};

constexpr std::expected<conv, scan_error> parse_conv(std::string_view fmt) {
    if (fmt == "%d") return conv::int_;
    if (fmt == "%s") return conv::string_;
    if (fmt == "%u") return conv::uint_;
    if (fmt == "%f") return conv::float_;
    return std::unexpected(scan_error{"unknown conversion specifier: " + std::string(fmt)});
}

static constexpr char spec_char(conv c) noexcept {
    switch (c) {
        case conv::string_: return 's';
        case conv::float_:  return 'f';
        case conv::uint_:   return 'u';
        case conv::int_:    return 'd';
    }
    return '?'; // на всякий
}

template <typename T>
constexpr std::unexpected<scan_error> spec_error(conv c) {
    return std::unexpected(scan_error{
        std::string("specifier '") + spec_char(c) + "' incompatible with destination type"
    });
}

// Функция для парсинга значения с учетом спецификатора формата
template <concepts::ParsableWithFormat T>
constexpr std::expected<T, scan_error> parse_value_with_format(std::string_view input, std::string_view fmt) {
auto conv_kind_res = parse_conv(fmt);
    if (!conv_kind_res)
        return std::unexpected(std::move(conv_kind_res.error()));

    const conv c = *conv_kind_res;

    if constexpr (std::is_same_v<T, std::string_view>) {
        if (c == conv::string_) return input;
    } else if constexpr (std::is_same_v<T, std::string>) {
        if (c == conv::string_) return std::string(input);
    } else if constexpr (std::is_floating_point_v<T>) {
        if (c == conv::float_)
            return parse_value::parse_float<T>(common::trim(input));
    } else if constexpr (std::is_integral_v<T>) {
        const std::string_view t = common::trim(input);
        if (c == conv::int_  && std::is_signed_v<T>)
            return parse_value::parse_int<T>(t, 10);
        if (c == conv::uint_ && std::is_unsigned_v<T>)
            return parse_value::parse_int<T>(t, 10);
    }

    // Если дошли до сюда, значит есть явная ошибка 
    return spec_error<T>(c);
}

// Функция для проверки корректности входных данных и выделения из обеих строк интересующих данных для парсинга
template <typename... Ts>
std::expected<std::pair<std::vector<std::string_view>, std::vector<std::string_view>>, scan_error>
parse_sources(std::string_view input, std::string_view format) {
    std::vector<std::string_view> format_parts;  // Части формата между {}
    std::vector<std::string_view> input_parts;
    size_t start = 0;
    while (true) {
        size_t open = format.find('{', start);
        if (open == std::string_view::npos) {
            break;
        }
        size_t close = format.find('}', open);
        if (close == std::string_view::npos) {
            break;
        }

        // Если между предыдущей } и текущей { есть текст,
        // проверяем его наличие во входной строке
        if (open > start) {
            std::string_view between = format.substr(start, open - start);
            auto pos = input.find(between);
            if (input.size() < between.size() || pos == std::string_view::npos) {
                return std::unexpected(scan_error{"Unformatted text in input and format string are different"});
            }
            if (start != 0) {
                input_parts.emplace_back(input.substr(0, pos));
            }

            input = input.substr(pos + between.size());
        }

        // Сохраняем спецификатор формата (то, что между {})
        format_parts.push_back(format.substr(open + 1, close - open - 1));
        start = close + 1;
    }

    // Проверяем оставшийся текст после последней }
    if (start < format.size()) {
        std::string_view remaining_format = format.substr(start);
        auto pos = input.find(remaining_format);
        if (input.size() < remaining_format.size() || pos == std::string_view::npos) {
            return std::unexpected(scan_error{"Unformatted text in input and format string are different"});
        }
        input_parts.emplace_back(input.substr(0, pos));
        input = input.substr(pos + remaining_format.size());
    } else {
        input_parts.emplace_back(input);
    }
    return std::pair{format_parts, input_parts};
}

} // namespace stdx::details