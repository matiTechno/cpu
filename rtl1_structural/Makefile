all:
	iverilog -g2005-sv -Wall test_alu.v -o talu.out
	iverilog -g2005-sv -Wall test_pc.v  -o tpc.out
	iverilog -g2005-sv -Wall test_progmem.v  -o tprogmem.out
	iverilog -g2005-sv -Wall test_mem.v  -o tmem.out
	iverilog -g2005-sv -Wall test_cpu.v  -o tcpu.out

cc:
	verilator -Wall -Wno-fatal --trace -cc --top-module cpu cpu.v --exe main.cpp
	make -C obj_dir -f Vcpu.mk

wave:
	gtkwave waveform.vcd
