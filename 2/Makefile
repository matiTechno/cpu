compile:
	iverilog -Wall -o test test.v half_adder.v

# xdot must be installed to execute this correctly
graph:
	yosys -q -s yosys.txt

wave:
	gtkwave test.vcd
