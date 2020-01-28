// test program
    addi sp, r0, 255 // initialize stack pointer

    add r1, r0, r0 // iterator
    addi r2, r0, 10 // max
    bl fun_add
    add r3, r0, r0 // result

begin_loop:
    beq r1, r2, end_loop
    addi r3, r3, -11
    addi r1, r1, 1
    b begin_loop

end_loop:
    str r3, 0(r0) // output the result

end:
    b end


fun_add:
    addi r2, r2, 5

    addi sp, sp, -1
    str lr, 0(sp)

    bl fun_add2

    ldr lr, 0(sp)
    addi sp, sp, 1

    bx lr


fun_add2:
    addi r2, r2, 1
    bx lr
