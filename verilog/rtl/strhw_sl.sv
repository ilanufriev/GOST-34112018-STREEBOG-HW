module strhw_sl import strhw_common_types::*; #() (
    input  logic              clk_i,
    input  logic              rst_i,
    input  logic              trg_i,

    input  uint512            a_i,
    output uint512            result_o,
    output logic              ready_o
  );

  localparam QWORD_COUNT = 8;
  localparam QWORD_SIZE  = 8;

  uint64     a_qw     [QWORD_COUNT];
  uint64     result_qw[QWORD_COUNT];
  uint64     c        [QWORD_COUNT];
  logic[3:0] j;
  uint8      cstep;

  // 2D ROM
  uint64 sl_table[8][256];

  initial begin
    $readmemh("sl_table.mem", sl_table);
  end

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
      j <= 0;
      cstep <= 0;
    end else begin // at clk_i
      case (cstep)
        8'd0: begin
          if (trg_i != 1'b1) begin
            ready_o <= 0;
          end else begin
            j <= 0;
            cstep <= cstep + 1;
          end
        end
        8'd1: begin
          if (j == (QWORD_SIZE - 1)) begin
            cstep <= cstep + 1;
            j <= 0;
          end else begin
            j <= j + 1;
          end
        end
        8'd2: begin
          cstep <= 0;
          ready_o <= 1'b1;
        end
        default: begin
          cstep <= 0;
          $display("Should not be here");
        end
      endcase
    end
  end

  generate
    for (geni = 0; geni < QWORD_COUNT; geni++) begin
      always_ff @(posedge clk_i) begin
        if (rst_i) begin
          result_qw[geni] <= 0;
          c[geni] <= 0;
        end else begin // at clk_i
          case (cstep)
            8'd0: begin
              if (trg_i != 1'b1) begin
                // do nothing
              end else begin
                result_qw[geni] <= 0;
                c[geni] <= 0;
              end
            end
            8'd1: begin
              /* verilator lint_off WIDTHTRUNC */
              c[geni] <= c[geni] ^ sl_table[j][(a_qw[geni] >> (j * 8)) & 64'hff];
              /* verilator lint_on WIDTHTRUNC */
            end
            8'd2: begin
              result_qw[geni] <= c[geni];
            end
            default: begin
              cstep <= 0;
              $display("Should not be here");
            end
          endcase
        end
      end
    end
  endgenerate
endmodule
