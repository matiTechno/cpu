#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "array.h"

#define ASM_ROM_SIZE 255

#define ASM_LR 31
#define ASM_SP 30
#define ASM_FP 29

#define ASM_S16 0
#define ASM_U16 1

enum asm_token_type
{
    TOK_EOF,
    TOK_LF,
    TOK_COMMA,
    TOK_COLON,
    TOK_PAREN_LEFT,
    TOK_PAREN_RIGHT,
    TOK_IDENTIFIER,
    TOK_INT_LITERAL,

    TOK_REG,

    TOK_ADD,
    TOK_SUB,
    TOK_AND,
    TOK_OR,
    TOK_XOR,
    TOK_NOR,
    TOK_SLL,
    TOK_SRL,
    TOK_SRA,
    TOK_MUL,
    TOK_DIV,
    TOK_SLT,
    TOK_SLTU,

    TOK_BEQ,
    TOK_BNE,

    TOK_B,

    TOK_LDR,
    TOK_STR,

    TOK_ADDI,
    TOK_ANDI,
    TOK_ORI,
    TOK_XORI,
    TOK_SLLI,
    TOK_SRLI,
    TOK_SRAI,
    TOK_MULI,
    TOK_DIVI,
    TOK_SLTI,
    TOK_SLTIU,

    TOK_BL,
    TOK_BX,

    TOK_LR,
    TOK_SP,
    TOK_FP,
};

struct asm_keyword
{
    const char* str;
    asm_token_type token_type;
};

static asm_keyword _keywords[] = {
    "add",  TOK_ADD,
    "sub",  TOK_SUB,
    "and",  TOK_AND,
    "or",   TOK_OR,
    "xor",  TOK_XOR,
    "nor",  TOK_NOR,
    "sll",  TOK_SLL,
    "srl",  TOK_SRL,
    "sra",  TOK_SRA,
    "mul",  TOK_MUL,
    "div",  TOK_DIV,
    "slt",  TOK_SLT,
    "sltu", TOK_SLTU,
    "beq",  TOK_BEQ,
    "bne",  TOK_BNE,
    "b",    TOK_B,
    "ldr",  TOK_LDR,
    "str",  TOK_STR,
    "addi", TOK_ADDI,
    "andi", TOK_ANDI,
    "ori",  TOK_ORI,
    "xori", TOK_XORI,
    "slli", TOK_SLLI,
    "srli", TOK_SRLI,
    "srai", TOK_SRAI,
    "muli", TOK_MULI,
    "divi", TOK_DIVI,
    "slti", TOK_SLTI,
    "sltiu",TOK_SLTIU,
    "bl",   TOK_BL,
    "bx",   TOK_BX,
    "lr",   TOK_LR,
    "sp",   TOK_SP,
    "fp",   TOK_FP,
};

struct asm_str
{
    const char* data;
    int len;
};

struct asm_token
{
    asm_token_type type;
    int line;
    int col;

    union
    {
        int reg_id;
        int int_literal;
        asm_str identifier_str;
    };
};

struct asm_label
{
    asm_str str;
    int addr;
};

struct asm_instr
{
    int opcode;
    int reg1;
    int reg2;
    union
    {
        struct
        {
            int reg3;
            int rtype_opcode;
        };
        int immediate;
    };
};

struct asm_lexer
{
    array<asm_token> tokens;
    int line;
    int col;
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

    void add_token(asm_token token)
    {
        token.line = token_line;
        token.col = token_col;
        tokens.push_back(token);
    }

    bool end()
    {
        return *source_it == '\0';
    }

    char* pos()
    {
        return source_it;
    }

    char peek()
    {
        return *source_it;
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
        if(end())
            return false;
        if(*source_it != c)
            return false;
        advance();
        return true;
    }

