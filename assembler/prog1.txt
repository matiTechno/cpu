; set each element of array with its index

; hmm, it seems that in logisim you have to wait on cycle after
; setting the address to read the data
; there is no need to wait when writing at the address

define count 8

data array count
data it 

mov a, it
mov [a], 0

loop:

; if(it == count) break;
mov a, it
0
mov d, [a]
mov a, count
mov d, a - d
mov a, end
d, je

mov a, it
0
mov d, [a]
mov a, array
mov a, a + d
; store it at array[it]
mov [a], d

; ++it
mov a, it
0
mov [a], [a] + 1

mov a, loop
jmp

end:
mov a, end
jmp
