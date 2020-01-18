`include "cpu.v"

module test();

    reg zero_lhs;
    reg invert_lhs;
    reg zero_rhs;
    reg invert_rhs;
    reg opcode;
    reg invert_result;
    reg [15:0] lhs, rhs, result;

    initial begin
        zero_lhs = 0;
        invert_lhs = 1;
        zero_rhs = 0;
        invert_rhs = 0;
        opcode = 1;
        invert_result = 1;
        lhs = 16'd20;
        rhs = 16'd5;
    end

    _alu alu(zero_lhs, invert_lhs, zero_rhs, invert_rhs, opcode, invert_result, lhs, rhs, result);

    initial begin
        #4
        $display("result: %d", result);
        #4
        $finish();
    end
endmodule
