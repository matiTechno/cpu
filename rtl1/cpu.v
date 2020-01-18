module mod_cpu(input clk, input reset, output dummy); // dummy, so yosys does not optimize out the cpu

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

    assign dummy = A_data[0];
    assign alu_rhs = alu_rhs_sel ? ram_data : A_data;
    assign mem_in_data = mem_in_sel ? imm_data : alu_result;
    assign jump = (alu_result[15] & jump_l0) | (~|alu_result[15:0] & jump_e0) | (~alu_result[15] & |alu_result[14:0] & jump_g0);

    mod_program_counter pc(
        clk,
        reset,
        jump,
        A_data,
        instr_addr);

    mod_program_memory program_memory(
        instr_addr,
        instr);

    mod_instr_decoder decoder(
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

    mod_alu alu(
        alu_zero_lhs,
        alu_invert_lhs,
        alu_zero_rhs,
        alu_invert_rhs,
        alu_opcode,
        alu_invert_result,
        D_data,
        alu_rhs,
        alu_result);

    mod_memory memory(
        clk,
        store_ram,
        store_D,
        store_A,
        mem_in_data,
        ram_data,
        A_data,
        D_data);

endmodule

module mod_program_memory(
    input [15:0] addr,
    output [15:0] instr);

    // note, 8 byte addr limit
    reg[15:0] rom[255:0];

    assign instr = rom[addr[7:0]];

endmodule

module mod_memory(
    input clk,
    input store_ram,
    input store_D,
    input store_A,
    input [15:0] in_data,
    output [15:0] ram_data,
    output reg [15:0] reg_A,
    output reg [15:0] reg_D);

    // note, 8 byte addr limit
    reg [15:0] ram[255:0];
    wire [7:0] addr;

    assign addr = reg_A[7:0];
    assign ram_data = ram[addr];

    always @(posedge clk) begin
        if(store_A)
            reg_A <= in_data;

        if(store_D)
            reg_D <= in_data;

        if(store_ram)
            ram[addr] <= in_data;
    end

endmodule

module mod_alu(
    input zero_lhs,
    input invert_lhs,
    input zero_rhs,
    input invert_rhs,
    input opcode,
    input invert_result,
    input [15:0] lhs_operand,
    input [15:0] rhs_operand,
    output [15:0] result);

    wire [15:0] lhs1, rhs1, lhs2, rhs2, result0;

    assign lhs1 = zero_lhs ? 0 : lhs_operand;
    assign rhs1 = zero_rhs ? 0 : rhs_operand;
    assign lhs2 = invert_lhs ? ~lhs1 : lhs1;
    assign rhs2 = invert_rhs ? ~rhs1 : rhs1;
    assign result0 = opcode ? lhs2 + rhs2 : lhs2 & rhs2;
    assign result = invert_result ? ~result0 : result0;

endmodule

module mod_program_counter(
    input clk,
    input reset,
    input jump,
    input [15:0] jump_addr,
    output reg [15:0] instr_addr);

    always @(posedge clk) begin
        if(reset)
            instr_addr <= 0;
        else if(jump)
            instr_addr <= jump_addr;
        else
            instr_addr <= instr_addr + 1;
    end

endmodule

module mod_instr_decoder(
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

    assign jump_l0 = instr[15] & instr[0];
    assign jump_e0 = instr[15] & instr[1];
    assign jump_g0 = instr[15] & instr[2];
    assign store_ram = instr[15] & instr[3];
    assign store_D = instr[15] & instr[4];
    assign store_A = ~instr[15] | instr[5];
    assign alu_invert_result = instr[15] & instr[6];
    assign alu_opcode = instr[15] & instr[7];
    assign alu_invert_rhs = instr[15] & instr[8];
    assign alu_zero_rhs = instr[15] & instr[9];
    assign alu_invert_lhs = instr[15] & instr[10];
    assign alu_zero_lhs = instr[15] & instr[11];
    assign alu_rhs_sel = instr[15] & instr[12];

    assign mem_in_sel = ~instr[15];
    assign imm_data = instr;

endmodule
