module strhw import strhw_common_types::*; #() (
    input  logic             rst_i,
    input  logic             clk_i,

    input  logic             trg_i,
    output state_t           state_o,

    input  uint512           block_i,
    input  uint7             block_size_i,
    input  logic             hash_size_i,

    output uint512           hash_o
  );

  // Control logic to outside connections
  logic                      cl_trg;
  uint512                    cl_block;
  uint7                      cl_block_size;
  logic                      cl_hash_size;
  uint512                    cl_hash;
  state_t                    cl_state;

  // Control logic to state connections
  state_t                    st_state;
  logic                      st_trg;
  uint512                    st_block;
  uint7                      st_block_size;
  uint512                    st_sigma;
  uint512                    st_n;
  uint512                    st_h;
  uint512                    st_sigma_new;
  uint512                    st_n_new;
  uint512                    st_h_new;

  // State to gn connections
  uint512                    g_n_result;
  state_t                    g_n_state;

  uint512                    g_n_m;
  uint512                    g_n_n;
  uint512                    g_n_h;
  logic                      g_n_trg;

  // Gn to transformations connections
  uint512                    sl_tr_result;
  uint512                    p_tr_result;

  uint512                    sl_tr_a;
  uint512                    p_tr_a;

  assign cl_trg        = trg_i;
  assign cl_block      = block_i;
  assign cl_block_size = block_size_i;
  assign cl_hash_size  = hash_size_i;
  assign hash_o        = cl_hash;
  assign state_o       = cl_state;

  strhw_control_logic control_logic_inst (
    .rst_i           (rst_i),
    .clk_i           (clk_i),
    .trg_i           (cl_trg),
    .state_o         (cl_state),
    .block_i         (cl_block),
    .block_size_i    (cl_block_size),
    .hash_size_i     (cl_hash_size),
    .hash_o          (cl_hash),
    .st_state_i      (st_state),
    .st_trg_o        (st_trg),
    .st_block_o      (st_block),
    .st_block_size_o (st_block_size),
    .st_sigma_o      (st_sigma),
    .st_n_o          (st_n),
    .st_h_o          (st_h),
    .st_sigma_new_i  (st_sigma_new),
    .st_n_new_i      (st_n_new),
    .st_h_new_i      (st_h_new)
  );

  // Instantiate the stage module
  strhw_stage stage_inst (
    .clk_i        (clk_i),
    .rst_i        (rst_i),
    .trg_i        (st_trg),
    .block_i      (st_block),
    .block_size_i (st_block_size),
    .sigma_i      (st_sigma),
    .n_i          (st_n),
    .h_i          (st_h),
    .sigma_new_o  (st_sigma_new),
    .n_new_o      (st_n_new),
    .h_new_o      (st_h_new),
    .state_o      (st_state),
    .g_n_result_i (g_n_result),
    .g_n_state_i  (g_n_state),
    .g_n_m_o      (g_n_m),
    .g_n_n_o      (g_n_n),
    .g_n_h_o      (g_n_h),
    .g_n_trg_o    (g_n_trg)
  );

  // Instantiate the strhw_gn module
  strhw_gn gn_inst (
    .clk_i          (clk_i),
    .rst_i          (rst_i),
    .trg_i          (g_n_trg),
    .m_i            (g_n_m),
    .n_i            (g_n_n),
    .h_i            (g_n_h),
    .result_o       (g_n_result),
    .state_o        (g_n_state),
    .sl_tr_result_i (sl_tr_result),
    .p_tr_result_i  (p_tr_result),
    .sl_tr_a_o      (sl_tr_a),
    .p_tr_a_o       (p_tr_a)
  );

  // Instantiate the strhw_sl module
  strhw_sl sl_inst (
    .a_i      (sl_tr_a),
    .result_o (sl_tr_result)
  );

  // Instantiate the strhw_p module
  strhw_p p_inst (
    .a_i      (p_tr_a),
    .result_o (p_tr_result)
  );

endmodule
