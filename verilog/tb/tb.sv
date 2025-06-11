`define CLOCK_PERIOD 10

module tb;

  /* verilator lint_off UNUSEDSIGNAL */
  import strhw_common_types::*;

  byte              buffer[BLOCK_SIZE];
  int               clock_counter;
  int               bytes_read;

  logic             rst;
  logic             clk;
  logic             trg;
  state_t           state;
  uint512           block;
  uint7             block_size;
  logic             hash_size;
  uint512           hash;

  strhw uut (
    .rst_i(rst),
    .clk_i(clk),
    .trg_i(trg),
    .state_o(state),
    .block_i(block),
    .block_size_i(block_size),
    .hash_size_i(hash_size),
    .hash_o(hash)
  );

  initial begin
    $dumpfile("waveforms.vcd");
    $dumpvars(0, tb);
  end

  always @(posedge clk) begin
    clock_counter <= clock_counter + 1;
  end

  initial begin
    clk = 0;
    clock_counter = 0;
    forever #(`CLOCK_PERIOD/2) clk = ~clk; // 10 ns clock period
  end

  task WAIT(input int clock_cycles);
  begin
    for (int i = 0; i < clock_cycles; i++) begin
      #(`CLOCK_PERIOD);
    end
  end
  endtask

  generate
    genvar geni;
    for (geni = 0; geni < BLOCK_SIZE; geni++) begin
      assign block[(geni * 8) + 7:(geni * 8)] = buffer[geni];
    end
  endgenerate

  initial begin
    int file;
    string filename;

    rst = 1;
    trg = 0;

    WAIT(5);

    rst = 0;
    trg = 0;

    WAIT(5);

    $value$plusargs("file=%s", filename);
    file = $fopen(filename, "rb");
    if (file != 0) begin
      while (1) begin
        for (int i = 0; i < BLOCK_SIZE; i++) begin // clear the buffer just in case
          buffer[i] = 0;
        end
        bytes_read = $fread(buffer, file);

        // set the block and hash size

        /* verilator lint_off WIDTHTRUNC */

        block_size = bytes_read;

        if ($test$plusargs("h256")) begin
          hash_size  = 1'b1;
        end else begin
          hash_size  = 1'b0;
        end

        /* verilator lint_on WIDTHTRUNC */

        if (ENABLE_DEBUG_OUTPUT) begin
          $display("TB block = %0x", block);
          $display("TB block_size = %0x", block_size);
          $display("TB hash_size = %0x", hash_size);
        end

        trg = 1'b1;

        WAIT(1);

        trg = 1'b0;

        while (state != READY && state != DONE) begin
          WAIT(1);
        end

        if (bytes_read < 64) begin
          break;
        end
      end

      if ($test$plusargs("h256")) begin
        $display("%h", hash[255:0]);
      end else begin
        $display("%h", hash);
      end

      $fclose(file);
    end else begin
      $display("File descriptor is %d. Could not read it.", file);
    end

    if ($test$plusargs("clkcount")) begin
      $display("clks: %d", clock_counter);
    end

    $finish;
  end
endmodule

