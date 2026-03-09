#pragma once

#if defined(__MINGW32__) && __GNUC__ >= 15
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