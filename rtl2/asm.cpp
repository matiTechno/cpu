#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

// todo:
// mov, neg, push, pop pseudo-instructions; this will require a change in a label processing

template<typename T>
struct asm_array
{
    int size;
    int capacity;
    T* buf;

    void init()
    {
        size = 0;
        capacity = 0;
        buf = nullptr;
        // give it a start
        resize(20);
        size = 0;
    }

    T* transfer()
    {
        T* temp = buf;
        buf = nullptr;
        return temp;
    }

    void free()
    {
        ::free(buf);
    }

    void clear()
    {
        size = 0;
    }

    void resize(int new_size)
    {
        size = new_size;
        _maybe_grow();
    }

    void push_back()
    {
        size += 1;
        _maybe_grow();
    }

    void push_back(T t)
    {
        size += 1;
        _maybe_grow();
        back() = t;
    }

    void pop_back()
    {
        size -= 1;
    }

    T& back()
    {
        return *(buf + size - 1);
    }

    T* begin()
    {
        return buf;
    }

    T* end()
    {
        return buf + size;
    }

    T& operator[](int i)
    {
        return buf[i];
    }

    void _maybe_grow()
    {
        if (size > capacity)
        {
            capacity = size * 2;
            buf = (T*)realloc(buf, capacity * sizeof(T));
            assert(buf);
        }
    }
};

#define ASM_ROM_SIZE 255
#define ASM_LR 31
#define ASM_SP 30
#define ASM_FP 29

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

    TOK_BEQ,
    TOK_BNE,
    TOK_BLT,
    TOK_BLTU,
    TOK_BLE,
    TOK_BLEU,
    TOK_BGT,
    TOK_BGTU,
    TOK_BGE,
    TOK_BGEU,

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
    "beq",  TOK_BEQ,
    "bne",  TOK_BNE,
    "blt",  TOK_BLT,
    "bltu", TOK_BLTU,
    "ble",  TOK_BLE,
    "bleu", TOK_BLEU,
    "bgt",  TOK_BGT,
    "bgtu", TOK_BGTU,
    "bge",  TOK_BGE,
    "bgeu", TOK_BGEU,
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
    asm_array<asm_token> tokens;
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
        return source_it - 1;
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
        char c = lexer.advance();
        char* token_begin = lexer.pos();

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

                int len = (lexer.pos() + 1) - token_begin;
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
                int len = (lexer.pos() + 1) - id_begin;

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
                str.len = (lexer.pos() + 1) - token_begin;

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

asm_array<asm_label> collect_labels(asm_token* token)
{
    int addr = 0;
    asm_array<asm_label> labels;
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

void consume(asm_token*& token, asm_token_type type, const char* err_msg)
{
    if(token->type != type)
    {
        printf("error; line %d, col %d; %s\n", token->line, token->col, err_msg);
        exit(1);
    }
    ++token;
}

void consume_reg(asm_token*& token)
{
    consume(token, TOK_REG, "expected a register name");
}

void consume_comma(asm_token*& token)
{
    consume(token, TOK_COMMA, "expected ','");
}

int resolve_label(asm_token token, asm_array<asm_label> labels)
{
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

asm_array<unsigned int> generate_code(asm_token* token, asm_array<asm_label> labels)
{
    asm_array<asm_instr> instructions;
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

            instr.reg3 = token->reg_id;
            consume_reg(token);
            consume_comma(token);

            instr.reg1 = token->reg_id;
            consume_reg(token);
            consume_comma(token);

            instr.reg2 = token->reg_id;
            consume_reg(token);
        }
        else if(type >= TOK_BEQ && type < TOK_B)
        {
            instr.opcode = 1 + (type - TOK_BEQ);
            ++token;

            instr.reg1 = token->reg_id;
            consume_reg(token);
            consume_comma(token);

            instr.reg2 = token->reg_id;
            consume_reg(token);
            consume_comma(token);

            instr.immediate = resolve_label(*token, labels);
            consume(token, TOK_IDENTIFIER, "expected a label identifier");
        }
        else if(type == TOK_B)
        {
            instr.opcode = 11;
            ++token;
            instr.immediate = resolve_label(*token, labels);
            consume(token, TOK_IDENTIFIER, "expected a label identifier");
        }
        else if(type == TOK_LDR || type == TOK_STR)
        {
            instr.opcode = 12 + (type - TOK_LDR);
            ++token;

            instr.reg2 = token->reg_id;
            consume_reg(token);
            consume_comma(token);

            instr.immediate = token->int_literal;

            if(instr.immediate >= (1 << 15) || instr.immediate < -(1 << 15))
            {
                printf("error; line %d, col %d; integer literal does not fit into 16 bits\n", token->line, token->col);
                exit(1);
            }

            consume(token, TOK_INT_LITERAL, "expected an integer literal");
            consume(token, TOK_PAREN_LEFT, "expected '('");

            instr.reg1 = token->reg_id;
            consume_reg(token);
            consume(token, TOK_PAREN_RIGHT, "expected ')'");
        }
        else if(type >= TOK_ADDI && type <= TOK_SRAI)
        {
            instr.opcode = 14 + (type - TOK_ADDI);
            ++token;

            instr.reg2 = token->reg_id;
            consume_reg(token);
            consume_comma(token);

            instr.reg1 = token->reg_id;
            consume_reg(token);
            consume_comma(token);

            instr.immediate = token->int_literal;

            if(type == TOK_ADDI && ( instr.immediate >= (1 << 15) || instr.immediate < -(1 << 15) ))
            {
                printf("error; line %d, col %d; integer literal does not fit into 16 bits\n", token->line, token->col);
                exit(1);
            }
            else if(instr.immediate > (1 << 16) - 1)
            {
                printf("error; line %d, col %d; bitmask literal does not fit into 16 bits\n", token->line, token->col);
                exit(1);
            }

            consume(token, TOK_INT_LITERAL, "expected an integer literal");
        }
        else if(type == TOK_BL)
        {
            instr.opcode = 21;
            ++token;

            instr.immediate = resolve_label(*token, labels);
            consume(token, TOK_IDENTIFIER, "expected a label identifier");

            instr.reg2 = ASM_LR;
        }
        else if(type == TOK_BX)
        {
            instr.opcode = 22;
            ++token;

            instr.reg1 = token->reg_id;
            consume_reg(token);
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
    asm_array<unsigned int> code;
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
    asm_array<asm_label> labels = collect_labels(tokens);
    asm_array<unsigned int> code = generate_code(tokens, labels);

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
