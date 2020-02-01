#pragma once
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "array.h"

enum ic_token_type
{
    TOK_EOF,
    TOK_IDENTIFIER,

    TOK_FOR,
    TOK_WHILE,
    TOK_IF,
    TOK_ELSE,
    TOK_RETURN,
    TOK_BREAK,
    TOK_CONTINUE,
    TOK_INT,
    TOK_FLOAT,
    TOK_VOID,
    TOK_STRUCT,
    TOK_SIZEOF,

    TOK_INT_LITERAL,
    TOK_FLOAT_LITERAL,

    TOK_LEFT_PAREN,
    TOK_RIGHT_PAREN,
    TOK_LEFT_BRACE,
    TOK_RIGHT_BRACE,
    TOK_LEFT_BRACKET,
    TOK_RIGHT_BRACKET,
    TOK_SEMICOLON,
    TOK_DOT,
    TOK_COMMA,

    TOK_EQUAL,
    TOK_GREATER,
    TOK_LESS,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_BANG,
    TOK_AMP,

    TOK_EQUAL_EQUAL,
    TOK_GREATER_EQUAL,
    TOK_LESS_EQUAL,
    TOK_PLUS_EQUAL,
    TOK_PLUS_PLUS,
    TOK_MINUS_EQUAL,
    TOK_MINUS_MINUS,
    TOK_ARROW,
    TOK_STAR_EQUAL,
    TOK_SLASH_EQUAL,
    TOK_BANG_EQUAL,
    TOK_AMP_AMP,
    TOK_VBAR_VBAR,
};

struct ic_keyword
{
    const char* str;
    ic_token_type token_type;
};

enum ic_basic_type
{
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_VOID,
    TYPE_STRUCT,
};

enum ic_stmt_type
{
    STMT_COMPOUND,
    STMT_FOR,
    STMT_IF,
    STMT_VAR_DECL,
    STMT_RETURN,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_EXPR,
};

enum ic_expr_type
{
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_SIZEOF,
    EXPR_CAST,
    EXPR_SUBSCRIPT,
    EXPR_MEMBER_ACCESS,
    EXPR_PARENS,
    EXPR_CALL,
    EXPR_PRIMARY,
};

struct ic_str
{
    const char* data;
    int len;
};

struct ic_token
{
    ic_token_type type;
    int line;
    int col;

    union
    {
        int int_literal;
        float float_literal;
        ic_str str;
    };
};

struct ic_struct;

struct ic_type
{
    ic_basic_type basic_type;
    int struct_id;
    int indirection;
};

struct ic_expr;

struct ic_stmt
{
    ic_stmt_type type;
    ic_token token;
    ic_stmt* next;

    union
    {
        struct
        {
            bool push_scope;
            ic_stmt* body;
        } compound;

        struct
        {
            ic_stmt* header1;
            ic_expr* header2;
            ic_expr* header3;
            ic_stmt* body;
        } _for;

        struct
        {
            ic_expr* header;
            ic_stmt* body_if;
            ic_stmt* body_else;
        } _if;

        struct
        {
            ic_type type;
            ic_token id_token;
            ic_expr* init_expr;
        } var_decl;

        ic_expr* expr;
    };
};

struct ic_expr
{
    ic_expr_type type;
    ic_token token;
    ic_expr* next;

    union
    {
        struct
        {
            ic_expr* lhs;
            ic_expr* rhs;
        } binary;

        struct
        {
            ic_type type;
            ic_expr* expr;
        } cast;

        struct
        {
           ic_expr* lhs;
           ic_expr* rhs;
        } subscript;

        struct
        {
            ic_expr* lhs;
            ic_token rhs_token;
        } member_access;
    };

    ic_expr* sub_expr;
    ic_type sizeof_type;
};

struct ic_param
{
    ic_type type;
    ic_token id_token;
};

struct ic_function
{
    ic_type return_type;
    ic_token id_token;
    ic_param* params;
    int params_size;
    ic_stmt* body;
};

struct ic_struct
{
    ic_token id_token;
    ic_param* members;
    int members_size;
    bool defined;
};

void ic_exit(int line, int col, const char* fmt, ...);
bool ic_strcmp(ic_str lhs, ic_str rhs);

ic_token* lex(char* source);

void parse(ic_token* _tokens, array<ic_function>& functions, array<ic_struct>& structures);
