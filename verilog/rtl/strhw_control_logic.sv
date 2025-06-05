module strhw_control_logic import strhw_common_types::*;  #() (
    input  logic             rst_i,
    input  logic             clk_i,

    // Initiator side of the control logic's base interface
    input  logic             trg_i,
    input  state_t           state_o, 

    // Initiator side of the control logic's data interface
    input  uint512           block_i,
    input  uint6             block_size_i,
    input  logic             hash_size_i,
    output uint512           hash_o,

    // Stage block's side of the control logic's base interface
    input  state_t           st_state_i,
    output logic             st_trg_o,

    // Stage block's side of the control logic's data interface
    output uint512           st_block_o,
    output uint6             st_block_size_o,
    
    // Algorithm's state passed to Stage
    output uint512           st_sigma_o,
    output uint512           st_n_o,
    output uint512           st_h_o,

    // Algorithm's NEW state passed to control logic from state
    input  uint512           st_sigma_new_i,
    input  uint512           st_n_new_i,
    input  uint512           st_h_new_i

  );

  state_t                    state_next; 
  uint512                    hash_next;
  logic                      st_trg_next;
  uint512                    st_block_next;
  uint6                      st_block_size_next;
  uint512                    st_sigma_next;
  uint512                    st_n_next;
  uint512                    st_h_next;

  always_ff @(posedge clk_i or posedge rst_i) begin : update_state_on_clk
    if (rst_i) begin
      // reset signals
      state_o         <= CLEAR; 
      hash_o          <= 512'h0;
      st_trg_o        <= 0;
      st_block_o      <= 512'h0;
      st_block_size_o <= 6'h0;
      st_sigma_o      <= 512'h0;
      st_n_o          <= 512'h0;
      st_h_o          <= 512'h0;
    end else begin
      state_o         <= state_next; 
      hash_o          <= hash_next;
      st_trg_o        <= st_trg_next;
      st_block_o      <= st_block_next;
      st_block_size_o <= st_block_size_next;
      st_sigma_o      <= st_sigma_next;
      st_n_o          <= st_n_next;
      st_h_o          <= st_h_next;
    end
  end : update_state_on_clk

  always_comb begin : state_machine
    case (state_o)
      CLEAR: begin
        hash_next = 512'h0;
        st_block_next = 512'h0;
        st_block_size_next = 6'h0;
        st_sigma_next = 512'h0;
      end
      BUSY:  begin

      end
      READY: begin

      end
      DONE: begin

      end
    endcase    
  end : state_machine

endmodule

