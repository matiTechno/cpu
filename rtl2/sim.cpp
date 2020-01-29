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
    int time = 0;
    int exit_count = -1;

    while(exit_count != 0)
    {
        top->eval();
        tfp->dump(time);

        if(top->reset && top->clk)
            top->reset = 0;

        if(top->dout_ready && !top->clk)
        {
            result = top->dout;
            exit_count = 100;
        }

        top->clk = !top->clk;
        ++time;
        --exit_count;
    }

    printf("result: %d\n", result);
    tfp->close();
    delete top;
    return 0;
}
