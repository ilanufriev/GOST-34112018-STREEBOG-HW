`include "strhw_common.svh"

module strhw_rom #(
    parameter WIDTH = 64,
    parameter CAPACITY = 512,
    parameter SL_TABLE_START = 0
  ) (
    input  logic              clk_i,
    input  logic[15:0]        addr_i,
    output logic[WIDTH - 1:0] data_o
  );

  logic[WIDTH - 1:0] rom[0:CAPACITY - 1] = {
    SL_TABLE[SL_TABLE_START], SL_TABLE[SL_TABLE_START + 1]
  };

  always_ff @(posedge clk_i) begin
    if (ENABLE_DEBUG_OUTPUT && 0) begin
      $display("STRHW ROM data_o = %x (%d)", rom[addr_i], addr_i);
    end

    data_o <= rom[addr_i];
  end
endmodule
