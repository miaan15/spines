#ifndef __SPINES_H
#define __SPINES_H

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

typedef enum {
    // all the ascii can be token type
    TOKEN_IDENT = 0,
    TOKEN_ID_IDENT,
    TOKEN_NUM,
    TOKEN_STR,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_EQ,
    TOKEN_COMMA
} SpinesTokenType;

typedef struct {
    uint8_t type;
    size_t len;
    size_t index;
} SpinesToken;

typedef enum {
    FIELD_INT = 0,
    FIELD_FLOAT,
    FIELD_STR
} SpinesFieldType;

typedef union {
    int64_t int_val;
    double float_val;
    struct {
        uint32_t begin, len;
    } str_val;
} SpinesField;

typedef struct {
    size_t name_begin, name_len;
    size_t fields_begin, fields_len;
    size_t parent_len;
} SpinesIdent;

typedef struct {
    void *buffer;
    size_t buffer_offset;
    size_t buffer_cap;

    // Mem layout: idents -> field_vals -> field_types
    //             -> ident_names -> string_data
    SpinesIdent *idents;
    size_t idents_cap;

    SpinesField *field_vals;
    uint8_t *field_types;
    size_t fields_cap;

    char *ident_names;
    size_t ident_names_size;

    char *string_data;
    size_t string_data_size;
} SpinesContext;

void spines_parse(SpinesContext *sc, const char *str_ptr, size_t str_len);

SpinesContext SpinesContext_make(void);
void SpinesContext_destroy(SpinesContext *sc);

#endif
