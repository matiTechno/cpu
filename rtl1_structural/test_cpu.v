`include "cpu.v"

module test();

    reg clk;
    reg reset;
    wire cpu_out;

    _cpu cpu(clk, reset, cpu_out);

    initial begin
        $dumpfile("waveform.vcd"); // gtkwave
        $dumpvars;
        clk = 0;
        reset = 1;
        #2
        reset = 0;
        #1000
        $display("result: %d", cpu.memory.ram_data8[0]);
        $finish;
    end

    always begin
        #1 clk = ~clk;
    end

endmodule
