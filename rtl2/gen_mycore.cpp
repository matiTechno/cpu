#include <stdio.h>
#include <stdarg.h>
#include "ic.h"

typedef int ic_vid; // virtual register id

enum ic_ir_instr_type
{
    IR_ASSIGN,
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_INT_LITERAL,
    IR_CALL,
    IR_RETURN,
    IR_RETURN_VOID,
    IR_ARG,
    IR_LABEL,
    IR_BEQ,
    IR_BNE,
};

// three-address code

struct ic_ir_instr
{
    ic_ir_instr_type type;
    ic_vid dst;
    ic_vid src1;
    ic_vid src2;
    int int_literal;
    int arg_id;
    ic_str fun_name;
    ic_vid call_ret_vid;
    int label_id;
};

struct ic_scope
{
    int prev_vars_size;
};

struct
{
    array<ic_scope> scopes;
    array<ic_vid> vars;
    array<ic_ir_instr> code;
    int next_vid;
    int next_label_id;

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

    void add_var(ic_vid vid)
    {
        assert(scopes.size);
        vars.push_back(vid);
    }

    void build_label(int label_id)
    {
        ic_ir_instr instr = {};
        instr.type = IR_LABEL;
        instr.label_id = label_id;
        code.push_back(instr);
    }

    // for return void set vid to 0
    void build_return(ic_vid vid)
    {
        ic_ir_instr instr = {};
        instr.type = vid ? IR_RETURN : IR_RETURN_VOID;
        instr.src1 = vid;
        code.push_back(instr);
    }

    ic_vid build_call(ic_str fun_name)
    {
        ic_ir_instr instr = {};
        instr.type = IR_CALL;
        instr.fun_name = fun_name;
        instr.call_ret_vid = alloc_vid();
        code.push_back(instr);
        return instr.call_ret_vid;
    }

    void build_arg(ic_vid src, int arg_id)
    {
        ic_ir_instr instr = {};
        instr.type = IR_ARG;
        instr.src1 = src;
        instr.arg_id = arg_id;
        code.push_back(instr);
    }

    ic_vid build_assign(ic_vid dst, ic_vid src)
    {
        ic_ir_instr instr = {}; // this is important, zero out vids
        instr.type = IR_ASSIGN;
        instr.dst = dst;
        instr.src1 = src;
        code.push_back(instr);
        return dst;
    }

    ic_vid build_add(ic_vid a, ic_vid b)
    {
        return build_binary(IR_ADD, a, b);
    }

    ic_vid build_sub(ic_vid a, ic_vid b)
    {
        return build_binary(IR_SUB, a, b);
    }

    ic_vid build_mul(ic_vid a, ic_vid b)
    {
        return build_binary(IR_MUL, a, b);
    }

    ic_vid build_div(ic_vid a, ic_vid b)
    {
        return build_binary(IR_DIV, a, b);
    }

    ic_vid build_int_literal(int val)
    {
        ic_ir_instr instr = {};
        instr.type = IR_INT_LITERAL;
        instr.dst = alloc_vid();
        instr.int_literal = val;
        code.push_back(instr);
        return instr.dst;
    }

    ic_vid build_binary(ic_ir_instr_type type, ic_vid lhs, ic_vid rhs)
    {
        ic_ir_instr instr = {};
        instr.type = type;
        instr.dst = alloc_vid();
        instr.src1 = lhs;
        instr.src2 = rhs;
        code.push_back(instr);
        return instr.dst;
    }

    ic_vid alloc_vid()
    {
        ic_vid vid = next_vid;
        ++next_vid;
        return vid;
    }

    int alloc_label()
    {
        int label = next_label_id;
        ++next_label_id;
        return label;
    }
} ctx;

