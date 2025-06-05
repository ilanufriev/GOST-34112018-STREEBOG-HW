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
    end else begin // on clk_i
      result_o  <= result_next;
      state_o   <= state_next;
      sl_tr_a_o <= sl_tr_a_next;
      p_tr_a_o  <= p_tr_a_next;
    end
  end : update_state_on_clk

endmodule

