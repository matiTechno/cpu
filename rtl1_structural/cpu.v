// first number in a mux / dmux name is a number of select lines, second is a line bit width
// todo, memory row and column decoding - right now I don't know much about high impedance which is necessary to accomplish this
// program calculates !7 and set reg_D_out to a result

module cpu(
    input clk,
    input reset,
    output [15:0] reg_D_out);

    // decoded instr
    wire jump_l0;
    wire jump_e0;
    wire jump_g0;
    //
    wire store_ram;
    wire store_A;
    wire store_D;
    wire [15:0] imm_data;
    wire mem_in_sel;
    //
    wire alu_zero_lhs;
    wire alu_invert_lhs;
    wire alu_zero_rhs;
    wire alu_invert_rhs;
    wire alu_opcode;
    wire alu_invert_result;
    wire alu_rhs_sel;

    wire [15:0] instr_addr;
    wire [15:0] instr;
    wire [15:0] alu_result;
    wire [15:0] ram_data;
    wire [15:0] A_data;
    wire [15:0] D_data;

    wire [15:0] alu_rhs;
    wire [15:0] mem_in_data;
    wire jump;

    assign reg_D_out = D_data;

    _mux1_16 mu1(alu_rhs_sel, A_data, ram_data, alu_rhs);
    _mux1_16 mu2(mem_in_sel, alu_result, imm_data, mem_in_data);

    wire en_jump_l0, en_jump_e0, en_jump_g0;
    wire w1;
    _or or1(en_jump_l0, en_jump_e0, w1);
    _or or2(w1, en_jump_g0, jump);

    // l0 condition
    _and an1(alu_result[15], jump_l0, en_jump_l0);

    // e0 condition
    wire reduced_res, n_reduced_res;
    _reduce_or16 red1(alu_result, reduced_res);
    _not no1(reduced_res, n_reduced_res);
    _and an2(n_reduced_res, jump_e0, en_jump_e0);

    // g0 condition
    wire n_sign, w2;
    _not no2(alu_result[15], n_sign);
    _and an3(n_sign, reduced_res, w2);
    _and an4(w2, jump_g0, en_jump_g0);

    _program_counter pc(
        clk,
        reset,
        jump,
        A_data,
        instr_addr);

    _progmem progmem(
        instr_addr,
        instr);

    _instr_decoder decoder(
        instr,
        jump_l0,
        jump_e0,
        jump_g0,
        store_ram,
        store_A,
        store_D,
        imm_data,
        mem_in_sel,
        alu_zero_lhs,
        alu_invert_lhs,
        alu_zero_rhs,
        alu_invert_rhs,
        alu_opcode,
        alu_invert_result,
        alu_rhs_sel);

    _alu alu(
        alu_zero_lhs,
        alu_invert_lhs,
        alu_zero_rhs,
        alu_invert_rhs,
        alu_opcode,
        alu_invert_result,
        D_data,
        alu_rhs,
        alu_result);

    _memory memory(
        clk,
        store_ram,
        store_D,
        store_A,
        mem_in_data,
        ram_data,
        A_data,
        D_data);

endmodule

module _reduce_or16(
    input [15:0] in16,
    output out);

    wire [7:0] in8;
    wire [3:0] in4;
    wire [1:0] in2;

    genvar i;
    generate
        for(i = 0; i < 16; i = i + 2)
            _or o(in16[i], in16[i + 1], in8[i / 2]);

        for(i = 0; i < 8; i = i + 2)
            _or o(in8[i], in8[i + 1], in4[i / 2]);

        for(i = 0; i < 4; i = i + 2)
            _or o(in4[i], in4[i + 1], in2[i / 2]);

    endgenerate

    _or o(in2[0], in2[1], out);

endmodule

