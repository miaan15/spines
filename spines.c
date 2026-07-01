#include "spines.h"

#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SPN_DISABLE_ERROR
char _spn_err_buffer[128] = "";
#   define _SET_ERR_CODE(err_code, val) (err_code = val)
#else
#   define _SET_ERR_CODE(err_code, val) ((void)0)
#endif

void buffer_init(spn_Context *cxt, size_t cap) {
    cxt->buffer = malloc(cap);
    cxt->buffer_offset = 0;
    cxt->buffer_cap = cap;
}

void *buffer_alloc(spn_Context *cxt, size_t size, size_t align) {
    size_t aligned = (cxt->buffer_offset + align - 1) & ~(align - 1);
    if (aligned + size > cxt->buffer_cap) return NULL;
    cxt->buffer_offset = aligned + size;
    char *ptr = (char *)cxt->buffer + aligned;
    memset(ptr, 0, size);
    return ptr;
}

size_t search_til_necessary(const char *str, size_t str_len) {
    bool IS_SPACE[128] = { [' '] = 1, ['\t'] = 1, ['\n'] = 1, ['\r'] = 1 };
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
void handle_out_of_group(spn_Context *cxt, GroupStackEntry *stack,
                         size_t *stack_len) {
    assert(*stack_len > 0);
    spn_Ident old_ident = cxt->idents[stack[*stack_len - 1].ident];

    --*stack_len;

    if (*stack_len > 0) {
        spn_Ident *cur_ident =
            &cxt->idents[stack[*stack_len - 1].ident];
        cur_ident->fields_len += old_ident.fields_len;
        cur_ident->parent_len += old_ident.parent_len;
    }
}

bool cal_data_and_check_syntax(const char *str, size_t str_len,
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

#ifndef SPN_DISABLE_ERROR
    const char *err_str = str;
    char err_code = 0;
#endif

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

            if (cur_group_stack == 0) {
                _SET_ERR_CODE(err_code, 1);
                goto ERROR;
            }
            --cur_group_stack;
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

            if (i >= str_len) {
                _SET_ERR_CODE(err_code, 2);
                goto ERROR;
            }
            str += i + 1; str_len -= i + 1;
            cur_index += i + 1;
            last_token = TOKEN_STR;

            ++*fields_cap;
            *string_data_size += i - 1 + 1;
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
            *ident_names_size += i + 1;

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
#ifndef SPN_DISABLE_ERROR
    size_t line = 1, col = 1;
    for (size_t i = 0; i < cur_index; ++i) {
        if (err_str[i] == '\n') {
            ++line;
            col = 1;
        } else {
            ++col;
        }
    }

    if (err_code == 1) {
        _SPN_SET_ERROR("spn_parse(): redundant \"}\" symbol at %zu:%zu", line, col);
    } else if (err_code == 2) {
        _SPN_SET_ERROR("spn_parse(): forgot to close string with \" at %zu:%zu", line, col);
    } else {
        _SPN_SET_ERROR("spn_parse(): syntax error at %zu:%zu", line, col);
    }
#endif
}
    return 0;
}

