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

    top->clk = 1;

    for(int i = 0; i < 1000; ++i) // run for 1k cycles
    {
        top->reset = i < 1;
        top->eval();
        tfp->dump(i);
        top->clk = !top->clk;
    }
    printf("result: %d\n", top->dout);
    tfp->close();
    delete top;
    return 0;
}
