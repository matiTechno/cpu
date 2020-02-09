#include "ic.h"

// order matters
enum ic_stmt_result
{
    SR_NULL,
    SR_BRANCH_LOOP,
    SR_RETURN,
};

struct ic_expr_result
{
    ic_token token; // error reporting
    ic_type type;
    bool lvalue;
};

struct ic_var
{
    ic_token token;
    ic_type type;
};

struct ic_scope
{
    int prev_var_size;
};

struct
{
    array<ic_function> functions;
    array<ic_struct> structures;
    array<ic_var> vars;
    array<ic_scope> scopes;
    int loop_count;
    ic_type return_type;
    bool leaf;

    void push_scope()
    {
        scopes.push_back({vars.size});
    }

    void pop_scope()
    {
        assert(scopes.size);
        vars.resize(scopes.back().prev_var_size);
        scopes.pop_back();
    }

    void add_var(ic_token token, ic_type type)
    {
        assert(scopes.size);

        for(ic_var var: vars)
        {
            if(ic_strcmp(var.token.str, token.str))
                ic_exit(token.line, token.col, "variable redeclaration");
        }
        vars.push_back({token, type});
    }

    ic_var* get_var(ic_str name)
    {
        for(ic_var& var: vars)
        {
            if(ic_strcmp(var.token.str, name))
                return &var;
        }
        return nullptr;
    }

    ic_function* get_function(ic_str name)
    {
        for(ic_function& f: functions)
        {
            if(ic_strcmp(f.id_token.str, name))
                return &f;
        }
        return nullptr;
    }

    ic_struct get_struct(int id)
    {
        assert(id < structures.size);
        return structures[id];
    }
} ctx;

bool implicit_cast_impl(ic_type lhs, ic_type rhs)
{
    if(lhs.basic_type == TYPE_STRUCT && rhs.basic_type == TYPE_STRUCT && lhs.struct_id != rhs.struct_id)
        return false;

    if(lhs.indirection)
    {
        if(!rhs.indirection)
            return false;

        if(rhs.basic_type == TYPE_NULLPTR)
            return true;

        if(lhs.basic_type == TYPE_VOID && lhs.indirection == 1)
            return true;

        if(lhs.basic_type != rhs.basic_type)
            return false;

        if(lhs.indirection != rhs.indirection)
            return false;

        return true;
    }

    if(rhs.indirection)
        return lhs.basic_type == TYPE_BOOL;

    if(lhs.basic_type == TYPE_STRUCT || lhs.basic_type == TYPE_VOID || rhs.basic_type == TYPE_STRUCT || rhs.basic_type == TYPE_VOID)
        return lhs.basic_type == rhs.basic_type;

    return true;
}

void implicit_cast(ic_type lhs, ic_expr_result result)
{
    if(!implicit_cast_impl(lhs, result.type))
        ic_exit(result.token.line, result.token.col, "implicit type conversion failure");
}

void assert_lvalue(ic_expr_result result)
{
    if(!result.lvalue)
        ic_exit(result.token.line, result.token.col, "expected an lvalue");
}

void assert_dereference(ic_expr_result result)
{
    ic_type type = result.type;

    if(!type.indirection || (type.basic_type == TYPE_VOID && type.indirection == 1) || type.basic_type == TYPE_NULLPTR)
        ic_exit(result.token.line, result.token.col, "an invalid type");
}

void assert_arithmetic(ic_expr_result result)
{
    implicit_cast(get_basic_type(TYPE_INT), result); // todo, weak error message
}

ic_type get_arithmetic_type(ic_expr_result lhs, ic_expr_result rhs)
{
    assert_arithmetic(lhs);
    assert_arithmetic(rhs);
    // todo, this is used to handle errors like, ptr + (1 + 0.3);
    return lhs.type.basic_type > rhs.type.basic_type ? lhs.type : rhs.type;
}

void assert_integer(ic_expr_result result)
{
    ic_type type = result.type;

    if(type.indirection || (type.basic_type != TYPE_INT && type.basic_type != TYPE_BOOL))
        ic_exit(result.token.line, result.token.col, "an invalid type");
}

void assert_same_type(ic_type lhs, ic_type rhs, ic_token token)
{
    if(lhs.basic_type != rhs.basic_type || lhs.indirection != rhs.indirection || lhs.struct_id != rhs.struct_id)
        ic_exit(token.line, token.col, "incompatible types");
}

