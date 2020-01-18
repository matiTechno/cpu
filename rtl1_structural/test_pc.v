`include "cpu.v"

module test();

    reg clk;
    reg reset;
    reg jump;
    reg [15:0] jump_addr, pc_out;

    initial begin
        $dumpfile("waveform.vcd"); // gtkwave
        $dumpvars;
        clk = 0;
        reset = 1;
        jump = 0;
        jump_addr = 666;
        #2
        reset = 0;

        #10
        jump = 1;
        #1
        jump = 0;

        #40
        $finish();
    end

    _program_counter pc(clk, reset, jump, jump_addr, pc_out);

    always begin
        #1 clk = ~clk;
        if(clk)
            $display("pc_out: %d", pc_out);
    end

endmodule
