module test;

    reg a;
    reg b;
    wire sum;
    wire carry;

    initial begin

        $dumpfile("test.vcd");
        $dumpvars(0, test);
        $monitor("a=%b, b=%b, sum=%b, carry=%b", a, b, sum, carry);

        a = 0;
        b = 1;

        #10;

        a = 1;
        b = 1;

        #10;

        a = 1;
        b = 0;

        #10;

        a = 0;
        b = 0;

        #10;
        $finish;
    end

    half_adder half_adder(a, b, sum, carry);

endmodule
