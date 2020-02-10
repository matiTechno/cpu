    addi r1, r0, 10 // input
    addi r2, r0, 2 // multiplier
    addi r3, r0, 1 // result

main_loop:
	// bgt r2, r1, end_main_loop
	slt r9, r1, r2
	bne r9, r0, end_main_loop

    add r4, r0, r3 // multiplicand
    addi r5, r0, 1 // mul iterator

loop_mul:
    beq r5, r2, end_loop_mul
    add r3, r3, r4
    addi r5, r5, 1
    b loop_mul

end_loop_mul:
    addi r2, r2, 1
    b main_loop

end_main_loop:
    str r3, 0(r0) // io output

end:
    b end
