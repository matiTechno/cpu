`include "cpu.v"

module test();

    reg clk;
    reg reset;
    wire [15:0] cpu_out;

    cpu cpu1(clk, reset, cpu_out);

    initial begin
        $dumpfile("waveform.vcd"); // gtkwave
        $dumpvars;
        clk = 0;
        reset = 1;
        #2
        reset = 0;
        #1000
        $display("result: %d", cpu_out);
        $finish;
    end

    always begin
        #1 clk = ~clk;
    end

endmodule
