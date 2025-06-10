module strhw_control_logic import strhw_common_types::*;  #() (
    input  logic             rst_i,
    input  logic             clk_i,

    // Initiator side of the control logic's base interface
    input  logic             trg_i,
    output state_t           state_o, 

    // Initiator side of the control logic's data interface
    input  uint512           block_i,
    input  uint7             block_size_i,
    input  logic             hash_size_i,
    output uint512           hash_o,

    // Stage block's side of the control logic's base interface
    input  state_t           st_state_i,
    output logic             st_trg_o,

    // Stage block's side of the control logic's data interface
    output uint512           st_block_o,
    output uint7             st_block_size_o,

    // Algorithm's state passed to Stage
    output uint512           st_sigma_o,
    output uint512           st_n_o,
    output uint512           st_h_o,

    // Algorithm's NEW state passed to control logic from state
    input  uint512           st_sigma_new_i,
    input  uint512           st_n_new_i,
    input  uint512           st_h_new_i

  );

  typedef enum {
    CLEAR_ST1,
    CLEAR_WAIT_TRG,
    CLEAR_ST2,
    BUSY_ST1,
    BUSY_ST2,
    BUSY_WAIT_ST_BUSY,
    BUSY_WAIT_ST_DONE,
    BUSY_ST3,
    READY_WAIT_TRG,
    DONE_WAIT_TRG
  } istate_t;

  istate_t                   istate;
  uint512                    sigma;
  uint512                    h;
  uint512                    n;
  uint512                    block;
  uint7                      block_size;
  logic                      hash_size;

  istate_t                   istate_next;
  uint512                    sigma_next;
  uint512                    h_next;
  uint512                    n_next;
  uint512                    block_next;
  uint7                      block_size_next;
  logic                      hash_size_next;

  state_t                    state_next; 
  uint512                    hash_next;
  logic                      st_trg_next;
  uint512                    st_block_next;
  uint7                      st_block_size_next;
  uint512                    st_sigma_next;
  uint512                    st_n_next;
  uint512                    st_h_next;

  always_ff @(posedge clk_i) begin : update_state_on_clk
    if (rst_i) begin
      // reset signals
      state_o         <= CLEAR; 
      hash_o          <= 512'h0;
      st_trg_o        <= 0;
      st_block_o      <= 512'h0;
      st_block_size_o <= 7'h0;
      st_sigma_o      <= 512'h0;
      st_n_o          <= 512'h0;
      st_h_o          <= 512'h0;
    end else begin
      // Outputs of the module
      state_o         <= state_next; 
      hash_o          <= hash_next;
      st_trg_o        <= st_trg_next;
      st_block_o      <= st_block_next;
      st_block_size_o <= st_block_size_next;
      st_sigma_o      <= st_sigma_next;
      st_n_o          <= st_n_next;
      st_h_o          <= st_h_next;

      // Internal registers
      istate          <= istate_next;
      sigma           <= sigma_next;
      n               <= n_next;
      h               <= h_next;
      block           <= block_next;
      block_size      <= block_size_next;
      hash_size       <= hash_size_next;
    end
  end : update_state_on_clk

  always_comb begin : state_machine
    istate_next        = istate;
    sigma_next         = sigma;
    h_next             = h;
    n_next             = n;
    block_next         = block;
    block_size_next    = block_size;
    hash_size_next     = hash_size;

    state_next         = state_o;
    hash_next          = hash_o;
    st_trg_next        = st_trg_o;
    st_block_next      = st_block_o;
    st_block_size_next = st_block_size_o;
    st_sigma_next      = st_sigma_o;
    st_n_next          = st_n_o;
    st_h_next          = st_h_o;

    case (istate)
      CLEAR_ST1: begin
        hash_next          = 512'h0;
        st_block_next      = 512'h0;
        st_block_size_next = 7'h0;
        st_sigma_next      = 512'h0;
        st_n_next          = 512'h0;
        st_h_next          = 512'h0;
        st_trg_next        = 1'b0;

        sigma_next         = 512'h0;
        n_next             = 512'h0;
        h_next             = 512'h0;

        istate_next        = CLEAR_WAIT_TRG;
      end
      CLEAR_WAIT_TRG: begin
        if (trg_i == 0) begin
          // do nothing
        end else begin
          istate_next = CLEAR_ST2;
        end
      end
      CLEAR_ST2: begin
        if (hash_size_i == 1'b0) begin
          h_next = INIT_VECTOR_512;
        end else begin
          h_next = INIT_VECTOR_256;
        end

        state_next = BUSY;
        istate_next = BUSY_ST1;
      end
      BUSY_ST1: begin
        // save block and it's size into internals
        block_next         = block_i;
        hash_size_next     = hash_size_i;

        if (block_size_i > 64) begin
          block_size_next = 64;
        end else begin
          block_size_next = block_size_i;
        end

        // set stage's inputs
        st_sigma_next      = sigma;
        st_h_next          = h;
        st_n_next          = n;

        st_block_next      = block_next;
        st_block_size_next = block_size_next;

        if (ENABLE_DEBUG_OUTPUT) begin
          $display("CONTROL st_h_next: %0x", st_h_next);
          $display("CONTROL st_n_next: %0x", st_n_next);
          $display("CONTROL st_block_next: %0x", st_block_next);
          $display("CONTROL st_block_size_next: %0x", st_block_size_next);
        end

        // start the stage
        st_trg_next        = 1'b1;
        istate_next        = BUSY_ST2;
      end
      BUSY_ST2: begin
        // unset the trigger, stage should be at BUSY
        // on the next clk
        st_trg_next        = 1'b0;
        istate_next        = BUSY_WAIT_ST_BUSY;
      end
      BUSY_WAIT_ST_BUSY: begin
        if (st_state_i != BUSY) begin
          // do nothing
        end else begin
          istate_next = BUSY_WAIT_ST_DONE;
        end
      end
      BUSY_WAIT_ST_DONE: begin
        if (st_state_i != DONE) begin
          // do nothing
        end else begin
          istate_next = BUSY_ST3;
        end
      end
      BUSY_ST3: begin
        sigma_next    = st_sigma_new_i;
        n_next        = st_n_new_i;
        h_next        = st_h_new_i;

        if (block_size < 64) begin
          istate_next = DONE_WAIT_TRG;
          state_next  = DONE;

          if (hash_size == 1'b0) begin // hash_size == 512
            hash_next = h_next;
          end else begin              // hash_size == 256
            hash_next = h_next >> 256;
          end

          if (ENABLE_DEBUG_OUTPUT) begin
            $display("CONTROL hash_next: %0x", hash_next);
          end
        end else begin
          istate_next = READY_WAIT_TRG;
          state_next  = READY;
        end
      end
      READY_WAIT_TRG: begin
        if (trg_i != 1'b1) begin
          // do nothing
        end else begin
          state_next  = BUSY;
          istate_next = BUSY_ST1;
        end
      end
      DONE_WAIT_TRG: begin
        if (trg_i != 1'b1) begin
          // do nothing
        end else begin
          state_next  = CLEAR;
          istate_next = CLEAR_ST1;
        end
      end
    endcase
  end : state_machine

endmodule

