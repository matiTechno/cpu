// calling convention
// 4 first arguments are passed in r1, r2, r3, r4
// return value is in r1
// all other registers are callee saved

    addi sp, r0, 255 // initialize stack pointer
    addi r1, r0, 15 // input
    bl fib
    str r1, 0(r0) // io output
end:
    b end

fib:
    beq r1, r0, ret_0
    addi r2, r0, 1
    beq r1, r2, ret_1

    // save lr
    addi sp, sp, -1
    str lr, 0(sp)

    addi r1, r1, -1 // arg1

    // save arg1
    addi sp, sp, -1
    str r1, 0(sp)

    bl fib

    add r2, r1, r0

    // restore arg1
    ldr r1, 0(sp)
    addi sp, sp, 1

    addi r1, r1, -1 // arg2

    // save result1
    addi sp, sp, -1
    str r2, 0(sp)

    bl fib

    // restore result1
    ldr r2, 0(sp)
    addi sp, sp, 1

    add r1, r1, r2 // result2 + result1

    // restore lr
    ldr lr, 0(sp)
    addi sp, sp, 1

    bx lr

ret_0:
    bx lr

ret_1:
    add r1, r2, r0
    bx lr
