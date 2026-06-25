#include "spines.h"

#include <stdalign.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void buffer_init(SpinesContext *sc, size_t cap) {
    sc->buffer = malloc(cap);
    sc->buffer_offset = 0;
    sc->buffer_cap = cap;
    memset(sc->buffer, 0, cap);
}

void *buffer_alloc(SpinesContext *sc, size_t size, size_t align) {
    size_t aligned = (sc->buffer_offset + align - 1) & ~(align - 1);
    if (aligned + size > sc->buffer_cap) return NULL;
    sc->buffer_offset = aligned + size;
    char *ptr = (char *)sc->buffer + aligned;
    memset(ptr, 0, size);
    return ptr;
}

size_t find_first_not_whitespace(const char *str, size_t str_len) {
    bool IS_SPACE[256] = { [' '] = 1, ['\t'] = 1, ['\n'] = 1, ['\r'] = 1 };
    size_t i = 0;
    while (i < str_len && IS_SPACE[str[i]]) { ++i; }
    return i;
}

size_t search_til_necessary(const char *str, size_t str_len) {
    bool IS_SPACE[256] = { [' '] = 1, ['\t'] = 1, ['\n'] = 1, ['\r'] = 1 };
    size_t i = 0;
    while (i < str_len) {
        while (i < str_len && IS_SPACE[str[i]]) { ++i; }
        if (i + 1 < str_len && str[i] == '/' && str[i + 1] == '/') {
            while (i < str_len && str[i] != '\n') { ++i; }
            ++i;
        }
    }
    return i;
}

void count_data_and_check_syntax(const char *str, size_t str_len,
                                 bool *IS_IDENT_CHAR,
                                 bool *IS_NUM,
                                 size_t *const idents_cap,
                                 size_t *const ident_names_size,
                                 size_t *const fields_cap,
                                 size_t *const string_data_size,
                                 size_t *const max_group_stack) {
    // Tokens: IDENT; ID_IDENT; NUM; STR; LBRACE; RBRACE; EQ; COMMA; NONE
    bool VALID_TOKEN_BEFORE_IDENT[9]    = {0, 0, 1, 1, 1, 1, 0, 1, 1};
    bool VALID_TOKEN_BEFORE_ID_IDENT[9] = {0, 1, 0, 0, 1, 1, 1, 1, 1};
    bool VALID_TOKEN_BEFORE_NUM[9]      = {0, 1, 0, 1, 1, 1, 1, 1, 1};
    bool VALID_TOKEN_BEFORE_STR[9]      = {0, 1, 0, 1, 1, 1, 1, 1, 1};
    bool VALID_TOKEN_BEFORE_LBRACE[9]   = {1, 0, 0, 0, 0, 0, 0, 0, 0};
    bool VALID_TOKEN_BEFORE_RBRACE[9]   = {0, 0, 1, 1, 0, 1, 0, 1, 0};
    bool VALID_TOKEN_BEFORE_EQ[9]       = {1, 0, 0, 0, 0, 0, 0, 0, 0};
    bool VALID_TOKEN_BEFORE_COMMA[9]    = {0, 0, 1, 1, 0, 1, 0, 0, 0};

    size_t cur_index = 0;
    uint8_t last_token = 8;
    size_t cur_group_stack = 0;

    while (true) {
        if (str_len == 0 || str_len == (size_t)-1) return;
        size_t necessary_index = search_til_necessary(str, str_len);
        if (necessary_index >= str_len) return;
        str += necessary_index; str_len -= necessary_index;

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
        break;

        case '=':
            if (!VALID_TOKEN_BEFORE_EQ[last_token]) goto ERROR;
            ++str; --str_len;
            ++cur_index;
            last_token = TOKEN_EQ;
        break;

        case '.':
            if (!VALID_TOKEN_BEFORE_ID_IDENT[last_token]) goto ERROR;
            str += 2; str_len -= 2;
            cur_index += 2;
            last_token = TOKEN_ID_IDENT;

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

        case '\'': {
            if (!VALID_TOKEN_BEFORE_STR[last_token]) goto ERROR;
            size_t i = 1;
            while (i < str_len && str[i] != '\'') { ++i; }

            if (i >= str_len) goto ERROR;
            str += i + 1; str_len -= i + 1;
            cur_index += i + 1;
            last_token = TOKEN_STR;

            ++*fields_cap;
            *string_data_size += i - 1;
        } break;

        default: break;
        }

        // Ident
        if (IS_IDENT_CHAR[front]) {
            if (!VALID_TOKEN_BEFORE_IDENT[last_token]) goto ERROR;
            size_t i = 0;
            while (i < str_len && IS_IDENT_CHAR[str[i]]) { ++i; }

            str += i; str_len -= i;
            cur_index += i;
            last_token = TOKEN_IDENT;

            ++*idents_cap;
            *ident_names_size += i;

            continue;
        }

        // Number
        if (IS_NUM[front] || front == '-') {
            if (!VALID_TOKEN_BEFORE_IDENT[last_token]) goto ERROR;
            size_t i = 0;
            while (i < str_len && IS_NUM[str[i]]) { ++i; }

            str += i; str_len -= i;
            cur_index += i;
            last_token = TOKEN_NUM;

            ++*fields_cap;

            continue;
        }

        goto ERROR;
    }

    return;