void spn_parse(spn_Context *cxt, const char *str, size_t str_len) {
    if (cxt->buffer) {
        _SPN_SET_ERROR("spn_parse(): spn_Context already initiated%s", "");
        return;
    }

    bool IS_IDENT_CHAR[128] = {0};
    for (char i = 'a'; i <= 'z'; ++i) IS_IDENT_CHAR[(unsigned char)i] = 1;
    for (char i = 'A'; i <= 'Z'; ++i) IS_IDENT_CHAR[(unsigned char)i] = 1;
    IS_IDENT_CHAR['_'] = 1;

    bool IS_NUM[128] = {0};
    for (char i = '0'; i <= '9'; ++i) IS_NUM[(unsigned char)i] = 1;
    IS_NUM['-'] = 1;
    IS_NUM['.'] = 1;

    bool IS_NOT_NUM_CHAR[128] = { [' '] = 1, ['\t'] = 1, ['\n'] = 1,
                                  ['\r'] = 1, ['*'] = 1, ['}'] = 1, [','] = 1 };

    size_t max_group_stack = 0;
    if (!cal_data_and_check_syntax(str, str_len,
                                   IS_IDENT_CHAR,
                                   IS_NUM,
                                   IS_NOT_NUM_CHAR,
                                   &cxt->idents_cap,
                                   &cxt->ident_names_size,
                                   &cxt->fields_cap,
                                   &cxt->string_data_size,
                                   &max_group_stack)) return;

    ++cxt->idents_cap; // global group
    size_t buffer_size = cxt->idents_cap * sizeof(spn_Ident); // idents
    buffer_size =
        (buffer_size + alignof(spn_Field) - 1) & ~(alignof(spn_Field) - 1);
    buffer_size += cxt->fields_cap * sizeof(spn_Field); // field_vals
    buffer_size += cxt->fields_cap // field_types
                   + cxt->ident_names_size // ident_names
                   + cxt->string_data_size; // string_data

    buffer_init(cxt, buffer_size);
    cxt->idents = (spn_Ident *)buffer_alloc(
        cxt, cxt->idents_cap * sizeof(spn_Ident), alignof(spn_Ident));
    cxt->field_vals = (spn_Field *)buffer_alloc(
        cxt, cxt->fields_cap * sizeof(spn_Field), alignof(spn_Field));
    cxt->field_types = (uint8_t *)buffer_alloc(cxt, cxt->fields_cap, 1);
    cxt->ident_names = (char *)buffer_alloc(cxt, cxt->ident_names_size, 1);
    cxt->string_data = (char *)buffer_alloc(cxt, cxt->string_data_size, 1);

    cxt->idents[0] =
        (spn_Ident){(size_t)-1, 0, 0, cxt->fields_cap, cxt->idents_cap};
    size_t idents_offset = 1;
    size_t fields_offset = 0;
    size_t ident_names_offset = 0;
    size_t string_data_offset = 0;

#ifndef SPN_DISABLE_ERROR
    const char *err_str = str;
    char err_code = 0;
#endif

    size_t cur_index = 0;

    GroupStackEntry group_stack[max_group_stack];
    memset(group_stack, 0, sizeof(group_stack));
    size_t group_stack_len = 0;

    size_t next_ident = 1;
    uint8_t just_after_eq = 0;

    bool flag = 0;
    while (true) {
        if (str_len == 0 || str_len == (size_t)-1) return;
        size_t necessary_index = search_til_necessary(str, str_len);
        if (necessary_index >= str_len) return;
        str += necessary_index; str_len -= necessary_index;
        cur_index += necessary_index;

        if (just_after_eq > 0) --just_after_eq;
        flag = 0;

        const char front = str[0];
        switch (front) {
        case '{':
        case ',':
            ++str; --str_len;
            ++cur_index;
        break;

        case '=':
            just_after_eq = 2;
            ++str; --str_len;
            ++cur_index;
        break;

        // End group
        case '}':
            handle_out_of_group(cxt, group_stack, &group_stack_len);
            ++str; --str_len;
            ++cur_index;
        break;

        // ID Ident
        case '*':
            cxt->idents[idents_offset++] = (spn_Ident){
                .name_begin = (size_t)-1,
                .name_len = group_stack[group_stack_len - 1].next_id_ident,
                .fields_begin = fields_offset,
                .fields_len = 0,
                .parent_len = 1};
            assert(idents_offset <= cxt->idents_cap);

            ++group_stack[group_stack_len - 1].next_id_ident;

            assert(group_stack_len < max_group_stack);
            group_stack[group_stack_len++] = (GroupStackEntry){next_ident, 0};
            ++next_ident;

            str += 2; str_len -= 2;
            cur_index += 2;
        break;

        // String
        case '\"': {
            size_t i = 1;
            while (i < str_len && str[i] != '\"') { ++i; }
            size_t len = i - 1;

            cxt->field_types[fields_offset] = FIELD_STR;
            cxt->field_vals[fields_offset] = (spn_Field){
                .str_val = {
                    .begin = (uint32_t)string_data_offset,
                    .len   = (uint32_t)len } };
            ++fields_offset;
            assert(fields_offset <= cxt->fields_cap);

            memcpy(cxt->string_data + string_data_offset, str + 1, len);
            cxt->string_data[len] = '\0';
            string_data_offset += len + 1;
            assert(string_data_offset <= cxt->string_data_size);

            assert(group_stack_len > 0);
            assert(group_stack[group_stack_len - 1].ident < idents_offset);
            ++cxt->idents[group_stack[group_stack_len - 1].ident].fields_len;

            if (just_after_eq)
                handle_out_of_group(cxt, group_stack, &group_stack_len);

            str += i + 1; str_len -= i + 1;
            cur_index += i + 1;
        } break;

        default: flag = 1; break;
        }

        if (!flag) continue;

        // Ident
        if (IS_IDENT_CHAR[(unsigned char)front]) {
            size_t i = 0;
            while (i < str_len && IS_IDENT_CHAR[(unsigned char)str[i]]) { ++i; }

            cxt->idents[idents_offset++] = (spn_Ident){
                .name_begin = ident_names_offset,
                .name_len = i,
                .fields_begin = fields_offset,
                .fields_len = 0,
                .parent_len = 1};
            assert(idents_offset <= cxt->idents_cap);

            memcpy(cxt->ident_names + ident_names_offset, str, i);
            cxt->string_data[ident_names_offset + i] = '\0';
            ident_names_offset += i + 1;
            assert(ident_names_offset <= cxt->ident_names_size);

            assert(group_stack_len < max_group_stack);
            group_stack[group_stack_len++] = (GroupStackEntry){next_ident, 0};
            ++next_ident;

            str += i; str_len -= i;
            cur_index += i;

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
                cxt->field_types[fields_offset] = FIELD_INT;
                int64_t val = 0;
                for (size_t j = begin; j < end; ++j) {
                    val = val * 10 + (str[j] - '0');
                }
                cxt->field_vals[fields_offset].int_val = val * sign;
            } break;

            case FLOAT: {
                cxt->field_types[fields_offset] = FIELD_FLOAT;
                char num_buf[64];
                size_t len = end < 63 ? end : 63;
                memcpy(num_buf, str, len);
                num_buf[len] = '\0';
                cxt->field_vals[fields_offset].float_val = atof(num_buf);
            } break;

            case HEX: {
                cxt->field_types[fields_offset] = FIELD_INT;
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
                cxt->field_vals[fields_offset].int_val = val * sign;
            } break;

            case BIN: {
                cxt->field_types[fields_offset] = FIELD_INT;
                int64_t val = 0;
                for (size_t j = begin; j < end; ++j) {
                    char c = str[j];
                    int digit = 0;
                    if (c >= '0' && c <= '1') digit = c - '0';
                    else goto ERROR;

                    val = (val << 1) | digit;
                }
                cxt->field_vals[fields_offset].int_val = val * sign;
            } break;

            case OCT: {
                cxt->field_types[fields_offset] = FIELD_INT;
                int64_t val = 0;
                for (size_t j = begin; j < end; ++j) {
                    char c = str[j];
                    int digit = 0;
                    if (c >= '0' && c <= '8') digit = c - '0';
                    else goto ERROR;

                    val = (val << 3) | digit;
                }
                cxt->field_vals[fields_offset].int_val = val * sign;
            } break;

            default:
                _SET_ERR_CODE(err_code, 1);
                goto ERROR;
            break;
            }

            ++fields_offset;
            assert(fields_offset <= cxt->fields_cap);

            assert(group_stack_len > 0);
            assert(group_stack[group_stack_len - 1].ident < idents_offset);
            ++cxt->idents[group_stack[group_stack_len - 1].ident].fields_len;

            if (just_after_eq)
                handle_out_of_group(cxt, group_stack, &group_stack_len);

            str += i; str_len -= i;
            cur_index += i;

            continue;
        }

        _SET_ERR_CODE(err_code, 1);
        goto ERROR;
    }

    return;

