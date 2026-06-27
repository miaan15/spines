extern "C" {
#include "spines.h"
}

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <chrono>

int main() {
    SpinesContext cxt{};

    const char *base_str =
R"(// Core Settings
Engine {
    Name = "Spines Custom Engine"
    Version = 1, 0, 4
    DebugMode = 1
}

// Display Configuration
Window {
    Resolution = 1920, 1080
    Vsync = 1
    ClearColor = 0.15, 0.15, 0.15
}

// Scene Setup (Testing nested groups and anonymous idents)
Entities {
    *{
        Type = "Player"
        Position = -50.5, 100.25
        Velocity = 0, -9.81
        Health = 100
    }
    *{
        Type = "Enemy_Horde"
        Position = 300, 100
        Velocity = -15.5, 0
        Health = 50
    }
}

some { kind { of { deep { group {*{1, 2, 3}}, 4}, 5} } }
long_string = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in r"

LayerMask {
    player = 0b000101
    enemy  = 0b000010
    0x1a
}

float = 2.4e5
more_float = .5

// Trailing global assignment
Gravity = -9.81;

//this is the end)";

    std::string final;
    size_t base_len = strlen(base_str);

    final.reserve((base_len + 1) * 1024 * 10 * 10);

    // 1 KB
    // =========================================================================
{
    size_t cnt = 1;
    for (size_t i = 0; i < cnt; ++i) {
        final += base_str;
        if (i < cnt - 1) {
            final += "\n";
        }
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    spines_parse(&cxt, final.data(), final.length());
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> parse_duration = end_time - start_time;

    if (!cxt.buffer) return 0;

    std::cout << "1KB:   " << parse_duration.count() << " ms" << std::endl;

    SpinesContext_destroy(&cxt);
}

    // 1 MB
    // =========================================================================
{
    size_t cnt = 1024 - 1;
    for (size_t i = 0; i < cnt; ++i) {
        final += base_str;
        if (i < cnt - 1) {
            final += "\n";
        }
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    spines_parse(&cxt, final.data(), final.length());
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> parse_duration = end_time - start_time;

    if (!cxt.buffer) return 0;

    std::cout << "1MB:   " << parse_duration.count() << " ms" << std::endl;

    SpinesContext_destroy(&cxt);
}

    // 10 MB
    // =========================================================================
{
    size_t cnt = 1024 * 10 - 1 - 1024;
    for (size_t i = 0; i < cnt; ++i) {
        final += base_str;
        if (i < cnt - 1) {
            final += "\n";
        }
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    spines_parse(&cxt, final.data(), final.length());
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> parse_duration = end_time - start_time;

    if (!cxt.buffer) return 0;

    std::cout << "10MB:  " << parse_duration.count() << " ms" << std::endl;

    SpinesContext_destroy(&cxt);
}

    // 100 MB
    // =========================================================================
{
    size_t cnt = 1024 * 10 * 10 - 1 - 1024 - 1024 * 10;
    for (size_t i = 0; i < cnt; ++i) {
        final += base_str;
        if (i < cnt - 1) {
            final += "\n";
        }
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    spines_parse(&cxt, final.data(), final.length());
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> parse_duration = end_time - start_time;

    if (!cxt.buffer) return 0;

    std::cout << "100MB: " << parse_duration.count() << " ms" << std::endl;

    SpinesContext_destroy(&cxt);
}

    // 200 MB
    // =========================================================================
{
    size_t cnt = 1024 * 10 * 10 * 2 - 1 - 1024 - 1024 * 10 - 1024 * 10 * 10;
    for (size_t i = 0; i < cnt; ++i) {
        final += base_str;
        if (i < cnt - 1) {
            final += "\n";
        }
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    spines_parse(&cxt, final.data(), final.length());
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> parse_duration = end_time - start_time;

    if (!cxt.buffer) return 0;

    std::cout << "200MB: " << parse_duration.count() << " ms" << std::endl;

    SpinesContext_destroy(&cxt);
}

    // SpinesContext_destroy(&cxt);

    return 0;
}
