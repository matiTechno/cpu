// test program

    add r1, r0, r0 // iterator
    addi r2, r0, 10 // max
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