ERROR: {
    if (cxt->buffer) free(cxt->buffer);
    cxt->buffer = NULL;
    cxt->idents = NULL;
    cxt->field_vals = NULL;
    cxt->field_types = NULL;
    cxt->ident_names = cxt->string_data = NULL;
    cxt->buffer_offset = cxt->buffer_cap = cxt->idents_cap = cxt->fields_cap
        = cxt->ident_names_size = cxt->string_data_size = 0;

#ifndef SPN_DISABLE_ERROR
    size_t line = 1, col = 1;
    for (size_t i = 0; i < cur_index; ++i) {
        if (err_str[i] == '\n') {
            ++line;
            col = 1;
        } else {
            ++col;
        }
    }

    if (err_code == 1) {
        _SPN_SET_ERROR("spn_parse(): syntax error at %zu:%zu", line, col);
    } else {
        _SPN_SET_ERROR("spn_parse(): number format error at %zu:%zu", line, col);
    }
#endif
}
    return;
}

void spn_move(spn_Group *gr, const char *dir) {
    spn_Context *cxt = gr->cxt;
    assert(cxt);
    assert(cxt->buffer);

    bool IS_SEPARATOR[128] = { ['/'] = 1 };
    size_t new_index = gr->index;
    while (true) {
        size_t l = 0;
        while (!IS_SEPARATOR[(unsigned char)dir[l]] && dir[l] != '\0') { ++l; }

        size_t idir = (size_t)-1;
        if (dir[0] == '*') {
            idir = 0;
            for (size_t i = 1; i < l; ++i) {
                idir = idir * 10 + (dir[i] - '0');
            }
        }

        size_t begin = new_index + 1;
        size_t end = new_index + cxt->idents[new_index].parent_len;
        assert(begin <= end);
        assert(end <= cxt->idents_cap);
        for (size_t i = begin; i < end;) {
            spn_Ident ident = cxt->idents[i];
            if (idir == (size_t)-1) {
                if (ident.name_begin != (size_t)-1 && ident.name_len == l
                    && strncmp(dir, cxt->ident_names + ident.name_begin, l) == 0) {
                    new_index = i;
                    break;
                }
            } else {
                if (ident.name_begin == (size_t)-1 && ident.name_len == idir) {
                    new_index = i;
                    break;
                }
            }

            i += ident.parent_len;

            if (i >= end) {
                _SPN_SET_ERROR("spn_move(): not found \"%.*s\"\n", (int)l, dir);
                return;
            }
        }

        if (dir[l] == '\0') break;
        dir += l + 1;
    }

    gr->index = new_index;
}

