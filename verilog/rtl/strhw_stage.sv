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

  typedef enum {
    S2_1,
    S2_2,
    S2_3,
    S2_4
  } s2_istate_t;

  typedef enum {
    S3_1,
    S3_2,
    S3_3,
    S3_4,
    S3_5,
    S3_6,
    S3_7,
    S3_8,
    S3_9
  } s3_istate_t;

  // Internal registers
  istate_t                   istate;
  s2_istate_t                s2_istate;
  s3_istate_t                s3_istate;
  uint512                    padded_block;
  uint512                    h;
  uint512                    n;
  uint512                    sigma;

  istate_t                   istate_next;
  s2_istate_t                s2_istate_next;
  s3_istate_t                s3_istate_next;
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

  always_ff @(posedge clk_i or posedge rst_i) begin : update_state_on_clk
    if (rst_i) begin
      sigma_new_o <= 512'h0;
      n_new_o     <= 512'h0;
      h_new_o     <= 512'h0;
      state_o    <= CLEAR;
      g_n_m_o    <= 512'h0;
      g_n_n_o    <= 512'h0;
      g_n_h_o    <= 512'h0;
      g_n_trg_o  <= 1'h0;
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
      s2_istate  <= s2_istate_next;
      s3_istate  <= s3_istate_next;

      padded_block <= padded_block_next;

      h     <= h_next;
      n     <= n_next;
      sigma <= sigma_next;
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
    s2_istate_next = s2_istate;
    s3_istate_next = s3_istate;

    padded_block_next = padded_block;

    h_next = h;
    n_next = n;
    sigma_next = sigma;

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
      BUSY_WAIT_STAGE: begin
        if (block_size_i == BLOCK_SIZE) begin
          case (s2_istate)
            S2_1: begin
              g_n_n_next = n_i;
              g_n_h_next = h_i;
              g_n_m_next = block_i;

              if (ENABLE_DEBUG_OUTPUT) begin
                $display("STAGE2 g_n_n_next: %0x", g_n_n_next);
                $display("STAGE2 g_n_h_next: %0x", g_n_h_next);
                $display("STAGE2 g_n_m_next: %0x", g_n_m_next);
              end

              g_n_trg_next = 1'b1;
              s2_istate_next = S2_2;
            end
            S2_2: begin
              g_n_trg_next = 1'b0;
              s2_istate_next = S2_3;
            end
            S2_3: begin
              if (g_n_state_i != DONE) begin
                // do nothing
              end else begin
                s2_istate_next = S2_4;
              end
            end
            S2_4: begin
              h_new_next = g_n_result_i;
              n_new_next = n_i + 512'd512;
              sigma_new_next = sigma_i + block_i;

              if (ENABLE_DEBUG_OUTPUT) begin
                $display("STAGE2 h_new_next: %0x", h_new_next);
                $display("STAGE2 n_new_next: %0x", n_new_next);
                $display("STAGE2 sigma_new_next: %0x", sigma_new_next);
              end

              s2_istate_next = S2_1;

              istate_next = DONE_WAIT_TRG;
              state_next = DONE;
            end
          endcase // stage 2 case
        end else begin
          case (s3_istate)
            S3_1: begin
              padded_block_next = block_i | (512'h1 << (block_size_i * 8));

              g_n_h_next = h_i;
              g_n_m_next = padded_block_next;
              g_n_n_next = n_i;

              if (ENABLE_DEBUG_OUTPUT) begin
                $display("STAGE3_1 g_n_h_next: %0x", g_n_h_next);
                $display("STAGE3_1 g_n_m_next: %0x", g_n_m_next);
                $display("STAGE3_1 g_n_n_next: %0x", g_n_n_next);
              end

              g_n_trg_next = 1'b1;
              s3_istate_next = S3_2;
            end
            S3_2: begin
              g_n_trg_next = 1'b0;
              s3_istate_next = S3_3;
            end
            S3_3: begin
              if (g_n_state_i != DONE) begin
                // do nothing
              end else begin
                h_next = g_n_result_i;
                n_next = n_i + block_size_i * 8;
                sigma_next = sigma_i + padded_block;

                s3_istate_next = S3_4;
              end
            end
            S3_4: begin
              g_n_h_next = h;
              g_n_m_next = n;
              g_n_n_next = 0;

              if (ENABLE_DEBUG_OUTPUT) begin
                $display("STAGE3_2 g_n_h_next: %0x", g_n_h_next);
                $display("STAGE3_2 g_n_m_next: %0x", g_n_m_next);
                $display("STAGE3_2 g_n_n_next: %0x", g_n_n_next);
              end

              g_n_trg_next = 1'b1;
              s3_istate_next = S3_5;
            end
            S3_5: begin
              g_n_trg_next = 1'b0;
              s3_istate_next = S3_6;
            end
            S3_6: begin
              if (g_n_state_i != DONE) begin
                // do nothing
              end else begin
                h_next = g_n_result_i;
                s3_istate_next = S3_7;
              end
            end
            S3_7: begin
              g_n_h_next = h;
              g_n_m_next = sigma;
              g_n_n_next = 0;

              if (ENABLE_DEBUG_OUTPUT) begin
                $display("STAGE3_3 g_n_h_next: %0x", g_n_h_next);
                $display("STAGE3_3 g_n_m_next: %0x", g_n_m_next);
                $display("STAGE3_3 g_n_n_next: %0x", g_n_n_next);
              end

              g_n_trg_next = 1'b1;
              s3_istate_next = S3_8;
            end
            S3_8: begin
              g_n_trg_next = 1'b0;
              s3_istate_next = S3_9;
            end
            S3_9: begin
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

                s3_istate_next = S3_1;

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
endmodule
