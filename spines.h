#ifndef _SPINES_H
#define _SPINES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef _SPN_INLINE
#  if defined(__cplusplus)
#    define _SPN_INLINE inline
#  elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#    define _SPN_INLINE static inline
#  else
#    define _SPN_INLINE static inline
#  endif
#endif

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

typedef struct {
    spn_Context *cxt;
    size_t index;
} spn_Group;

/**
 * This is C, just remember to call this
 */
_SPN_INLINE void spn_destroy(spn_Context *cxt) {
    if (cxt->buffer) free(cxt->buffer);
    cxt->buffer = NULL;
}

/**
 * Minh lam tu xuong va da, ca phe, thuoc la va 250 lit do co con
 *
 * Con bao o trong cho ta dien vao?
 */
void spn_parse(spn_Context *cxt, const char *str_ptr, size_t str_len);

_SPN_INLINE spn_Field *spn_fields(spn_Group *gr) {
    return &gr->cxt->field_vals[gr->cxt->idents[gr->index].fields_begin];
}

/**
 * Retrieves the top-level group of the context
 */
_SPN_INLINE spn_Group spn_root(spn_Context *cxt) {
    return (spn_Group){cxt, 0};
}

/**
 * Mutates the group to a child-group
 */
void spn_move(spn_Group *gr, const char *dir);

/**
 * Mutates the group to an auto-indexed child-group
 */
void spn_move_id(spn_Group *gr, size_t id);

/**
 * Returns a new group from the child-group
 */
_SPN_INLINE spn_Group spn_find(spn_Group gr, const char *dir) {
    spn_move(&gr, dir);
    return gr;
}

/**
 * Returns a new group from the child-group
 */
_SPN_INLINE spn_Group spn_find_id(spn_Group gr, size_t id) {
    spn_move_id(&gr, id);
    return gr;
}

/**
 * Advances group to the next sibling group (in the same level)
 *
 * Returns true if successful, or false if the end is reached
 */
bool spn_step(spn_Group *gr);

/**
 * Returns the next sibling group (in the same level)
 */
_SPN_INLINE spn_Group spn_next(spn_Group gr) {
    spn_step(&gr);
    return gr;
}

/**
 * Advances group to the absolute next group
 *
 * Returns true if successful, or false if the end is reached
 */
bool spn_step_flat(spn_Group *gr);

/**
 * Returns the absolute next group
 */
_SPN_INLINE spn_Group spn_next_flat(spn_Group gr) {
    spn_step_flat(&gr);
    return gr;
}

/**
 * Returns the string field's data
 */
_SPN_INLINE const char *spn_str(spn_Context *cxt, spn_Field field) {
    return cxt->string_data + field.str_val.begin;
}

#endif