void assert_ptr_cmp_types(ic_type lhs, ic_type rhs, ic_token token)
{
    if(!lhs.indirection || !rhs.indirection)
        ;
    else
    {
        if(lhs.basic_type == TYPE_NULLPTR || rhs.basic_type == TYPE_NULLPTR)
            return;
        if(lhs.basic_type == TYPE_VOID && lhs.indirection == 1)
            return;
        if(rhs.basic_type == TYPE_VOID && rhs.indirection == 1)
            return;
    }
    assert_same_type(lhs, rhs, token);
}

ic_type resolve_add(ic_expr_result lhs, ic_expr_result rhs)
{
    if(lhs.type.indirection)
    {
        assert_dereference(lhs);
        assert_integer(rhs);
        return lhs.type;
    }
    else if(rhs.type.indirection)
    {
        assert_dereference(rhs);
        assert_integer(lhs);
        return rhs.type;
    }
    else
        return get_arithmetic_type(lhs, rhs);
}

ic_type resolve_sub(ic_expr_result lhs, ic_expr_result rhs, ic_token token)
{
    if(lhs.type.indirection && rhs.type.indirection)
    {
        assert_dereference(lhs);
        assert_same_type(lhs.type, rhs.type, token);
        return get_basic_type(TYPE_INT);
    }
    else if(rhs.type.indirection)
    {
        ic_exit(lhs.token.line, lhs.token.col, "an invalid type");
        return {};
    }
    else if(lhs.type.indirection)
    {
        assert_dereference(lhs);
        assert_arithmetic(rhs);
        return lhs.type;
    }
    else
        return get_arithmetic_type(lhs, rhs);
}

