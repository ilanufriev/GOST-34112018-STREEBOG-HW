module strhw_stage import strhw_common_types::*; #() (

    // Inputs from the Control logic
    input  logic             clk_i,
    input  logic             rst_i,
    input  logic             trg_i,
    input  uint512           block_i,
    input  uint6             block_size_i,
    input  uint512           sigma_i,
    input  uint512           n_i,
    input  uint512           h_i,

    // Outputs to the Control logic
    output uint512           sigma_nx_o,
    output uint512           n_nx_o,
    output uint512           h_nx_o,
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
  
  uint512                    sigma_nx_next;
  uint512                    n_nx_next;
  uint512                    h_nx_next;
  state_t                    state_next;

  uint512                    g_n_m_next;
  uint512                    g_n_n_next;
  uint512                    g_n_h_next;
  logic                      g_n_trg_next;

  always_ff @(posedge clk_i or posedge rst_i) begin : update_state_on_clk
    if (rst_i) begin
      sigma_nx_o <= 512'h0;
      n_nx_o     <= 512'h0;
      h_nx_o     <= 512'h0;
      state_o    <= CLEAR;
      g_n_m_o    <= 512'h0;
      g_n_n_o    <= 512'h0;
      g_n_h_o    <= 512'h0;
      g_n_trg_o  <= 1'h0;
    end else begin // on clk_i
      sigma_nx_o <= sigma_nx_next;
      n_nx_o     <= n_nx_next;
      h_nx_o     <= h_nx_next;
      state_o    <= state_next;
      g_n_m_o    <= g_n_m_next;
      g_n_n_o    <= g_n_n_next;
      g_n_h_o    <= g_n_h_next;
      g_n_trg_o  <= g_n_trg_next;
    end
  end : update_state_on_clk

endmodule
