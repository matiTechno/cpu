Here I try to build a cpu, assembler and this kind of stuff.
Learning more about the hardware.

Some questions and topics to investigate:
* power supply - cathode, anode

learn basics of electronics and hardware architecture and electric physics (I'm just curious how it works and I'm working at Intel...); how silicon is used...
https://lwn.net/Articles/250967/ - this might be a part of this course (how memory works)
actually the fact that I learned a lot of assembly during fractal project will help me here
The code books, and from nand to teris are also a good books. I could end this project with circut level simulation, emulator, and this kind of stuff.
own HDL - and compile it to circuit simulation
Yea, I would like to define my own hardware in HDL code - be able to simulate it, test, and that kind of stuff. Write an assembler for it. And also
compile to graphical representation down to the transistor level and see it operate in real time.
And maybe try to implement intel 8080?
And some microcontroller?
+ emulate display, and audio, and input like keyboard?

It was a good move to start this projects in parallel (read last sentances of n-body project).
I feel that my motivation is comeing back.
I can try to go with pure x11 opengl c99 to improve compile times and stuff.
I don't need to be corss-platform and stuff. I think it is a great time and project to expiriment with things like that.
I have not started yet with these two projects but they seem very fun and promising for me. And I'm happy about it. They are also
quite long term. And if I succeed with them they will be a game changers in my portfolio.

  So my idea has clarified a little bit, here is what I plan to do:
    * design a processor in logisim, this will get me a view of what it takes to build such a processor
    * create own digital logic simulator - both graphical editor and HDL (HDL can be compiled to a graphical form)
    * first port existing cpu and then work on more powerful cpus
    * create assembler for ISA
    * create C compiler with my assembly backend
    * write some cool programs + visualize RAM as image, interface with keyboard, etc... (keyboard and stuff can be 'abastraced away' at least at the beginning)
    * have also an emulator for performance reasons (might be validated by comparing results with results from simulator)
    * option to simulate step by step in the graphical editor
    * option to show graphics in more detailed way - e.g. even replace gates with transistors - for fun and to show how complex
      it becomes, and to have cool pictures (so replace all blocks with the their lowest possible implementation)
    * cull not visible elements; spatial partitioning (optimizing element selection, etc..)
    * an option that when zooming components will be broken down to more basic components and so on
    * displaying my processor down to a transistor level would be impressive
    * sending data over uart?
    * culling not visible elements - too zoomed out to see
    * seeing each individual component of the processor is fucking epic and cool idea
    * final design could be sending data over rs-232 and reading program from rom - so the final design would have 'complete' io
    * I could do a pulsating light effect of how electrons move
    * adjustable bus width for custom components - this can't be done in logisim editor
    * first I could try to have simulation without graphical part - just backend C++ code
    * zoom level by level into the cpu hardware - abstraction by abstraction.
    * slow-motion clock could be useful for electric potential transfer visualization - it would look epic
    * is propagation delay needed to simulate correctly digital circuit?

    nandgame.com - this is amazing
    'code' book
    'from nand to tetris' book
    https://www-users.cs.york.ac.uk/~mjf/simple_cpu/index.html
    asic-world.com
    verilog, yoses, gtkwave

    Quite a challenge will be to assembly all these pieces of work into one place. And I will also write
    a very nice and informative readme.
    I could also write my simple processor in verilog and validate by printing the results for example.

    Fuck, I encountered a bug in logisim evolution, with ram chip. Luckily after restarting it was resolved...

    How to make sure that all memory is initialized correctly during first cycle. And what if clock will immiedietly produce
    rising signal and second instruction will be selected and overwrite results from the first one?
    Moving this to readme.txt project because it grows out of control.
