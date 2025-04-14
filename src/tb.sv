`timescale 10 ns

module tb 
    #(
        CLOCK_PERIOD = 10        
    )
    (        
    );

logic clk;

// Ticking the clock
always #(CLOCK_PERIOD/2) clk = ~clk;

// WAIT-ing for a certain amount of clock cycles
task WAIT(input logic[31:0] clock_cycles);
    begin
        for (integer i = 0; i < clock_cycles; i++)
        begin
            #(CLOCK_PERIOD);
        end
    end
endtask

// Test modules 


initial
begin
    WAIT(10);
    $finish;
end

endmodule