ic_vid build_expr(ic_expr* expr)
{
    assert(expr);

    switch(expr->type)
    {
    case EXPR_ASSIGN:
    {
        ic_vid lhs = build_expr(expr->binary.lhs);
        ic_vid rhs = build_expr(expr->binary.rhs);
        return ctx.build_assign(lhs, rhs);
    }
    case EXPR_ADD:
    {
        ic_vid lhs = build_expr(expr->binary.lhs);
        ic_vid rhs = build_expr(expr->binary.rhs);
        return ctx.build_add(lhs, rhs);
    }
    case EXPR_SUB:
    {
        ic_vid lhs = build_expr(expr->binary.lhs);
        ic_vid rhs = build_expr(expr->binary.rhs);
        return ctx.build_sub(lhs, rhs);
    }
    case EXPR_MUL:
    {
        ic_vid lhs = build_expr(expr->binary.lhs);
        ic_vid rhs = build_expr(expr->binary.rhs);
        return ctx.build_mul(lhs, rhs);
    }
    case EXPR_DIV:
    {
        ic_vid lhs = build_expr(expr->binary.lhs);
        ic_vid rhs = build_expr(expr->binary.rhs);
        return ctx.build_div(lhs, rhs);
    }
    case EXPR_CALL:
    {
        ic_expr* it = expr->call.args;
        int arg_id = 0;

        while(it)
        {
            ic_vid vid = build_expr(it);
            ctx.build_arg(vid, arg_id);
            it = it->next;
            ++arg_id;
        }
        return ctx.build_call(expr->token.str);
    }
    case EXPR_INT_LITERAL:
        return ctx.build_int_literal(expr->int_literal);
    case EXPR_VAR_ID:
        return ctx.vars[expr->var_id];
    case EXPR_CMP_EQ:
    case EXPR_CMP_NEQ:
    case EXPR_CMP_GT:
    case EXPR_CMP_GE:
    case EXPR_CMP_LT:
    case EXPR_CMP_LE:
        assert(false);
        break;
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
    case STMT_IF:
    {
        assert(false);
        int label = ctx.alloc_label();
        ic_vid cond = build_expr(stmt->_if.header);
        (void)cond;
        build_stmt(stmt->_if.body_if);
        ctx.build_label(label);

        if(stmt->_if.body_else)
            build_stmt(stmt->_if.body_else);
        return;
    }
    case STMT_EXPR:
        if(stmt->expr)
            build_expr(stmt->expr);
        return;
    case STMT_VAR_DECL:
    {
        if(stmt->var_decl.init_expr)
        {
            ic_vid vid = build_expr(stmt->var_decl.init_expr);
            ctx.add_var(vid);
        }
        else
        {
            ic_vid vid = ctx.alloc_vid();
            ctx.add_var(vid);
            ctx.build_assign(vid, 0);
        }
        return;
    }
    case STMT_RETURN:
    {
        if(stmt->expr)
        {
            ic_vid vid = build_expr(stmt->expr);
            ctx.build_return(vid);
        }
        else
            ctx.build_return(0);
        return;
    }
    default:
        assert(false);
    }
}

// normal reg assignment range
// r0 is never assigned
// [r1:r4] are assigned only for arguments just before a call
// r29, r30, r31 are special registers: fp, sp, lr
#define RA_BEGIN 5
#define RA_END 29

// reg read reange
#define RR_BEGIN 1
#define RR_END 29

#define MAX_DELAY 1000000

// register file size
#define RF_SIZE 32

