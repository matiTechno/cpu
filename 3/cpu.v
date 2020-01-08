module mod_cpu(input clk, input reset);

    wire [15:0] instr_addr;
    wire [15:0] instr;

    // decoded instr
    wire jump_l0;
    wire jump_e0;
    wire jump_g0;
    // 
    wire store_ram;
    wire store_a;
    wire store_d;
    wire [15:0] instr_data;
    wire data_sel;
    //
    wire alu_zero_lhs;
    wire alu_invert_lhs;
    wire alu_zero_rhs;
    wire alu_invert_rhs;
    wire alu_opcode;
    wire alu_invert_result;
    wire alu_rhs_sel;

    wire [15:0] alu_rhs;
    wire [15:0] alu_result;

    wire do_jump;

    wire [15:0] mem_data;
    wire [15:0] ram_data;
    wire [15:0] a_data;
    wire [15:0] d_data;

    assign alu_rhs = alu_rhs_sel ? ram_data : a_data;
    assign mem_data = data_sel ? instr_data : alu_result;

    mod_rom rom(
        instr_addr,
        instr);

    mod_instr_decoder decoder(
        instr,
        jump_l0,
        jump_e0,
        jump_g0,
        store_ram,
        store_a,
        store_d,
        instr_data,
        data_sel,
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
        d_data,
        alu_rhs,
        alu_result);

    mod_program_counter pc(
        clk,
        reset,
        do_jump,
        a_data,
        instr_addr);

    mod_memory memory(
        clk,
        store_ram,
        store_d,
        store_a,
        mem_data,
        ram_data,
        a_data,
        d_data);

    mod_jump jump(
        alu_result,
        jump_l0,
        jump_e0,
        jump_g0,
        do_jump);

endmodule

module mod_rom(
    input [15:0] addr,
    output [15:0] instr);

    // note, 8 byte addr limit
    reg[15:0] mem[255:0];

    assign instr = mem[addr[7:0]];

endmodule

module mod_memory(
    input clk,
    input store_ram,
    input store_d,
    input store_a,
    input [15:0] data,
    output [15:0] ram_data,
    output reg [15:0] reg_a,
    output reg [15:0] reg_d);

    // note, 8 byte addr limit
    reg [15:0] ram[255:0];
    wire [7:0] addr;

    assign addr = reg_a[7:0];
    assign ram_data = ram[addr];

    always @(posedge clk) begin
        if(store_a)
            reg_a <= data;

        if(store_d)
            reg_d <= data;
    end

    always @(negedge clk) begin
        if(store_ram)
            ram[addr] <= data;
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

module mod_jump(
    input [15:0] data,
    input jump_l0,
    input jump_e0,
    input jump_g0,
    output do_jump);

    assign do_jump = (data[15] & jump_l0) | (!data & jump_e0) | (~data[15] & |data[14:0] & jump_g0); 

endmodule

module mod_program_counter(
    input clk,
    input reset,
    input do_jump,
    input [15:0] jump_addr,
    output reg [15:0] instr_addr);

    always @(posedge clk) begin
        if(reset)
            instr_addr <= 0;
        else if(do_jump)
            instr_addr <= jump_addr;
        else
            instr_addr = instr_addr + 1;
    end

endmodule

module mod_instr_decoder(
    input [15:0] instr,
    output jump_l0,
    output jump_e0,
    output jump_g0,
    output store_ram,
    output store_a,
    output store_d,
    output [15:0] data,
    output data_sel,
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
    assign store_d = instr[15] & instr[4];
    assign store_a = ~instr[15] | instr[5];
    assign alu_invert_result = instr[15] & instr[6];
    assign alu_opcode = instr[15] & instr[7];
    assign alu_invert_rhs = instr[15] & instr[8];
    assign alu_zero_rhs = instr[15] & instr[9];
    assign alu_invert_lhs = instr[15] & instr[10];
    assign alu_zero_lhs = instr[15] & instr[11];
    assign alu_rhs_sel = instr[15] & instr[12];

    assign data_sel = ~instr[15];
    assign data = {1'b0, instr};

endmodule
