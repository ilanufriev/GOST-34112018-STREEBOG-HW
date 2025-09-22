`include "strhw_common.svh"

module strhw_control_logic #() (
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
  uint512                    h;
  uint7                      block_size;

  istate_t                   istate_next;
  uint512                    h_next;
  uint7                      block_size_next;

  state_t                    state_next;
  uint512                    hash_next;
  logic                      st_trg_next;

  assign st_block_o        = block_i;
  assign st_block_size_o   = block_size;
  assign st_sigma_o        = st_sigma_new_i;
  assign st_n_o            = st_n_new_i;
  assign st_h_o            = h;

  always_ff @(posedge clk_i) begin : update_state_on_clk
    if (rst_i) begin
      // reset signals
      state_o         <= CLEAR;
      hash_o          <= 512'h0;
      st_trg_o        <= 0;
    end else begin
      // Outputs of the module
      state_o         <= state_next; 
      hash_o          <= hash_next;
      st_trg_o        <= st_trg_next;

      // Internal registers
      istate          <= istate_next;
      h               <= h_next;
      block_size      <= block_size_next;
    end
  end : update_state_on_clk

  always_comb begin : state_machine
    istate_next        = istate;
    h_next             = h;
    block_size_next    = block_size;

    state_next         = state_o;
    hash_next          = hash_o;
    st_trg_next        = st_trg_o;

    case (istate)
      CLEAR_ST1: begin
        hash_next          = 512'h0;
        st_trg_next        = 1'b0;

        h_next             = 512'h0;

        istate_next        = CLEAR_WAIT_TRG;
      end
      CLEAR_WAIT_TRG: begin
        if (trg_i == 0) begin
          // do nothing
        end else begin
          if (hash_size_i == 1'b0) begin
            h_next = INIT_VECTOR_512;
          end else begin
            h_next = INIT_VECTOR_256;
          end

          state_next = BUSY;
          istate_next = BUSY_ST1;
        end
      end
      BUSY_ST1: begin
        // save block and it's size into internals
        if (block_size_i > 64) begin
          block_size_next = 64;
        end else begin
          block_size_next = block_size_i;
        end

        // start the stage
        st_trg_next        = 1'b1;
        istate_next        = BUSY_ST2;
      end
      BUSY_ST2: begin
        // unset the trigger, stage should be at BUSY
        // on the next clk
        st_trg_next        = 1'b0;
        istate_next        = BUSY_WAIT_ST_DONE;
      end
      BUSY_WAIT_ST_DONE: begin
        if (st_state_i != DONE) begin
          // do nothing
        end else begin
          istate_next = BUSY_ST3;
        end
      end
      BUSY_ST3: begin
        h_next = st_h_new_i;

        if (block_size < 64) begin
          istate_next = DONE_WAIT_TRG;
          state_next  = DONE;

          if (hash_size_i == 1'b0) begin // hash_size == 512
            hash_next = h_next;
          end else begin                 // hash_size == 256
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

