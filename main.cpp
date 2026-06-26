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

    int multiplier = 1;
    std::cout << "> ";
    if (!(std::cin >> multiplier) || multiplier < 1) {
        std::cout << "Invalid input. Defaulting to 1." << std::endl;
        multiplier = 1;
    }

    std::string duplicated_str;
    size_t base_len = strlen(base_str);

    duplicated_str.reserve((base_len + 1) * multiplier);

    for (int i = 0; i < multiplier; ++i) {
        duplicated_str += base_str;
        if (i < multiplier - 1) {
            duplicated_str += "\n"; // Add a newline between duplications
        }
    }

    // =========================================================================
    auto start_time = std::chrono::high_resolution_clock::now();
    spines_parse(&cxt, duplicated_str.data(), duplicated_str.length());
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> parse_duration = end_time - start_time;

    if (!cxt.buffer) return 0;

    std::cout << 1.0 * duplicated_str.length() / 1024.0 << " KB"
              << " in " << parse_duration.count() << " ms" << std::endl;

    // printf("\n\n");
    // printf("--- IDENTS (Cap: %zu) ---\n", cxt.idents_cap);
    // for (size_t i = 0; i < cxt.idents_cap; ++i) {
    //     SpinesIdent p = cxt.idents[i];
    //
    //     if (p.name_begin != (size_t)-1) {
    //         printf("\t %zu: [%.*s]: fields %zu - %zu | parent of %zu\n",
    //                i,
    //                (int)p.name_len, cxt.ident_names + p.name_begin,
    //                p.fields_begin, p.fields_len,
    //                p.parent_len);
    //     } else {
    //         printf("\t %zu: [.%zu]: fields %zu - %zu | parent of %zu\n",
    //                i,
    //                p.name_len,
    //                p.fields_begin, p.fields_len,
    //                p.parent_len);
    //     }
    // }
    //
    // printf("\n--- FIELDS (Cap: %zu) ---\n", cxt.fields_cap);
    // for (size_t i = 0; i < cxt.fields_cap; ++i) {
    //     SpinesField p = cxt.field_vals[i];
    //     uint8_t t = cxt.field_types[i];
    //
    //     const char *name = "int";
    //     if (t == FIELD_FLOAT) name = "float";
    //     if (t == FIELD_STR) name = "string";
    //
    //     printf("\t %zu: [%s]: ", i, name);
    //
    //     if (t == FIELD_INT) {
    //         printf("%lld\n", (long long)p.int_val);
    //     } else if (t == FIELD_FLOAT) {
    //         printf("%f\n", p.float_val);
    //     } else if (t == FIELD_STR) {
    //         printf("\"%.*s\"\n", (int)p.str_val.len, cxt.string_data + p.str_val.begin);
    //     }
    // }

    SpinesContext_destroy(&cxt);

    return 0;
}
