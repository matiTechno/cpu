`include "cpu.v"

module test();

    reg clk;
    reg store_ram;
    reg storeD;
    reg storeA;
    reg [15:0] din, outram, outA, outD;

    _memory mem(clk, store_ram, storeD, storeA, din, outram, outA, outD);

    initial begin
        $dumpfile("waveform.vcd"); // gtkwave
        $dumpvars;

        store_ram = 0;
        storeD = 0;
        storeA = 1;
        din = 3;
        clk = 0;

        #3
        #1
        din = 666;


        #100

        $finish();
    end

    always begin
        #3 clk = ~clk;
    end

endmodule
