extern "C" {
#include "spines.h"
}

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <chrono>

int main() {
    const char *base_str =
R"(// Core Settings
Engine {
    Name = "spn_ Custom Engine"
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
    spn_Context cxt = {};

    size_t cnt = 1;
    for (size_t i = 0; i < cnt; ++i) {
        final += base_str;
        if (i < cnt - 1) {
            final += "\n";
        }
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    spn_parse(&cxt, final.data(), final.length());
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> parse_duration = end_time - start_time;

    if (!cxt.buffer) return 0;

    std::cout << "1KB:   " << parse_duration.count() << " ms" << std::endl;

    spn_destroy(&cxt);
}

    // 1 MB
    // =========================================================================
{
    spn_Context cxt = {};

    size_t cnt = 1024 - 1;
    for (size_t i = 0; i < cnt; ++i) {
        final += base_str;
        if (i < cnt - 1) {
            final += "\n";
        }
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    spn_parse(&cxt, final.data(), final.length());
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> parse_duration = end_time - start_time;

    if (!cxt.buffer) return 0;

    std::cout << "1MB:   " << parse_duration.count() << " ms" << std::endl;

    spn_destroy(&cxt);
}

    // 10 MB
    // =========================================================================
{
    spn_Context cxt = {};

    size_t cnt = 1024 * 10 - 1 - 1024;
    for (size_t i = 0; i < cnt; ++i) {
        final += base_str;
        if (i < cnt - 1) {
            final += "\n";
        }
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    spn_parse(&cxt, final.data(), final.length());
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> parse_duration = end_time - start_time;

    if (!cxt.buffer) return 0;

    std::cout << "10MB:  " << parse_duration.count() << " ms" << std::endl;

    spn_destroy(&cxt);
}

    // 100 MB
    // =========================================================================
{
    spn_Context cxt = {};

    size_t cnt = 1024 * 10 * 10 - 1 - 1024 - 1024 * 10;
    for (size_t i = 0; i < cnt; ++i) {
        final += base_str;
        if (i < cnt - 1) {
            final += "\n";
        }
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    spn_parse(&cxt, final.data(), final.length());
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> parse_duration = end_time - start_time;

    if (!cxt.buffer) return 0;

    std::cout << "100MB: " << parse_duration.count() << " ms" << std::endl;

    spn_destroy(&cxt);
}

    printf("\n\n");

    // Example
    const char *str =
R"(// Comments are allowed

// Make a group:
Base {
    36, 3.141592, 1.6e9, 0x1A6F, // support multiple kind of number
    "Lorem ipsum dolor sit amet", "consectetur adipiscing elit" // support string
    Sub { // nested group
        6.7, 3.6
        Value = "funny nummbers" // direct assign
    }
}

Frame {
    // auto indexed group
    // each *{ will be considered as "*<index>"
    *{ 0, 1, 9 } // 0
    *{ 0, 2, 8 } // 1
    *{ 1, 3, 7 } // 2
    *{ 1, 4, 6 } // 3
})";
    spn_Context cxt = {};
    spn_parse(&cxt, str, strlen(str));

    spn_Group global_gr = spn_root(&cxt);
    spn_Group sub_gr = spn_find(global_gr, "Base/Sub");
    float v0 = (float)sub_gr.fields[0].float_val;
    float v1 = (float)sub_gr.fields[1].float_val;

    printf("In Base/Sub: [0] = %.2f; [1] = %.2f\n", v0, v1);

    spn_Group v_gr = spn_root(&cxt);
    spn_move(&v_gr, "Base/Sub/Value");
    const char *vs = spn_str(&cxt, v_gr.fields[0]);

    printf("In Base/Sub/Value: %s\n", vs);

    int v3 = (int)spn_find(global_gr, "Frame/*1").fields[0].int_val;
    int v4 = (int)spn_find_id(spn_find(global_gr, "Frame"), 3).fields[2].int_val;

    printf("In Frame: *1[0] = %d; *3[2] = %d\n", v3, v4);

    spn_Group frame_iter_gr = spn_next_flat(spn_find(global_gr, "Frame"));
    printf("Iterate Frame: ");
    do {
        int x = frame_iter_gr.fields[1].int_val;
        printf("%d ", x);
    } while (spn_step(&frame_iter_gr));

    spn_destroy(&cxt);

    return 0;
}
