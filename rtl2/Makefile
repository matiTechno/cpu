all: obj_dir asm

obj_dir: sim.cpp core.sv
	verilator -Wall -Wno-fatal --trace -cc --top-module core core.sv --exe sim.cpp
	make -C obj_dir -f Vcore.mk

asm: asm.cpp
	g++ -g -Wall -Wextra asm.cpp -o asm -Wno-missing-field-initializers -Wno-switch

.PHONY: clean synth

clean:
	rm -rf obj_dir
	rm asm
	rm a.hex
	rm wave.vcd

synth:
	yosys synth.ys
