#ifndef _SPINES_H
#define _SPINES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum {
    TOKEN_IDENT = 0,
    TOKEN_ID_IDENT,
    TOKEN_NUM,
    TOKEN_STR,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_EQ,
    TOKEN_COMMA
} spn_TokenType;

typedef struct {
    uint8_t type;
    size_t len;
    size_t index;
} spn_Token;

typedef enum {
    FIELD_INT = 0,
    FIELD_FLOAT,
    FIELD_STR
} spn_FieldType;

typedef union {
    int64_t int_val;
    double float_val;
    struct {
        uint32_t begin, len;
    } str_val;
} spn_Field;

typedef struct {
    size_t name_begin, name_len;
    size_t fields_begin, fields_len;
    size_t parent_len;
} spn_Ident;

typedef struct {
    void *buffer;
    size_t buffer_offset;
    size_t buffer_cap;

    // Mem layout: idents -> field_vals -> field_types
    //             -> ident_names -> string_data
    spn_Ident *idents;
    size_t idents_cap;

    spn_Field *field_vals;
    uint8_t *field_types;
    size_t fields_cap;

    char *ident_names;
    size_t ident_names_size;

    char *string_data;
    size_t string_data_size;
} spn_Context;

void spn_destroy(spn_Context *cxt);

void spn_parse(spn_Context *cxt, const char *str_ptr, size_t str_len);

typedef struct {
    spn_Context *cxt;
    size_t index;
    spn_Field *fields;
} spn_Group;

spn_Group spn_root(spn_Context *cxt);

void spn_move(spn_Group *gr, const char *dir);

void spn_move_id(spn_Group *gr, size_t id);

spn_Group spn_find(spn_Group *gr, const char *dir);

spn_Group spn_find_id(spn_Group *gr, size_t id);

bool spn_step(spn_Group *gr);

spn_Group spn_next(spn_Group *gr);

bool spn_step_flat(spn_Group *gr);

spn_Group spn_next_flat(spn_Group *gr);

const char *spn_str(spn_Context *cxt, spn_Field field);

#endif
