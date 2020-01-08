`include "cpu.v"

module test();

    reg clk;
    reg reset;

    mod_cpu cpu(clk, reset);
    
    initial begin
        $dumpfile("waveform.vcd"); // gtkwave
        $dumpvars;
        $readmemh("prog2.hex", cpu.rom.mem, 0, 41); // todo, pad to ROM size
        clk = 0;
        reset = 1;
        #2
        reset = 0;
        #1000
        $display("result: %d", cpu.memory.ram[0]);
        $finish;
    end

    always begin
        #1 clk = ~clk;
    end

endmodule