module _instr_decoder(
    input [15:0] instr,
    output jump_l0,
    output jump_e0,
    output jump_g0,
    output store_ram,
    output store_A,
    output store_D,
    output [15:0] imm_data,
    output mem_in_sel,
    output alu_zero_lhs,
    output alu_invert_lhs,
    output alu_zero_rhs,
    output alu_invert_rhs,
    output alu_opcode,
    output alu_invert_result,
    output alu_rhs_sel);

    _and an1(instr[15], instr[0], jump_l0);
    _and an2(instr[15], instr[1], jump_e0);
    _and an3(instr[15], instr[2], jump_g0);
    _and an4(instr[15], instr[3], store_ram);
    _and an5(instr[15], instr[4], store_D);

    _not no1(instr[15], mem_in_sel);
    _or  or1(mem_in_sel, instr[5], store_A);

    _and an6(instr[15], instr[6], alu_invert_result);
    _and an7(instr[15], instr[7], alu_opcode);
    _and an8(instr[15], instr[8], alu_invert_rhs);
    _and an9(instr[15], instr[9], alu_zero_rhs);
    _and an10(instr[15], instr[10], alu_invert_lhs);
    _and an11(instr[15], instr[11], alu_zero_lhs);
    _and an12(instr[15], instr[12], alu_rhs_sel);
    assign imm_data = instr;

endmodule

module _memory(
    input clk,
    input store_ram,
    input store_D,
    input store_A,
    input [15:0] in_data,
    output [15:0] ram_data,
    output [15:0] reg_A,
    output [15:0] reg_D);

    _reg16 A(in_data, store_A, clk, reg_A);
    _reg16 D(in_data, store_D, clk, reg_D);

    wire [2:0] addr;

    assign addr = reg_A[2:0]; // only 8 ram cells

    // pass store signal to ram cells with a demultiplexer
    wire [1:0] store_ram2;
    wire [3:0] store_ram4;
    wire [7:0] store_ram8;

    _dmux1 dm(addr[2], store_ram, store_ram2[0], store_ram2[1]);

    genvar i;
    generate
        for(i = 0; i < 2; i = i + 1)
            _dmux1 dm(addr[1], store_ram2[i], store_ram4[i * 2], store_ram4[i * 2 + 1]);

        for(i = 0; i < 4; i = i + 1)
            _dmux1 dm(addr[0], store_ram4[i], store_ram8[i * 2], store_ram8[i * 2 + 1]);
    endgenerate

    // select ram output data with a multiplexer
    wire [15:0] ram_data8 [7:0];
    wire [15:0] ram_data4 [3:0];
    wire [15:0] ram_data2 [1:0];

    generate
        for(i = 0; i < 8; i = i + 1)
            _reg16 r(in_data, store_ram8[i], clk, ram_data8[i]); // actual memory cells

        for(i = 0; i < 8; i = i + 2)
            _mux1_16 mu(addr[0], ram_data8[i], ram_data8[i + 1], ram_data4[i / 2]);

        for(i = 0; i < 4; i = i + 2)
            _mux1_16 mu(addr[1], ram_data4[i], ram_data4[i + 1], ram_data2[i / 2]);
    endgenerate

    _mux1_16 mu(addr[2], ram_data2[0], ram_data2[1], ram_data);

endmodule

module _dmux1(
    input sel,
    input in,
    output out0,
    output out1);

    wire n_sel;
    _not no(sel, n_sel);
    _and an1(n_sel, in, out0);
    _and an2(sel, in, out1);

endmodule

