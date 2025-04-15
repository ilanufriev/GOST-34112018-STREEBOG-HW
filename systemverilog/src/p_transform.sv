module p_transform 
    #(

    )
    (
        input  logic[511:0] a_i,
        output logic[511:0] a_o
    )

logic[7:0] TAU[64] = {
    0,  8, 16, 24, 32, 40, 48, 56,
    1,  9, 17, 25, 33, 41, 49, 57,
    2, 10, 18, 26, 34, 42, 50, 58,
    3, 11, 19, 27, 35, 43, 51, 59,
    4, 12, 20, 28, 36, 44, 52, 60,
    5, 13, 21, 29, 37, 45, 53, 61,
    6, 14, 22, 30, 38, 46, 54, 62,
    7, 15, 23, 31, 39, 47, 55, 63
};

genvar i;
generate
for (i = 0; i < 64; i++)
begin
    assign a_o[i * 8 + 7:i * 8] = a_i[TAU[a[i]] * 8 + 7:TAU[a[i]] * 8];
end
endgenerate;

endmodule

