`define ALU_ADD 0
`define ALU_SUB 1
`define ALU_AND 2
`define ALU_OR  3
`define ALU_XOR 4
`define ALU_NOR 5
`define ALU_SLL 6
`define ALU_SRL 7
`define ALU_SRA 8

module core(
    input clk,
    input reset,
    output [31:0] dout,
    output dout_ready
    );

    wire [31:0] instr_addr, instr;

    rom rom(instr_addr, instr);

    wire [5:0] opcode, rtype_opcode;
    wire [4:0] reg1, reg2, reg3;
    wire [15:0] imm16;

    instr_decoder instr_decoder(instr, opcode, rtype_opcode, reg1, reg2, reg3, imm16);

    wire [5:0] alu_opcode;
    wire sel_reg_write, sel_reg_din, sel_alu_rhs, we_reg, we_ram, beq, bne, blt, bltu, bgt, bgtu;

    control_unit control_unit(opcode, rtype_opcode, alu_opcode, sel_reg_write, sel_reg_din, sel_alu_rhs, we_reg, we_ram,
        beq, bne, blt, bltu, bgt, bgtu);

    wire [31:0] ram_dout, alu_dout;

    wire [31:0] reg_din = sel_reg_din ? ram_dout : alu_dout;
    wire [4:0] reg_write = sel_reg_write ? reg3 : reg2;
    wire [31:0] reg_dout1, reg_dout2;

    register_file register_file(clk, reg_din, we_reg, reg_write, reg1, reg2, reg_dout1, reg_dout2);

    reg [31:0] imm32;

    // sign extend imm16 for arithmetic operations, else zero extend
    always_comb begin
        if(alu_opcode == `ALU_ADD | alu_opcode == `ALU_SUB)
            imm32 = { {16{imm16[15]}}, imm16};
        else
            imm32 = { {16{1'b0}}, imm16};
    end

    wire [31:0] alu_din_rhs = sel_alu_rhs ? imm32 : reg_dout2;
    wire carry, borrow;

    alu alu(alu_opcode, reg_dout1, alu_din_rhs, alu_dout, carry, borrow);

    ram ram(clk, alu_dout, reg_dout2, we_ram, ram_dout);

    wire branch_en;

    branch_unit branch_unit(~|alu_dout, borrow, reg_dout1[31], alu_din_rhs[31], beq, bne, blt, bltu, bgt, bgtu, branch_en);

    pc pc(clk, reset, imm32, branch_en, instr_addr);

    // IO, writes at address 0 are used to communicate with the outside world
    assign dout = reg_dout2;
    assign dout_ready = ~|alu_dout & we_ram;

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
    input [5:0] opcode,
    input [5:0] rtype_opcode,
    output reg [5:0] alu_opcode,
    output reg sel_reg_write,
    output reg sel_reg_din,
    output reg sel_alu_rhs,
    output reg we_reg,
    output reg we_ram,
    output reg beq,
    output reg bne,
    output reg blt,
    output reg bltu,
    output reg bgt,
    output reg bgtu
    );

    always_comb begin
        alu_opcode = 0;
        sel_reg_write = 0;
        sel_reg_din = 0;
        sel_alu_rhs = 0;
        we_reg = 0;
        we_ram = 0;
        beq = 0;
        bne = 0;
        blt = 0;
        bltu = 0;
        bgt = 0;
        bgtu = 0;

        case(opcode)
            0: begin // r-tytpe
                alu_opcode = rtype_opcode;
                sel_reg_write = 1;
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
            end
            13: begin // str
                alu_opcode = `ALU_ADD;
                sel_alu_rhs = 1;
                we_ram = 1;
            end
            14: begin // addi
                alu_opcode = `ALU_ADD;
                sel_alu_rhs = 1;
                we_reg = 1;
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
        endcase
    end

endmodule

module pc(
    input clk,
    input reset,
    input [31:0] din,
    input branch_en,
    output reg [31:0] dout
    );

    wire [31:0] next;
    wire carry;
    add32 add32(dout, 32'd1, next, carry);

    always_ff @(posedge clk) begin
        if(reset)
            dout <= 32'd0;
        else
            dout <= branch_en ? din : next;
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

    reg [31:0] reg_file[31:1];
    wire [31:0] r0 = 32'd0;

    assign dout1 = |reg_read1 ? reg_file[reg_read1] : r0;
    assign dout2 = |reg_read2 ? reg_file[reg_read2] : r0;

    always_ff @(posedge clk) begin
        if(we & |reg_write) // don't write to r0
            reg_file[reg_write] <= din;
    end

endmodule

module alu(
    input [5:0] opcode,
    input [31:0] din_lhs,
    input [31:0] din_rhs,
    output reg [31:0] dout,
    output carry,
    output borrow
    );

    reg [31:0] sum, diff;
    wire [4:0] shift_count = din_rhs[4:0];
    wire [31:0] srl; // shift right logical
    
    add32 add32(din_lhs, din_rhs, sum, carry);
    sub32 sub32(din_lhs, din_rhs, diff, borrow);
    assign srl = din_lhs >> shift_count;

    always_comb begin
        dout = 0;

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
        endcase
    end

endmodule

module rom(
    input [31:0] addr,
    output [31:0] dout
    );

    reg [31:0] reg_file[255:0];

    assign dout = reg_file[addr[7:0]];

    initial $readmemh("a.hex", reg_file);

endmodule

module ram(
    input clk,
    input [31:0] addr,
    input [31:0] din,
    input we, // write enable
    output [31:0] dout
    );

    reg [31:0] reg_file[255:0];

    assign dout = reg_file[addr[7:0]];

    always_ff @(posedge clk) begin
        if(we)
            reg_file[addr[7:0]] <= din;
    end

endmodule

// todo, add32 and sub32 could be merged into a one module
module add32(
    input [31:0] a,
    input [31:0] b,
    output [31:0] sum,
    output cout
    );

    wire [32:0] carry;

    assign carry[0] = 0;
    assign cout = carry[32];

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
