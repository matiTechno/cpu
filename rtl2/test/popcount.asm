    addi r1, r0, 79 // input
    addi r2, r0, 0 // count

loop:
    beq r0, r1, loop_end
    andi r3, r1, 1
    add r2, r2, r3
    srli r1, r1, 1
    b loop

loop_end:
    str r2, 0(r0)

end:
    b end
