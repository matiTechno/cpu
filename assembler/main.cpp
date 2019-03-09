#include <stdio.h>
#include <assert.h>
#include <vector>
#include <string.h>
#include <stdlib.h>

struct Inst_desc
{
    // don't change the order of enum elements
    enum Inst_type
    {
        DATA,
        ALU
    };

    enum Alu_y
    {
        A,
        AT_A
    };

    enum Alu_op
    {
        BITWISE_AND,
        ADD
    };

    Inst_type type = ALU;

    // data for compute op
    // I didn't go with union to be able to default initialize
    // alu_x has always input from reg D

    Alu_y alu_y = A;
    bool zero_alu_x = 0;
    bool invert_alu_x = 0;
    bool zero_alu_y = 0;
    bool invert_alu_y = 0;
    Alu_op alu_op = BITWISE_AND;
    bool invert_alu_output = 0;

    bool write_to_A = 0;
    bool write_to_D = 0;
    bool write_at_A = 0;

    bool jgz = 0; // jump if alu output is greater than zero
    bool jez = 0;
    bool jlz = 0;

    // data for data op
    int data;
};

int produce_inst(const Inst_desc& desc)
{
    if(desc.type == Inst_desc::DATA)
    {
        if(desc.data >= (1 << 15))
        {
            printf("error: number %d does not fit into 15 bits, exiting...\n", desc.data);
            exit(0);
        }
        return desc.data;
    }

    // I know I could automate this by storing each field as an integer
    // and iterating over all of them as iterating over an array.
    // But I will stick with this solution.

    int inst = 0;
    inst += 1 << 15;
    inst += desc.alu_y << 12;
    inst += desc.zero_alu_x << 11;
    inst += desc.invert_alu_x << 10;
    inst += desc.zero_alu_y << 9;
    inst += desc.invert_alu_y << 8;
    inst += desc.alu_op << 7;
    inst += desc.invert_alu_output << 6;
    inst += desc.write_to_A << 5;
    inst += desc.write_to_D << 4;
    inst += desc.write_at_A << 3;
    inst += desc.jgz << 2;
    inst += desc.jez << 1;
    inst += desc.jlz << 0;

    return inst;
}

enum class Token_type
{
    MOV,
    COMMA,
    PLUS,
    MINUS,
    INVERT,
    AND,
    OR,
    JMP,
    JE,
    JL,
    JG,
    JLE,
    JGE,
    A,
    D,
    AT_A,
    DEFINE,
    IDENTIFIER,
    INT_CONSTANT,
    COLON,
    DATA,

    // not used during initial tokenization but later after some processing
    ADDRESS,
    ZERO,
    ONE,
    END
};

struct Keyword
{
    Token_type type;
    const char* string;
};

static Keyword _keywords[] = {
    {Token_type::MOV, "mov"},
    {Token_type::JMP, "jmp"},
    {Token_type::JE, "je"},
    {Token_type::JL, "jl"},
    {Token_type::JG, "jg"},
    {Token_type::JLE, "jle"},
    {Token_type::JGE, "jge"},
    {Token_type::A, "a"}, // lower-case is easier to type
    {Token_type::D, "d"},
    {Token_type::AT_A, "[a]"},
    {Token_type::DATA, "data"},
    {Token_type::DEFINE, "define"},
};

bool is_keyword(const char* begin, int len, Keyword& output)
{
    for(Keyword& keyword: _keywords)
    {
        if(strlen(keyword.string) == len && strncmp(keyword.string, begin, len) == 0)
        {
            output = keyword;
            return true;
        }
    }
    return false;
}

struct Token
{
    Token_type type;
    int line;

    union
    {
        int value;

        struct
        {
            const char* begin;
            int len;
        } string;
    };
};

struct Key
{
    const char* begin;
    int len;
    int value;
    Token_type type_replace;
};

// 0 on error, we make sure that keys[0] is a dummy element
int find_key_idx(std::vector<Key>& keys, const char* begin, int len)
{
    for(int idx = 1; idx < keys.size(); ++ idx)
    {
        if(keys[idx].len == len && strncmp(keys[idx].begin, begin, len) == 0)
            return idx;
    }
    return 0;
}

bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

