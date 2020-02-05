#include "ic.h"
#include <stdio.h>

struct ic_reg
{
    int id;

    bool operator==(ic_reg rhs)
    {
        return id == rhs.id;
    }
};

enum ic_instr_type
{
    I_ASSIGN,
    I_ADD,
    I_SUB,
    I_MUL,
    I_DIV,
    I_INT_LITERAL,
};

struct ic_instr
{
    ic_instr_type type;
    ic_reg dst;
    ic_reg src1;
    ic_reg src2;
    int int_literal;
};

struct ic_scope
{
    int prev_vars_size;
};

struct
{
    array<ic_reg> vars;
    array<ic_instr> instructions;
    array<ic_scope> scopes;
    int regs_size;

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

    void add_var()
    {
        vars.push_back(alloc_reg());
    }

    ic_reg build_add(ic_reg a, ic_reg b)
    {
        return build_binary(I_ADD, a, b);
    }

    ic_reg build_sub(ic_reg a, ic_reg b)
    {
        return build_binary(I_SUB, a, b);
    }

    ic_reg build_mul(ic_reg a, ic_reg b)
    {
        return build_binary(I_MUL, a, b);
    }

    ic_reg build_div(ic_reg a, ic_reg b)
    {
        return build_binary(I_DIV, a, b);
    }

    void build_assign(ic_reg dst, ic_reg src)
    {
        ic_instr instr;
        instr.type = I_ASSIGN;
        instr.dst = dst;
        instr.src1 = src;
        instr.src2.id = -1;
        instructions.push_back(instr);
    }


    ic_reg build_int_literal(int val)
    {
        ic_reg dst = alloc_reg();
        ic_instr instr;
        instr.type = I_INT_LITERAL;
        instr.dst = dst;
        instr.int_literal = val;
        instr.src1.id = -1;
        instr.src2.id = -1;
        instructions.push_back(instr);
        return dst;
    }

    // internal

    ic_reg build_binary(ic_instr_type type, ic_reg lhs, ic_reg rhs)
    {
        ic_reg dst = alloc_reg();
        ic_instr instr;
        instr.type = type;
        instr.dst = dst;
        instr.src1 = lhs;
        instr.src2 = rhs;
        instructions.push_back(instr);
        return dst;
    }

    ic_reg alloc_reg()
    {
        ic_reg reg = {regs_size};
        ++regs_size;
        return reg;
    }
} ctx;

ic_reg build_expr(ic_expr* expr)
{
    assert(expr);

    switch(expr->type)
    {
    case EXPR_ADD:
    {
        ic_reg lhs = build_expr(expr->binary.lhs);
        ic_reg rhs = build_expr(expr->binary.rhs);
        return ctx.build_add(lhs, rhs);
    }
    case EXPR_SUB:
    {
        ic_reg lhs = build_expr(expr->binary.lhs);
        ic_reg rhs = build_expr(expr->binary.rhs);
        return ctx.build_sub(lhs, rhs);
    }
    case EXPR_MUL:
    {
        ic_reg lhs = build_expr(expr->binary.lhs);
        ic_reg rhs = build_expr(expr->binary.rhs);
        return ctx.build_mul(lhs, rhs);
    }
    case EXPR_DIV:
    {
        ic_reg lhs = build_expr(expr->binary.lhs);
        ic_reg rhs = build_expr(expr->binary.rhs);
        return ctx.build_div(lhs, rhs);
    }
    case EXPR_INT_LITERAL:
        return ctx.build_int_literal(expr->int_literal);
    case EXPR_VAR_ID:
        return ctx.vars[expr->var_id];
    case EXPR_ASSIGN:
    {
        ic_reg lhs = build_expr(expr->binary.lhs);
        ic_reg rhs = build_expr(expr->binary.rhs);
        ctx.build_assign(lhs, rhs);
        return lhs;
    }
    default:
        assert(false);
        return {};
    }
}

void build_stmt(ic_stmt* stmt)
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
            build_stmt(it);
            it = it->next;
        }
        if(stmt->compound.push_scope)
            ctx.pop_scope();
        return;
    }
    case STMT_EXPR:
        if(stmt->expr)
            build_expr(stmt->expr);
        return;
    case STMT_VAR_DECL:
    {
        ctx.add_var();

        if(stmt->var_decl.init_expr)
        {
            ic_reg dst = ctx.vars.back();
            ic_reg src = build_expr(stmt->var_decl.init_expr);
            ctx.build_assign(dst, src);
        }
        return;
    }
    default:
        assert(false);
    }
}

#define IC_REGS_SIZE 3
#define IC_MAX_DELAY 1000000

struct
{
    ic_reg regs[IC_REGS_SIZE];
    array<ic_reg> mem;

