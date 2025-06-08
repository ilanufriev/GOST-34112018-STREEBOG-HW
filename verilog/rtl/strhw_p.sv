module strhw_p import strhw_common_types::*; #() (
    input  uint512            a_i,
    output uint512            result_o
  );

  logic[5:0] TAU[64] = '{
    6'd0,  6'd8, 6'd16, 6'd24, 6'd32, 6'd40, 6'd48, 6'd56,
    6'd1,  6'd9, 6'd17, 6'd25, 6'd33, 6'd41, 6'd49, 6'd57,
    6'd2, 6'd10, 6'd18, 6'd26, 6'd34, 6'd42, 6'd50, 6'd58,
    6'd3, 6'd11, 6'd19, 6'd27, 6'd35, 6'd43, 6'd51, 6'd59,
    6'd4, 6'd12, 6'd20, 6'd28, 6'd36, 6'd44, 6'd52, 6'd60,
    6'd5, 6'd13, 6'd21, 6'd29, 6'd37, 6'd45, 6'd53, 6'd61,
    6'd6, 6'd14, 6'd22, 6'd30, 6'd38, 6'd46, 6'd54, 6'd62,
    6'd7, 6'd15, 6'd23, 6'd31, 6'd39, 6'd47, 6'd55, 6'd63
  };

  uint8 a_bytes     [BLOCK_SIZE];
  uint8 result_bytes[BLOCK_SIZE];

  generate;
    genvar i;
    for (i = 0; i < BLOCK_SIZE; i++) begin
      assign a_bytes[i] = a_i[(i * 8) + 7:(i * 8)];
    end

    for (i = 0; i < BLOCK_SIZE; i++) begin
      assign result_o[(i * 8) + 7:(i * 8)] = result_bytes[i];
    end

    for (i = 0; i < BLOCK_SIZE; i++) begin
      assign result_bytes[i] = a_bytes[TAU[i]];
    end
  endgenerate

endmodule
