    addi r1, r0, 55
    muli r3, r1, -213
    addi r2, r0, -3
    mul r3, r3, r2
    addi r1,  r0, 9
    div r3, r3, r1
    divi r3, r3, -3905
    str r3, 0(r0)
end:
    b end
