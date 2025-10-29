#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <charconv>

#include "types.hpp"

namespace stdx::details {

namespace parse_value {
template <class Int>
std::expected<Int, scan_error> parse_int(std::string_view s, int base) {
    if (s.empty())
        return std::unexpected(scan_error{"empty input for integer"});

    if constexpr (std::is_unsigned_v<Int>) {
        if (!s.empty() && s.front() == '-')
            return std::unexpected(scan_error{"negative value for unsigned type"});
    }

    Int value{};
    auto* first = s.data();
    auto* last  = s.data() + s.size();
    auto res = std::from_chars(first, last, value, base);
    if (res.ec != std::errc{} || res.ptr != last)
        return std::unexpected(scan_error{"invalid integer format"});
    return value;
}

template <class Float>
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


enum class conv {
    int_, uint_, float_, string_
};

inline std::expected<conv, scan_error> parse_conv(std::string_view fmt) {
    if (fmt == "d") return conv::int_;
    if (fmt == "s") return conv::string_;
    if (fmt == "u") return conv::uint_;
    if (fmt == "f") return conv::float_;
    return std::unexpected(scan_error{"unknown conversion specifier: " + std::string(fmt)});
}

// Функция для парсинга значения с учетом спецификатора формата
template <typename T>
std::expected<T, scan_error> parse_value_with_format(std::string_view input, std::string_view fmt) {
    auto conv_kind_temp = parse_conv(fmt);
    if (!conv_kind_temp)
        return std::unexpected(std::move(conv_kind_temp.error()));

    auto conv_kind = *conv_kind_temp;

    switch (conv_kind)
    {
    case conv::string_:
        if (!(std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>))
            return std::unexpected(scan_error{"specifier 's' incompatible with destination type"});
        if constexpr (std::is_same_v<T, std::string_view>)
            return input;
        else
            return std::string(input);
    case conv::float_:
        if constexpr (std::is_floating_point_v<T>)
            return parse_value::parse_float<T>(input);
        else 
            return std::unexpected(scan_error{"specifier 'f' incompatible with destination type"});        
    case conv::uint_:
        if constexpr (!std::is_unsigned_v<T>)
            return std::unexpected(scan_error{"specifier 'u' incompatible with destination type"});
        else
            return parse_value::parse_int<T>(input, 10);
    case conv::int_:
        if constexpr (!std::is_signed_v<T>)
            return std::unexpected(scan_error{"specifier 'd' incompatible with destination type"});
        else 
            return parse_value::parse_int<T>(input, 10);    
    default:
        return std::unexpected(scan_error{"destination type incompatible with conversion specifier"});
    }    
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