    void consume(char c)
    {
        if(end() || *source_it != c)
        {
            printf("error; line %d, col %d; expected '%c'\n", line, col, c);
            exit(1);
        }
        advance();
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

bool asm_strcmp(asm_str lhs, asm_str rhs)
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

asm_token* lex(char* source)
{
    asm_lexer lexer;
    lexer.init(source);

    while(!lexer.end())
    {
        lexer.token_line = lexer.line;
        lexer.token_col = lexer.col;
        char* token_begin = lexer.pos();
        char c = lexer.advance();

        switch(c)
        {
        case ' ':
        case '\t':
        case '\r':
            break;
        case '\n':
            if(lexer.tokens.size && lexer.tokens.back().type != TOK_LF)
                lexer.add_token({TOK_LF});
            break;
        case ':':
            lexer.add_token({TOK_COLON});
            break;
        case ',':
            lexer.add_token({TOK_COMMA});
            break;
        case '(':
            lexer.add_token({TOK_PAREN_LEFT});
            break;
        case ')':
            lexer.add_token({TOK_PAREN_RIGHT});
            break;
        case '/':
            lexer.consume('/');
            while(!lexer.end() && lexer.peek() != '\n')
                lexer.advance();
            break;
        default:
        {
            if(digit_char(c) || c == '-')
            {
                while(!lexer.end() && digit_char(lexer.peek()))
                    lexer.advance();

                int len = lexer.pos() - token_begin;
                char buf[1024];
                assert(len < sizeof(buf));
                memcpy(buf, token_begin, len);
                buf[len] = '\0';
                asm_token tok;
                tok.type = TOK_INT_LITERAL;
                tok.int_literal = atoi(buf);
                lexer.add_token(tok);
                break;
            }

            if(c == 'r')
            {
                while(!lexer.end() && digit_char(lexer.peek()))
                    lexer.advance();

                char* id_begin = token_begin + 1;
                int len = lexer.pos() - id_begin;

                if(len)
                {
                    char buf[1024];
                    assert(len < sizeof(buf));
                    memcpy(buf, id_begin, len);
                    buf[len] = '\0';
                    int id = atoi(buf);

                    if(id >= 0 && id <= 31)
                    {
                        asm_token tok;
                        tok.type = TOK_REG;
                        tok.reg_id = id;
                        lexer.add_token(tok);
                        break;
                    }
                }
            }

            if(identifier_char(c))
            {
                while(!lexer.end() && identifier_char(lexer.peek()))
                    lexer.advance();

                asm_str str;
                str.data = token_begin;
                str.len = lexer.pos() - token_begin;

                asm_token_type token_type = TOK_EOF;

                for(asm_keyword keyword: _keywords)
                {
                    asm_str str_keyword = {keyword.str, int(strlen(keyword.str))};

                    if(asm_strcmp(str, str_keyword))
                    {
                        token_type = keyword.token_type;
                        break;
                    }
                }

                if(token_type != TOK_EOF)
                {
                    asm_token tok;
                    tok.type = token_type;

                    // resolve register aliases
                    switch(token_type)
                    {
                    case TOK_LR:
                        tok.type = TOK_REG;
                        tok.reg_id = ASM_LR;
                        break;
                    case TOK_SP:
                        tok.type = TOK_REG;
                        tok.reg_id = ASM_SP;
                        break;
                    case TOK_FP:
                        tok.type = TOK_REG;
                        tok.reg_id = ASM_FP;
                        break;
                    }

                    lexer.add_token(tok);
                    break;
                }

                asm_token tok;
                tok.type = TOK_IDENTIFIER;
                tok.identifier_str = str;
                lexer.add_token(tok);
                break;
            }

            printf("error; line %d, col %d; an unexpected character '%c'\n", lexer.token_line, lexer.token_col, c);
            exit(1);
        }
        } // switch
    }
    asm_token tok;
    tok.type = TOK_EOF;
    tok.line = lexer.line;
    tok.col = lexer.col;
    lexer.tokens.push_back(tok);
    return lexer.tokens.transfer();
}

// error checking is deffered to generate_code()

array<asm_label> collect_labels(asm_token* token)
{
    int addr = 0;
    array<asm_label> labels;
    labels.init();

    while(token->type != TOK_EOF)
    {
        if(token->type == TOK_IDENTIFIER && (token + 1)->type == TOK_COLON)
        {
            labels.push_back({token->identifier_str, addr});
            token += 3; // skip identifier, colon, newline
        }
        else
        {
            while(token->type != TOK_LF && token->type != TOK_EOF)
                ++token;
            ++token; // skip newline
            ++addr;
        }
    }
    return labels;
}

asm_token consume(asm_token*& it, asm_token_type type, const char* err_msg)
{
    if(it->type != type)
    {
        printf("error; line %d, col %d; %s\n", it->line, it->col, err_msg);
        exit(1);
    }
    asm_token token = *it;
    ++it;
    return token;
}

int consume_reg(asm_token*& it)
{
    return consume(it, TOK_REG, "expected a register name").reg_id;
}

int consume_int_literal(asm_token*& it, int type)
{
    asm_token token = consume(it, TOK_INT_LITERAL, "expected an integer literal");
    bool error;

    if(type == ASM_S16)
        error = token.int_literal >= (1 << 15) || token.int_literal < -(1 << 15);
    else
    {
        assert(type == ASM_U16);
        error = token.int_literal > (1 << 16) - 1;
    }

    if(error)
    {
        printf("error; line %d, col %d; integer literal does not fit into 16 bits\n", token.line, token.col);
        exit(1);
    }
    return token.int_literal;
}

void consume_comma(asm_token*& it)
{
    consume(it, TOK_COMMA, "expected ','");
}

int consume_label_identifier(asm_token*& it, array<asm_label> labels)
{
    asm_token token = consume(it, TOK_IDENTIFIER, "expected a label identifier");

    for(asm_label label: labels)
    {
        if(asm_strcmp(token.identifier_str, label.str))
            return label.addr;
    }
    printf("error; line %d, col %d; could not resolve a label identifier\n", token.line, token.col);
    exit(1);
    return {};
}

// read architecture.txt for more information on instruction encoding

array<unsigned int> generate_code(asm_token* token, array<asm_label> labels)
{
    array<asm_instr> instructions;
    instructions.init();

    while(token->type != TOK_EOF)
    {
        asm_token_type type = token->type;
        asm_instr instr;
        memset(&instr, 0, sizeof(instr)); // cosmetic change for debugging

        if(type == TOK_IDENTIFIER && (token + 1)->type == TOK_COLON) // skip label
        {
            token += 2;
            consume(token, TOK_LF, "expected a newline");
            continue;
        }
        else if(type >= TOK_ADD && type < TOK_BEQ)
        {
            instr.opcode = 0;
            instr.rtype_opcode = type - TOK_ADD;
            ++token;
            instr.reg3 = consume_reg(token);
            consume_comma(token);
            instr.reg1 = consume_reg(token);
            consume_comma(token);
            instr.reg2 = consume_reg(token);
        }
        else if(type == TOK_BEQ || type == TOK_BNE)
        {
            instr.opcode = 1 + (type - TOK_BEQ);
            ++token;
            instr.reg1 = consume_reg(token);
            consume_comma(token);
            instr.reg2 = consume_reg(token);
            consume_comma(token);
            instr.immediate = consume_label_identifier(token, labels);
        }
        else if(type == TOK_B)
        {
            instr.opcode = 3;
            ++token;
            instr.immediate = consume_label_identifier(token, labels);
        }
        else if(type == TOK_LDR || type == TOK_STR)
        {
            instr.opcode = 4 + (type - TOK_LDR);
            ++token;
            instr.reg2 = consume_reg(token);
            consume_comma(token);
            instr.immediate = consume_int_literal(token, ASM_S16);
            consume(token, TOK_PAREN_LEFT, "expected '('");
            instr.reg1 = consume_reg(token);
            consume(token, TOK_PAREN_RIGHT, "expected ')'");
        }
        else if(type >= TOK_ADDI && type < TOK_BL)
        {
            instr.opcode = 6 + (type - TOK_ADDI);
            ++token;
            instr.reg2 = consume_reg(token);
            consume_comma(token);
            instr.reg1 = consume_reg(token);
            consume_comma(token);

            if(type == TOK_ADDI || type == TOK_MULI || type == TOK_DIVI || type == TOK_SLTI)
                instr.immediate = consume_int_literal(token, ASM_S16);
            else
                instr.immediate = consume_int_literal(token, ASM_U16);
        }
        else if(type == TOK_BL)
        {
            instr.opcode = 17;
            ++token;
            instr.immediate = consume_label_identifier(token, labels);
            instr.reg2 = ASM_LR;
        }
        else if(type == TOK_BX)
        {
            instr.opcode = 18;
            ++token;
            instr.reg1 = consume_reg(token);
        }
        else
        {
            printf("error; line %d, col %d; an unexpected token\n", token->line, token->col);
            exit(1);
        }
        consume(token, TOK_LF, "expected a newline");
        instructions.push_back(instr);
    }

    assert(sizeof(unsigned int) == 4);
    array<unsigned int> code;
    code.init();

    for(asm_instr instr: instructions)
    {
        unsigned int word = instr.opcode << 26;
        word = word | (instr.reg1 << 21);
        word = word | (instr.reg2 << 16);

        if(!instr.opcode)
        {
            word = word | (instr.reg3 << 11);
            word = word | (instr.rtype_opcode << 5);
        }
        else
            word = word | (instr.immediate & 0xffff);

        code.push_back(word);
    }
    return code;
}

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
    asm_token* tokens = lex(file_data);
    array<asm_label> labels = collect_labels(tokens);
    array<unsigned int> code = generate_code(tokens, labels);

    if(code.size > ASM_ROM_SIZE) // rom size
    {
        printf("error; code (%d words) does not fit into %d-word rom image\n", code.size, ASM_ROM_SIZE);
        return 1;
    }

    file = fopen("a.hex", "wb");

    if(!file)
    {
        printf("could not create output file (a.hex)\n");
        exit(1);
    }

    for(int i = 0; i < code.size; ++i)
    {
        int rc = fprintf(file, "%x\n", code[i]);
        if(rc < 1)
        {
            printf("fprintf() error\n");
            exit(1);
        }
    }
    fclose(file);
    return 0;
}
