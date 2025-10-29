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
        : values_(std::move(values)), error_(nullptr) {}

    const std::tuple<Ts...>& values() const noexcept {
        return values_;
    }

    bool has_error() const noexcept {
        return error_ != nullptr;
    }

    const scan_error* error() const noexcept {
        return error_.get();
    }

private:
    std::tuple<Ts...> values_;
    std::unique_ptr<scan_error> error_;
};

} // namespace stdx::details
