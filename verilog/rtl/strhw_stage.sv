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
  s2_cstep_t                s2_cstep;
  s3_cstep_t                s3_cstep;
  uint512                    padded_block;
  uint512                    h;
  uint512                    n;
  uint512                    sigma;

  istate_t                   istate_next;
  s2_cstep_t                s2_cstep_next;
  s3_cstep_t                s3_cstep_next;
  uint512                    padded_block_next;
  uint512                    h_next;
  uint512                    n_next;
  uint512                    sigma_next;

  uint512                    sigma_new_next;
  uint512                    n_new_next;
  uint512                    h_new_next;
  state_t                    state_next;

  uint512                    g_n_m_next;
  uint512                    g_n_n_next;
  uint512                    g_n_h_next;
  logic                      g_n_trg_next;

  // adder signals
  logic           adder_trg, adder_trg_next;
  uint512         adder_a,   adder_a_next;
  uint512         adder_b,   adder_b_next;
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

  always_ff @(posedge clk_i) begin : update_state_on_clk
    if (rst_i) begin
      sigma_new_o <= 512'h0;
      n_new_o     <= 512'h0;
      h_new_o     <= 512'h0;
      state_o    <= CLEAR;
      g_n_m_o    <= 512'h0;
      g_n_n_o    <= 512'h0;
      g_n_h_o    <= 512'h0;
      g_n_trg_o  <= 1'h0;
      adder_trg  <= 1'h0;
      adder_a    <= 512'h0;
      adder_b    <= 512'h0;
    end else begin // on clk_i
      sigma_new_o <= sigma_new_next;
      n_new_o     <= n_new_next;
      h_new_o     <= h_new_next;
      state_o     <= state_next;
      g_n_m_o     <= g_n_m_next;
      g_n_n_o     <= g_n_n_next;
      g_n_h_o     <= g_n_h_next;
      g_n_trg_o   <= g_n_trg_next;

      istate     <= istate_next;
      s2_cstep  <= s2_cstep_next;
      s3_cstep  <= s3_cstep_next;

      padded_block <= padded_block_next;

      h     <= h_next;
      n     <= n_next;
      sigma <= sigma_next;

      adder_trg <= adder_trg_next;
      adder_a   <= adder_a_next;
      adder_b   <= adder_b_next;
    end
  end : update_state_on_clk

  always_comb begin : state_machine
    h_new_next     = h_new_o;
    n_new_next     = n_new_o;
    sigma_new_next = sigma_new_o;

    g_n_m_next     = g_n_m_o;
    g_n_n_next     = g_n_n_o;
    g_n_h_next     = g_n_h_o;

    g_n_trg_next   = g_n_trg_o;
    istate_next    = istate;
    s2_cstep_next = s2_cstep;
    s3_cstep_next = s3_cstep;
    state_next     = state_o;

    padded_block_next = padded_block;

    h_next = h;
    n_next = n;
    sigma_next = sigma;

    adder_a_next = adder_a;
    adder_b_next = adder_b;
    adder_trg_next = adder_trg;

    case (istate)
      CLEAR_ST1: begin
        h_new_next     = 512'h0;
        n_new_next     = 512'h0;
        sigma_new_next = 512'h0;

        g_n_m_next     = 512'h0;
        g_n_n_next     = 512'h0;
        g_n_h_next     = 512'h0;

        g_n_trg_next   = 0;
        istate_next    = CLEAR_WAIT_TRG;
      end
      CLEAR_WAIT_TRG: begin
        if (trg_i != 1'b1) begin
          // do nothing
        end else begin
          istate_next = BUSY_WAIT_STAGE;
          state_next  = BUSY;
        end
      end
      /* verilator lint_off CASEINCOMPLETE */
      BUSY_WAIT_STAGE: begin
        if (block_size_i == BLOCK_SIZE) begin
          case (s2_cstep)
            8'd0: begin
              h_next = h_i;
              n_next = n_i;
              sigma_next = sigma_i;

              g_n_n_next = n_i;
              g_n_h_next = h_i;
              g_n_m_next = block_i;

              if (ENABLE_DEBUG_OUTPUT) begin
                $display("STAGE2 g_n_n_next: %0x", g_n_n_next);
                $display("STAGE2 g_n_h_next: %0x", g_n_h_next);
                $display("STAGE2 g_n_m_next: %0x", g_n_m_next);
              end

              g_n_trg_next = 1'b1;
              s2_cstep_next = s2_cstep + 1;
            end
            8'd1: begin
              g_n_trg_next = 1'b0;
              s2_cstep_next = s2_cstep + 1;
            end
            8'd2: begin
              if (g_n_state_i != DONE) begin
                // do nothing
              end else begin
                h_new_next = g_n_result_i;
                s2_cstep_next = s2_cstep + 1;
              end
            end
            8'd3: begin
              adder_a_next = n_i;
              adder_b_next = 512'd512;
              adder_trg_next = 1'b1;

              s2_cstep_next = s2_cstep + 1;
            end
            8'd4: begin
              adder_trg_next = 0;
              s2_cstep_next = s2_cstep + 1;
            end
            8'd5: begin
              if (adder_ready != 1'b1) begin
                // do nothing
              end else begin
                n_new_next = adder_result;
                s2_cstep_next = s2_cstep + 1;
              end
            end
            8'd6: begin
              // sigma_new_next = sigma_i + block_i;
              adder_a_next = sigma_i;
              adder_b_next = block_i;
              adder_trg_next = 1'b1;

              s2_cstep_next = s2_cstep + 1;
            end
            8'd7: begin
              adder_trg_next = 0;
              s2_cstep_next = s2_cstep + 1;
            end
            8'd8: begin
              if (adder_ready != 1'b1) begin
                // do nothing
              end else begin
                sigma_new_next = adder_result;
                s2_cstep_next = s2_cstep + 1;
              end
            end
            8'd9: begin
              if (ENABLE_DEBUG_OUTPUT) begin
                $display("STAGE2 h_new_next: %0x", h_new_next);
                $display("STAGE2 n_new_next: %0x", n_new_next);
                $display("STAGE2 sigma_new_next: %0x", sigma_new_next);
              end

              s2_cstep_next = 8'd0;
              istate_next = DONE_WAIT_TRG;
              state_next = DONE;
            end
          endcase // stage 2 case
        end else begin
          case (s3_cstep)
            8'd0: begin
              h_next = h_i;
              n_next = n_i;
              sigma_next = sigma_i;

              s3_cstep_next = s3_cstep + 1;
            end
            8'd1: begin
              padded_block_next = block_i; // block_i | (512'h1 << (block_size_i * 8));
              padded_block_next[block_size_i * 8] = 1'b1;

              s3_cstep_next = s3_cstep + 1;
            end
            8'd2: begin
              g_n_h_next = h_i;
              g_n_m_next = padded_block_next;
              g_n_n_next = n_i;

              if (ENABLE_DEBUG_OUTPUT) begin
                $display("STAGE3_1 g_n_h_next: %0x", g_n_h_next);
                $display("STAGE3_1 g_n_m_next: %0x", g_n_m_next);
                $display("STAGE3_1 g_n_n_next: %0x", g_n_n_next);
              end

              g_n_trg_next = 1'b1;
              s3_cstep_next = s3_cstep + 1;
            end
            8'd3: begin
              g_n_trg_next = 1'b0;
              s3_cstep_next = s3_cstep + 1;
            end
            8'd4: begin
              if (g_n_state_i != DONE) begin
                // do nothing
              end else begin
                h_next = g_n_result_i;
                s3_cstep_next = s3_cstep + 1;
              end
            end
            8'd5: begin
              // n_next = n_i + block_size_i * 8;
              adder_a_next = n_i;
              adder_b_next = block_size_i * 8;
              adder_trg_next = 1'b1;

              s3_cstep_next = s3_cstep + 1;
            end
            8'd6: begin
              adder_trg_next = 0;
              s3_cstep_next = s3_cstep + 1;
            end
            8'd7: begin
              if (adder_ready != 1'b1) begin
                // do nothing
              end else begin
                n_next = adder_result;
                s3_cstep_next = s3_cstep + 1;
              end
            end
            8'd8: begin
              // sigma_next = sigma_i + padded_block;
              adder_a_next = sigma_i;
              adder_b_next = padded_block;
              adder_trg_next = 1'b1;

              s3_cstep_next = s3_cstep + 1;
            end
            8'd9: begin
              adder_trg_next = 1'b0;
              s3_cstep_next = s3_cstep + 1;
            end
            8'd10: begin
              if (adder_ready != 1'b1) begin
                // do nothing
              end else begin
                sigma_next = adder_result;
                s3_cstep_next = s3_cstep + 1;
              end
            end
            8'd11: begin
              g_n_h_next = h;
              g_n_m_next = n;
              g_n_n_next = 0;

              if (ENABLE_DEBUG_OUTPUT) begin
                $display("STAGE3_2 g_n_h_next: %0x", g_n_h_next);
                $display("STAGE3_2 g_n_m_next: %0x", g_n_m_next);
                $display("STAGE3_2 g_n_n_next: %0x", g_n_n_next);
              end

              g_n_trg_next = 1'b1;
              s3_cstep_next = s3_cstep + 1;
            end
            8'd12: begin
              g_n_trg_next = 1'b0;
              s3_cstep_next = s3_cstep + 1;
            end
            8'd13: begin
              if (g_n_state_i != DONE) begin
                // do nothing
              end else begin
                h_next = g_n_result_i;
                s3_cstep_next = s3_cstep + 1;
              end
            end
            8'd14: begin
              g_n_h_next = h;
              g_n_m_next = sigma;
              g_n_n_next = 0;

              if (ENABLE_DEBUG_OUTPUT) begin
                $display("STAGE3_3 g_n_h_next: %0x", g_n_h_next);
                $display("STAGE3_3 g_n_m_next: %0x", g_n_m_next);
                $display("STAGE3_3 g_n_n_next: %0x", g_n_n_next);
              end

              g_n_trg_next = 1'b1;
              s3_cstep_next = s3_cstep + 1;
            end
            8'd15: begin
              g_n_trg_next = 1'b0;
              s3_cstep_next = s3_cstep + 1;
            end
            8'd16: begin
              if (g_n_state_i != DONE) begin
                // do nothing
              end else begin
                h_new_next = g_n_result_i;
                n_new_next = n;
                sigma_new_next = sigma;

                if (ENABLE_DEBUG_OUTPUT) begin
                  $display("STAGE3_4 h_new_next: %0x", h_new_next);
                  $display("STAGE3_4 n_new_next: %0x", n_new_next);
                  $display("STAGE3_4 sigma_new_next: %0x", sigma_new_next);
                end

                s3_cstep_next = 8'd0;

                istate_next = DONE_WAIT_TRG;
                state_next = DONE;
              end
            end
          endcase // stage 3 case
        end
      end // BUSY_WAIT_STAGE
      DONE_WAIT_TRG: begin
        if (trg_i != 1'b1) begin
          // do nothing
        end else begin
          istate_next = BUSY_WAIT_STAGE;
          state_next = BUSY;
        end
      end
    endcase
  end : state_machine
  /* verilator lint_on CASEINCOMPLETE */
endmodule