ic_expr_result resolve_expr(ic_expr* expr)
{
    assert(expr);

    switch(expr->type)
    {
    case EXPR_ASSIGN:
    {
        ic_expr_result lhs = resolve_expr(expr->binary.lhs);
        ic_expr_result rhs = resolve_expr(expr->binary.rhs);
        assert_lvalue(lhs);
        implicit_cast(lhs.type, rhs);
        return {expr->token, lhs.type, false};
    }
    case EXPR_ADD_ASSIGN:
    {
        ic_expr_result lhs = resolve_expr(expr->binary.lhs);
        ic_expr_result rhs = resolve_expr(expr->binary.rhs);
        assert_lvalue(lhs);
        rhs = {expr->token, resolve_add(lhs, rhs), false};
        implicit_cast(lhs.type, rhs);
        return {expr->token, lhs.type, false};
    }
    case EXPR_SUB_ASSIGN:
    {
        ic_expr_result lhs = resolve_expr(expr->binary.lhs);
        ic_expr_result rhs = resolve_expr(expr->binary.rhs);
        assert_lvalue(lhs);
        rhs = {expr->token, resolve_sub(lhs, rhs, expr->token), false};
        implicit_cast(lhs.type, rhs);
        return {expr->token, lhs.type, false};
    }
    case EXPR_MUL_ASSIGN:
    case EXPR_DIV_ASSIGN:
    {
        ic_expr_result lhs = resolve_expr(expr->binary.lhs);
        ic_expr_result rhs = resolve_expr(expr->binary.rhs);
        assert_lvalue(lhs);
        rhs = {expr->token, get_arithmetic_type(lhs, rhs), false};
        implicit_cast(lhs.type, rhs);
        return {expr->token, lhs.type, false};
    }
    case EXPR_LOGICAL_OR:
    case EXPR_LOGICAL_AND:
    {
        ic_expr_result lhs = resolve_expr(expr->binary.lhs);
        ic_expr_result rhs = resolve_expr(expr->binary.rhs);
        implicit_cast(get_basic_type(TYPE_BOOL), lhs);
        implicit_cast(get_basic_type(TYPE_BOOL), rhs);
        return {expr->token, get_basic_type(TYPE_BOOL), false};
    }
    case EXPR_CMP_EQ:
    case EXPR_CMP_NEQ:
    case EXPR_CMP_GT:
    case EXPR_CMP_GE:
    case EXPR_CMP_LT:
    case EXPR_CMP_LE:
    {
        ic_expr_result lhs = resolve_expr(expr->binary.lhs);
        ic_expr_result rhs = resolve_expr(expr->binary.rhs);

        if(lhs.type.indirection || rhs.type.indirection)
            assert_ptr_cmp_types(lhs.type, rhs.type, expr->token);
        else
            get_arithmetic_type(lhs, rhs);
        return {expr->token, get_basic_type(TYPE_BOOL), false};
    }
    case EXPR_ADD:
    {
        ic_expr_result lhs = resolve_expr(expr->binary.lhs);
        ic_expr_result rhs = resolve_expr(expr->binary.rhs);
        return {expr->token, resolve_add(lhs, rhs), false};

    }
    case EXPR_SUB:
    {
        ic_expr_result lhs = resolve_expr(expr->binary.lhs);
        ic_expr_result rhs = resolve_expr(expr->binary.rhs);
        return {expr->token, resolve_sub(lhs, rhs, expr->token), false};

    }
    case EXPR_MUL:
    case EXPR_DIV:
    {
        ic_expr_result lhs = resolve_expr(expr->binary.lhs);
        ic_expr_result rhs = resolve_expr(expr->binary.rhs);
        return {expr->token, get_arithmetic_type(lhs, rhs), false};
    }
    case EXPR_LOGICAL_NOT:
    {
        ic_expr_result result = resolve_expr(expr->sub_expr);
        implicit_cast(get_basic_type(TYPE_BOOL), result);
        return {expr->token, get_basic_type(TYPE_BOOL), false};
    }
    case EXPR_NEG:
    {
        ic_expr_result result = resolve_expr(expr->sub_expr);
        assert_arithmetic(result);
        return {expr->token, result.type, false};
    }
    case EXPR_INC:
    case EXPR_DEC:
    {
        ic_expr_result result = resolve_expr(expr->sub_expr);
        assert_lvalue(result);
        return {expr->token, result.type, true};
    }
    case EXPR_ADDR:
    {
        ic_expr_result result = resolve_expr(expr->sub_expr);
        assert_lvalue(result);
        result.type.indirection += 1;
        return {expr->token, result.type, false};
    }
    case EXPR_DEREF:
    {
        ic_expr_result result = resolve_expr(expr->sub_expr);
        assert_dereference(result);
        result.type.indirection -= 1;
        return {expr->token, result.type, true};
    }
    case EXPR_SIZEOF:
        return {expr->token, get_basic_type(TYPE_INT), false};

    case EXPR_CALL:
    {
        ctx.leaf = false;
        ic_token token = expr->token;
        ic_function* fun = ctx.get_function(token.str);

        if(!fun)
            ic_exit(token.line, token.col, "an undeclared function identifier");

        ic_expr* arg = expr->call.args;
        int args_size = 0;

        while(arg)
        {
            ++args_size;
            arg = arg->next;
        }

        if(args_size != fun->params_size)
            ic_exit(token.line, token.col, "not matching number of arguments");

        arg = expr->call.args;

        for(int i = 0; i < fun->params_size; ++i, arg = arg->next)
        {
            ic_expr_result arg_result = resolve_expr(arg);
            ic_param param = fun->params[i];
            implicit_cast(param.type, arg_result);
        }
        expr->call.fun_id = fun - ctx.functions.begin();
        return {token, fun->return_type, false};
    }
    case EXPR_INT_LITERAL:
        return {expr->token, get_basic_type(TYPE_INT), false};

    case EXPR_FLOAT_LITERAL:
        return {expr->token, get_basic_type(TYPE_FLOAT), false};

    case EXPR_IDENTIFIER:
    {
        // note, currently only variable identifiers are supported

        ic_token token = expr->token;
        ic_var* var = ctx.get_var(token.str);

        if(!var)
            ic_exit(token.line, token.col, "an undeclared variable identifier");

        expr->type = EXPR_VAR_ID;
        expr->var_id = var - ctx.vars.begin();
        return {token, var->type, true};
    }
    case EXPR_NULLPTR:
    {
        ic_type type = {TYPE_NULLPTR, 1, 0};
        return {expr->token, type, false};
    }
    case EXPR_CAST:
    {
        ic_expr_result rhs = resolve_expr(expr->binary.lhs);
        ic_type lhs = expr->cast.type;

        if(lhs.indirection)
        {
            if(!rhs.type.indirection)
                assert_integer(rhs);
        }
        else
            implicit_cast(lhs, rhs);
        return {expr->token, lhs, false};
    }
    case EXPR_MEMBER_ACCESS:
    case EXPR_DEREF_MEMBER_ACCESS:
    {
        ic_expr_result result = resolve_expr(expr->member_access.lhs);
        bool error = result.type.basic_type != TYPE_STRUCT;
        bool lvalue;

        if(expr->type == EXPR_DEREF_MEMBER_ACCESS)
        {
            error = result.type.indirection != 1;
            lvalue = true;
        }
        else
        {
            error = result.type.indirection;
            lvalue = result.lvalue;
        }

        if(error)
            ic_exit(result.token.line, result.token.col, "an invalid type");

        ic_struct _struct = ctx.get_struct(result.type.struct_id);
        ic_token token = expr->member_access.rhs_token;
        ic_param param;
        int member_id = -1;

        for(int i = 0; i <_struct.members_size; ++i)
        {
            param = _struct.members[i];

            if(ic_strcmp(param.id_token.str, token.str))
            {
                member_id = i;
                break;
            }
        }

        if(member_id == -1)
            ic_exit(token.line, token.col, "not a struct member");
        expr->member_access.member_id = member_id;
        return {expr->token, param.type, lvalue};
    }
    case EXPR_SUBSCRIPT:
    {
        ic_expr_result lhs = resolve_expr(expr->binary.lhs);
        ic_expr_result rhs = resolve_expr(expr->binary.rhs);
        lhs = {expr->token, resolve_add(lhs, rhs), false};
        assert_dereference(lhs);
        lhs.type.indirection -= 1;
        lhs.lvalue = true;
        return lhs;
    }
    default:
        assert(false);
        return {};
    }
}

