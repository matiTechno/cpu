#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
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

ic_keyword _keywords[] = {
    "for",      TOK_FOR,
    "while",    TOK_WHILE,
    "if",       TOK_IF,
    "else",     TOK_ELSE,
    "return",   TOK_RETURN,
    "break",    TOK_BREAK,
    "continue", TOK_CONTINUE,
    "int",      TOK_INT,
    "float",    TOK_FLOAT,
    "void",     TOK_VOID,
    "struct",   TOK_STRUCT,
    "sizeof",   TOK_SIZEOF,
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
    char indirection;
    ic_struct* _struct;
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
    // compilation pass
    int size;
};

void ic_exit(int line, int col, const char* fmt, ...)
{
    printf("error: line %d, col %d: ", line, col);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    exit(1);
}

struct ic_lexer
{
    array<ic_token> tokens;
    int line;
    int col;
    char* token_begin;
    int token_line;
    int token_col;
    char* source_it;

    void init(char* source)
    {
        tokens.init();
        line = 1;
        col = 1;
        source_it = source;
    }

    void begin_new_token()
    {
        token_begin = source_it;
        token_line = line;
        token_col = col;
    }
    void add_token(ic_token_type token_type)
    {
        ic_token token;
        token.type = token_type;
        token.line = token_line;
        token.col = token_col;
        tokens.push_back(token);
    }

    void add_token(ic_token token)
    {
        token.line = token_line;
        token.col = token_col;
        tokens.push_back(token);
    }

    char advance()
    {
        assert(!end());
        char c = *source_it;
        ++source_it;

        if(c == '\n')
        {
            ++line;
            col = 1;
        }
        else
            ++col;
        return c;
    }

    bool try_consume(char c)
    {
        assert(c != '\0');

        if(*source_it != c)
            return false;
        advance();
        return true;
    }

    void consume(char c)
    {
        assert(c != '\0');

        if(*source_it != c)
            ic_exit(line, col, "expected '%c'", c);
        advance();
    }

    char peek()
    {
        return *source_it;
    }

    bool end()
    {
        return *source_it == '\0';
    }

    char* pos()
    {
        return source_it;
    }
};

bool digit_char(char c)
{
    return c >= '0' && c <= '9';
}

