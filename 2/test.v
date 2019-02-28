module half_adder(a, b, sum, carry);

    input a;
    input b;
    output sum;
    output carry;

    and(a, b, carry);
    xor(a, b, sum);

endmodule

module test;

    reg r1;
    reg r2;
    wire sum;
    wire carry;

    half_adder half_adder( .a(r1), .b(r2), .sum(sum), .carry(carry) );

    initial begin

        $dumpfile("test.vcd");
        $dumpvars(0, test);

        r1 = 1'b0;
        r2 = 1'b0;
        #10;
        r1 = 1'b0;
        r2 = 1'b1;
        #10;
        r1 = 1'b1;
        r2 = 1'b0;
        #10;
        r1 = 1'b1;
        r2 = 1'b1;
        #10;
    end

endmodule
