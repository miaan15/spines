#include "spines.h"
#include <stdio.h>
#include <string.h>

int main() {
    SpinesContext cxt = {0};

    const char *str =
"// Core Settings\n"
"Engine {\n"
"    Name = \"Spines Custom Engine\"\n"
"    Version = 1, 0, 4\n"
"    DebugMode = 1\n"
"}\n"
"\n"
"// Display Configuration\n"
"Window {\n"
"    Resolution = 1920, 1080\n"
"    Vsync = 1\n"
"    ClearColor = 0.15, 0.15, 0.15\n"
"}\n"
"\n"
"// Scene Setup (Testing nested groups and anonymous idents)\n"
"Entities {\n"
"    .0 {\n"
"        Type = \"Player\"\n"
"        Position = -50.5, 100.25\n"
"        Velocity = 0, -9.81\n"
"        Health = 100\n"
"    }\n"
"    .1 {\n"
"        Type = \"Enemy_Horde\"\n"
"        Position = 300, 100\n"
"        Velocity = -15.5, 0\n"
"        Health = 50\n"
"    }\n"
"}\n"
"\n"
"// Trailing global assignment\n"
"Gravity = -9.81\n";

    spines_parse(&cxt, str, strlen(str));

    if (!cxt.buffer) return 0;

    printf("\n\n");
    printf("--- IDENTS (Cap: %zu) ---\n", cxt.idents_cap);
    for (size_t i = 0; i < cxt.idents_cap; ++i) {
        SpinesIdent p = cxt.idents[i];

        if (p.name_begin != (size_t)-1) {
            printf("\t %zu: [%.*s]: fields %zu - %zu | parent of %zu\n",
                   i,
                   (int)p.name_len, cxt.ident_names + p.name_begin,
                   p.fields_begin, p.fields_len,
                   p.parent_len);
        } else {
            printf("\t %zu: [.%zu]: fields %zu - %zu | parent of %zu\n",
                   i,
                   p.name_len,
                   p.fields_begin, p.fields_len,
                   p.parent_len);
        }
    }

    printf("\n--- FIELDS (Cap: %zu) ---\n", cxt.fields_cap);
    for (size_t i = 0; i < cxt.fields_cap; ++i) {
        SpinesField p = cxt.field_vals[i];
        uint8_t t = cxt.field_types[i];

        const char *name = "int";
        if (t == FIELD_FLOAT) name = "float";
        if (t == FIELD_STR) name = "string";

        printf("\t %zu: [%s]: ", i, name);

        if (t == FIELD_INT) {
            printf("%lld\n", (long long)p.int_val);
        } else if (t == FIELD_FLOAT) {
            printf("%f\n", p.float_val);
        } else if (t == FIELD_STR) {
            printf("\"%.*s\"\n", (int)p.str_val.len, cxt.string_data + p.str_val.begin);
        }
    }

    SpinesContext_destroy(&cxt);

    return 0;
}
