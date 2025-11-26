`include "strhw_common.svh"

module strhw_sl_table_rom #() (
    input  logic       clk_i,
    input  logic[10:0] addr_i,
    output uint64      data_o
  );

  logic[15:0] addr;
  uint64      data [0:3];
  logic[1:0]  choice;

  strhw_rom #(.SL_TABLE_START(0)) b1 (
      .clk_i(clk_i),
      .addr_i(addr),
      .data_o(data[0])
    );

  strhw_rom #(.SL_TABLE_START(2)) b2 (
      .clk_i(clk_i),
      .addr_i(addr),
      .data_o(data[1])
    );

  strhw_rom #(.SL_TABLE_START(4)) b3 (
      .clk_i(clk_i),
      .addr_i(addr),
      .data_o(data[2])
    );

  strhw_rom #(.SL_TABLE_START(6)) b4 (
      .clk_i(clk_i),
      .addr_i(addr),
      .data_o(data[3])
    );

  always_comb begin : data_gathering
    data_o = data[choice];

    if (ENABLE_DEBUG_OUTPUT && 0) begin
      for (int i = 0; i < 4; i++) begin
        $display("SL TABLE ROM data[%d] = %x", i, data[i]);
      end

      $display("SL TABLE ROM data_o = %x", data_o);
    end
  end : data_gathering

  /* verilator lint_off WIDTHEXPAND */
  /* verilator lint_off WIDTHTRUNC */
  always_comb begin : address_mapping
    addr = 0;
    choice = 0;

    for (logic[10:0] i = 0; i < 4; i++) begin

      if (ENABLE_DEBUG_OUTPUT && 0) begin
        $display("SL TABLE ROM addr_i = %x", addr_i);
      end

      if ((addr_i >= (i * 'd512)) && (addr_i < (i * 'd512 + 'd512))) begin
        addr = addr_i - 11'd512 * i;

        if (ENABLE_DEBUG_OUTPUT && 0) begin
          $display("SL TABLE ROM addr = %x", addr);
        end

        choice = i;
      end
    end
  end : address_mapping

  /* verialtor lint_on WIDTHEXPAND */
  /* verialtor lint_on WIDTHTRUNC */
endmodule