    int allocate_reg(bool ld, ic_reg new_reg, ic_instr* begin, ic_instr* end)
    {
        if(new_reg.id == -1)
            return {};

        for(ic_reg& reg: regs)
        {
            // already loaded
            if(reg == new_reg)
                return &reg - regs;
        }

        for(ic_reg& reg: regs)
        {
            // take an unused register
            if(reg.id == -1)
            {
                assert(!ld); // if there are unused register it means that register to load was not spilled yet, and must be alrady loaded
                reg = new_reg;
                return &reg - regs;
            }
        }

        // spill a register with the longest use delay
        int delays[IC_REGS_SIZE];

        for(int& delay: delays)
            delay = IC_MAX_DELAY;

        for(ic_instr* it = begin; it != end; ++it)
        {
            ic_instr instr = *it;
            int delay = it - begin;

            for(ic_reg& reg: regs)
            {
                if(reg == instr.dst || reg == instr.src1 || reg == instr.src2)
                {
                    int id = &reg - regs;
                    if(delays[id] > delay)
                        delays[id] = delay;
                }
            }
        }

        int id_spill;
        int max_delay = delays[0];

        for(int& delay: delays)
        {
            if(delay >= max_delay)
            {
                id_spill = &delay - delays;
                max_delay = delay;
            }
        }

        // no need to spill, register data will never be used again and can be discarded
        if(max_delay == IC_MAX_DELAY)
        {
            if(ld)
                load(id_spill, new_reg);
            regs[id_spill] = new_reg;
            return id_spill;
        }

        // find an unused memory or push sp
        bool done = false;

        for(ic_reg& cell: mem)
        {
            if(cell.id == -1)
            {
                done = true;
                cell = regs[id_spill];
                emit_store(id_spill, &cell - mem.begin());
                break;
            }
        }

        if(!done)
        {
            int cell_id = mem.size;
            mem.push_back(regs[id_spill]);
            emit_push_sp();
            emit_store(id_spill, cell_id);

        }
        if(ld)
            load(id_spill, new_reg);

        regs[id_spill] = new_reg;
        return id_spill;
    }

    void load(int reg_id, ic_reg reg)
    {
        int mem_id = -1;

        for(ic_reg& cell: mem)
        {
            if(cell == reg)
            {
                cell.id = -1;
                mem_id = &cell - mem.begin();
                break;
            }
        }
        assert(mem_id != -1);
        printf("    ldr r%d, %d(fp)\n", reg_id + 1, -mem_id - 1); // + 1 - r0 is hardwired to 0
    }

    void emit_push_sp()
    {
        printf("    addi sp, r0, -1\n");
    }

    void emit_store(int reg_idx, int mem_idx)
    {
        printf("    str r%d, %d(fp)\n", reg_idx + 1, -mem_idx - 1);
    }
} hw;

void generate_asm(array<ic_instr> instructions)
{
    hw.mem.init();

    for(ic_reg& reg: hw.regs)
        reg.id = -1;

    printf("    addi fp, r0, 255\n");
    printf("    addi sp, r0, 255\n");

    int last_dst;

    for(ic_instr& instr: instructions)
    {
        // r0 is hardwired to 0
        int dst = 1 + hw.allocate_reg(false, instr.dst, &instr, instructions.end());
        int src1 = 1 + hw.allocate_reg(true, instr.src1, &instr, instructions.end());
        int src2 = 1 + hw.allocate_reg(true, instr.src2, &instr, instructions.end());

        last_dst = dst;

        switch(instr.type)
        {
        case I_ASSIGN:
            printf("    add r%d, r%d, r0\n", dst, src1);
            break;
        case I_ADD:
            printf("    add r%d, r%d, r%d\n", dst, src1, src2);
            break;
        case I_SUB:
            printf("    sub r%d, r%d, r%d\n", dst, src1, src2);
            break;
        case I_MUL:
            printf("    mul r%d, r%d, r%d\n", dst, src1, src2);
            break;
        case I_DIV:
            printf("    div r%d, r%d, r%d\n", dst, src1, src2);
            break;
        case I_INT_LITERAL:
            printf("    addi r%d, r0, %d\n", dst, instr.int_literal);
            break;
        default:
            assert(false);
        }
    }
    printf("    str r%d, 0(r0)\n", last_dst);
    printf("end_loop:\n");
    printf("    b end_loop\n");
}

void gen_mycore(array<ic_function> functions, array<ic_struct> structures)
{
    (void)structures;
    ctx.vars.init();
    ctx.instructions.init();
    ctx.scopes.init();
    ctx.regs_size = 0;
    build_stmt(functions[0].body);
    generate_asm(ctx.instructions);
    return;
}
