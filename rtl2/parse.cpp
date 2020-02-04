#include "ic.h"

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
    switch(parser.peek())
    {
    case TOK_INT:
    case TOK_FLOAT:
    case TOK_BOOL:
        return true;
    case TOK_VOID:
        return parser.peek2() == TOK_STAR;
    case TOK_IDENTIFIER:
        for(ic_struct& _struct: *parser.structures)
        {
            if(ic_strcmp(_struct.id_token.str, parser.peek_full().str))
                return true;
        }
    }
    return false;
}

ic_type produce_type()
{
    if(!peek_type())
        ic_exit(parser.peek_full().line, parser.peek_full().col, "expected a type");

    ic_token token = parser.advance();
    ic_type type;
    type.indirection = 0;
    type.struct_id = 0;

    switch(token.type)
    {
    case TOK_INT:
        type.basic_type = TYPE_INT;
        break;
    case TOK_FLOAT:
        type.basic_type = TYPE_FLOAT;
        break;
    case TOK_BOOL:
        type.basic_type = TYPE_BOOL;
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
                type.struct_id = &_struct - parser.structures->begin();
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
    switch(parser.peek())
    {
    case TOK_INT_LITERAL:
    {
        ic_expr* expr = alloc_expr(EXPR_INT_LITERAL, parser.peek_full());
        expr->int_literal = parser.advance().int_literal;
        return expr;
    }
    case TOK_FLOAT_LITERAL:
    {
        ic_expr* expr = alloc_expr(EXPR_FLOAT_LITERAL, parser.peek_full());
        expr->float_literal = parser.advance().float_literal;
        return expr;
    }
    case TOK_TRUE:
    case TOK_FALSE:
    {
        ic_expr* expr = alloc_expr(EXPR_INT_LITERAL, parser.peek_full());
        expr->int_literal = parser.advance().type == TOK_TRUE;
        return expr;
    }
    case TOK_NULLPTR:
        return alloc_expr(EXPR_NULLPTR, parser.advance());

    case TOK_IDENTIFIER:
        if(parser.peek2() == TOK_LEFT_PAREN)
        {
            ic_expr* expr = alloc_expr(EXPR_CALL, parser.advance());
            parser.consume(TOK_LEFT_PAREN);
            ic_expr** arg_tail = &expr->call.args;

            while(!parser.try_consume(TOK_RIGHT_PAREN))
            {
                *arg_tail = produce_expr();
                arg_tail = &(**arg_tail).next;

                if(!parser.try_consume(TOK_COMMA))
                {
                    parser.consume(TOK_RIGHT_PAREN);
                    break;
                }
            }
            return expr;
        }
        else
            return alloc_expr(EXPR_IDENTIFIER, parser.advance());
    case TOK_LEFT_PAREN:
    {
        parser.advance();
        ic_expr* expr = produce_expr();
        parser.consume(TOK_RIGHT_PAREN);
        return expr;
    }
    default:
        ic_exit(parser.peek_full().line, parser.peek_full().col, "expected a primary expression");
        return nullptr;
    }
}

ic_expr* produce_expr_access()
{
     ic_expr* lhs = produce_expr_primary();

     for(;;)
     {
         switch(parser.peek())
         {
         case TOK_LEFT_BRACKET:
         {
             ic_expr* expr = alloc_expr(EXPR_SUBSCRIPT, parser.advance());
             expr->subscript.lhs = lhs;
             expr->subscript.rhs = produce_expr();
             parser.consume(TOK_RIGHT_BRACKET);
             lhs = expr;
             break;
         }
         case TOK_DOT:
         case TOK_ARROW:
         {
             ic_expr_type type = parser.peek() == TOK_DOT ? EXPR_MEMBER_ACCESS : EXPR_DEREF_MEMBER_ACCESS;
             ic_expr* expr = alloc_expr(type, parser.advance());
             expr->member_access.lhs = lhs;
             expr->member_access.rhs_token = parser.consume(TOK_IDENTIFIER);
             lhs = expr;
             break;
         }
         default:
             return lhs;
         }
     }
}

ic_expr* produce_expr_unary()
{
    ic_expr_type type;

    switch(parser.peek())
    {
    case TOK_BANG:
        type = EXPR_LOGICAL_NOT;
        break;
    case TOK_MINUS:
        type = EXPR_NEG;
        break;
    case TOK_PLUS_PLUS:
        type = EXPR_INC;
        break;
    case TOK_MINUS_MINUS:
        type = EXPR_DEC;
        break;
    case TOK_AMP:
        type = EXPR_ADDR;
        break;
    case TOK_STAR:
        type = EXPR_DEREF;
        break;
    case TOK_LEFT_PAREN:
    {
        ic_expr* expr = alloc_expr(EXPR_CAST, parser.advance());

        if(!peek_type())
        {
            --parser.token_it; // hack, there is no peek2_type()
            return produce_expr_access();
        }
        expr->cast.type = produce_type();
        parser.consume(TOK_RIGHT_PAREN);
        expr->cast.sub_expr = produce_expr_unary();
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
    default:
        return produce_expr_access();
    }
    ic_expr* expr = alloc_expr(type, parser.advance());
    expr->sub_expr = produce_expr_unary();
    return expr;
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
    ic_expr_type expr_types[5] = {};
    assert(TOK_EOF == 0);

    switch(precedence)
    {
    case PREC_LOGICAL_OR:
        token_types[0] = TOK_VBAR_VBAR;
        expr_types[0] = EXPR_LOGICAL_OR;
        break;
    case PREC_LOGICAL_AND:
        token_types[0] = TOK_AMP_AMP;
        expr_types[0] = EXPR_LOGICAL_AND;
        break;
    case PREC_COMPARE_EQUAL:
        token_types[0] = TOK_EQUAL_EQUAL;
        token_types[1] = TOK_BANG_EQUAL;
        expr_types[0] = EXPR_CMP_EQ;
        expr_types[1] = EXPR_CMP_NEQ;
        break;
    case PREC_COMPARE_GREATER:
        token_types[0] = TOK_GREATER;
        token_types[1] = TOK_GREATER_EQUAL;
        token_types[2] = TOK_LESS;
        token_types[3] = TOK_LESS_EQUAL;
        expr_types[0] = EXPR_CMP_GT;
        expr_types[1] = EXPR_CMP_GE;
        expr_types[2] = EXPR_CMP_LT;
        expr_types[3] = EXPR_CMP_LE;
        break;
    case PREC_ADD:
        token_types[0] = TOK_PLUS;
        token_types[1] = TOK_MINUS;
        expr_types[0] = EXPR_ADD;
        expr_types[1] = EXPR_SUB;
        break;
    case PREC_MULTIPLY:
        token_types[0] = TOK_STAR;
        token_types[1] = TOK_SLASH;
        expr_types[0] = EXPR_MUL;
        expr_types[1] = EXPR_DIV;
        break;
    case PREC_UNARY:
        return produce_expr_unary();
    }

    ic_expr* lhs = produce_expr_binary((ic_precedence)(precedence + 1));

    for(;;)
    {
        ic_token_type* it = token_types;
        ic_expr_type* eit = expr_types;

        while(*it != TOK_EOF)
        {
            if(*it == parser.peek())
                break;
            ++it;
            ++eit;
        }

        if(*it == TOK_EOF)
            break;

        ic_expr* expr = alloc_expr(*eit, parser.advance());
        expr->binary.lhs = lhs;
        expr->binary.rhs = produce_expr_binary((ic_precedence)(precedence + 1));
        lhs = expr;
    }
    return lhs;
}

ic_expr* produce_expr_assignment()
{
    ic_expr* lhs = produce_expr_binary(PREC_LOGICAL_OR);
    ic_expr_type type;

    switch(parser.peek())
    {
    case TOK_EQUAL:
        type = EXPR_ASSIGN;
        break;
    case TOK_PLUS_EQUAL:
        type = EXPR_ADD_ASSIGN;
        break;
    case TOK_MINUS_EQUAL:
        type = EXPR_SUB_ASSIGN;
        break;
    case TOK_STAR_EQUAL:
        type = EXPR_MUL_ASSIGN;
        break;
    case TOK_SLASH_EQUAL:
        type = EXPR_DIV_ASSIGN;
        break;
    default:
        return lhs;
    }
    ic_expr* expr = alloc_expr(type, parser.advance());
    expr->binary.lhs = lhs;
    expr->binary.rhs = produce_expr();
    return expr;
}

ic_expr* produce_expr()
{
    return produce_expr_assignment();
}

ic_expr* produce_expr_condition()
{
    bool in_parens = parser.peek() == TOK_LEFT_PAREN;
    ic_expr* expr = produce_expr();

    switch(expr->type)
    {
    case EXPR_ASSIGN:
    case EXPR_ADD_ASSIGN:
    case EXPR_SUB_ASSIGN:
    case EXPR_MUL_ASSIGN:
    case EXPR_DIV_ASSIGN:
        if(!in_parens)
            ic_exit(expr->token.line, expr->token.col, "condition can't be an assignment");
    }
    return expr;
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
            tail = &(**tail).next;
        }
        return stmt;
    }
    case TOK_WHILE:
    {
        ic_stmt* stmt = alloc_stmt(STMT_FOR, parser.advance());
        parser.consume(TOK_LEFT_PAREN);
        stmt->_for.header2 = produce_expr_condition();
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
            stmt->_for.header2 = produce_expr_condition();
            parser.consume(TOK_SEMICOLON);
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
        stmt->_if.header = produce_expr_condition();
        parser.consume(TOK_RIGHT_PAREN);
        stmt->_if.body_if = produce_stmt();

        if(parser.try_consume(TOK_ELSE))
            stmt->_if.body_else = produce_stmt();
        return stmt;
    }
    case TOK_RETURN:
    {
        ic_stmt* stmt = alloc_stmt(STMT_RETURN, parser.advance());

        if(!parser.try_consume(TOK_SEMICOLON))
        {
            stmt->expr = produce_expr();
            parser.consume(TOK_SEMICOLON);
        }
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
    default:
        return produce_stmt_var_decl();
    }
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
        params.push_back(param);
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
