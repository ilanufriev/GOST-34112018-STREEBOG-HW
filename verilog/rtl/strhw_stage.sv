module strhw_stage import strhw_common_types::*; #() (

    // Inputs from the Control logic
    input  logic             clk_i,
    input  logic             rst_i,
    input  logic             trg_i,
    input  uint512           block_i,
    input  uint7             block_size_i,
    input  uint512           sigma_i,
    input  uint512           n_i,
    input  uint512           h_i,

    // Outputs to the Control logic
    output uint512           sigma_new_o,
    output uint512           n_new_o,
    output uint512           h_new_o,
    output state_t           state_o,

    // Inputs from the Gn
    input  uint512           g_n_result_i,
    input  state_t           g_n_state_i,

    // Outputs to Gn
    output uint512           g_n_m_o,
    output uint512           g_n_n_o,
    output uint512           g_n_h_o,
    output logic             g_n_trg_o

  );

  typedef enum {
    CLEAR_ST1,
    CLEAR_WAIT_TRG,
    BUSY_WAIT_STAGE,
    DONE_WAIT_TRG
  } istate_t;

  typedef logic[7:0] s2_cstep_t;

  typedef logic[7:0] s3_cstep_t;

  // Internal registers
  istate_t                   istate;
  s2_cstep_t                 s2_cstep;
  s3_cstep_t                 s3_cstep;
  uint512                    padded_block;
  uint512                    h;
  uint512                    n;
  uint512                    sigma;

  logic                      s2_ready;
  logic                      s3_ready;

  // adder signals
  logic           adder_trg;
  uint512         adder_a;
  uint512         adder_b;
  uint512         adder_result;
  logic           adder_ready;

  strhw_adder512 adder(
      .clk_i     (clk_i),
      .rst_i     (rst_i),
      .trg_i     (adder_trg),
      .a_i       (adder_a),
      .b_i       (adder_b),
      .result_o  (adder_result),
      .ready_o   (adder_ready)
    );

  always_ff @(posedge clk_i) begin
    if (rst_i) begin
      istate  <= CLEAR_ST1;
      state_o <= CLEAR;
    end else begin // on clk_i
      case (istate)
        CLEAR_ST1: begin
          istate <= CLEAR_WAIT_TRG;
        end
        CLEAR_WAIT_TRG: begin
          if (trg_i != 1'b1) begin
            // do nothing
          end else begin
            istate  <= BUSY_WAIT_STAGE;
            state_o <= BUSY;
          end
        end
        BUSY_WAIT_STAGE: begin
            if (s2_ready != 1'b1 && s3_ready != 1'b1) begin
              // do nothing
            end else begin
              istate <= DONE_WAIT_TRG;
              state_o <= DONE;
            end
        end
        DONE_WAIT_TRG: begin
          if (trg_i != 1'b1) begin
            // do nothing
          end else begin
            istate  <= BUSY_WAIT_STAGE;
            state_o <= BUSY;
          end
        end
      endcase
    end
  end

  always_ff @(posedge clk_i) begin : stage
    if (rst_i) begin
      s2_cstep     <= 8'b0;
      s3_cstep     <= 8'b0;

      sigma_new_o  <= 512'h0;
      n_new_o      <= 512'h0;
      h_new_o      <= 512'h0;

      g_n_m_o      <= 512'h0;
      g_n_n_o      <= 512'h0;
      g_n_h_o      <= 512'h0;
      g_n_trg_o    <= 1'b0;

      padded_block <= 512'h0;
      h            <= 512'h0;
      n            <= 512'h0;
      sigma        <= 512'h0;
      
      adder_a      <= 512'h0;
      adder_b      <= 512'h0;

    end else begin // at clk_i

      if (istate != BUSY_WAIT_STAGE) begin
        s2_ready     <= 1'd0;
        s3_ready     <= 1'd0;
      end else begin
        if (block_size_i == BLOCK_SIZE) begin : stage2
          case (s2_cstep)
            8'd0: begin
              h         <= h_i;
              n         <= n_i;
              sigma     <= sigma_i;

              g_n_n_o   <= n_i;
              g_n_h_o   <= h_i;
              g_n_m_o   <= block_i;

              g_n_trg_o <= 1'b1;
              s2_cstep  <= s2_cstep + 1;
            end
            8'd1: begin
              if (ENABLE_DEBUG_OUTPUT) begin
                $display("STAGE2 g_n_n_o: %0x", g_n_n_o);
                $display("STAGE2 g_n_h_o: %0x", g_n_h_o);
                $display("STAGE2 g_n_m_o: %0x", g_n_m_o);
              end

              g_n_trg_o <= 1'b0;
              s2_cstep  <= s2_cstep + 1;
            end
            8'd2: begin
              if (g_n_state_i != DONE) begin
                // do nothing
              end else begin
                h_new_o  <= g_n_result_i;
                s2_cstep <= s2_cstep + 1;
              end
            end
            8'd3: begin
              adder_a   <= n_i;
              adder_b   <= 512'd512;
              adder_trg <= 1'b1;

              s2_cstep  <= s2_cstep + 1;
            end
            8'd4: begin
              adder_trg <= 1'b0;
              s2_cstep  <= s2_cstep + 1;
            end
            8'd5: begin
              if (adder_ready != 1'b1) begin
                // do nothing
              end else begin
                n_new_o <= adder_result;
                s2_cstep <= s2_cstep + 1;
              end
            end
            8'd6: begin
              // sigma_new_o = sigma_i + block_i;
              adder_a   <= sigma_i;
              adder_b   <= block_i;
              adder_trg <= 1'b1;

              s2_cstep <= s2_cstep + 1;
            end
            8'd7: begin
              adder_trg <= 0;
              s2_cstep  <= s2_cstep + 1;
            end
            8'd8: begin
              if (adder_ready != 1'b1) begin
                // do nothing
              end else begin
                sigma_new_o <= adder_result;
                s2_cstep <= s2_cstep + 1;
              end
            end
            8'd9: begin
              if (ENABLE_DEBUG_OUTPUT) begin
                $display("STAGE2 h_new_o: %0x", h_new_o);
                $display("STAGE2 n_new_o: %0x", n_new_o);
                $display("STAGE2 sigma_new_o: %0x", sigma_new_o);
              end

              s2_ready <= 1'b1;
              s2_cstep <= s2_cstep + 1;
            end
            8'd10: begin
              s2_ready <= 1'b0;
              s2_cstep <= 8'd0;
            end
            default: begin
              $display("Stage 2 should not be here");
            end
          endcase
        end : stage2
        else begin : stage3
          case (s3_cstep)
            8'd0: begin
              h <= h_i;
              n <= n_i;
              sigma <= sigma_i;

              s3_cstep <= s3_cstep + 1;
            end
            8'd1: begin
              padded_block <= block_i; // block_i | (512'h1 << (block_size_i * 8));
              padded_block [block_size_i * 8] <= 1'b1;

              s3_cstep <= s3_cstep + 1;
            end
            8'd2: begin
              g_n_h_o <= h_i;
              g_n_m_o <= padded_block;
              g_n_n_o <= n_i;

              g_n_trg_o <= 1'b1;
              s3_cstep  <= s3_cstep + 1;
            end
            8'd3: begin
              if (ENABLE_DEBUG_OUTPUT) begin
                $display("STAGE3_1 g_n_h_o: %0x", g_n_h_o);
                $display("STAGE3_1 g_n_m_o: %0x", g_n_m_o);
                $display("STAGE3_1 g_n_n_o: %0x", g_n_n_o);
              end

              g_n_trg_o <= 1'b0;
              s3_cstep  <= s3_cstep + 1;
            end
            8'd4: begin
              if (g_n_state_i != DONE) begin
                // do nothing
              end else begin
                h        <= g_n_result_i;
                s3_cstep <= s3_cstep + 1;
              end
            end
            8'd5: begin
              // n_next = n_i + block_size_i * 8;
              adder_a   <= n_i;
              adder_b   <= block_size_i * 8;
              adder_trg <= 1'b1;

              s3_cstep  <= s3_cstep + 1;
            end
            8'd6: begin
              adder_trg <= 1'b0;
              s3_cstep  <= s3_cstep + 1;
            end
            8'd7: begin
              if (adder_ready != 1'b1) begin
                // do nothing
              end else begin
                n        <= adder_result;
                s3_cstep <= s3_cstep + 1;
              end
            end
            8'd8: begin
              // sigma = sigma_i + padded_block;
              adder_a   <= sigma_i;
              adder_b   <= padded_block;
              adder_trg <= 1'b1;

              s3_cstep  <= s3_cstep + 1;
            end
            8'd9: begin
              adder_trg <= 1'b0;
              s3_cstep  <= s3_cstep + 1;
            end
            8'd10: begin
              if (adder_ready != 1'b1) begin
                // do nothing
              end else begin
                sigma    <= adder_result;
                s3_cstep <= s3_cstep + 1;
              end
            end
            8'd11: begin
              g_n_h_o <= h;
              g_n_m_o <= n;
              g_n_n_o <= 0;

              g_n_trg_o <= 1'b1;
              s3_cstep  <= s3_cstep + 1;
            end
            8'd12: begin
              if (ENABLE_DEBUG_OUTPUT) begin
                $display("STAGE3_2 g_n_h_o: %0x", g_n_h_o);
                $display("STAGE3_2 g_n_m_o: %0x", g_n_m_o);
                $display("STAGE3_2 g_n_n_o: %0x", g_n_n_o);
              end

              g_n_trg_o <= 1'b0;
              s3_cstep  <= s3_cstep + 1;
            end
            8'd13: begin
              if (g_n_state_i != DONE) begin
                // do nothing
              end else begin
                h        <= g_n_result_i;
                s3_cstep <= s3_cstep + 1;
              end
            end
            8'd14: begin
              g_n_h_o <= h;
              g_n_m_o <= sigma;
              g_n_n_o <= 0;

              g_n_trg_o <= 1'b1;
              s3_cstep  <= s3_cstep + 1;
            end
            8'd15: begin
              if (ENABLE_DEBUG_OUTPUT) begin
                $display("STAGE3_3 g_n_h_o: %0x", g_n_h_o);
                $display("STAGE3_3 g_n_m_o: %0x", g_n_m_o);
                $display("STAGE3_3 g_n_n_o: %0x", g_n_n_o);
              end

              g_n_trg_o <= 1'b0;
              s3_cstep  <= s3_cstep + 1;
            end
            8'd16: begin
              if (g_n_state_i != DONE) begin
                // do nothing
              end else begin
                h_new_o     <= g_n_result_i;
                n_new_o     <= n;
                sigma_new_o <= sigma;

                s3_cstep <= s3_cstep + 1;
                s3_ready <= 1'b1;
              end
            end
            8'd17: begin
              if (ENABLE_DEBUG_OUTPUT) begin
                $display("STAGE3_4 h_new_o: %0x", h_new_o);
                $display("STAGE3_4 n_new_o: %0x", n_new_o);
                $display("STAGE3_4 sigma_new_o: %0x", sigma_new_o);
              end

              s3_cstep <= 8'd0;
              s3_ready <= 1'b0;
            end
            default: begin
              $display("Stage 3 should not be here");
            end
          endcase
        end : stage3
      end
    end
  end : stage
endmodule

