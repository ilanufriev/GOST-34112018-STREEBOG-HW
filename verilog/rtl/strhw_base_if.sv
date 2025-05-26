interface strhw_base_if #() (
    input clk,
    input rst
  );

  logic       trg;
  logic [1:0] state;

  modport initor(input  state, output trg);
  modport target(output state, input  trg);
endinterface

