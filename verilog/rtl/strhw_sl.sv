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
  uint64     c;
  logic[6:0] i;
  logic[3:0] j;
  uint8      cstep;

  // 2D ROM
  uint64 sl_table[8][256];

  initial begin
    $readmemh("sl_table.mem", sl_table);
  end

  generate
  genvar geni;
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

      c <= 0;
      i <= 0;
      j <= 0;
      ready_o <= 0;

      cstep <= 8'd0;
    end else begin // at clk_i
      case (cstep)
        8'd0: begin
          if (trg_i != 1'b1) begin
            ready_o <= 0;
            // do nothing
          end else begin
            c <= 0;
            i <= 0;
            j <= 0;
            cstep <= cstep + 1;

            for (int k = 0; k < QWORD_COUNT; k++) begin
              result_qw[k] <= 0;
            end
            
            $display("c = %0x", c);
            $display("i = %0x", i);
            $display("j = %0x", j);
            
            $display("a_i = %0x", a_i);
          end
        end
        8'd1: begin
          /* verilator lint_off WIDTHTRUNC */
          c <= c ^ sl_table[j][(a_qw[i] >> (j * 8)) & 64'hff];
          /* verilator lint_on WIDTHTRUNC */

          if (j == (QWORD_SIZE - 1)) begin
            cstep <= cstep + 1;
            j <= 0;
          end else begin
            j <= j + 1;
          end
        end
        8'd2: begin
          result_qw[i[2:0]] <= c;
          c <= 0;

          if (i == (QWORD_COUNT - 1)) begin
            cstep <= 8'd0;
            ready_o <= 1'b1;
            i <= 0;
          end else begin
            cstep <= 8'd1;
            i <= i + 1;
          end
        end
        default: begin
          $display("Not supposed to be here");
        end
      endcase
    end
  end

  // always_ff @(posedge clk_i) begin
  //   if (rst_i) begin
  //     c        <= 'h0;
  //     i        <= 'h0;
  //     j        <= 'h0;
  //     cstep    <= 'h0;
  //     ready_o  <= 'h0;

  //     slt_row  <= 'h0;
  //     slt_col  <= 'h0;

  //     for (int k = 0; k < QWORD_COUNT; k++) begin
  //       result_qw[k] <= 0;
  //     end
  //   end else begin
  //     case (cstep)
  //       8'd0: begin
  //         if (trg_i != 1'b1) begin
  //           // do nothing
  //         end else begin
  //           cstep   <= cstep + 1;
  //           i       <= 0;
  //           j       <= 0;
  //           c       <= 0;
  //           slt_row <= 0;
  //           slt_col <= 0;
  //           ready_o <= 1'b0;

  //           for (int k = 0; k < QWORD_COUNT; k++) begin
  //             result_qw[k] <= 0;
  //           end
  //         end
  //       end
  //       8'd1: begin
  //         /* verilator lint_off WIDTHTRUNC */
  //         slt_row <= j;
  //         slt_col <= (a_qw[i] >> (j * 8)) & 64'hff;
  //         cstep   <= cstep + 1;

  //         /* verilator lint_on WIDTHTRUNC */
  //       end
  //       8'd2: begin
  //         // if this is the last iteration of j
  //         if ((j + 1) == QWORD_SIZE[3:0]) begin
  //           j <= 'h0;

  //           result_qw[i[2:0]] <= c ^ slt_data;

  //           // if this is the last iteration of i
  //           if ((i + 1) == BLOCK_SIZE) begin
  //             cstep <= 8'd0;
  //             ready_o <= 1'b1;
  //           end else begin
  //             i <= i + 1;
  //             c <= 64'h0;
  //             cstep <= 8'd1; // jump back to the beginning of the loop
  //           end
  //         end else begin
  //           c <= c ^ slt_data;
  //           j <= j + 1;
  //           cstep <= 8'd1;
  //         end
  //       end
  //       default: begin
  //         $display("cstep of strhw_sl should not be here");
  //       end
  //     endcase
  //   end
  // end
endmodule
