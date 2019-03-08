mov A, D
mov A, 0
mov A, 1
mov D, A*
mov A*, D
mov A, 666

; for D and A* only 0 and 1 can be immediates
; for A it can be any 15-bit value

add a, b ; store results in a
add a,



mov A, 666



mov A*; add D, A*; jmp // comment

mov D; add ~A, 1; jle

mov A, 666

mov A*, D

mov A*, 1

mov D, 0

mov D





; this is syntax version 3
; seems quite legit
; much better than 'from nand to tetris' syntax

; and is bitwise
; at the moment I prefer 'and' and 'add' instead of '&' and '+'
; ~ is bitwise invert
; it doesn't really make sense to use both *A and A in the same instruction so
; I will output error
; mov instrcution must be followed by alu operation
; jmp can be standalone
; jl, jle, jg, jge must be preceed with alu operation - because it makes the most
; sense

mov *A, D and ~*A, jmp ; comment

; you can move to A any 15-bit immediate value, but if it is
; different than 1 or 0 you can't do any other operation in this
; instruction
mov A, 666

; but only 0 or 1 to D or *A
mov D, 0
mov D, 1
mov *A, 0

; labels point at the next instruction
label_0:

mov *A, D add 1, jle ; some comment

; yea man, this syntax rocks

; one more thing 

define lolo 666
mov A, lolo

; oh and one more

var var1

;
var array 1000


; so we don't have to manually assign addresses

and we can do something like this

mov A, var1
mov *A, 1

; or

define array_size 1024

var array array_size

; for int i = 0; i < array_size; ++i ...

; I allow ram and rom address space to overlap. I don't see how it can
; become an issue here


validate if the number is in 16 bit range

changing var to data


ok I should also handle minus operation, forgot about it
adding sub keyword to the assembler

I restructed the syntax a little bit, refer not to main.cpp
And I changed from add, sub, and, or to +, -, |, &
