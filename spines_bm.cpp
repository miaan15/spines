#include "spines.h"
#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <chrono>

SpinesContext SpinesContext_make(void) {
    return SpinesContext{};
}
void SpinesContext_destroy(SpinesContext *sc) {
    if (sc->buffer) free(sc->buffer);
    sc->buffer = NULL;
}

void buffer_init(SpinesContext *sc, size_t cap) {
    sc->buffer = malloc(cap);
    sc->buffer_offset = 0;
    sc->buffer_cap = cap;
}

void *buffer_alloc(SpinesContext *sc, size_t size, size_t align) {
    size_t aligned = (sc->buffer_offset + align - 1) & ~(align - 1);
    if (aligned + size > sc->buffer_cap) return NULL;
    sc->buffer_offset = aligned + size;
    char *ptr = (char *)sc->buffer + aligned;
    memset(ptr, 0, size);
    return ptr;
}

size_t search_til_necessary(const char *str, size_t str_len) {
    bool IS_SPACE[128] = { 0 };
    IS_SPACE[(unsigned char)' '] = 1;
    IS_SPACE[(unsigned char)'\t'] = 1;
    IS_SPACE[(unsigned char)'\n'] = 1;
    IS_SPACE[(unsigned char)'\r'] = 1;
    size_t i = 0;
    while (i < str_len) {
        while (i < str_len && IS_SPACE[(unsigned char)str[i]]) { ++i; }
        if (i + 1 < str_len && str[i] == '/' && str[i + 1] == '/') {
            while (i < str_len && str[i] != '\n') { ++i; }
            ++i;
            continue;
        }
        break;
    }
    return i;
}

typedef struct { size_t ident; size_t next_id_ident; } GroupStackEntry;
void handle_out_of_group(SpinesContext *sc, GroupStackEntry *stack,
                         size_t *stack_len) {
    assert(*stack_len > 0);
    SpinesIdent old_ident = sc->idents[stack[*stack_len - 1].ident];

    --*stack_len;

    if (*stack_len > 0) {
        SpinesIdent *cur_ident =
            &sc->idents[stack[*stack_len - 1].ident];
        cur_ident->fields_len += old_ident.fields_len;
        cur_ident->parent_len += old_ident.parent_len;
    }
}

