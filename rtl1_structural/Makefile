all:
	iverilog -g2005-sv -Wall test_alu.v -o talu.out
	iverilog -g2005-sv -Wall test_pc.v  -o tpc.out
	iverilog -g2005-sv -Wall test_progmem.v  -o tprogmem.out
	iverilog -g2005-sv -Wall test_mem.v  -o tmem.out
	iverilog -g2005-sv -Wall test_cpu.v  -o tcpu.out

wave:
	gtkwave waveform.vcd
