#include <unistd.h>
#include <llvm-c/Core.h>
#include "ic.h"

struct ic_scope
{
    int prev_vars_size;
};

struct
{
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    array<LLVMValueRef> functions;
    array<LLVMValueRef> vars;
    array<ic_scope> scopes;

    void push_scope()
    {
        scopes.push_back({vars.size});
    }

    void pop_scope()
    {
        assert(scopes.size);
        vars.resize(scopes.back().prev_vars_size);
        scopes.pop_back();
    }

    void add_var(LLVMValueRef v)
    {
        vars.push_back(v);
    }

} ctx;

LLVMTypeRef get_llvm_type(ic_type type)
{
    (void)type;
    return LLVMInt32Type();
}

LLVMValueRef gen_expr(ic_expr* expr, bool load = true)
{
    assert(expr);

    switch(expr->type)
    {
    case EXPR_INT_LITERAL:
        return LLVMConstInt(LLVMInt32Type(), expr->int_literal, false);
    case EXPR_VAR_ID:
    {
        LLVMValueRef ptr = ctx.vars[expr->var_id];

        if(load)
            return LLVMBuildLoad(ctx.builder, ptr, "");
        return ptr;
    }
    case EXPR_CALL:
    {
        array<LLVMValueRef> args;
        args.init();
        ic_expr* it = expr->call.args;

        while(it)
        {
            args.push_back(gen_expr(it));
            it = it->next;
        }
        return LLVMBuildCall(ctx.builder, ctx.functions[expr->call.fun_id], args.begin(), args.size, "");
    }
    case EXPR_ADD:
    {
        LLVMValueRef lhs = gen_expr(expr->binary.lhs);
        LLVMValueRef rhs = gen_expr(expr->binary.rhs);
        return LLVMBuildAdd(ctx.builder, lhs, rhs, "");
    }
    case EXPR_MUL:
    {
        LLVMValueRef lhs = gen_expr(expr->binary.lhs);
        LLVMValueRef rhs = gen_expr(expr->binary.rhs);
        return LLVMBuildMul(ctx.builder, lhs, rhs, "");
    }
    case EXPR_ASSIGN:
    {
        LLVMValueRef lhs = gen_expr(expr->binary.lhs, false);
        LLVMValueRef rhs = gen_expr(expr->binary.rhs);
        LLVMBuildStore(ctx.builder, rhs, lhs);
        return LLVMBuildLoad(ctx.builder, lhs, "");
    }
    default:
        assert(false);
        return {};
    }
}

void gen_stmt(ic_stmt* stmt)
{
    assert(stmt);

    switch(stmt->type)
    {
    case STMT_COMPOUND:
    {
        if(stmt->compound.push_scope)
            ctx.push_scope();
        ic_stmt* it = stmt->compound.body;

        while(it)
        {
            gen_stmt(it);
            it = it->next;
        }
        if(stmt->compound.push_scope)
            ctx.pop_scope();
        break;
    }
    case STMT_RETURN:
        if(stmt->expr)
        {
            LLVMValueRef tmp = gen_expr(stmt->expr);
            LLVMBuildRet(ctx.builder, tmp);
        }
        else
            LLVMBuildRetVoid(ctx.builder);
        break;
    case STMT_EXPR:
        if(stmt->expr)
            gen_expr(stmt->expr);
        break;
    case STMT_VAR_DECL:
    {
        LLVMTypeRef ltype = get_llvm_type(stmt->var_decl.type);
        LLVMValueRef var = LLVMBuildAlloca(ctx.builder, ltype, "");
        ctx.add_var(var);

        if(stmt->var_decl.init_expr)
        {
            LLVMValueRef tmp = gen_expr(stmt->var_decl.init_expr);
            LLVMBuildStore(ctx.builder, tmp, var);
        }
        break;
    }
    default:
        assert(false);
    }
}

char* make_cstr(ic_token token)
{
    char* str = (char*)malloc(token.str.len + 1);
    memcpy(str, token.str.data, token.str.len);
    str[token.str.len] = '\0';
    return str;
}

void gen_llvm(array<ic_function> functions, array<ic_struct> structures)
{
    (void)structures;
    ctx.module = LLVMModuleCreateWithName("");
    ctx.builder = LLVMCreateBuilder();
    ctx.functions.init();
    ctx.vars.init();
    ctx.scopes.init();

    array<LLVMTypeRef> param_types;
    param_types.init();

    for(ic_function& fun: functions)
    {
        assert(ctx.scopes.size == 0);
        assert(ctx.vars.size == 0);
        ctx.scopes.push_back();
        param_types.size = 0;
        LLVMTypeRef return_type = get_llvm_type(fun.return_type);

        for(int i = 0; i < fun.params_size; ++i)
        {
            ic_param param = fun.params[i];
            param_types.push_back(get_llvm_type(param.type));
        }
        LLVMTypeRef fun_type = LLVMFunctionType(return_type, param_types.begin(), param_types.size, false);
        LLVMValueRef lfun = LLVMAddFunction(ctx.module, make_cstr(fun.id_token), fun_type);
        ctx.functions.push_back(lfun);
        LLVMBasicBlockRef entry = LLVMAppendBasicBlock(lfun, "");
        LLVMPositionBuilderAtEnd(ctx.builder, entry);

        for(int i = 0; i < fun.params_size; ++i)
        {
            LLVMValueRef var = LLVMBuildAlloca(ctx.builder, param_types[i], "");
            LLVMValueRef param = LLVMGetParam(lfun, i);
            LLVMBuildStore(ctx.builder, param, var);
            ctx.add_var(var);
        }
        gen_stmt(fun.body);
        ctx.pop_scope();
    }
    dup2(1, 2);
    LLVMDumpModule(ctx.module);
}
