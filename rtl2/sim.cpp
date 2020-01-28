#include <stdio.h>
#include "Vcore.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

int main(int argc, char** argv)
{
    Verilated::commandArgs(argc, argv);
    Verilated::traceEverOn(true);

    Vcore* top = new Vcore;
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open("wave.vcd");

    top->clk = 0;
    top->reset = 1;
    int result = 666;

    for(int i = 0; i < 1000; ++i) // run for 1k cycles
    {
        top->eval();

        if(top->dout_ready)
            result = top->dout;

        tfp->dump(i);

        top->clk = !top->clk;

        if(i == 1)
            top->reset = 0;
    }
    printf("result: %d\n", result);
    tfp->close();
    delete top;
    return 0;
}
