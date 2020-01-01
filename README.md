cpu and cpu1 are doing the same thing, cpu1 is more clean visually  
this cpu is the 'hack' machine from nand to tetris course
and you can build it in this game http://nandgame.com/  
it is a single cycle cpu, each instruction is build from three segments:  
where to store, ALU output, jump condition  
In some cases some parts of a instruction may be skipped (assembly).  
todo - complete explanation  
In logisim a RAM address is buffered and there is some latency in accessing data, for this reason nop (0) instructions
are needed. Maybe I'm doing something wrong, I don't quite understand this design in logisim (why addressing data
is sequential).

prog2.txt
```
; factorial program

define input 7

data addr_result
data addr_fact_count
data addr_mul_count
data addr_mul_op

mov a, addr_result
mov [a], 1

mov a, 2
mov d, a
mov a, addr_fact_count
mov [a], d

loop_fact:

mov a, addr_fact_count
0
mov d, [a]
mov a, input
mov d, a - d
mov a, fact_end
d, jl

mov a, addr_mul_count
mov [a], 1
mov a, addr_result
0
mov d, [a]
mov a, addr_mul_op
mov [a], d

loop_mul:

mov a, addr_mul_op
0
mov d, [a]
mov a, addr_result
0
mov [a], d + [a]
mov a, addr_mul_count
0
mov d, [a] + 1
mov [a], d
mov a, addr_fact_count
0
mov d, [a] - d
mov a, loop_mul
d, jg

mov a, addr_fact_count
0
mov [a], [a] + 1
mov a, loop_fact
jmp

fact_end:
mov a, fact_end
jmp

```

is assembled to this and placed in a cpu ROM
```
0
8fc8
2
8c10
1
8308
1
8a00
9c10
7
81d0
28
8301
2
8fc8
0
8a00
9c10
3
8308
3
8a00
9c10
0
8a00
9088
2
8a00
9dd0
8308
1
8a00
91d0
14
8304
1
8a00
9dc8
6
8007
28
8007
```