void print_asm(const char* fmt, ...)
{
    printf("    ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

typedef int ic_pid; // physical register id

struct ic_reg
{
    ic_vid vid;
    bool saved;
    ic_pid pid; // only used fore restoring saved registers
};

struct ic_frame
{
    ic_reg reg_file[RF_SIZE];
    array<ic_reg> memory;
    int fp_offset;
    ic_ir_instr* ip;
    ic_ir_instr* iend;
};

void remove_unused_vids(ic_frame& frame, ic_reg* regs, int regs_size)
{
    array<int> delays;
    delays.init();
    delays.resize(regs_size);

    for(int& i: delays)
        i = MAX_DELAY;

    for(ic_ir_instr* it = frame.ip; it != frame.iend; ++it)
    {
        for(int i = 0; i < regs_size; ++i)
        {
            ic_reg& reg = regs[i];

            if(reg.vid == it->src1 || reg.vid == it->src2) // we don't check dst vid, because it discards previous data
            {
                int delay = it - frame.ip;

                if(delay < delays[i])
                    delays[i] = delay;
            }
        }
    }

    for(int i = 0; i < regs_size; ++i)
    {
        if(delays[i] == MAX_DELAY)
            regs[i].vid = 0;
    }
}

ic_pid find_spill_target(ic_frame& frame)
{
    array<int> delays;
    delays.init();
    delays.resize(RF_SIZE);

    for(int& i: delays)
        i = MAX_DELAY;

    for(ic_ir_instr* it = frame.ip; it != frame.iend; ++it)
    {
        for(int i = RA_BEGIN; i < RA_END; ++i)
        {
            ic_reg& reg = frame.reg_file[i];

            if(reg.saved)
                continue;

            if(reg.vid == it->src1 || reg.vid == it->src2) // we don't check dst vid, because it discards previous data
            {
                int delay = it - frame.ip;

                if(delay < delays[i])
                    delays[i] = delay;
            }
        }
    }

    int max_delay = delays[RA_BEGIN];
    ic_pid pid = RA_BEGIN;

    for(int i = RA_BEGIN; i < RA_END; ++i)
    {
        int delay = delays[i];

        if(delay >= max_delay)
        {
            max_delay = delay;
            pid = i;
        }
    }
    if(max_delay == MAX_DELAY)
        assert(frame.reg_file[pid].saved); // remove_unused_vids() should be called before this function
    return pid;
}

bool is_unused(ic_reg reg)
{
    return !reg.vid && !reg.saved;
}

void spill_reg(ic_frame& frame, ic_pid pid)
{
    assert(!is_unused(frame.reg_file[pid])); // no need to spill unused register

    int offset = 0;

    // find unused reg in memory
    for(ic_reg& reg: frame.memory)
    {
        if(is_unused(reg))
        {
            offset = frame.fp_offset - (&reg - frame.memory.begin());
            reg = frame.reg_file[pid];
            break;
        }
    }
    // no success, allocate new one
    if(!offset)
    {
        print_asm("addi sp, sp, -1");
        frame.memory.push_back(frame.reg_file[pid]);
        offset = frame.fp_offset - (frame.memory.size - 1);
    }

    frame.reg_file[pid].vid = 0;
    frame.reg_file[pid].saved = false;
    print_asm("str r%d, %d(fp)", pid, offset);
}

ic_pid load_into_reg(ic_frame& frame, ic_vid vid)
{
    if(!vid)
        return {};

    // is it already loaded?
    for(int i = RR_BEGIN; i < RR_END; ++i)
    {
        if(frame.reg_file[i].vid == vid)
            return i;
    }

    ic_pid pid = 0;

    // find unused reg
    for(int i = RA_BEGIN; i < RA_END; ++i)
    {
        if(is_unused(frame.reg_file[i]))
        {
            pid = i;
            break;
        }
    }

    if(!pid)
    {
        pid = find_spill_target(frame);
        spill_reg(frame, pid);
    }

    frame.reg_file[pid].vid = vid;
    int offset = 0;

    for(ic_reg& reg: frame.memory)
    {
        if(reg.vid == vid)
        {
            reg.vid = 0; // free memory location
            offset = frame.fp_offset - (&reg - frame.memory.begin());
        }
    }
    assert(offset);
    print_asm("ldr r%d, %d(fp)", pid, offset);
    return pid;
}

ic_pid assign_reg(ic_frame& frame, ic_vid vid)
{
    if(!vid)
        return {};

    // if vid is already assigned, exit; iteration is done over the read range because function parameters are assigned there
    for(int i = RR_BEGIN; i < RR_END; ++i)
    {
        if(frame.reg_file[i].vid == vid)
            return i;
    }

    // invalidate corresponding memory location if exists
    for(ic_reg& reg: frame.memory)
    {
        if(reg.vid == vid)
            reg.vid = 0;
    }

    // find empty reg, so no spilling is necesary
    for(int i = RA_BEGIN; i < RA_END; ++i)
    {
        if(is_unused(frame.reg_file[i]))
        {
            frame.reg_file[i].vid = vid;
            return i;
        }
    }
    ic_pid pid = find_spill_target(frame);
    spill_reg(frame, pid);
    frame.reg_file[pid].vid = vid;
    return pid;
}

void gen_function(ic_function& fun, array<ic_ir_instr> code)
{
    printf("\n__%.*s:\n", fun.id_token.str.len, fun.id_token.str.data);
    print_asm("addi sp, sp, -1");
    print_asm("str fp, 0(sp)");
    print_asm("add fp, sp, r0");

    ic_frame frame;
    frame.memory.init();
    frame.fp_offset = 0;
    frame.iend = code.end();

    for(int i = RR_BEGIN; i < RR_END; ++i)
    {
        ic_reg& reg = frame.reg_file[i];
        reg.pid = i;
        reg.saved = i >= 5;
        reg.vid = 0;
    }

    // initialize function parameters (first 4 in [r1-r4] registers, rest on a stack)
    for(int i = 0; i < fun.params_size; ++i)
    {
        int arg_vid = i + 1; // first allocated vid is 1
        ic_pid pid = i + 1; // skip r0

        if(pid < 5)
            frame.reg_file[pid].vid = arg_vid;
        else
        {
            ++frame.fp_offset;
            ic_reg reg;
            reg.vid = arg_vid;
            reg.saved = false;
            frame.memory.push_back(reg);
        }
    }

    frame.memory.push_back({0, true, 0}); // add dummy entry for fp - easier memory offset calculation

    if(!fun.leaf)
    {
        print_asm("addi sp, sp, -1");
        print_asm("str lr, -1(fp)");
        frame.memory.push_back({0, true, 0}); // same for lr
    }

    for(ic_ir_instr& instr: code)
    {
        frame.ip = &instr;
        remove_unused_vids(frame, frame.memory.begin(), frame.memory.size);
        remove_unused_vids(frame, frame.reg_file + RR_BEGIN, RR_END - RR_BEGIN); // read range, so we can remove unused paramter vids,
        // so a next function call doesn't need to spill them

        // first load src registers, assigning dst can discard previous data if it was in memory
        // functions ignore 0 vids, so it is safe to run load and assign for all instructions, because they are zero initialized

        ic_pid src1 = load_into_reg(frame, instr.src1);
        ic_pid src2 = load_into_reg(frame, instr.src2);
        ic_pid dst = assign_reg(frame, instr.dst);

        switch(instr.type)
        {
        case IR_ASSIGN:
            print_asm("add r%d, r%d, r0", dst, src1);
            break;
        case IR_ADD:
            print_asm("add r%d, r%d, r%d", dst, src1, src2);
            break;
        case IR_SUB:
            print_asm("sub r%d, r%d, r%d", dst, src1, src2);
            break;
        case IR_MUL:
            print_asm("mul r%d, r%d, r%d", dst, src1, src2);
            break;
        case IR_DIV:
            print_asm("div r%d, r%d, r%d", dst, src1, src2);
            break;
        case IR_INT_LITERAL:
            print_asm("addi r%d, r0, %d", dst, instr.int_literal);
            break;
        case IR_CALL:
            print_asm("bl __%.*s", instr.fun_name.len, instr.fun_name.data);
            frame.reg_file[1].vid = instr.call_ret_vid;
            break;
        case IR_RETURN:
            print_asm("add r1, r0, r%d", src1);
            print_asm("b __end__%.*s", fun.id_token.str.len, fun.id_token.str.data);
            break;
        case IR_RETURN_VOID:
            print_asm("b __end__%.*s", fun.id_token.str.len, fun.id_token.str.data);
            break;
        case IR_ARG:
        {
            // first four arguments are passed in the registers, rest on the stack
            if(instr.arg_id < 4)
            {
                ic_pid dst_pid = instr.arg_id + 1;

                if(!is_unused(frame.reg_file[dst_pid]))
                    spill_reg(frame, dst_pid);
                print_asm("add r%d, r0, r%d", dst_pid, src1);
            }
            else
            {
                static int idx; // I don't like this, this is so arguments on a stack do not overwrite each other

                if(instr.arg_id == 4)
                {
                    idx = frame.memory.size;

                    for(int i = idx - 1; i >= 0; --i)
                    {
                        if(!is_unused(frame.memory[i]))
                            break;
                        idx = i;
                    }
                }
                else
                    idx += 1;

                if(idx == frame.memory.size)
                {
                    frame.memory.push_back();
                    print_asm("addi sp, sp, -1");
                }

                int offset = frame.fp_offset - idx;
                print_asm("str r%d, %d(fp)", src1, offset);
            }
            break;
        }
        default:
            assert(false);
        }
    }

    printf("__end__%.*s:\n", fun.id_token.str.len, fun.id_token.str.data);

    // hack, so fp and lr won't be restored twice; fp must be restored at the last one
    frame.memory[frame.fp_offset].saved = false;

    if(!fun.leaf)
    {
        frame.memory[frame.fp_offset + 1].saved = false;
        print_asm("ldr lr, -1(fp)");
    }

    // restore callee saved registers
    for(ic_reg& reg: frame.memory)
    {
        int offset = frame.fp_offset + -(&reg - frame.memory.begin());

        if(reg.saved)
            print_asm("ldr r%d, %d(fp)", reg.pid, offset);
    }
    print_asm("addi sp, fp, 1");
    print_asm("ldr fp, 0(fp)"); // only now restore fp
    print_asm("bx lr");
}

void gen_mycore(array<ic_function> functions, array<ic_struct>)
{
    print_asm("addi sp, r0, 255");
    print_asm("addi fp, r0, 255");
    print_asm("bl __main");
    print_asm("str r1, 0(r0)");
    printf("__end:\n");
    print_asm("b __end");

    ctx.scopes.init();
    ctx.vars.init();
    ctx.code.init();
    ctx.next_label_id = 0;

    for(ic_function& fun: functions)
    {
        assert(!ctx.scopes.size);
        assert(!ctx.vars.size);
        ctx.code.size = 0;
        ctx.next_vid = 1;
        ctx.push_scope();

        for(int i = 0; i < fun.params_size; ++i)
            ctx.add_var(ctx.alloc_vid());

        build_stmt(fun.body);
        ctx.pop_scope();
        gen_function(fun, ctx.code);
    }
}
