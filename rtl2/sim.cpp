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

    for(;;)
    {
        top->eval();

        tfp->dump(time);

        if(top->dout_ready)
        {
            result = top->dout;
            break;
        }

        if(top->reset && top->clk)
            top->reset = 0;

        top->clk = !top->clk;
        ++time;
    }

    printf("result: %d\n", result);
    tfp->close();
    delete top;
    return 0;
}
