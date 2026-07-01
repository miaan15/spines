#ifndef _SPINES_H
#define _SPINES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef _SPN_INLINE
#   if defined(__cplusplus)
#       define _SPN_INLINE inline
#   else
#       define _SPN_INLINE static inline
#   endif
#endif

#ifndef SPN_DISABLE_ERROR
extern char _spn_err_buffer[128];
#define _SPN_SET_ERROR(format, ...) \
    snprintf(_spn_err_buffer, sizeof(_spn_err_buffer), format, __VA_ARGS__)
_SPN_INLINE const char *spn_error(void) { return _spn_err_buffer; }
#else
#define _SPN_SET_ERROR(format, ...) ((void)0)
_SPN_INLINE const char *spn_error(void) { return ""; }
#endif

typedef enum {
    TOKEN_IDENT = 0,
    TOKEN_ID_IDENT,
    TOKEN_NUM,
    TOKEN_STR,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COLON,
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
} spn_Mark;

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

_SPN_INLINE spn_Field *spn_fields(spn_Mark *gr) {
    return &gr->cxt->field_vals[gr->cxt->idents[gr->index].fields_begin];
}

/**
 * Retrieves the top-level mark of the context
 */
_SPN_INLINE spn_Mark spn_root(spn_Context *cxt) {
    return (spn_Mark){cxt, 0};
}

/**
 * Mutates the mark to a child-mark
 *
 * Do nothing if error
 */
void spn_move(spn_Mark *gr, const char *dir);

/**
 * Mutates the mark to an auto-indexed child-mark
 *
 * Do nothing if error
 */
void spn_move_id(spn_Mark *gr, size_t id);

/**
 * Returns a new mark from the child-mark
 *
 * Return the same as original mark if error
 */
_SPN_INLINE spn_Mark spn_find(spn_Mark gr, const char *dir) {
    spn_move(&gr, dir);
    return gr;
}

/**
 * Returns a new mark from the child-mark
 *
 * Return the same as original mark if error
 */
_SPN_INLINE spn_Mark spn_find_id(spn_Mark gr, size_t id) {
    spn_move_id(&gr, id);
    return gr;
}

/**
 * Advances mark to the next sibling mark (in the same level)
 *
 * Returns false and do nothing if the end is reached
 */
bool spn_step(spn_Mark *gr);

/**
 * Returns the next sibling mark (in the same level)
 *
 * Return the same as original mark if error
 */
_SPN_INLINE spn_Mark spn_next(spn_Mark gr) {
    spn_step(&gr);
    return gr;
}

/**
 * Advances mark to the absolute next mark
 *
 * Returns false and do nothing if the end is reached
 */
bool spn_step_flat(spn_Mark *gr);

/**
 * Returns the absolute next mark
 *
 * Return the same as original mark if error
 */
_SPN_INLINE spn_Mark spn_next_flat(spn_Mark gr) {
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