bool count_data_and_check_syntax(const char *str, size_t str_len,
                                 bool *IS_IDENT_CHAR,
                                 bool *IS_NUM,
                                 bool *IS_NOT_NUM_CHAR,
                                 size_t *const idents_cap,
                                 size_t *const ident_names_size,
                                 size_t *const fields_cap,
                                 size_t *const string_data_size,
                                 size_t *const max_group_stack) {
    // Tokens: IDENT; ID_IDENT; NUM; STR; LBRACE; RBRACE; EQ; COMMA; NONE
    bool VALID_TOKEN_BEFORE_IDENT[9]    = {0, 1, 1, 1, 1, 1, 0, 1, 1};
    bool VALID_TOKEN_BEFORE_ID_IDENT[9] = {0, 1, 0, 0, 1, 1, 1, 1, 1};
    bool VALID_TOKEN_BEFORE_NUM[9]      = {0, 1, 1, 1, 1, 1, 1, 1, 1};
    bool VALID_TOKEN_BEFORE_STR[9]      = {0, 1, 1, 1, 1, 1, 1, 1, 1};
    bool VALID_TOKEN_BEFORE_LBRACE[9]   = {1, 0, 0, 0, 0, 0, 0, 0, 0};
    bool VALID_TOKEN_BEFORE_RBRACE[9]   = {0, 0, 1, 1, 0, 1, 0, 1, 0};
    bool VALID_TOKEN_BEFORE_EQ[9]       = {1, 0, 0, 0, 0, 0, 0, 0, 0};
    bool VALID_TOKEN_BEFORE_COMMA[9]    = {0, 0, 1, 1, 0, 1, 0, 0, 0};

    const char *err_str = str;
    size_t cur_index = 0;
    uint8_t last_token = 8;
    size_t cur_group_stack = 0;

    bool flag = 0;
    while (true) {
        if (str_len == 0 || str_len == (size_t)-1) return 1;
        size_t necessary_index = search_til_necessary(str, str_len);
        if (necessary_index >= str_len) return 1;
        str += necessary_index; str_len -= necessary_index;
        cur_index += necessary_index;

        flag = 0;

        const char front = str[0];
        switch (front) {
        case '{':
            if (!VALID_TOKEN_BEFORE_LBRACE[last_token]) goto ERROR;
            ++str; --str_len;
            ++cur_index;
            last_token = TOKEN_LBRACE;

            ++cur_group_stack;
            if (cur_group_stack > *max_group_stack)
                *max_group_stack = cur_group_stack;
        break;

        case '}':
            if (!VALID_TOKEN_BEFORE_RBRACE[last_token]) goto ERROR;
            ++str; --str_len;
            ++cur_index;
            last_token = TOKEN_RBRACE;

            --cur_group_stack;
            if (cur_group_stack == (size_t)-1) goto ERROR;
        break;

        case '=':
            if (!VALID_TOKEN_BEFORE_EQ[last_token]) goto ERROR;
            ++str; --str_len;
            ++cur_index;
            last_token = TOKEN_EQ;

            if (cur_group_stack + 1 > *max_group_stack)
                *max_group_stack = cur_group_stack + 1;
        break;

        case '*':
            if (!VALID_TOKEN_BEFORE_ID_IDENT[last_token]) goto ERROR;
            if (str_len < 2 && str[1] != '{') goto ERROR;
            str += 2; str_len -= 2;
            cur_index += 2;
            last_token = TOKEN_ID_IDENT;

            ++cur_group_stack;
            if (cur_group_stack > *max_group_stack)
                *max_group_stack = cur_group_stack;

            ++*idents_cap;
        break;

        case ',':
            if (!VALID_TOKEN_BEFORE_COMMA[last_token]) goto ERROR;
            ++str; --str_len;
            ++cur_index;
            last_token = TOKEN_COMMA;
        break;

        case '\"': {
            if (!VALID_TOKEN_BEFORE_STR[last_token]) goto ERROR;
            size_t i = 1;
            while (i < str_len && str[i] != '\"') { ++i; }

            if (i >= str_len) goto ERROR;
            str += i + 1; str_len -= i + 1;
            cur_index += i + 1;
            last_token = TOKEN_STR;

            ++*fields_cap;
            *string_data_size += i - 1;
        } break;

        default: flag = 1; break;
        }

        if (!flag) continue;

        // Ident
        if (IS_IDENT_CHAR[(unsigned char)front]) {
            if (!VALID_TOKEN_BEFORE_IDENT[last_token]) goto ERROR;
            size_t i = 0;
            while (i < str_len && IS_IDENT_CHAR[(unsigned char)str[i]]) { ++i; }

            str += i; str_len -= i;
            cur_index += i;
            last_token = TOKEN_IDENT;

            ++*idents_cap;
            *ident_names_size += i;

            continue;
        }

        // Number
        if (IS_NUM[(unsigned char)front]) {
            if (!VALID_TOKEN_BEFORE_NUM[last_token]) goto ERROR;
            size_t i = 0;
            while (i < str_len && !IS_NOT_NUM_CHAR[(unsigned char)str[i]]) {
                ++i;
            }

            str += i; str_len -= i;
            cur_index += i;
            last_token = TOKEN_NUM;

            ++*fields_cap;

            continue;
        }

        goto ERROR;
    }

    return 1;

ERROR: {
    size_t line = 1, col = 1;
    for (size_t i = 0; i < cur_index; ++i) {
        if (err_str[i] == '\n') {
            ++line;
            col = 1;
        } else {
            ++col;
        }
    }
    printf("Spines syntax error at %zu:%zu\n", line, col);
}
    return 0;
}

