#pragma once

#include "parse.hpp"
#include "types.hpp"

namespace stdx {

// замените болванку функции scan на рабочую версию
template <typename... Ts>
std::expected<details::scan_result<Ts...>, details::scan_error> scan(std::string_view input, std::string_view format) {
    return std::unexpected(details::scan_error{"Dumb implementation"});
}

} // namespace stdx
