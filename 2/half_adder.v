module half_adder(a, b, sum, carry);

    input a, b;
    output sum, carry;

    assign carry = a & b;
    assign sum = a ^ b;

endmodule
