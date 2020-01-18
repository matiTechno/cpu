`include "cpu.v"

module test();
    wire [15:0] instr_data;
    reg [15:0] addr;
    _progmem progmem(addr, instr_data);

    initial begin
        addr = 16'd1;
        #1
        $display("%x", instr_data);
        addr = 16'd7;
        #1
        $display("%x", instr_data);
        addr = 16'd22;
        #1
        $display("%x", instr_data);
        $finish();
    end

endmodule