ic_stmt_result resolve_stmt(ic_stmt* stmt)
{
    assert(stmt);

    switch(stmt->type)
    {
    case STMT_COMPOUND:
    {
        if(stmt->compound.push_scope)
            ctx.push_scope();

        ic_stmt* it = stmt->compound.body;
        ic_stmt_result result = SR_NULL;

        while(it)
        {
            result = resolve_stmt(it);

            if(result != SR_NULL && it->next)
                ic_exit(it->next->token.line, it->next->token.col, "an unreachable code");
            it = it->next;
        }
        if(stmt->compound.push_scope)
            ctx.pop_scope();
        return result;
    }
    case STMT_FOR:
    {
        ctx.push_scope();
        ++ctx.loop_count;
        resolve_stmt(stmt->_for.header1);

        if(stmt->_for.header2)
            implicit_cast(get_basic_type(TYPE_BOOL), resolve_expr(stmt->_for.header2));

        resolve_expr(stmt->_for.header3);
        ic_stmt_result result = resolve_stmt(stmt->_for.body);
        --ctx.loop_count;
        ctx.pop_scope();

        if(result == SR_BRANCH_LOOP)
            return SR_NULL;
        return result;
    }
    case STMT_IF:
    {
        implicit_cast(get_basic_type(TYPE_BOOL), resolve_expr(stmt->_if.header));
        ctx.push_scope();
        ic_stmt_result result_if = resolve_stmt(stmt->_if.body_if);
        ctx.pop_scope();
        ic_stmt_result result_else = SR_NULL;

        if(stmt->_if.body_else)
        {
            ctx.push_scope();
            result_else = resolve_stmt(stmt->_if.body_else);
            ctx.pop_scope();
        }
        return result_if < result_else ? result_if : result_else;
    }
    case STMT_VAR_DECL:
    {
        ctx.add_var(stmt->var_decl.id_token, stmt->var_decl.type);

        if(stmt->var_decl.init_expr)
        {
            ic_expr_result result = resolve_expr(stmt->var_decl.init_expr);
            implicit_cast(stmt->var_decl.type, result);
        }
        return SR_NULL;
    }
    case STMT_RETURN:
    {
        if(stmt->expr)
        {
            ic_expr_result result = resolve_expr(stmt->expr);
            implicit_cast(ctx.return_type, result);
        }
        else if(!is_void_type(ctx.return_type))
            ic_exit(stmt->token.line, stmt->token.col, "an invalid return type");
        return SR_RETURN;
    }
    case STMT_BREAK:
    case STMT_CONTINUE:
    {
        if(!ctx.loop_count)
            ic_exit(stmt->token.line, stmt->token.col, "use outside of a loop");
        return SR_BRANCH_LOOP;
    }
    case STMT_EXPR:
    {
        if(stmt->expr)
            resolve_expr(stmt->expr);
        return SR_NULL;
    }
    default:
        assert(false);
        return {};
    }
}

void resolve_function(ic_function& function)
{
    ctx.return_type = function.return_type;
    ctx.leaf = true;
    ctx.push_scope();

    for(int i = 0; i < function.params_size; ++i)
    {
        ic_param param = function.params[i];
        ctx.add_var(param.id_token, param.type);
    }

    ic_stmt_result result = resolve_stmt(function.body);

    if(result == SR_NULL && !is_void_type(function.return_type))
        ic_exit(function.id_token.line, function.id_token.col, "return statement missing");

    ctx.pop_scope();
    function.leaf = ctx.leaf;
}

void sema(array<ic_function> functions, array<ic_struct> structures)
{
    ctx.functions = functions;
    ctx.structures = structures;
    ctx.vars.init();
    ctx.scopes.init();
    ctx.loop_count = 0;

    for(ic_function& f: functions)
    {
        resolve_function(f);
        assert(ctx.vars.size == 0);
        assert(ctx.scopes.size == 0);
        assert(ctx.loop_count == 0);
    }
}
