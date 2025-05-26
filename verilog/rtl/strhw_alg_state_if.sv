interface strhw_alg_state_if #() ();
  logic [511:0] sigma;
  logic [511:0] n;
  logic [511:0] h;

  modport initor(output sigma, output h, output n);
  modport target(input  sigma, input  h, input  n);
endinterface