bool is_alphenumeric(char c)
{
    return is_digit(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool is_whitespace(char c)
{
    return c == '\n' || c == '\t' || c == ' ';
}

// returns 0 on error
int lex(std::vector<Token>& tokens, const char* input_buf)
{
    int line = 1;

    for(;;)
    {
        char c = *input_buf;
        if(c == '\0')
            break;

        switch(c)
        {
        case ';':
        {
            for(;;)
            {
                ++input_buf;

                if(*input_buf == '\0' || *input_buf == '\n')
                    break;
            }
            continue; // skip ++input_buf
        }

        case ' ':
        case '\t': break;

        case '\n':
        {
            ++line;
            break;
        }

        case '-':
        {
            tokens.push_back({Token_type::MINUS, line});
            break;
        }

        case '+':
        {
            tokens.push_back({Token_type::PLUS, line});
            break;
        }

        case '&':
        {
            tokens.push_back({Token_type::AND, line});
            break;
        }

        case '|':
        {
            tokens.push_back({Token_type::OR, line});
            break;
        }

        case '~':
        {
            tokens.push_back({Token_type::INVERT, line});
            break;
        }

        case ',':
        {
            tokens.push_back({Token_type::COMMA, line});
            break;
        }

        case ':':
        {
            tokens.push_back({Token_type::COLON, line});
            break;
        }

        default:
        {
            if(is_digit(c))
            {
                Token token = {Token_type::INT_CONSTANT, line};

                const char* begin = input_buf;
                int len = 0;

                for(;;)
                {
                    ++input_buf;
                    ++len;

                    if(!is_digit(*input_buf))
                        break;
                }

                assert(len < 128);
                char temp[128] = {};
                memcpy(temp, begin, len);

                sscanf(temp, "%d", &token.value);

                tokens.push_back(token);
                continue; // skip ++input_buf
            }
            else if(is_alphenumeric(c) || c == '_' || '[')
            {
                const char* begin = input_buf;
                int len = 0;

                for(;;)
                {
                    ++input_buf;
                    ++len;

                    if(!is_alphenumeric(*input_buf) && *input_buf != '_' && *input_buf != ']')
                        break;
                }

                Keyword keyword;
                if(is_keyword(begin, len, keyword))
                {
                    tokens.push_back({keyword.type, line});
                }
                else
                {
                    Token token = {Token_type::IDENTIFIER, line};
                    token.string.begin = begin;
                    token.string.len = len;
                    tokens.push_back(token);
                }

                continue; // skip ++input_buf
            }
            else
            {
                printf("line %d, error: not valid character '%c'\n", line, c);
                return 0;
            }
        }
        }

        ++input_buf;
    }

    return 1;
}

bool parse_alu_jmp(Inst_desc& desc, Token* const token)
{
    int count = 0;
    Token* tok_it = token;

    for(;;)
    {
        if(tok_it->type == Token_type::END || tok_it->type == Token_type::COMMA)
            break;

        ++count;
        ++tok_it;
    }

    for(int i = 0; i < count; ++i)
    {
        if(token[i].type == Token_type::AT_A)
        {
            token[i].type = Token_type::A;
            desc.alu_y = Inst_desc::AT_A;
        }
        else if(token[i].type == Token_type::INT_CONSTANT)
        {
            if(token[i].value == 0) token[i].type = Token_type::ZERO;
            else if(token[i].value == 1) token[i].type = Token_type::ONE;
            else
                printf("line %d, warning: ALU can't handle values that 'abs(n) > 1'; data instruction only accepts positive numbers\n",
                       token[i].line);
        }
    }

    static std::vector<Token_type> alu_token_sets[] = {
        {Token_type::ZERO},
        {Token_type::ONE},
        {Token_type::MINUS, Token_type::ONE},
        {Token_type::D},
        {Token_type::A},
        {Token_type::INVERT, Token_type::D},
        {Token_type::INVERT, Token_type::A},
        {Token_type::MINUS, Token_type::D},
        {Token_type::MINUS, Token_type::A},
        {Token_type::D, Token_type::PLUS, Token_type::ONE},
        {Token_type::A, Token_type::PLUS, Token_type::ONE},
        {Token_type::D, Token_type::MINUS, Token_type::ONE},
        {Token_type::A, Token_type::MINUS, Token_type::ONE},

        // these two output the same instruction
        {Token_type::D, Token_type::PLUS, Token_type::A},
        {Token_type::A, Token_type::PLUS, Token_type::D},

        {Token_type::D, Token_type::MINUS, Token_type::A},
        {Token_type::A, Token_type::MINUS, Token_type::D},

        // thse two output the same instruction
        {Token_type::A, Token_type::AND, Token_type::D},
        {Token_type::D, Token_type::AND, Token_type::A},

        // thse two output the same instruction
        {Token_type::A, Token_type::OR, Token_type::D},
        {Token_type::D, Token_type::OR, Token_type::A},
    };

    int set_idx = -1;
    int it = -1;
    for(auto& set: alu_token_sets)
    {
        ++it;

        if(set.size() != count)
            continue;

        bool match = true;
        for(int j = 0; j < count; ++j)
        {
            if(token[j].type != set[j])
            {
                match = false;
                break;
            }
        }

        if(!match)
            continue;

        set_idx = it;
    }

    switch(set_idx)
    {
    case 0: // 0
        desc.zero_alu_x = true;
        desc.zero_alu_y = true;
        break;
    case 1: // 1
        desc.zero_alu_x = true;
        desc.zero_alu_y = true;
        desc.invert_alu_x = true;
        desc.invert_alu_y = true;
        desc.alu_op = Inst_desc::ADD;
        desc.invert_alu_output = true;
        break;
    case 2: // -1
        desc.zero_alu_x = true;
        desc.zero_alu_y = true;
        desc.invert_alu_x = true;
        desc.alu_op = Inst_desc::ADD;
        break;
    case 3: // D
        desc.zero_alu_y = true;
        desc.invert_alu_y = true;
        break;
    case 4: // A
        desc.zero_alu_x = true;
        desc.invert_alu_x = true;
        break;
    case 5: // ~D
        desc.zero_alu_y = true;
        desc.invert_alu_y = true;
        desc.invert_alu_x = true;
        break;
    case 6: // ~A
        desc.zero_alu_x = true;
        desc.invert_alu_x = true;
        desc.invert_alu_y = true;
        break;
    case 7: // -D; why -D = ~D + 1 = ~(~0 + D)
        desc.zero_alu_y = true;
        desc.invert_alu_y = true;
        desc.alu_op = Inst_desc::ADD;
        desc.invert_alu_output = true;
        break;
    case 8: // -A
        desc.zero_alu_x = true;
        desc.invert_alu_x = true;
        desc.alu_op = Inst_desc::ADD;
        desc.invert_alu_output = true;
        break;
    case 9: // D + 1; why = ~(~D + ~0)
        desc.invert_alu_x = true;
        desc.zero_alu_y = true;
        desc.invert_alu_y = true;
        desc.alu_op = Inst_desc::ADD;
        desc.invert_alu_output = true;
        break;
    case 10: // A + 1
        desc.zero_alu_x = true;
        desc.invert_alu_x = true;
        desc.invert_alu_y = true;
        desc.alu_op = Inst_desc::ADD;
        desc.invert_alu_output = true;
        break;
    case 11: // D - 1
        desc.zero_alu_y = true;
        desc.invert_alu_y = true;
        desc.alu_op = Inst_desc::ADD;
        break;
    case 12: // A - 1
        desc.zero_alu_x = true;
        desc.invert_alu_x = true;
        desc.alu_op = Inst_desc::ADD;
        break;

    case 13: // A + D
    case 14: // D + A
        desc.alu_op = Inst_desc::ADD;
        break;

    case 15: // D - A; why
        desc.invert_alu_x = true;
        desc.alu_op = Inst_desc::ADD;
        desc.invert_alu_output = true;
        break;
    case 16: // A - D
        desc.invert_alu_y = true;
        desc.alu_op = Inst_desc::ADD;
        desc.invert_alu_output = true;
        break;

    case 17: // D & A
    case 18: // A & D
        //Inst_desc is initialized to this by default
        break;

    case 19: // D | A
    case 20: // A | D
        desc.invert_alu_x = true;
        desc.invert_alu_y = true;
        desc.invert_alu_output = true;
        break;

    default:
        printf("line %d, error: not supported ALU operation\n", token->line);
        return false;
    }

    if(tok_it->type == Token_type::END)
        return true;

    // skip comma
    ++tok_it;

    switch(tok_it->type)
    {
    case Token_type::JMP:
        desc.jez = true;
        desc.jlz = true;
        desc.jgz = true;
        break;

    case Token_type::JE:
        desc.jez = true;
        break;

    case Token_type::JL:
        desc.jlz = true;
        break;

    case Token_type::JG:
        desc.jgz = true;
        break;

    case Token_type::JLE:
        desc.jlz = true;
        desc.jez = true;
        break;

    case Token_type::JGE:
        desc.jgz = true;
        desc.jez = true;
        break;

    default:
        printf("line %d, error: expecting one of these: jmp, je, jl, jg, jle, jge\n", tok_it->line);
        return false;
    }

    ++tok_it;
    if(tok_it->type != Token_type::END)
    {
        printf("line %d, error: expecting end of line after jump operation\n", tok_it->line);
        return false;
    }

    return true;
}

int main(int argc, const char** argv)
{
    if(argc != 2)
    {
        printf("one argument required (filename), exiting...\n");
        return 0;
    }

    FILE* file = fopen(argv[1], "r");

    if(!file)
    {
        printf("could not open file\n");
        return 0;
    }

    int rcode = fseek(file, 0, SEEK_END);
    assert(rcode != -1);

    rcode = ftell(file);
    assert(rcode != -1);

    int buf_size = rcode;

    rewind(file);

    std::vector<char> buffer;
    buffer.resize(buf_size + 1);
    buffer[buf_size] = '\0';

    rcode = fread(buffer.data(), 1, buf_size, file);
    assert(rcode = buf_size);

    rcode = fclose(file);
    assert(rcode == 0);

    std::vector<Token> tokens;

    rcode = lex(tokens, buffer.data());
    if(rcode == 0)
        return 0;

    tokens.push_back({Token_type::END, tokens.back().line + 1});

    // first pass:
    // create map of all labels, defines, and variables (string - number pairs)

    std::vector<Key> keys;
    keys.push_back({}); // dummy element

    int next_instruction_addr = 0;
    int next_data_addr = 0;

    if(tokens.size() == 0)
    {
        printf("program is empty... exiting\n");
        return 0;
    }

    int current_line = tokens.front().line;

    std::vector<Token> line_of_tokens;

    for(Token& _token: tokens)
    {
        if(_token.line != current_line)
        {
            int line_size = line_of_tokens.size();

            if(line_of_tokens[0].type == Token_type::DATA)
            {
                if(line_size != 2 && line_size != 3)
                {
                    printf("line %d, error: 'data' requires one or two operands\n", current_line);
                    return 0;
                }

                const char* begin = line_of_tokens[1].string.begin;
                int len = line_of_tokens[1].string.len;

                if(find_key_idx(keys, begin, len))
                {
                    printf("line %d, error: identifier already in use\n", current_line);
                    return 0;
                }

                keys.push_back({begin, len, next_data_addr, Token_type::ADDRESS});

                int data_size = 1;

                if(line_size == 3)
                {
                    Token& token = line_of_tokens[2];
                    if(token.type == Token_type::INT_CONSTANT)
                    {
                        data_size = token.value;
                    }
                    else if(token.type == Token_type::IDENTIFIER)
                    {
                        int key_idx = find_key_idx(keys, token.string.begin, token.string.len);
                        if(key_idx == 0)
                        {
                            printf("line %d, error: could not resolve indentifier\n", current_line);
                            return 0;
                        }

                        if(keys[key_idx].type_replace != Token_type::INT_CONSTANT)
                        {
                            printf("line %d, error: identifier for data size can only be specified with 'define'\n", current_line);
                            return 0;
                        }

                        data_size = keys[key_idx].value;
                    }
                    else
                    {
                        printf("line %d, error: data size must be a positive number or an identifier\n", current_line);
                        return 0;
                    }
                }

                next_data_addr += data_size;
            }
            else if(line_size > 1 && line_of_tokens[1].type == Token_type::COLON)
            {
                if(line_size != 2)
                {
                    printf("line %d, ':' must be preceeded by an indentifier and must end a line\n", current_line);
                    return 0;
                }

                if(line_of_tokens[0].type != Token_type::IDENTIFIER)
                {
                    printf("line %d, label must be an identifier\n", current_line);
                    return 0;
                }

                const char* begin = line_of_tokens[0].string.begin;
                int len = line_of_tokens[0].string.len;

                if(find_key_idx(keys, begin, len))
                {
                    printf("line %d, error: identifier already in use\n", current_line);
                    return 0;
                }

                keys.push_back({begin, len, next_instruction_addr, Token_type::ADDRESS});
            }
            else if(line_of_tokens[0].type == Token_type::DEFINE)
            {
                if(line_size != 3)
                {
                    printf("line %d, error: 'define' requires two operands\n", current_line);
                    return 0;
                }
                if(line_of_tokens[1].type != Token_type::IDENTIFIER)
                {
                    printf("line %d, error: first operand of 'define' must be an identifier\n", current_line);
                    return 0;
                }
                if(line_of_tokens[2].type != Token_type::INT_CONSTANT)
                {
                    printf("line %d, error: second operand of 'define' must be a number\n", current_line);
                    return 0;
                }

                const char* begin = line_of_tokens[1].string.begin;
                int len = line_of_tokens[1].string.len;

                if(find_key_idx(keys, begin, len))
                {
                    printf("line %d, error: identifier already in use\n", current_line);
                    return 0;
                }

                keys.push_back({begin, len, line_of_tokens[2].value, Token_type::INT_CONSTANT});
            }
            else
            {
                ++next_instruction_addr;
            }

            line_of_tokens.clear();
            current_line = _token.line;
            line_of_tokens.push_back(_token);
        }
        else
        {
            line_of_tokens.push_back(_token);
        }
    }

    // second pass

    // resolve identifiers
    for(Token& token: tokens)
    {
        if(token.type == Token_type::IDENTIFIER)
        {
            int idx = find_key_idx(keys, token.string.begin, token.string.len);
            if(idx == 0)
            {
                printf("line %d, error: could not resolve '%.*s' identifier\n", token.line, token.string.len, token.string.begin);
                return 0;
            }

            token.type = keys[idx].type_replace;
            token.value = keys[idx].value;
        }
    }

    std::vector<int> instructions;

    line_of_tokens.clear();
    current_line = tokens.front().line;

    for(Token& _token: tokens)
    {
        if(_token.line != current_line)
        {
            int line_size = line_of_tokens.size();
            line_of_tokens.push_back({Token_type::END}); // this is needed by parse_alu_jmp()

            if(line_of_tokens[0].type == Token_type::DEFINE || line_of_tokens[0].type == Token_type::DATA ||
                    (line_size == 2 && line_of_tokens[1].type == Token_type::COLON) ||
                    line_of_tokens[0].type == Token_type::END
                    )
            {} // do nothing..., these are already parsed
            else
            {
                Inst_desc desc;

                if(line_of_tokens[0].type == Token_type::MOV)
                {
                    if(line_size < 4)
                    {
                        printf("line %d, error: incomplete mov\n", current_line);
                        return 0;
                    }

                    Token* t = &line_of_tokens[1];

                    if(t->type != Token_type::A && t->type != Token_type::D && t->type != Token_type::AT_A)
                    {
                        printf("line %d, error: first operand of mov must be a register (a, d, ata)\n", current_line);
                        return 0;
                    }

                    if(t->type == Token_type::A) desc.write_to_A = true;
                    else if(t->type == Token_type::D) desc.write_to_D = true;
                    else if(t->type == Token_type::AT_A) desc.write_at_A = true;

                    ++t;

                    if(t->type != Token_type::COMMA)
                    {
                        printf("line %d, error: missing comma after first mov operand\n", current_line);
                        return 0;
                    }

                    ++t;

                    if((t->type == Token_type::INT_CONSTANT && t->value > 1) || t->type == Token_type::ADDRESS)
                    {
                        if(desc.write_to_A == false)
                        {
                            printf("line %d, error: numbers bigger than 1 and addresses might be only moved to the A register\n", current_line);
                            return 0;
                        }

                        if(line_size > 4)
                        {
                            printf("line %d, error: no more operations might be specified after 'mov a, n' where n is bigger than 1 "
                                   "or an address\n", current_line);
                            return 0;
                        }

                        desc.data = t->value;
                        desc.type = Inst_desc::DATA;
                    }
                    else
                    {
                        if(parse_alu_jmp(desc, t) == false)
                            return 0;
                    }
                }
                else if(line_of_tokens[0].type == Token_type::JMP)
                {
                    if(line_size != 1)
                    {
                        printf("line %d, error: 'jmp' must end the line\n", current_line);
                        return 0;
                    }

                    desc.jez = true;
                    desc.jlz = true;
                    desc.jgz = true;
                }
                else
                {
                    if(parse_alu_jmp(desc, &line_of_tokens[0]) == false)
                        return 0;
                }

                instructions.push_back(produce_inst(desc));
            }

            line_of_tokens.clear();
            current_line = _token.line;
            line_of_tokens.push_back(_token);
        }
        else
        {
            line_of_tokens.push_back(_token);
        }
    }

    // write to output file
    int filename_len = strlen(argv[1]);
    int append_idx = filename_len - 1;
    for(; append_idx >= 0; --append_idx)
    {
        if(argv[1][append_idx] == '.')
            break;
    }

    if(append_idx == -1)
        append_idx = filename_len;

    char tmp_buf[1024];
    const char* str_ext = ".logisim_hex";
    assert(strlen(str_ext) + filename_len + 1 < sizeof(tmp_buf));

    memcpy(tmp_buf, argv[1], append_idx);
    strcpy(tmp_buf + append_idx, str_ext);

    file = fopen(tmp_buf, "w");

    if(!file)
    {
        printf("could not create file for output: %s\n", tmp_buf);
        return 0;
    }

    rcode = fprintf(file, "v2.0 raw\n");
    assert(rcode >= 0);

    for(int inst: instructions)
    {
        fprintf(file, "%x\n", inst);
        assert(rcode >= 0);
    }

    rcode = fclose(file);
    assert(rcode == 0);

    return 0;
}
