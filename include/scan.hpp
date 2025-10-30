#pragma once

#include <optional>
#include <tuple>
#include <utility>
#include <cstddef>
#include <expected>

#include "parse.hpp"
#include "types.hpp"

namespace stdx {

template <typename... Ts>
auto parse_all_to_tuple(std::string_view* in_parts, std::string_view* fmt_parts) {
    size_t i = 0;
    return std::tuple{stdx::details::parse_value_with_format<Ts>(in_parts[i++], fmt_parts[i - 1])...};
}

template <typename... Ts>
std::expected<details::scan_result<Ts...>, details::scan_error> scan(std::string_view input, std::string_view format) {
    auto parsing_res = stdx::details::parse_sources<Ts...>(input, format);
 
    if (!parsing_res) 
        return std::unexpected(std::move(parsing_res.error()));

    auto [fmt_parts, in_parts] = std::move(*parsing_res);

    constexpr std::size_t N = sizeof...(Ts);
    if (fmt_parts.size() != N || in_parts.size() != N) {
        return std::unexpected(details::scan_error{
            "Format placeholders count does not match destination type pack size"
        });
    }

    auto parsed_tuple = parse_all_to_tuple<Ts...>(in_parts.data(), fmt_parts.data());

    std::optional<details::scan_error> first_err;
    std::size_t first_err_idx = N;
    std::size_t idx = 0;

    std::apply([&](auto&&... exps){
        (([&]{
            if (!exps && !first_err) {
                first_err = exps.error();
                first_err_idx = idx;
            }
            ++idx;
        }()), ...);
    }, parsed_tuple);

    if (first_err)
        return std::unexpected(std::move(*first_err));    

    auto values_tuple = std::apply([](auto&&... exps){
        return std::tuple<Ts...>(std::move(exps.value())...);
    }, std::move(parsed_tuple));

    return details::scan_result<Ts...>{std::move(values_tuple)};
}

} // namespace stdx