void spn_move_id(spn_Group *gr, size_t id) {
    spn_Context *cxt = gr->cxt;
    assert(cxt);
    assert(cxt->buffer);

    size_t new_index = gr->index;
    size_t begin = new_index + 1;
    size_t end = new_index + cxt->idents[new_index].parent_len;
    assert(begin <= end);
    assert(end <= cxt->idents_cap);
    for (size_t i = begin; i < end;) {
        spn_Ident ident = cxt->idents[i];
        if (ident.name_begin == (size_t)-1 && ident.name_len == id) {
            new_index = i;
            break;
        }

        i += ident.parent_len;

        if (i >= end) {
            _SPN_SET_ERROR("spn_move_id(): not found %zu\n", id);
            return;
        }
    }

    gr->index = new_index;
}

bool spn_step(spn_Group *gr) {
    spn_Context *cxt = gr->cxt;
    assert(cxt);
    assert(cxt->buffer);

    size_t new_index = gr->index + cxt->idents[gr->index].parent_len;
    if (new_index >= cxt->idents_cap) {
        return false;
    }

    gr->index = new_index;

    return true;
}

bool spn_step_flat(spn_Group *gr) {
    spn_Context *cxt = gr->cxt;
    assert(cxt);
    assert(cxt->buffer);

    size_t new_index = gr->index + 1;
    if (new_index >= cxt->idents_cap) {
        return false;
    }

    gr->index = new_index;

    return true;
}
