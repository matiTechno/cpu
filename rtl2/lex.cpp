#include "ic.h"

static ic_keyword _keywords[] = {
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
                assert(len < (int)sizeof(buf));
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
