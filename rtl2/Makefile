all:
	verilator -Wall -Wno-fatal --trace -cc --top-module core core.sv --exe main.cpp
	make -C obj_dir -f Vcore.mk
	obj_dir/Vcore

clean:
	rm -rf obj_dir
	rm wave.vcd

asm:
	g++ -g -Wall -Wextra asm.cpp -o asm

synth:
	yosys synth.ys
