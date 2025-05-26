interface strhw_data_if #() ();
  logic [511:0] block;
  logic [5:0]   block_size;
  logic         hash_size;

  modport initor(output block, output block_size, output hash_size);
  modport target(input  block, input  block_size, input  hash_size);
endinterface

