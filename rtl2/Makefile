all: obj_dir asm ic

obj_dir: sim.cpp core.sv
	verilator -Wall -Wno-fatal --trace -cc --top-module core core.sv --exe sim.cpp
	make -C obj_dir -f Vcore.mk

asm: asm.cpp array.h
	g++ -g -Wall -Wextra asm.cpp -o asm -Wno-missing-field-initializers -Wno-switch

.PHONY: clean synth

ic: ic.cpp lex.cpp parse.cpp sema.cpp gen_llvm.cpp gen_mycore.cpp ic.h array.h
	g++ -fno-exceptions -fno-rtti -g -Wall -Wextra ic.cpp lex.cpp parse.cpp sema.cpp gen_llvm.cpp gen_mycore.cpp -o ic -Wno-switch -I$(shell llvm-config --includedir --ldflags --libs)


clean:
	rm -rf obj_dir
	rm asm
	rm ic
	rm a.hex
	rm wave.vcd

synth:
	yosys synth.ys
