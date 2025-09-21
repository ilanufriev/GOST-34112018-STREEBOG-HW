`include "strhw_common.svh"

module strhw_sl #() (
    input  logic              clk_i,
    input  logic              rst_i,
    input  logic              trg_i,

    input  uint512            a_i,
    output uint512            result_o,
    output logic              ready_o
  );

  localparam QWORD_COUNT = 8;

  uint64     a_qw     [QWORD_COUNT];
  uint64     result_qw[QWORD_COUNT];
  uint64     c;
  logic[3:0] j;
  logic[3:0] i;
  uint8      cstep;

  // Fields for ROM
  logic[10:0] rom_addr;
  uint64      rom_data;
  logic[1:0]  rom_cstep;

  // 2D ROM
  strhw_sl_table_rom sl_rom(
      .clk_i(clk_i),
      .addr_i(rom_addr),
      .data_o(rom_data)
    );

  genvar geni;
  generate
  for (geni = 0; geni < QWORD_COUNT; geni++) begin
    assign a_qw[geni] = a_i[(geni * 64) + 63:(geni * 64)];
  end

  for (geni = 0; geni < QWORD_COUNT; geni++) begin
    assign result_o[(geni * 64) + 63:(geni * 64)] = result_qw[geni];
  end
  endgenerate

  always_ff @(posedge clk_i) begin
    if (rst_i) begin
      for (int k = 0; k < QWORD_COUNT; k++) begin
        result_qw[k] <= 0;
      end

      c         <= 0;
      j         <= 0;
      i         <= 0;
      cstep     <= 0;
      ready_o   <= 0;

      rom_addr  <= 0;
      rom_cstep <= 0;
    end else begin // at clk_i
      case (cstep)
        8'd0: begin
          if (trg_i != 1'b1) begin
            cstep   <= 8'd0;
            ready_o <= 0;
          end else begin
            c       <= 0;
            j       <= 0;
            i       <= 0;
            cstep   <= cstep + 1;

            rom_addr  <= 0;
            rom_cstep <= 0;
          end
        end
        8'd1: begin
          if (i == (QWORD_COUNT)) begin
            // break the loop
            cstep <= 8'd0;
            ready_o <= 1'b1;
          end else begin
            c <= 0;
            j <= 0;
            cstep <= cstep + 1;
          end
        end
        8'd2: begin
          if (j == (QWORD_COUNT)) begin
            // break the loop
            result_qw[i] <= c;

            i <= i + 1;
            cstep <= 8'd1;
          end else begin
            if (rom_cstep < 2) begin
              // Get data from ROM
              rom_addr  <= j * SL_TABLE_WIDTH
                         + ((a_qw[i] >> (j * 8)) & 64'hff);

              rom_cstep <= rom_cstep + 1;
            end else begin
              if (ENABLE_DEBUG_OUTPUT && 1) begin
                $display("SL a_qw[%d] = %0x", i, a_qw[i]);
                $display("SL rom_addr = %0x", rom_addr);
                $display("SL rom_data = %0x", rom_data);
                $display("SL c ^ rom_data = %0x", c ^ rom_data);
              end

              c <= c ^ rom_data;

              j <= j + 1;
              rom_cstep <= 0;
            end
          end
        end
      endcase
    end
  end
endmodule
