module strhw_gn import strhw_common_types::*; #() (

    input  logic            clk_i,
    input  logic            rst_i,
    input  logic            trg_i,

    // Inputs from stage
    input  uint512          m_i,
    input  uint512          n_i,
    input  uint512          h_i,

    // Outputs to stage
    output uint512          result_o,
    output state_t          state_o,

    // Results from SL and P transformations 
    input  uint512          sl_tr_result_i,
    input  uint512          p_tr_result_i,

    // Outputs to SL and P transformations
    output uint512          sl_tr_a_o,
    output uint512          p_tr_a_o

  );

  typedef enum {
    CLEAR_ST1,
    CLEAR_WAIT_TRG,
    BUSY_ST1,
    BUSY_ST2,
    BUSY_WAIT_COMPUTE,
    DONE_WAIT_TRG
  } istate_t;

  typedef enum {
    IDLE,
    G_N_STEP1_1,
    G_N_STEP1_2,
    G_N_STEP1_3,
    G_N_STEP2,
    E_STEP1_1,
    E_STEP1_2,
    E_STEP1_3,
    E_STEP2,
    E_STEP3_1,
    E_STEP3_2,
    E_STEP3_3,
    E_STEP4,
    K_I_STEP1,
    K_I_STEP2,
    K_I_STEP3
  } gn_istate_t;

  uint512 C[12] = '{
    512'hb1085bda1ecadae9ebcb2f81c0657c1f2f6a76432e45d016714eb88d7585c4fc4b7ce09192676901a2422a08a460d31505767436cc744d23dd806559f2a64507,
    512'h6fa3b58aa99d2f1a4fe39d460f70b5d7f3feea720a232b9861d55e0f16b501319ab5176b12d699585cb561c2db0aa7ca55dda21bd7cbcd56e679047021b19bb7,
    512'hf574dcac2bce2fc70a39fc286a3d843506f15e5f529c1f8bf2ea7514b1297b7bd3e20fe490359eb1c1c93a376062db09c2b6f443867adb31991e96f50aba0ab2,
    512'hef1fdfb3e81566d2f948e1a05d71e4dd488e857e335c3c7d9d721cad685e353fa9d72c82ed03d675d8b71333935203be3453eaa193e837f1220cbebc84e3d12e,
    512'h4bea6bacad4747999a3f410c6ca923637f151c1f1686104a359e35d7800fffbdbfcd1747253af5a3dfff00b723271a167a56a27ea9ea63f5601758fd7c6cfe57,
    512'hae4faeae1d3ad3d96fa4c33b7a3039c02d66c4f95142a46c187f9ab49af08ec6cffaa6b71c9ab7b40af21f66c2bec6b6bf71c57236904f35fa68407a46647d6e,
    512'hf4c70e16eeaac5ec51ac86febf240954399ec6c7e6bf87c9d3473e33197a93c90992abc52d822c3706476983284a05043517454ca23c4af38886564d3a14d493,
    512'h9b1f5b424d93c9a703e7aa020c6e41414eb7f8719c36de1e89b4443b4ddbc49af4892bcb929b069069d18d2bd1a5c42f36acc2355951a8d9a47f0dd4bf02e71e,
    512'h378f5a541631229b944c9ad8ec165fde3a7d3a1b258942243cd955b7e00d0984800a440bdbb2ceb17b2b8a9aa6079c540e38dc92cb1f2a607261445183235adb,
    512'habbedea680056f52382ae548b2e4f3f38941e71cff8a78db1fffe18a1b3361039fe76702af69334b7a1e6c303b7652f43698fad1153bb6c374b4c7fb98459ced,
    512'h7bcd9ed0efc889fb3002c6cd635afe94d8fa6bbbebab076120018021148466798a1d71efea48b9caefbacd1d7d476e98dea2594ac06fd85d6bcaa4cd81f32d1b,
    512'h378ee767f11631bad21380b00449b17acda43c32bcdf1d77f82012d430219f9b5d80ef9d1891cc86e71da4aa88e12852faf417d5d9b21b9948bc924af11bd720
  };

  // Gn computation internals
  uint512                   e_st1_k;
  uint512                   e_st1_m;

  uint512                   e_st2_prev_k;

  uint512                   e_st3_prev_k;
  uint512                   e_st3_new_m;

  uint512                   e_st4_prev_k;
  uint512                   e_st4_new_m;

  uint512                   k_i_prev_k;
  logic[10:0]               k_i_i;

  uint512                   e_result;

  uint512                   e_st1_k_next;
  uint512                   e_st1_m_next;

  uint512                   e_st2_prev_k_next;

  uint512                   e_st3_prev_k_next;
  uint512                   e_st3_new_m_next;

  uint512                   e_st4_prev_k_next;
  uint512                   e_st4_new_m_next;

  uint512                   k_i_prev_k_next;
  logic[10:0]               k_i_i_next;

  uint512                   e_result_next;

  istate_t                  istate;
  gn_istate_t               gn_istate;
  logic                     gn_compute_start;
  uint512                   gn_result;

  istate_t                  istate_next;
  gn_istate_t               gn_istate_next;
  logic                     gn_compute_start_next;
  uint512                   gn_result_next;

  uint512                   result_next;
  state_t                   state_next;

  uint512                   sl_tr_a_next;
  uint512                   p_tr_a_next;

  always_ff @(posedge clk_i or posedge rst_i) begin : update_state_on_clk
    if (rst_i) begin
      result_o  <= 512'h0;
      state_o   <= CLEAR;
      sl_tr_a_o <= 512'h0;
      p_tr_a_o  <= 512'h0;

      istate           <= CLEAR_ST1;
      gn_istate        <= IDLE;
      gn_compute_start <= 1'b0;
      gn_result        <= 512'h0;

      e_st1_k          <= 512'h0;
      e_st1_m          <= 512'h0;

      e_st2_prev_k     <= 512'h0;

      e_st3_prev_k     <= 512'h0;
      e_st3_new_m      <= 512'h0;

      e_st4_prev_k     <= 512'h0;
      e_st4_new_m      <= 512'h0;

      k_i_prev_k       <= 512'h0;
      k_i_i            <= 11'h0;

      e_result         <= 512'h0;

    end else begin // on clk_i
      // Output values
      result_o  <= result_next;
      state_o   <= state_next;
      sl_tr_a_o <= sl_tr_a_next;
      p_tr_a_o  <= p_tr_a_next;

      // Internal registers
      istate           <= istate_next;
      gn_istate        <= gn_istate_next;
      gn_compute_start <= gn_compute_start_next;
      gn_result        <= gn_result_next;

      e_st1_k          <= e_st1_k_next;
      e_st1_m          <= e_st1_m_next;

      e_st2_prev_k     <= e_st2_prev_k_next;

      e_st3_prev_k     <= e_st3_prev_k_next;
      e_st3_new_m      <= e_st3_new_m_next;

      e_st4_prev_k     <= e_st4_prev_k_next;
      e_st4_new_m      <= e_st4_new_m_next;

      k_i_prev_k       <= k_i_prev_k_next;
      k_i_i            <= k_i_i_next;

      e_result         <= e_result_next;
    end
  end : update_state_on_clk

  always_comb begin : main_state_machine
    istate_next           = istate;
    gn_compute_start_next = gn_compute_start;

    result_next   = result_o;
    state_next    = state_o;

    case (istate)
      CLEAR_ST1: begin
        result_next  = 512'h0;
        istate_next  = CLEAR_WAIT_TRG;
      end
      CLEAR_WAIT_TRG: begin
        if (trg_i != 1'b1) begin
          // do nothing
        end else begin
          istate_next = BUSY_ST1;
          state_next  = BUSY;
        end
      end
      BUSY_ST1: begin
        gn_compute_start_next = 1'b1;
        istate_next = BUSY_ST2;
      end
      BUSY_ST2: begin
        gn_compute_start_next = 1'b0;
        istate_next = BUSY_WAIT_COMPUTE;
      end
      BUSY_WAIT_COMPUTE: begin
        if (gn_istate != IDLE) begin
          // do nothing
        end else begin
          istate_next = DONE_WAIT_TRG;
          state_next  = DONE;
          result_next = gn_result;
        end
      end
      DONE_WAIT_TRG: begin
        if (trg_i != 1'b1) begin
          // do nothing
        end else begin
          istate_next = BUSY_ST1;
          state_next  = BUSY;
        end
      end
    endcase
  end : main_state_machine

  always_comb begin : compute_gn_state_machine
    gn_istate_next = gn_istate;
    sl_tr_a_next   = sl_tr_a_o;
    p_tr_a_next    = p_tr_a_o;
    gn_result_next = gn_result;

    e_st1_k_next   = e_st1_k;
    e_st1_m_next   = e_st1_m;

    e_st2_prev_k_next = e_st2_prev_k;

    e_st3_prev_k_next = e_st3_prev_k;
    e_st3_new_m_next  = e_st3_new_m;

    e_st4_prev_k_next = e_st4_prev_k;
    e_st4_new_m_next  = e_st4_new_m;

    k_i_prev_k_next = k_i_prev_k;
    k_i_i_next      = k_i_i;

    e_result_next   = e_result;

    case (gn_istate)
      IDLE: begin
        if (gn_compute_start != 1'b1) begin
          // do nothing
        end else begin
          k_i_i_next = 0;
          gn_istate_next = G_N_STEP1_1;
        end
      end
      G_N_STEP1_1: begin
        p_tr_a_next = h_i ^ n_i;

        if (ENABLE_DEBUG_OUTPUT) begin
          $display("G_N_STEP1_1 p_tr_a_next = %0x", p_tr_a_next);
        end

        gn_istate_next = G_N_STEP1_2;
      end
      G_N_STEP1_2: begin
        sl_tr_a_next = p_tr_result_i;

        if (ENABLE_DEBUG_OUTPUT) begin
          $display("G_N_STEP1_2 sl_tr_a_next = %0x", sl_tr_a_next);
        end

        gn_istate_next = G_N_STEP1_3;
      end
      G_N_STEP1_3: begin
        e_st1_k_next = sl_tr_result_i;
        e_st1_m_next = m_i;

        if (ENABLE_DEBUG_OUTPUT) begin
          $display("G_N_STEP1_3 e_st1_k_next = %0x", e_st1_k_next);
          $display("G_N_STEP1_3 e_st1_m_next = %0x", e_st1_m_next);
        end

        gn_istate_next = E_STEP1_1;
      end
      E_STEP1_1: begin
        p_tr_a_next = e_st1_k ^ e_st1_m;

        if (ENABLE_DEBUG_OUTPUT) begin
          $display("E_STEP1_1 p_tr_a_next = %0x", p_tr_a_next);
        end

        gn_istate_next = E_STEP1_2;
      end
      E_STEP1_2: begin
        sl_tr_a_next = p_tr_result_i;

        if (ENABLE_DEBUG_OUTPUT) begin
          $display("E_STEP1_2 sl_tr_a_next = %0x", sl_tr_a_next);
        end

        gn_istate_next = E_STEP1_3;
      end
      E_STEP1_3: begin
        e_st3_new_m_next = sl_tr_result_i;
        e_st2_prev_k_next = e_st1_k;

        if (ENABLE_DEBUG_OUTPUT) begin
          $display("E_STEP1_3 e_st3_new_m_next = %0x", e_st3_new_m_next);
          $display("E_STEP1_3 e_st2_prev_k_next = %0x", e_st2_prev_k_next);
        end

        gn_istate_next = E_STEP2;
      end
      E_STEP2: begin
        k_i_i_next = k_i_i + 1;
        k_i_prev_k_next = e_st2_prev_k;

        if (ENABLE_DEBUG_OUTPUT) begin
          $display("E_STEP2 k_i_i_next = %0d", k_i_i_next);
          $display("E_STEP2 k_i_prev_k_next = %0x", k_i_prev_k_next);
        end

        if (k_i_i_next <= C_SIZE) begin
          gn_istate_next = K_I_STEP1;
        end else begin
          e_st4_prev_k_next = e_st2_prev_k;

          if (ENABLE_DEBUG_OUTPUT) begin
            $display("E_STEP2 e_st4_prev_k_next = %0x", e_st4_prev_k_next);
          end

          gn_istate_next = E_STEP4;
        end
      end
      K_I_STEP1: begin
        p_tr_a_next = k_i_prev_k ^ C[k_i_i - 1];

        if (ENABLE_DEBUG_OUTPUT) begin
          $display("K_I_STEP1 p_tr_a_next = %0x", p_tr_a_next);
        end

        gn_istate_next = K_I_STEP2;
      end
      K_I_STEP2: begin
        sl_tr_a_next = p_tr_result_i;

        if (ENABLE_DEBUG_OUTPUT) begin
          $display("K_I_STEP2 sl_tr_a_next = %0x", sl_tr_a_next);
        end

        gn_istate_next = K_I_STEP3;
      end
      K_I_STEP3: begin
        if (k_i_i < C_SIZE) begin
          e_st3_prev_k_next = sl_tr_result_i;

          if (ENABLE_DEBUG_OUTPUT) begin
            $display("K_I_STEP3 e_st3_prev_k_next = %0x", e_st3_prev_k_next);
          end

          gn_istate_next = E_STEP3_1;
        end else begin
          e_st2_prev_k_next = sl_tr_result_i;

          if (ENABLE_DEBUG_OUTPUT) begin
            $display("K_I_STEP3 e_st2_prev_k_next = %0x", e_st2_prev_k_next);
          end

          gn_istate_next = E_STEP2;
        end
      end
      E_STEP3_1: begin
        p_tr_a_next = e_st3_new_m ^ e_st3_prev_k;

        if (ENABLE_DEBUG_OUTPUT) begin
          $display("E_STEP3_1 p_tr_a_next = %0x", p_tr_a_next);
        end

        gn_istate_next = E_STEP3_2;
      end
      E_STEP3_2: begin
        sl_tr_a_next = p_tr_result_i;

        if (ENABLE_DEBUG_OUTPUT) begin
          $display("E_STEP3_2 sl_tr_a_next = %0x", sl_tr_a_next);
        end

        gn_istate_next = E_STEP3_3;
      end
      E_STEP3_3: begin
        e_st3_new_m_next = sl_tr_result_i;
        e_st2_prev_k_next = e_st3_prev_k;

        if (ENABLE_DEBUG_OUTPUT) begin
          $display("E_STEP3_3 e_st3_new_m_next = %0x", e_st3_new_m_next);
          $display("E_STEP3_3 e_st2_prev_k_next = %0x", e_st2_prev_k_next);
        end

        gn_istate_next = E_STEP2;
      end
      E_STEP4: begin
        e_result_next = e_st3_new_m ^ e_st4_prev_k;

        if (ENABLE_DEBUG_OUTPUT) begin
          $display("E_STEP4 e_result_next = %0x", e_result_next);
        end

        gn_istate_next = G_N_STEP2;
      end
      G_N_STEP2: begin
        gn_result_next = e_result ^ h_i ^ m_i;

        if (ENABLE_DEBUG_OUTPUT) begin
          $display("G_N_STEP2 gn_result_next = %0x", gn_result_next);
        end

        gn_istate_next = IDLE;
      end
    endcase
  end : compute_gn_state_machine
endmodule

