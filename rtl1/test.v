`include "cpu.v"

module test();

    reg clk;
    reg reset;
    wire cpu_out;

    mod_cpu cpu(clk, reset, cpu_out);
    
    initial begin
        $dumpfile("waveform.vcd"); // gtkwave
        $dumpvars;
        $readmemh("prog.hex", cpu.program_memory.rom, 0, 34); // todo, pad to rom size
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
