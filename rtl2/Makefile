sim:
	verilator -Wall -Wno-fatal --trace -cc --top-module core core.sv --exe main.cpp
	make -C obj_dir -f Vcore.mk

clean:
	rm -rf obj_dir

.PHONY: asm
asm:
	g++ -g -Wall -Wextra asm.cpp -o asm -Wno-missing-field-initializers

synth:
	yosys synth.ys
