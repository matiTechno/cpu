// test program

    add r1, r0, r0 // iterator
    addi r2, r0, 10 // max
    add r3, r0, r0 // out
begin:
    beq r1, r2, end
    addi r3, r3, 11
    addi r1, r1, 1
    b begin
end:
    add r3, r3, r0 // show result in debug
    b end
