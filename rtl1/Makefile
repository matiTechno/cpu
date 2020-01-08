all:
	iverilog -Wall test.v
	verilator --lint-only cpu.v

synth:
	yosys synth.ys

wave:
	gtkwave waveform.vcd
