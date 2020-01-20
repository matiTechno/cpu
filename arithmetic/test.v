// $strobe prints signals after all events in the current timestep are executed,
// $display is different
// look for verilog scheduling

module test();

    reg clk, reset;
    reg [15:0] a, b, result_add, result_sub, result_mul, result_mul_comb, result_div;

    adder16 adder16(a, b, result_add);

    subtractor16 subtractor16(a, b, result_sub);

    mul16 mul16(clk, reset, a, b, result_mul);

    mul16_comb mul16_comb(a, b, result_mul_comb);

    div16 div16(clk, reset, a, b, result_div);

    initial begin
        $dumpfile("wave.vcd");
        $dumpvars;

        clk = 0;
        reset = 1;
        a = 3;
        b = 55;
        #2
        reset = 0;
        #20

        $display("result_mul: %d", result_mul);
        reset = 1;
        a = 9;
        b = 3;
        #2
        reset = 0;
        #20

        $display("result_mul: %d", result_mul);

        #10
        a = 5;
        b = 9;
        #1

        $display("result_mul_comb: %d", result_mul_comb);

        a = -25;
        b = 64;
        #1
        $display("result_add: %d", result_add);
        $display("result_sub: %d", $signed(result_sub));

        reset = 1;
        a = 666;
        b = 21;
        #2
        reset = 0;
        #31
        $strobe("result_div: %d", result_div); // div result must be fetched exactly after 16 cycles
        $finish;
    end

    always begin
        #1 clk = ~clk;
    end

endmodule

// result is ready 16 cycles after reset
module div16(
    input clk,
    input reset,
    input [15:0] a,
    input [15:0] b,
    output reg [15:0] quotient);

    reg [31:0] divisor;
    reg [31:0] reminder;
    wire [31:0] diff;

    subtractor32 sub32(reminder, divisor, diff);

    always_ff @(posedge clk) begin
        if(reset) begin
            divisor <= {1'b0, b, 15'b0};
            reminder <= {16'b0, a};
            quotient <= 0;
        end else begin
            divisor <= divisor >> 1;

            if(~diff[31]) begin
                reminder <= diff;
                quotient <= (quotient << 1) | 16'b1;
            end else
                quotient <= quotient << 1;
        end
    end

endmodule

// note: this could probably be done with a module parameter and a recursive generation

module mul16_comb(
    input [15:0] a,
    input [15:0] b,
    output [15:0] product);

    reg [15:0] parts [15:0];
    reg [15:0] sum8 [7:0];
    reg [15:0] sum4 [3:0];
    reg [15:0] sum2 [1:0];

    always_comb begin
        int i;
        for(i = 0; i < 16; i = i + 1)
            parts[i] = ( a & {16{b[i]}} ) << i;
    end

    genvar i;
    generate
        for(i = 0; i < 16; i = i + 2)
            adder16 adder16(parts[i], parts[i + 1], sum8[i / 2]);

        for(i = 0; i < 8; i = i + 2)
            adder16 adder16(sum8[i], sum8[i + 1], sum4[i / 2]);

        for(i = 0; i < 4; i = i + 2)
            adder16 adder16(sum4[i], sum4[i + 1], sum2[i / 2]);
    endgenerate

    adder16 adder16(sum2[0], sum2[1], product);

endmodule

module mul16(
    input clk,
    input reset,
    input [15:0] a,
    input [15:0] b,
    output reg [15:0] product);

    reg [15:0] product_next, reg_a, reg_b;

    wire [15:0] part_mul = reg_b[0] ? reg_a : 16'b0;

    adder16 adder16(part_mul, product, product_next);

    always_ff @(posedge clk) begin
        if(reset) begin
            reg_a <= a;
            reg_b <= b;
            product <= 16'd0;
        end else begin
            reg_a <= reg_a << 1;
            reg_b <= reg_b >> 1;
            product <= product_next;
        end
    end

endmodule

module adder16(
    input [15:0] a,
    input [15:0] b,
    output [15:0] sum);

    wire [15:-1] carry;

    assign carry[-1] = 0;

    genvar i;
    generate
        for(i = 0; i < 16; i = i + 1)
            full_adder full_adder(a[i], b[i], carry[i - 1], sum[i], carry[i]);
    endgenerate

endmodule

module full_adder(
    input a,
    input b,
    input cin,
    output sum,
    output cout);

    assign sum = a ^ b ^ cin;
    assign cout = (a & b) | (cin & a) | (cin & b);

endmodule

module subtractor16(
    input [15:0] a,
    input [15:0] b,
    output [15:0] diff);

    wire [15:-1] borrow;

    assign borrow[-1] = 0;

    genvar i;
    generate
        for(i = 0; i < 16; i = i + 1)
            full_subtractor full_subtractor(a[i], b[i], borrow[i - 1], diff[i], borrow[i]);
    endgenerate

endmodule

// todo, use parameters
module subtractor32(
    input [31:0] a,
    input [31:0] b,
    output [31:0] diff);

    wire [31:-1] borrow;

    assign borrow[-1] = 0;

    genvar i;
    generate
        for(i = 0; i < 32; i = i + 1)
            full_subtractor full_subtractor(a[i], b[i], borrow[i - 1], diff[i], borrow[i]);
    endgenerate

endmodule

module full_subtractor(
    input a,
    input b,
    input bin,
    output diff,
    output bout);

    assign diff = a ^ b ^ bin;
    assign bout = (~a & bin) | (~a & b) | (b & bin);

endmodule