module _progmem(
    input [15:0] addr,
    output [15:0] instr);

    wire [15:0] data [63:0];
    wire [15:0] out1 [31:0];
    wire [15:0] out2 [15:0];
    wire [15:0] out3 [7:0];
    wire [15:0] out4 [3:0];
    wire [15:0] out5 [1:0];

    // computes factorial 7
    assign data[0] =  16'h0;
    assign data[1] =  16'h8fc8;
    assign data[2] =  16'h2;
    assign data[3] =  16'h8c10;
    assign data[4] =  16'h1;
    assign data[5] =  16'h8308;
    assign data[6] =  16'h1;
    assign data[7] =  16'h9c10;
    assign data[8] =  16'h7;
    assign data[9] =  16'h81d0;
    assign data[10] = 16'h21;
    assign data[11] = 16'h8301;
    assign data[12] = 16'h2;
    assign data[13] = 16'h8fc8;
    assign data[14] = 16'h0;
    assign data[15] = 16'h9c10;
    assign data[16] = 16'h3;
    assign data[17] = 16'h8308;
    assign data[18] = 16'h3;
    assign data[19] = 16'h9c10;
    assign data[20] = 16'h0;
    assign data[21] = 16'h9088;
    assign data[22] = 16'h2;
    assign data[23] = 16'h9dd0;
    assign data[24] = 16'h8308;
    assign data[25] = 16'h1;
    assign data[26] = 16'h91d0;
    assign data[27] = 16'h12;
    assign data[28] = 16'h8304;
    assign data[29] = 16'h1;
    assign data[30] = 16'h9dc8;
    assign data[31] = 16'h6;
    assign data[32] = 16'h8007;
    assign data[33] = 16'h0;
    assign data[34] = 16'h9c10;
    assign data[35] = 16'h23;
    assign data[36] = 16'h8007;

    genvar i;

    generate
        for(i = 0; i < 64; i = i + 2)
            _mux1_16 mu(addr[0], data[i], data[i + 1], out1[i / 2]);

        for(i = 0; i < 32; i = i + 2)
            _mux1_16 mu(addr[1], out1[i], out1[i + 1], out2[i / 2]);

        for(i = 0; i < 16; i = i + 2)
            _mux1_16 mu(addr[2], out2[i], out2[i + 1], out3[i / 2]);

        for(i = 0; i < 8; i = i + 2)
            _mux1_16 mu(addr[3], out3[i], out3[i + 1], out4[i / 2]);

        for(i = 0; i < 4; i = i + 2)
            _mux1_16 mu(addr[4], out4[i], out4[i + 1], out5[i / 2]);
    endgenerate

    _mux1_16 mu(addr[5], out5[0], out5[1], instr);

endmodule

