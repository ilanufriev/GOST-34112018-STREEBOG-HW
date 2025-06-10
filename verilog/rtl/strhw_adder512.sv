module strhw_adder512 import strhw_common_types::*; #() (
    input  logic              clk_i,
    input  logic              rst_i,
    input  logic              trg_i,
    input  uint512            a_i,
    input  uint512            b_i,
    output uint512            result_o,
    output logic              ready_o
  );

  logic[255:0]                result_lsb;
  logic[255:0]                result_msb;
  logic                       carry;

  logic[255:0]                a_msb, b_msb;
  logic                       trg_st1;

  always @(posedge clk_i) begin
    if (rst_i) begin
      result_lsb <= 0;
      carry      <= 0;
      a_msb      <= 0;
      b_msb      <= 0;
      trg_st1    <= 0;
    end else begin // at clk
      {carry, result_lsb} <= a_i[255:0] + b_i[255:0];
      a_msb <= a_i[511:256];
      b_msb <= b_i[511:256];
      trg_st1 <= trg_i;
    end
  end

  always @(posedge clk_i) begin
    if (rst_i) begin
      ready_o    <= 0;
    end else begin
      /* verilator lint_off WIDTHEXPAND */

      result_msb <= a_msb + b_msb + carry;

      /* verilator lint_on WIDTHEXPAND */

      ready_o <= trg_st1;
    end
  end

  assign result_o = {result_msb, result_lsb};

endmodule
