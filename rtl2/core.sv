`define ALU_ADD 0
`define ALU_SUB 1
`define ALU_AND 2
`define ALU_OR  3
`define ALU_XOR 4
`define ALU_NOR 5
`define ALU_SLL 6
`define ALU_SRL 7
`define ALU_SRA 8
`define ALU_MUL 9
`define ALU_DIV 10

module core(
    input clk,
    input reset,
    output [31:0] dout,
    output dout_ready
    );

    // outputs

    // rom
    wire [31:0] instr;

    // decoder
    wire [5:0] opcode, rtype_opcode;
    wire [4:0] reg1, reg2, reg3;
    wire [15:0] imm16;

    //control unit
    wire [5:0] alu_opcode;
    wire [1:0] sel_reg_din;
    wire sel_reg_write, sel_alu_rhs, sel_branch_addr, we_reg, we_ram, sign_extend_imm, beq, bne, blt, bltu, bgt, bgtu;
    wire update_pc, reset_alu;

    // register file
    wire [31:0] reg_dout1, reg_dout2;

    // alu
    wire [31:0] alu_dout;
    wire borrow;

    // ram
    wire [31:0] ram_dout;

    // branch unit
    wire branch_en;



    // additional inputs, state

    reg [31:0] pc;
    wire [31:0] pc_inc1, pc_next;
    reg [31:0] reg_din;
    wire [4:0] reg_write = sel_reg_write ? reg3 : reg2;
    wire [31:0] imm32 = sign_extend_imm ? { {16{imm16[15]}}, imm16} : { {16{1'b0}}, imm16};
    wire [31:0] alu_din_rhs = sel_alu_rhs ? imm32 : reg_dout2;
    wire [31:0] branch_addr = sel_branch_addr ? reg_dout1 : imm32;



    // additional logic

    // IO, writes at the address 0 are used to communicate with the outside world
    assign dout = reg_dout2;
    assign dout_ready = ~|alu_dout & we_ram;

    add32 add32(pc, 32'd1, pc_inc1);

    assign pc_next = update_pc ? (branch_en ? branch_addr : pc_inc1) : pc;

    always_comb begin
        if(sel_reg_din == 0)
            reg_din = alu_dout;
        else if(sel_reg_din == 1)
            reg_din = ram_dout;
        else
            reg_din = pc_inc1;
    end

    always_ff @(posedge clk) begin
        pc <= reset ? 32'd0 : pc_next;
    end



    // modules

    rom rom(pc, instr);

    instr_decoder instr_decoder(instr, opcode, rtype_opcode, reg1, reg2, reg3, imm16);

    control_unit control_unit(clk, reset, opcode, rtype_opcode, alu_opcode, sel_reg_din, sel_reg_write, sel_alu_rhs,
        sel_branch_addr, we_reg, we_ram, sign_extend_imm, beq, bne, blt, bltu, bgt, bgtu, update_pc, reset_alu);

    register_file register_file(clk, reg_din, we_reg, reg_write, reg1, reg2, reg_dout1, reg_dout2);

    alu alu(clk, reset_alu, alu_opcode, reg_dout1, alu_din_rhs, alu_dout, borrow);

    ram ram(clk, alu_dout, reg_dout2, we_ram, ram_dout);

    branch_unit branch_unit(~|alu_dout, borrow, reg_dout1[31], alu_din_rhs[31], beq, bne, blt, bltu, bgt, bgtu, branch_en);

endmodule

module branch_unit(
    input zero,
    input borrow,
    input sign_lhs,
    input sign_rhs,
    input beq,
    input bne,
    input blt,
    input bltu,
    input bgt,
    input bgtu,
    output branch_en
    );

    wire beq_cond, bne_cond, blt_cond, bltu_cond, bgt_cond, bgtu_cond;

    assign branch_en = (beq & beq_cond)  | (bne & bne_cond) | (blt & blt_cond) | (bltu & bltu_cond) | (bgt & bgt_cond) | (bgtu & bgtu_cond);

    assign beq_cond = zero & ~borrow;
    assign bne_cond = ~zero | borrow;
    assign blt_cond = (sign_lhs & ~sign_rhs) | borrow;
    assign bltu_cond = borrow;
    assign bgt_cond = ~beq_cond & ~blt_cond;
    assign bgtu_cond = ~beq_cond & ~bltu_cond;

endmodule

module instr_decoder(
    input [31:0] din,
    output [5:0] opcode,
    output [5:0] rtype_opcode,
    output [4:0] reg1,
    output [4:0] reg2,
    output [4:0] reg3,
    output [15:0] imm16
    );

    assign opcode = din[31:26];
    assign rtype_opcode = din[10:5];
    assign reg1 = din[25:21];
    assign reg2 = din[20:16];
    assign reg3 = din[15:11];
    assign imm16 = din[15:0];

endmodule

module control_unit(
    input clk,
    input reset,
    input [5:0] opcode,
    input [5:0] rtype_opcode,
    output reg [5:0] alu_opcode,
    output reg [1:0] sel_reg_din,
    output reg sel_reg_write,
    output reg sel_alu_rhs,
    output reg sel_branch_addr,
    output reg we_reg,
    output reg we_ram,
    output reg sign_extend_imm,
    output reg beq,
    output reg bne,
    output reg blt,
    output reg bltu,
    output reg bgt,
    output reg bgtu,
    output reg update_pc,
    output reg reset_alu
    );

    reg [5:0] counter;

    always_ff @(posedge clk) begin
        if(reset)
            counter <= 0;
        else
            counter <= update_pc ? 0 : counter + 1;
    end

    always_comb begin
        alu_opcode = 0;
        sel_reg_din = 0;
        sel_reg_write = 0;
        sel_alu_rhs = 0;
        sel_branch_addr = 0;
        we_reg = 0;
        we_ram = 0;
        sign_extend_imm = 0;
        beq = 0;
        bne = 0;
        blt = 0;
        bltu = 0;
        bgt = 0;
        bgtu = 0;
        update_pc = 1;
        reset_alu = 0;

        case(opcode)
            0: begin // r-tytpe
                alu_opcode = rtype_opcode;
                sel_reg_write = 1;

                if( (rtype_opcode == `ALU_MUL) | (rtype_opcode == `ALU_DIV) ) begin
                    if(counter == 32)
                        we_reg = 1;
                    else begin
                        update_pc = 0;
                        reset_alu = ~|counter;
                    end
                end else
                    we_reg = 1;
            end

            // i-type
            1: begin // beq
                alu_opcode = `ALU_SUB;
                beq = 1;
            end
            2: begin // bne
                alu_opcode = `ALU_SUB;
                bne = 1;
            end
            3: begin // blt
                alu_opcode = `ALU_SUB;
                blt = 1;
            end
            4: begin // bltu
                alu_opcode = `ALU_SUB;
                bltu = 1;
            end
            5: begin // ble
                alu_opcode = `ALU_SUB;
                beq = 1;
                blt = 1;
            end
            6: begin // bleu
                alu_opcode = `ALU_SUB;
                beq = 1;
                bltu = 1;
            end
            7: begin // bgt
                alu_opcode = `ALU_SUB;
                bgt = 1;
            end
            8: begin // bgtu
                alu_opcode = `ALU_SUB;
                bgtu = 1;
            end
            9: begin // bge
                alu_opcode = `ALU_SUB;
                beq = 1;
                bgt = 1;
            end
            10: begin // bgeu
                alu_opcode = `ALU_SUB;
                beq = 1;
                bgtu = 1;
            end
            11: begin // b
                beq = 1;
                bne = 1;
            end
            12: begin // ldr
                alu_opcode = `ALU_ADD;
                sel_reg_din = 1;
                sel_alu_rhs = 1;
                we_reg = 1;
                sign_extend_imm = 1;
            end
            13: begin // str
                alu_opcode = `ALU_ADD;
                sel_alu_rhs = 1;
                we_ram = 1;
                sign_extend_imm = 1;
            end
            14: begin // addi
                alu_opcode = `ALU_ADD;
                sel_alu_rhs = 1;
                we_reg = 1;
                sign_extend_imm = 1;
            end
            15: begin // andi
                alu_opcode = `ALU_AND;
                sel_alu_rhs = 1;
                we_reg = 1;
            end
            16: begin // ori
                alu_opcode = `ALU_OR;
                sel_alu_rhs = 1;
                we_reg = 1;
            end
            17: begin // xori
                alu_opcode = `ALU_XOR;
                sel_alu_rhs = 1;
                we_reg = 1;
            end
            18: begin // slli
                alu_opcode = `ALU_SLL;
                sel_alu_rhs = 1;
                we_reg = 1;
            end
            19: begin // srli
                alu_opcode = `ALU_SRL;
                sel_alu_rhs = 1;
                we_reg = 1;
            end
            20: begin // srai
                alu_opcode = `ALU_SRA;
                sel_alu_rhs = 1;
                we_reg = 1;
            end
            21: begin // muli
                alu_opcode = `ALU_MUL;
                sel_alu_rhs = 1;
                sign_extend_imm = 1;

                if(counter == 32)
                    we_reg = 1;
                else begin
                    update_pc = 0;
                    reset_alu = ~|counter;
                end
            end
            22: begin // divi
                alu_opcode = `ALU_DIV;
                sel_alu_rhs = 1;
                sign_extend_imm = 1;

                if(counter == 32)
                    we_reg = 1;
                else begin
                    update_pc = 0;
                    reset_alu = ~|counter;
                end
            end
            23: begin // bl
                sel_reg_din = 2;
                we_reg = 1;
                beq = 1;
                bne = 1;
            end
            24: begin // bx
                sel_branch_addr = 1;
                beq = 1;
                bne = 1;
            end
        endcase
    end

endmodule

module register_file(
    input clk,
    input [31:0] din,
    input we,
    input [4:0] reg_write,
    input [4:0] reg_read1,
    input [4:0] reg_read2,
    output [31:0] dout1,
    output [31:0] dout2
    );

    reg [31:0] reg_array[31:1];
    wire [31:0] r0 = 32'd0;

    assign dout1 = |reg_read1 ? reg_array[reg_read1] : r0;
    assign dout2 = |reg_read2 ? reg_array[reg_read2] : r0;

    always_ff @(posedge clk) begin
        if(we & |reg_write) // don't write to r0
            reg_array[reg_write] <= din;
    end

endmodule

module alu(
    input clk,
    input reset,
    input [5:0] opcode,
    input [31:0] din_lhs,
    input [31:0] din_rhs,
    output reg [31:0] dout,
    output borrow
    );

    reg [31:0] sum, diff, mul;
    wire [4:0] shift_count = din_rhs[4:0];
    wire [31:0] srl; // shift right logical
    
    add32 add32(din_lhs, din_rhs, sum);
    sub32 sub32(din_lhs, din_rhs, diff, borrow);
    mul32 mul32(clk, reset, din_lhs, din_rhs, mul);
    assign srl = din_lhs >> shift_count;

    always_comb begin
        case(opcode)
            `ALU_ADD: dout = sum;
            `ALU_SUB: dout = diff;
            `ALU_AND: dout = din_lhs & din_rhs;
            `ALU_OR:  dout = din_lhs | din_rhs;
            `ALU_XOR: dout = din_lhs ^ din_rhs;
            `ALU_NOR: dout = ~(din_lhs | din_rhs);
            // todo, use own shifter modules
            `ALU_SLL: dout = din_lhs << shift_count;
            `ALU_SRL: dout = srl;
            `ALU_SRA: dout = {din_lhs[31], srl[30:0]}; // shift right arithmetic
            `ALU_MUL: dout = mul;
            `ALU_DIV: dout = mul; // todo
            default:  dout = 0;
        endcase
    end

endmodule

module rom(
    input [31:0] addr,
    output [31:0] dout
    );

    reg [31:0] reg_array[255:0];

    assign dout = reg_array[addr[7:0]];

    initial $readmemh("a.hex", reg_array);

endmodule

module ram(
    input clk,
    input [31:0] addr,
    input [31:0] din,
    input we, // write enable
    output [31:0] dout
    );

    reg [31:0] reg_array[255:0];

    assign dout = reg_array[addr[7:0]];

    always_ff @(posedge clk) begin
        if(we)
            reg_array[addr[7:0]] <= din;
    end

endmodule

module mul32(
    input clk,
    input reset,
    input [31:0] a,
    input [31:0] b,
    output [31:0] product
    );

    reg sign;
    reg [31:0] sum, ra, rb;
    wire [31:0] a_neg, b_neg, sum_next, sum_next_neg;

    add32 add1(~a, 1, a_neg);
    add32 add2(~b, 1, b_neg);
    add32 add3(sum, ra & {32{rb[0]}}, sum_next);
    add32 add4(~sum_next, 1, sum_next_neg);

    assign product = sign ? sum_next_neg : sum_next;

    always_ff @(posedge clk) begin
        if(reset) begin
            sign <= a[31] ^ b[31];
            ra <= a[31] ? a_neg : a;
            rb <= b[31] ? b_neg : b;
            sum <= 0;
        end else begin
            ra <= ra << 1;
            rb <= rb >> 1;
            sum <= sum_next;
        end
    end

endmodule

// todo, add32 and sub32 could be merged into a one module; currently input/output carries are not needed
module add32(
    input [31:0] a,
    input [31:0] b,
    output [31:0] sum
    );

    wire [32:0] carry;

    assign carry[0] = 0;

    genvar i;
    generate
        for(i = 0; i < 32; i = i + 1)
            add1 add1(a[i], b[i], carry[i], sum[i], carry[i + 1]);
    endgenerate

endmodule

module sub32(
    input [31:0] a,
    input [31:0] b,
    output [31:0] diff,
    output bout
    );

    wire [32:0] borrow;

    assign borrow[0] = 0;
    assign bout = borrow[32];

    genvar i;
    generate
        for(i = 0; i < 32; i = i + 1)
            sub1 sub1(a[i], b[i], borrow[i], diff[i], borrow[i + 1]);
    endgenerate

endmodule

module add1(
    input a,
    input b,
    input cin,
    output sum,
    output cout
    );

    assign sum = a ^ b ^ cin;
    assign cout = (a & b) | (cin & (a | b));

endmodule

module sub1(
    input a,
    input b,
    input bin, // borrow
    output diff,
    output bout
    );

    assign diff = a ^ b ^ bin;
    assign bout = (~a & bin) | (~a & b) | (b & bin);

endmodule