ERROR: {
    size_t line = 1, col = 1;
    for (size_t i = 0; i < cur_index; ++i) {
        if (str[i] == '\n') {
            ++line;
            col = 1;
        } else {
            ++col;
        }
    }
    printf("Spines syntax error at %zu:%zu", line, col);
}
    return;
}

void spines_init(SpinesContext *sc, const char *str, size_t str_len) {
    if (sc->buffer) {
        printf("spines_init: already initiated");
        return;
    }

    bool IS_IDENT_CHAR[256] = {0};
    for (char i = 'a'; i <= 'z'; ++i) IS_IDENT_CHAR[i] = 1;
    for (char i = 'A'; i <= 'Z'; ++i) IS_IDENT_CHAR[i] = 1;
    IS_IDENT_CHAR['_'] = 1;

    bool IS_NUM[256] = {0};
    for (char i = '0'; i <= '9'; ++i) IS_NUM[i] = 1;
    IS_NUM['.'] = 1;
    IS_NUM['-'] = 1;

    size_t max_group_stack = 0;
    count_data_and_check_syntax(str, str_len,
                                IS_IDENT_CHAR,
                                IS_NUM,
                                &sc->idents_cap,
                                &sc->ident_names_size,
                                &sc->fields_cap,
                                &sc->string_data_size,
                                &max_group_stack);

    size_t buffer_size = sc->idents_cap * sizeof(SpinesIdent); // idents
    buffer_size =
        (buffer_size + alignof(SpinesField) - 1) & ~(alignof(SpinesField) - 1);
    buffer_size += sc->fields_cap * sizeof(SpinesField); // field_vals
    buffer_size += sc->fields_cap // field_types
                   + sc->ident_names_size // ident_names
                   + sc->string_data_size; // string_data

    buffer_init(sc, buffer_size);
    sc->idents = (SpinesIdent *)buffer_alloc(sc, sizeof(SpinesIdent),
                                                 alignof(SpinesIdent));
    sc->field_vals = (SpinesField *)buffer_alloc(sc, sizeof(SpinesField),
                                                     alignof(SpinesField));
    sc->field_types = (uint8_t *)buffer_alloc(sc, sizeof(uint8_t),
                                                  alignof(uint8_t));
    sc->ident_names = (char *)buffer_alloc(sc, sizeof(char), alignof(char));
    sc->string_data = (char *)buffer_alloc(sc, sizeof(char), alignof(char));

    size_t idents_offset = 0;
    size_t fields_offset = 0;
    size_t ident_names_offset = 0;
    size_t string_data_offset = 0;

    typedef struct { size_t ident; size_t next_id_ident; } GroupStackEntry;
    GroupStackEntry group_stack[max_group_stack];
    memset(group_stack, 0, sizeof(group_stack));
    size_t group_stack_head = 0;

    while (true) {
        if (str_len == 0 || str_len == (size_t)-1) return;
        size_t necessary_index = search_til_necessary(str, str_len);
        if (necessary_index >= str_len) return;
        str += necessary_index; str_len -= necessary_index;

        const char front = str[0];
        switch (front) {
        case '{':
        case '=':
        case ',':
            ++str; --str_len;
        break;

        // End group
        case '}':

        break;

        // ID Ident
        case '.':

        break;

        // String
        case '\"':
        case '\'':

        break;

        default: break;
        }

        // Ident
        if (IS_IDENT_CHAR[front]) {
            size_t i = 0;
            while (i < str_len && IS_IDENT_CHAR[str[i]]) { ++i; }

            continue;
        }

        // Number
        if (IS_NUM[front] || front == '-') {
            size_t i = 0;
            while (i < str_len && IS_NUM[str[i]]) { ++i; }

            continue;
        }

        goto ERROR;
    }

    return;

ERROR:
    return;
}