void spines_parse(SpinesContext *sc, const char *str, size_t str_len) {
    if (sc->buffer) {
        printf("spines_init: already initiated\n");
        return;
    }

    // --- TIMING: START ---
    auto t_start = std::chrono::high_resolution_clock::now();

    bool IS_IDENT_CHAR[128] = {0};
    for (char i = 'a'; i <= 'z'; ++i) IS_IDENT_CHAR[(unsigned char)i] = 1;
    for (char i = 'A'; i <= 'Z'; ++i) IS_IDENT_CHAR[(unsigned char)i] = 1;
    IS_IDENT_CHAR['_'] = 1;

    bool IS_NUM[128] = {0};
    for (char i = '0'; i <= '9'; ++i) IS_NUM[(unsigned char)i] = 1;
    IS_NUM['-'] = 1;
    IS_NUM['.'] = 1;

    bool IS_NOT_NUM_CHAR[128] = { 0 };
    IS_NOT_NUM_CHAR[(unsigned char)' ']  = 1;
    IS_NOT_NUM_CHAR[(unsigned char)'\t'] = 1;
    IS_NOT_NUM_CHAR[(unsigned char)'\n'] = 1;
    IS_NOT_NUM_CHAR[(unsigned char)'\r'] = 1;
    IS_NOT_NUM_CHAR[(unsigned char)'*']  = 1;
    IS_NOT_NUM_CHAR[(unsigned char)'}']  = 1;
    IS_NOT_NUM_CHAR[(unsigned char)',']  = 1;

    size_t max_group_stack = 0;

    if (!count_data_and_check_syntax(str, str_len,
                                     IS_IDENT_CHAR,
                                     IS_NUM,
                                     IS_NOT_NUM_CHAR,
                                     &sc->idents_cap,
                                     &sc->ident_names_size,
                                     &sc->fields_cap,
                                     &sc->string_data_size,
                                     &max_group_stack)) return;

    // --- TIMING: END OF COUNTING PHASE ---
    auto t_count_done = std::chrono::high_resolution_clock::now();

    size_t buffer_size = sc->idents_cap * sizeof(SpinesIdent); // idents
    buffer_size =
        (buffer_size + alignof(SpinesField) - 1) & ~(alignof(SpinesField) - 1);
    buffer_size += sc->fields_cap * sizeof(SpinesField); // field_vals
    buffer_size += sc->fields_cap // field_types
                   + sc->ident_names_size // ident_names
                   + sc->string_data_size; // string_data

    buffer_init(sc, buffer_size);
    sc->idents = (SpinesIdent *)buffer_alloc(
        sc, sc->idents_cap * sizeof(SpinesIdent), alignof(SpinesIdent));
    sc->field_vals = (SpinesField *)buffer_alloc(
        sc, sc->fields_cap * sizeof(SpinesField), alignof(SpinesField));
    sc->field_types = (uint8_t *)buffer_alloc(sc, sc->fields_cap, 1);
    sc->ident_names = (char *)buffer_alloc(sc, sc->ident_names_size, 1);
    sc->string_data = (char *)buffer_alloc(sc, sc->string_data_size, 1);

    // --- TIMING: END OF ALLOCATION PHASE ---
    auto t_alloc_done = std::chrono::high_resolution_clock::now();

    size_t idents_offset = 0;
    size_t fields_offset = 0;
    size_t ident_names_offset = 0;
    size_t string_data_offset = 0;

    GroupStackEntry group_stack[max_group_stack];
    memset(group_stack, 0, sizeof(group_stack));
    size_t group_stack_len = 0;

    size_t next_ident = 0;
    uint8_t just_after_eq = 0;

    bool flag = 0;
    while (true) {
        if (str_len == 0 || str_len == (size_t)-1) break; // Replaced early return with break to print timers
        size_t necessary_index = search_til_necessary(str, str_len);
        if (necessary_index >= str_len) break; // Replaced early return with break
        str += necessary_index; str_len -= necessary_index;

        if (just_after_eq > 0) --just_after_eq;
        flag = 0;

        const char front = str[0];
        switch (front) {
        case '{':
        case ',':
            ++str; --str_len;
        break;

        case '=':
            just_after_eq = 2;
            ++str; --str_len;
        break;

        // End group
        case '}':
            handle_out_of_group(sc, group_stack, &group_stack_len);
            ++str; --str_len;
        break;

        // ID Ident
        case '*':
            sc->idents[idents_offset++] = (SpinesIdent){
                .name_begin = (size_t)-1,
                .name_len = group_stack[group_stack_len - 1].next_id_ident,
                .fields_begin = fields_offset,
                .fields_len = 0,
                .parent_len = 1};
            assert(idents_offset <= sc->idents_cap);

            ++group_stack[group_stack_len - 1].next_id_ident;

            assert(group_stack_len < max_group_stack);
            group_stack[group_stack_len++] = (GroupStackEntry){next_ident, 0};
            ++next_ident;

            str += 2; str_len -= 2;
        break;

        // String
        case '\"': {
            size_t i = 1;
            while (i < str_len && str[i] != '\"') { ++i; }
            size_t len = i - 1;

            sc->field_types[fields_offset] = FIELD_STR;
            sc->field_vals[fields_offset] = (SpinesField){
                .str_val = {
                    .begin = (uint32_t)string_data_offset,
                    .len   = (uint32_t)len } };
            ++fields_offset;
            assert(fields_offset <= sc->fields_cap);

            memcpy(sc->string_data + string_data_offset, str + 1, len);
            string_data_offset += len;
            assert(string_data_offset <= sc->string_data_size);

            assert(group_stack_len > 0);
            assert(group_stack[group_stack_len - 1].ident < idents_offset);
            ++sc->idents[group_stack[group_stack_len - 1].ident].fields_len;

            if (just_after_eq)
                handle_out_of_group(sc, group_stack, &group_stack_len);

            str += i + 1; str_len -= i + 1;
        } break;

        default: flag = 1; break;
        }

        if (!flag) continue;

        // Ident
        if (IS_IDENT_CHAR[(unsigned char)front]) {
            size_t i = 0;
            while (i < str_len && IS_IDENT_CHAR[(unsigned char)str[i]]) { ++i; }

            sc->idents[idents_offset++] = (SpinesIdent){
                .name_begin = ident_names_offset,
                .name_len = i,
                .fields_begin = fields_offset,
                .fields_len = 0,
                .parent_len = 1};
            assert(idents_offset <= sc->idents_cap);

            memcpy(sc->ident_names + ident_names_offset, str, i);
            ident_names_offset += i;
            assert(ident_names_offset <= sc->ident_names_size);

            assert(group_stack_len < max_group_stack);
            group_stack[group_stack_len++] = (GroupStackEntry){next_ident, 0};
            ++next_ident;

            str += i; str_len -= i;

            continue;
        }

        // Number
        if (IS_NUM[(unsigned char)front]) {
            enum { INT, FLOAT, HEX, BIN, OCT } type = INT;

            size_t i = 0;

            int64_t sign = 1;
            if (str[i] == '-') {
                sign = -1;
                ++i;
            }

            if (i + 1 < str_len && str[i] == '0') {
                char sec = str[i + 1];
                if (sec == 'x' || sec == 'X') { type = HEX; i += 2; }
                else if (sec == 'b' || sec == 'B') { type = BIN; i += 2; }
                else if (sec == 'o' || sec == 'O') { type = OCT; i += 2; }
            }

            size_t e_count = 0, dot_count = 0;
            size_t begin = i;
            while (i < str_len && !IS_NOT_NUM_CHAR[(unsigned char)str[i]]) {
                if (type == INT || type == FLOAT) {
                    if (str[i] == 'e' || str[i] == 'E') e_count++;
                    if (str[i] == '.') dot_count++;
                }
                ++i;
            }
            size_t end = i;

            if (type == INT && (e_count > 0 || dot_count > 0)) {
                if (e_count > 1 || dot_count > 1) goto ERROR;
                type = FLOAT;
            }

            switch (type) {
            case INT: {
                sc->field_types[fields_offset] = FIELD_INT;
                int64_t val = 0;
                for (size_t j = begin; j < end; ++j) {
                    val = val * 10 + (str[j] - '0');
                }
                sc->field_vals[fields_offset].int_val = val * sign;
            } break;

            case FLOAT: {
                sc->field_types[fields_offset] = FIELD_FLOAT;
                char num_buf[64];
                size_t len = end < 63 ? end : 63;
                memcpy(num_buf, str, len);
                num_buf[len] = '\0';
                sc->field_vals[fields_offset].float_val = atof(num_buf);
            } break;

            case HEX: {
                sc->field_types[fields_offset] = FIELD_INT;
                int64_t val = 0;
                for (size_t j = begin; j < end; ++j) {
                    char c = str[j];
                    int digit = 0;
                    if (c >= '0' && c <= '9') digit = c - '0';
                    else if (c >= 'a' && c <= 'f') digit = 10 + (c - 'a');
                    else if (c >= 'A' && c <= 'F') digit = 10 + (c - 'A');
                    else goto ERROR;

                    val = (val << 4) | digit;
                }
                sc->field_vals[fields_offset].int_val = val * sign;
            } break;

            case BIN: {
                sc->field_types[fields_offset] = FIELD_INT;
                int64_t val = 0;
                for (size_t j = begin; j < end; ++j) {
                    char c = str[j];
                    int digit = 0;
                    if (c >= '0' && c <= '1') digit = c - '0';
                    else goto ERROR;

                    val = (val << 1) | digit;
                }
                sc->field_vals[fields_offset].int_val = val * sign;
            } break;

            case OCT: {
                sc->field_types[fields_offset] = FIELD_INT;
                int64_t val = 0;
                for (size_t j = begin; j < end; ++j) {
                    char c = str[j];
                    int digit = 0;
                    if (c >= '0' && c <= '8') digit = c - '0';
                    else goto ERROR;

                    val = (val << 3) | digit;
                }
                sc->field_vals[fields_offset].int_val = val * sign;
            } break;

            default: goto ERROR; break;
            }

            ++fields_offset;
            assert(fields_offset <= sc->fields_cap);

            assert(group_stack_len > 0);
            assert(group_stack[group_stack_len - 1].ident < idents_offset);
            ++sc->idents[group_stack[group_stack_len - 1].ident].fields_len;

            if (just_after_eq)
                handle_out_of_group(sc, group_stack, &group_stack_len);

            str += i; str_len -= i;

            continue;
        }

        goto ERROR;
    }

    // --- TIMING: END OF PARSE PHASE ---
    {
        auto t_parse_done = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> diff_count = t_count_done - t_start;
        std::chrono::duration<double, std::milli> diff_alloc = t_alloc_done - t_count_done;
        std::chrono::duration<double, std::milli> diff_parse = t_parse_done - t_alloc_done;
        std::chrono::duration<double, std::milli> diff_total = t_parse_done - t_start;

        printf("\n=== SPINES PARSER PROFILE ===\n");
        printf("Phase 1 (Syntax Check & Count) : %8.4f ms\n", diff_count.count());
        printf("Phase 2 (Memory Allocation)    : %8.4f ms\n", diff_alloc.count());
        printf("Phase 3 (Data Extraction)      : %8.4f ms\n", diff_parse.count());
        printf("-----------------------------------------\n");
        printf("Total execution time           : %8.4f ms\n", diff_total.count());
        printf("=================================\n\n");
    }
    return;

ERROR:
    if (sc->buffer) free(sc->buffer);
    sc->buffer = NULL;
    sc->idents = NULL;
    sc->field_vals = NULL;
    sc->field_types = NULL;
    sc->ident_names = sc->string_data = NULL;
    sc->buffer_offset = sc->buffer_cap = sc->idents_cap = sc->fields_cap
        = sc->ident_names_size = sc->string_data_size = 0;
    // TODO proper error
    printf("error, idk\n");
    return;
}
