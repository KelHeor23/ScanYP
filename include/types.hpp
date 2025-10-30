#pragma once

#include <tuple>

namespace stdx::details {

// Класс для хранения ошибки неуспешного сканирования

struct scan_error {
public:
    explicit scan_error(std::string msg) : message_(std::move(msg)){}

    const std::string& message() const noexcept {
        return message_;
    }

private:
    std::string message_;
};

// Шаблонный класс для хранения результатов успешного сканирования

template <typename... Ts>
struct scan_result {
public:
    scan_result() = delete;
    
    explicit scan_result(std::tuple<Ts...> values)
        : values_(std::move(values)) {}

    const std::tuple<Ts...>& values() const noexcept {
        return values_;
    }

private:
    std::tuple<Ts...> values_;
};

} // namespace stdx::details