module _program_counter(
    input clk,
    input reset,
    input jump,
    input [15:0] jump_addr,
    output reg[15:0] instr_addr);

    wire [15:0] reg_in;
    wire [15:0] addr0 = 16'b0;
    wire [15:0] addr_next;
    wire [15:0] m1_out;

    _ripple_adder16 adder(instr_addr, 16'b1, addr_next);
    _mux1_16 m1(jump, addr_next, jump_addr, m1_out);
    _mux1_16 m2(reset, m1_out, addr0, reg_in);
    _reg16 r(reg_in, 1'b1, clk, instr_addr);

endmodule

module _reg16(
    input [15:0] data,
    input enable,
    input clk,
    output [15:0] out);

    genvar i;
    generate
        for(i = 0; i < 16; i = i + 1)
            _D_flip_flop ff(data[i], enable, clk, out[i]);
    endgenerate

endmodule

module _D_flip_flop(
    input data,
    input enable,
    input clk,
    output Q);

    // I suspect it may not work because of an icarus verilog bug,
    // commented out and replaced with a reg primitive.
    // Well, it works even worse in verilator, haha. I give up.
    // It was fun exercise but I don't know too many things to debug or evaluate this.
    /*
    wire Q_master, enable_master, enable_slave, n_clk;

    _not no(clk, n_clk);
    _and an1(enable, n_clk, enable_master);
    _and an2(enable, clk, enable_slave);
    _gated_D_latch master(data, enable_master, Q_master);
    _gated_D_latch slave(Q_master, enable_slave, Q);
    */

    reg r;
    assign Q = r;

    always @(posedge clk) begin
        if(enable)
            r <= data;
    end

endmodule


module _gated_D_latch(
    input data,
    input enable,
    output Q);

    wire n_set, n_reset;

    _nand na1(data, enable, n_set);
    _nand na2(n_set, enable, n_reset);
    _sr_nand_latch latch(n_set, n_reset, Q);

endmodule

module _sr_nand_latch(
    input n_set,
    input n_reset,
    output Q);

    wire w1, w2;
    _nand na1(n_set, w1, w2);
    _nand na2(n_reset, w2, w1);
    assign Q = w2;

endmodule

module _alu(
    input zero_lhs,
    input invert_lhs,
    input zero_rhs,
    input invert_rhs,
    input opcode,
    input invert_result,
    input [15:0] lhs_operand,
    input [15:0] rhs_operand,
    output [15:0] result);

    wire [15:0] w1, w2, w3, w4, w5, w6;

    _unary_alu u1(zero_lhs, invert_lhs, lhs_operand, w1);
    _unary_alu u2(zero_rhs, invert_rhs, rhs_operand, w2);

    _and16 an(w1, w2, w3);
    _ripple_adder16 ad(w1, w2, w4);

    _mux1_16 m1(opcode, w3, w4, w5);

    _not16 no(w5, w6);

    _mux1_16 m2(invert_result, w5, w6, result);

endmodule

module _ripple_adder16(
    input [15:0] lhs_operand,
    input [15:0] rhs_operand,
    output [15:0] out);

    wire [15:-1] carry;
    assign carry[-1] = 1'b0;

    genvar i;
    generate
        for(i = 0; i < 16; i = i + 1)
            _full_adder adder(lhs_operand[i], rhs_operand[i], carry[i - 1], out[i], carry[i]);
    endgenerate

endmodule

module _full_adder(
    input a,
    input b,
    input cin,
    output sum,
    output cout);

    wire sum1, carry1, carry2;
    _half_adder ha1(a, b, sum1, carry1);
    _half_adder ha2(sum1, cin, sum, carry2);
    _or o(carry1, carry2, cout);

endmodule

module _half_adder(
    input a,
    input b,
    output sum,
    output carry);

    _xor xo(a, b, sum);
    _and an(a, b, carry);

endmodule

module _unary_alu(
    input zero_out,
    input invert,
    input [15:0] operand,
    output [15:0] out);

    wire [15:0] w1, w2;

    _mux1_16 m1(zero_out, operand, 16'b0, w1);
    _not16 no2(w1, w2);
    _mux1_16 m2(invert, w1, w2, out);

endmodule

module _mux1(
    input sel,
    input in0,
    input in1,
    output out);

    wire w1, w2, w3;

    _not no(sel, w1);
    _and an1(w1, in0, w2);
    _and an2(sel, in1, w3);
    _or o(w2, w3, out);

endmodule

module _mux1_16(
    input sel,
    input [15:0] in0,
    input [15:0] in1,
    output [15:0] out);

    genvar i;
    generate
        for(i = 0; i < 16; i = i + 1)
            _mux1 m(sel, in0[i], in1[i], out[i]);
    endgenerate

endmodule

module _nand(
    input in1,
    input in2,
    output out);

    nand(out, in1, in2); // verilog primitive

endmodule

module _not(
    input in,
    output out);

    _nand na(in, in, out);

endmodule

module _and(
    input in1,
    input in2,
    output out);

    wire w1;

    _nand na(in1, in2, w1);
    _not no(w1, out);

endmodule

module _or(
    input in1,
    input in2,
    output out);

    wire w1, w2, w3;

    _not no1(in1, w1);
    _not no2(in2, w2);
    _and an(w1, w2, w3);
    _not no3(w3, out);

endmodule

module _xor(
    input in1,
    input in2,
    output out);

    wire w1, w2;
    _nand na(in1, in2, w1);
    _or o(in1, in2, w2);
    _and an(w1, w2, out);

endmodule

module _nand16(
    input [15:0] in1,
    input [15:0] in2,
    output [15:0] out);

    genvar i;
    generate
        for(i = 0; i < 16; i = i + 1)
            _nand na(in1[i], in2[i], out[i]);
    endgenerate

endmodule

module _not16(
    input [15:0] in1,
    output [15:0] out);

    genvar i;
    generate
        for(i = 0; i < 16; i = i + 1)
            _not no(in1[i], out[i]);
    endgenerate

endmodule

module _and16(
    input [15:0] in1,
    input [15:0] in2,
    output [15:0] out);

    genvar i;
    generate
        for(i = 0; i < 16; i = i + 1)
            _and a(in1[i], in2[i], out[i]);
    endgenerate

endmodule

module _or16(
    input [15:0] in1,
    input [15:0] in2,
    output [15:0] out);

    genvar i;
    generate
        for(i = 0; i < 16; i = i + 1)
            _or o(in1[i], in2[i], out[i]);
    endgenerate

endmodule

module _xor16(
    input [15:0] in1,
    input [15:0] in2,
    output [15:0] out);

    genvar i;
    generate
        for(i = 0; i < 16; i = i + 1)
            _xor o(in1[i], in2[i], out[i]);
    endgenerate

endmodule
