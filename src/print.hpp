#pragma once

// <print> requires GCC 14+ / MSVC 2022 17.7+ / libc++ 18+.
// Fall back to std::cout + std::format on older toolchains.
#if defined(__GNUC__) && __GNUC__ < 14 && !defined(__clang__)
#include <iostream>
#include <format>
template<typename... Args>
void nero_print(std::format_string<Args...> fmt, Args&&... args) {
    std::cout << std::format(fmt, std::forward<Args>(args)...);
}
template<typename... Args>
void nero_println(std::format_string<Args...> fmt, Args&&... args) {
    std::cout << std::format(fmt, std::forward<Args>(args)...) << '\n';
}
#else
#include <print>
template<typename... Args>
void nero_print(std::format_string<Args...> fmt, Args&&... args) {
    std::print(fmt, std::forward<Args>(args)...);
}
template<typename... Args>
void nero_println(std::format_string<Args...> fmt, Args&&... args) {
    std::println(fmt, std::forward<Args>(args)...);
}
#endif