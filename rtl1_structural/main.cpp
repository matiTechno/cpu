#include "Vcpu.h"
#include "verilated.h"
#include "verilated_vcd_c.h"
#include <stdio.h>

int main(int argc, char** argv)
{
    Verilated::commandArgs(argc, argv);
    Verilated::traceEverOn(true);

    Vcpu* top = new Vcpu;
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open("wave.vcd");

    for(int i = 0; i < 1000; ++i) // run for 1k cycles
    {
        top->reset = i < 1;
        top->clk = !top->clk;
        top->eval();
        tfp->dump(i);
    }
    printf("result: %d\n", top->reg_D_out);
    tfp->close();
    delete top;
    return 0;
}