bool identifier_char(char c)
{
    return digit_char(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool ic_strcmp(ic_str lhs, ic_str rhs)
{
    if(lhs.len != rhs.len)
        return false;

    for(int i = 0; i < lhs.len; ++i)
    {
        if(lhs.data[i] != rhs.data[i])
            return false;
    }
    return true;
}

ic_token* lex(char* source)
{
    ic_lexer lexer;
    lexer.init(source);

    while(!lexer.end())
    {
        lexer.begin_new_token();
        char c = lexer.advance();

        switch(c)
        {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            break;
        case '(':
            lexer.add_token(TOK_LEFT_PAREN);
            break;
        case ')':
            lexer.add_token(TOK_RIGHT_PAREN);
            break;
        case '{':
            lexer.add_token(TOK_LEFT_BRACE);
            break;
        case '}':
            lexer.add_token(TOK_RIGHT_BRACE);
            break;
        case '[':
            lexer.add_token(TOK_LEFT_BRACKET);
            break;
        case ']':
            lexer.add_token(TOK_RIGHT_BRACKET);
            break;
        case ';':
            lexer.add_token(TOK_SEMICOLON);
            break;
        case '.':
            lexer.add_token(TOK_DOT);
            break;
        case ',':
            lexer.add_token(TOK_COMMA);
            break;
        case '=':
            lexer.add_token(lexer.try_consume('=') ? TOK_EQUAL_EQUAL : TOK_EQUAL);
            break;
        case '>':
            lexer.add_token(lexer.try_consume('=') ? TOK_GREATER_EQUAL : TOK_GREATER);
            break;
        case '<':
            lexer.add_token(lexer.try_consume('=') ? TOK_LESS_EQUAL : TOK_LESS);
            break;
        case '+':
            lexer.add_token(lexer.try_consume('=') ? TOK_PLUS_EQUAL : lexer.try_consume('+') ? TOK_PLUS_PLUS : TOK_PLUS);
            break;
        case '-':
            lexer.add_token(lexer.try_consume('=') ? TOK_MINUS_EQUAL : lexer.try_consume('-') ? TOK_MINUS_MINUS :
                lexer.try_consume('>') ? TOK_ARROW : TOK_MINUS);
            break;
        case '*':
            lexer.add_token(lexer.try_consume('=') ? TOK_STAR_EQUAL : TOK_STAR);
            break;
        case '!':
            lexer.add_token(lexer.try_consume('=') ? TOK_BANG_EQUAL : TOK_BANG);
            break;
        case '&':
            lexer.add_token(lexer.try_consume('=') ? TOK_AMP_AMP : TOK_AMP);
            break;
        case '|':
            lexer.consume('|');
            lexer.add_token(TOK_VBAR_VBAR);
            break;
        case '/':
            if(lexer.try_consume('/'))
            {
                while(!lexer.end() && lexer.advance() != '\n')
                    ;
            }
            else
                lexer.add_token(lexer.try_consume('=') ? TOK_SLASH_EQUAL : TOK_SLASH);
            break;
        default:
            if(digit_char(c))
            {
                ic_token token;
                token.type = TOK_INT_LITERAL;

                while(!lexer.end() && digit_char(lexer.peek()))
                    lexer.advance();

                if(lexer.try_consume('.'))
                    token.type = TOK_FLOAT_LITERAL;

                while(!lexer.end() && digit_char(lexer.peek()))
                    lexer.advance();

                int len = lexer.pos() - lexer.token_begin;
                char buf[1024];
                assert(len < sizeof(buf));
                memcpy(buf, lexer.token_begin, len);
                buf[len] = '\0';

                if(token.type == TOK_INT_LITERAL)
                    token.int_literal = atoi(buf);
                else
                    token.float_literal = atof(buf);

                lexer.add_token(token);
            }
            else if(identifier_char(c))
            {
                while(!lexer.end() && identifier_char(lexer.peek()))
                    lexer.advance();

                ic_token token;
                token.type = TOK_IDENTIFIER;
                token.str = {lexer.token_begin, (int)(lexer.pos() - lexer.token_begin)};

                for(ic_keyword keyword: _keywords)
                {
                    ic_str str_keyword = {keyword.str, (int)strlen(keyword.str)};

                    if(ic_strcmp(token.str, str_keyword))
                    {
                        token.type = keyword.token_type;
                        break;
                    }
                }
                lexer.add_token(token);
            }
            else
                ic_exit(lexer.token_line, lexer.token_col, "an unexpected character '%c'", c);
        }
    }
    lexer.begin_new_token();
    lexer.add_token({TOK_EOF});
    return lexer.tokens.transfer();
}

struct ic_parser
{
    ic_token* token_it;
    array<ic_struct>* structures;

    ic_token advance()
    {
        assert(token_it->type != TOK_EOF);
        ic_token token = *token_it;
        ++token_it;
        return token;
    }

    bool try_consume(ic_token_type type)
    {
        assert(type != TOK_EOF);

        if(token_it->type == type)
        {
            advance();
            return true;
        }
        return false;
    }

    ic_token consume(ic_token_type type)
    {
        assert(type != TOK_EOF);

        if(token_it->type != type)
            ic_exit(token_it->line, token_it->col, "an unexpected token");
        return advance();
    }

    ic_token peek_full()
    {
        return *token_it;
    }

    ic_token_type peek()
    {
        return token_it->type;
    }

    ic_token_type peek2()
    {
        if(end())
            return token_it->type;
        else
            return (token_it + 1)->type;
    }

    bool end()
    {
        return token_it->type == TOK_EOF;
    }
};

ic_parser parser;

ic_stmt* alloc_stmt(ic_stmt_type type, ic_token token)
{
    ic_stmt* stmt = (ic_stmt*)malloc(sizeof(ic_stmt));
    memset(stmt, 0, sizeof(ic_stmt));
    stmt->type = type;
    stmt->token = token;
    return stmt;
}

ic_expr* alloc_expr(ic_expr_type type, ic_token token)
{
    ic_expr* expr = (ic_expr*)malloc(sizeof(ic_expr));
    memset(expr, 0, sizeof(ic_expr));
    expr->type = type;
    expr->token = token;
    return expr;
}

// peek_type() and produce_type() return false on a void type

bool peek_type()
{
    ic_token token = parser.peek_full();

    if(token.type == TOK_VOID && parser.peek2() == TOK_STAR)
        return true;

    switch(token.type)
    {
    case TOK_INT:
    case TOK_FLOAT:
        return true;
    case TOK_IDENTIFIER:
        for(ic_struct& _struct: *parser.structures)
        {
            if(ic_strcmp(_struct.id_token.str, token.str))
                return true;
        }
    }
    return false;
}

ic_type produce_type()
{
    if(!peek_type())
        ic_exit(parser.token_it->line, parser.token_it->col, "expected a type");

    ic_token token = parser.advance();
    ic_type type;
    type.indirection = 0;

    switch(token.type)
    {
    case TOK_INT:
        type.basic_type = TYPE_INT;
        break;
    case TOK_FLOAT:
        type.basic_type = TYPE_FLOAT;
        break;
    case TOK_VOID:
        type.basic_type = TYPE_VOID;
        break;
    case TOK_IDENTIFIER:
        type.basic_type = TYPE_STRUCT;

        for(ic_struct& _struct: *parser.structures)
        {
            if(ic_strcmp(_struct.id_token.str, token.str))
            {
                type._struct = &_struct;
                break;
            }
        }
        break;
    default:
        assert(false);
    }

    while(parser.try_consume(TOK_STAR))
        ++type.indirection;
    return type;
}

ic_expr* produce_expr();

ic_expr* produce_expr_primary()
{

}

ic_expr* produce_expr_access()
{

}

ic_expr* produce_expr_unary()
{
    switch(parser.peek())
    {
    case TOK_BANG:
    case TOK_MINUS:
    case TOK_PLUS_PLUS:
    case TOK_MINUS_MINUS:
    case TOK_AMP:
    case TOK_STAR:
    {
        ic_expr* expr = alloc_expr(EXPR_UNARY, parser.advance());
        expr->sub_expr = produce_expr_unary();
        return expr;
    }
    case TOK_LEFT_PAREN:
    {
        ic_expr* expr = alloc_expr(EXPR_CALL, parser.advance());

        if(!peek_type())
        {
            --parser.token_it; // hack, there is no peek2_type()
            break;
        }
        expr->cast.type = produce_type();
        parser.consume(TOK_RIGHT_PAREN);
        expr->cast.expr = produce_expr_unary();
        return expr;
    }
    case TOK_SIZEOF:
    {
        ic_expr* expr = alloc_expr(EXPR_SIZEOF, parser.advance());
        parser.consume(TOK_LEFT_PAREN);
        expr->sizeof_type = produce_type();
        parser.consume(TOK_RIGHT_PAREN);
        return expr;
    }
    }
    return produce_expr_access();
}

enum ic_precedence
{
    PREC_LOGICAL_OR,
    PREC_LOGICAL_AND,
    PREC_COMPARE_EQUAL,
    PREC_COMPARE_GREATER,
    PREC_ADD,
    PREC_MULTIPLY,
    PREC_UNARY,
};

ic_expr* produce_expr_binary(ic_precedence precedence)
{
    ic_token_type token_types[5] = {}; // note, initialize all to TOK_EOF
    assert(TOK_EOF == 0);

    switch(precedence)
    {
    case PREC_LOGICAL_OR:
        token_types[0] = TOK_VBAR_VBAR;
        break;
    case PREC_LOGICAL_AND:
        token_types[0] = TOK_AMP_AMP;
        break;
    case PREC_COMPARE_EQUAL:
        token_types[0] = TOK_EQUAL_EQUAL;
        token_types[1] = TOK_BANG_EQUAL;
        break;
    case PREC_COMPARE_GREATER:
        token_types[0] = TOK_GREATER;
        token_types[1] = TOK_GREATER_EQUAL;
        token_types[2] = TOK_LESS;
        token_types[3] = TOK_LESS_EQUAL;
        break;
    case PREC_ADD:
        token_types[0] = TOK_PLUS;
        token_types[1] = TOK_MINUS;
        break;
    case PREC_MULTIPLY:
        token_types[0] = TOK_STAR;
        token_types[0] = TOK_SLASH;
        break;
    case PREC_UNARY:
        return produce_expr_unary();
    }

    ic_expr* lhs = produce_expr_binary((ic_precedence)(precedence + 1));

    for(;;)
    {
        ic_token_type* it = token_types;

        while(*it != TOK_EOF)
        {
            if(*it == parser.peek())
                break;
            ++it;
        }

        if(*it == TOK_EOF)
            break;

        ic_expr* expr = alloc_expr(EXPR_BINARY, parser.advance());
        expr->binary.lhs = lhs;
        expr->binary.rhs = produce_expr_binary((ic_precedence)(precedence + 1));
        lhs = expr;
    }
    return lhs;
}

ic_expr* produce_expr_assignment()
{
    ic_expr* lhs = produce_expr_binary(PREC_LOGICAL_OR);

    switch(parser.peek())
    {
    case TOK_EQUAL:
    case TOK_PLUS_EQUAL:
    case TOK_MINUS_EQUAL:
    case TOK_STAR_EQUAL:
    case TOK_SLASH_EQUAL:
        ic_expr* expr = alloc_expr(EXPR_BINARY, parser.advance());
        expr->binary.lhs = lhs;
        expr->binary.rhs = produce_expr();
        return expr;
    }
    return lhs;
}

ic_expr* produce_expr()
{
    return produce_expr_assignment();
}

ic_stmt* produce_stmt_var_decl()
{
    if(peek_type())
    {
        ic_stmt* stmt = alloc_stmt(STMT_VAR_DECL, parser.peek_full());
        stmt->var_decl.type = produce_type();
        stmt->var_decl.id_token = parser.consume(TOK_IDENTIFIER);

        if(parser.try_consume(TOK_EQUAL))
            stmt->var_decl.init_expr = produce_expr();
        parser.consume(TOK_SEMICOLON);
        return stmt;
    }

    ic_stmt* stmt = alloc_stmt(STMT_EXPR, parser.peek_full());

    if(!parser.try_consume(TOK_SEMICOLON))
    {
        stmt->expr = produce_expr();
        parser.consume(TOK_SEMICOLON);
    }
    return stmt;
}

#define IC_SAME_SCOPE 0

ic_stmt* produce_stmt(bool push_scope = true)
{
    switch(parser.peek())
    {
    case TOK_LEFT_BRACE:
    {
        ic_stmt* stmt = alloc_stmt(STMT_COMPOUND, parser.advance());
        stmt->compound.push_scope = push_scope;
        ic_stmt** tail = &stmt->compound.body;

        while(!parser.try_consume(TOK_RIGHT_BRACE))
        {
            *tail = produce_stmt();
            tail = &(*tail)->next;
        }
        return stmt;
    }
    case TOK_WHILE:
    {
        ic_stmt* stmt = alloc_stmt(STMT_FOR, parser.advance());
        parser.consume(TOK_LEFT_PAREN);
        stmt->_for.header2 = produce_expr();
        parser.consume(TOK_RIGHT_PAREN);
        stmt->_for.body = produce_stmt(IC_SAME_SCOPE);
        return stmt;
    }
    case TOK_FOR:
    {
        ic_stmt* stmt = alloc_stmt(STMT_FOR, parser.advance());
        parser.consume(TOK_LEFT_PAREN);
        stmt->_for.header1 = produce_stmt_var_decl();

        if(!parser.try_consume(TOK_SEMICOLON))
        {
            stmt->_for.header2 = produce_expr();
            parser.consume(TOK_SEMICOLON);
            ic_token token = stmt->_for.header2->token;

            if(token.type == TOK_EQUAL)
                ic_exit(token.line, token.col, "condition can't be an assignment");
        }
        if(!parser.try_consume(TOK_RIGHT_PAREN))
            stmt->_for.header3 = produce_expr();
        parser.consume(TOK_RIGHT_PAREN);
        produce_stmt(IC_SAME_SCOPE);
        return stmt;
    }
    case TOK_IF:
    {
        ic_stmt* stmt = alloc_stmt(STMT_IF, parser.advance());
        parser.consume(TOK_LEFT_PAREN);
        stmt->_if.header = produce_expr();
        {
            ic_token token = stmt->_if.header->token;
            if(token.type == TOK_EQUAL)
                ic_exit(token.line, token.col, "condition can't be an assignment");
        }
        parser.consume(TOK_RIGHT_PAREN);
        stmt->_if.body_if = produce_stmt();

        if(parser.try_consume(TOK_ELSE))
            stmt->_if.body_else = produce_stmt();
        return stmt;
    }
    case TOK_RETURN:
    {
        ic_stmt* stmt = alloc_stmt(STMT_RETURN, parser.advance());
        stmt->expr = produce_expr();
        parser.consume(TOK_SEMICOLON);
        return stmt;
    }
    case TOK_BREAK:
    {
        ic_stmt* stmt = alloc_stmt(STMT_BREAK, parser.advance());
        parser.consume(TOK_SEMICOLON);
        return stmt;
    }
    case TOK_CONTINUE:
    {
        ic_stmt* stmt = alloc_stmt(STMT_CONTINUE, parser.advance());
        parser.consume(TOK_SEMICOLON);
        return stmt;
    }
    } // switch
    return produce_stmt_var_decl();
}

ic_function produce_function()
{
    ic_function fun;

    if(peek_type())
        fun.return_type = produce_type();
    else
    {
        parser.consume(TOK_VOID);
        fun.return_type.basic_type = TYPE_VOID;
        fun.return_type.indirection = 0;
    }

    fun.id_token = parser.consume(TOK_IDENTIFIER);
    parser.consume(TOK_LEFT_PAREN);
    array<ic_param> params;
    params.init();

    while(!parser.try_consume(TOK_RIGHT_PAREN))
    {
        if(params.size)
            parser.consume(TOK_COMMA);

        ic_param param;
        param.type = produce_type();
        param.id_token = parser.consume(TOK_IDENTIFIER);
    }

    fun.params_size = params.size;
    fun.params = params.transfer();
    fun.body = produce_stmt(IC_SAME_SCOPE);

    if(fun.body->type != STMT_COMPOUND)
        ic_exit(fun.body->token.line, fun.body->token.col, "expepcted a compound statement");
    return fun;
}

ic_struct produce_struct()
{
    ic_struct _struct;
    _struct.id_token = parser.consume(TOK_IDENTIFIER);
    _struct.defined = false;

    if(parser.try_consume(TOK_SEMICOLON))
        return _struct;

    parser.consume(TOK_LEFT_BRACE);
    _struct.defined = true;
    array<ic_param> members;
    members.init();

    while(!parser.try_consume(TOK_RIGHT_BRACE))
    {
        ic_param member;
        member.type = produce_type();
        member.id_token = parser.consume(TOK_IDENTIFIER);
        parser.consume(TOK_SEMICOLON);
        members.push_back(member);
    }
    parser.consume(TOK_SEMICOLON);

    _struct.members_size = members.size;
    _struct.members = members.transfer();
    return _struct;
}

void parse(ic_token* _tokens, array<ic_function>& functions, array<ic_struct>& structures)
{
    parser.token_it = _tokens;
    parser.structures = &structures;

    while(!parser.end())
    {
        if(parser.try_consume(TOK_STRUCT))
        {
            ic_struct new_struct = produce_struct();
            bool done = false;

            for(ic_struct& _struct: structures)
            {
                if(ic_strcmp(_struct.id_token.str, new_struct.id_token.str))
                {
                    if(_struct.defined && new_struct.defined)
                        ic_exit(new_struct.id_token.line, new_struct.id_token.col, "struct redefinition");
                    else if(new_struct.defined)
                        _struct = new_struct;

                    done = true;
                    break;
                }
            }
            if(!done)
                structures.push_back(new_struct);
        }
        else
        {
            ic_function new_fun = produce_function();

            for(ic_function& fun: functions)
            {
                if(ic_strcmp(fun.id_token.str, new_fun.id_token.str))
                    ic_exit(new_fun.id_token.line, new_fun.id_token.col, "function redefinition");
            }
            functions.push_back(new_fun);
        }
    }
}

#define IC_MAX_STRUCT 100

int main(int argc, const char** argv)
{
    if(argc != 2)
    {
        printf("expected a file name argument\n");
        return 1;
    }

    FILE* file = fopen(argv[1], "rb");

    if(!file)
    {
        printf("could not open a file\n");
        return 1;
    }

    int rc = fseek(file, 0, SEEK_END);
    assert(!rc);
    int file_size = ftell(file);
    assert(file_size != -1);
    rewind(file);
    char* file_data = (char*)malloc(file_size + 1);
    fread(file_data, 1, file_size, file);
    fclose(file);
    file_data[file_size] = '\0';
    ic_token* tokens = lex(file_data);
    array<ic_function> functions;
    array<ic_struct> structures;
    functions.init();
    structures.init();
    structures.resize(IC_MAX_STRUCT);
    structures.size = 0;
    parse(tokens, functions, structures);
    assert(structures.size <= IC_MAX_STRUCT); // pointers to structures data must not be invalidated, see ic_type
    // compilation, code gen
    return 0;
}
