`timescale 1 ns / 1 ps

module system (
	input            clk,
	input            resetn,
	output           trap,
        output reg [7:0] out_byte,
	output reg       out_byte_en
);

	// 32768 32bit words = 128kB memory
	parameter MEM_SIZE = 32768;

        integer            tb_idx;

	wire mem_valid;
	wire mem_instr;
	reg mem_ready;
	wire [31:0] mem_addr;
	wire [31:0] mem_wdata;
	wire [3:0] mem_wstrb;
	reg [31:0] mem_rdata;


        wire [31:0] irqs;
        wire [31:0] eois;
        wire spi_cs;
       	wire [3:0] spi_wstrb;
        wire [31:0] spi_rdata;

	picorv32 picorv32_core (
		.clk         (clk         ),
		.resetn      (resetn      ),
		.trap        (trap        ),
		.mem_valid   (mem_valid   ),
		.mem_instr   (mem_instr   ),
		.mem_ready   (mem_ready   ),
		.mem_addr    (mem_addr    ),
		.mem_wdata   (mem_wdata   ),
		.mem_wstrb   (mem_wstrb   ),
		.mem_rdata   (mem_rdata   ),
                .irq(irqs),
                .eoi(eois)
	);


      
            simsd sd(
		.clk(clk),
		.cs(spi_cs),
		.bus_addr(mem_addr),
		.bus_wr_val(mem_wdata),
		.bus_bytesel(spi_wstrb),
		.bus_ack(),
		.bus_data(spi_rdata)
             );


       assign irqs[31:1] = 31'b0;
       assign spi_cs =  mem_addr[31:4] == 28'h4000000 && mem_valid;
       assign spi_wstrb = mem_wstrb & mem_ready;
   
	reg [31:0] memory [0:MEM_SIZE-1];
	initial begin
                for (tb_idx=0; tb_idx < MEM_SIZE; tb_idx=tb_idx+1)
                       memory[tb_idx] = 32'b0;
                       $readmemh("../firmware.hex", memory);
        end

	reg [31:0] m_read_data;
	reg m_read_en;

	always @(posedge clk) begin
			m_read_en <= 0;
			mem_ready <= mem_valid && !mem_ready && m_read_en;

                        out_byte_en <= 0;
                   			
			(* parallel_case *)
			case (1)
				mem_valid && !mem_ready && !mem_wstrb && (mem_addr >> 2) < MEM_SIZE: begin
					m_read_en <= 1;
				        m_read_data <= memory[mem_addr >> 2];
				        mem_rdata <= m_read_data;
				end
				mem_valid && !mem_ready && |mem_wstrb && (mem_addr >> 2) < MEM_SIZE: begin
					if (mem_wstrb[0]) memory[mem_addr >> 2][ 7: 0] <= mem_wdata[ 7: 0];
					if (mem_wstrb[1]) memory[mem_addr >> 2][15: 8] <= mem_wdata[15: 8];
					if (mem_wstrb[2]) memory[mem_addr >> 2][23:16] <= mem_wdata[23:16];
					if (mem_wstrb[3]) memory[mem_addr >> 2][31:24] <= mem_wdata[31:24];
					mem_ready <= 1;
				end
				

                                mem_valid && !mem_ready && !mem_wstrb && spi_cs: begin
					m_read_en <= 1;
                                        mem_rdata <= spi_rdata;
                                        //mem_rdata <= m_read_data;
				end
                                mem_valid && !mem_ready && |mem_wstrb && mem_addr == 32'h2000_0000: begin
					out_byte_en <= 1;
					out_byte <= mem_wdata;
					mem_ready <= 1;
				end
				mem_valid && !mem_ready && |mem_wstrb : begin
					mem_ready <= 1;
				end
			endcase
                        if (resetn && out_byte_en) begin
			   $write("%c", out_byte);
		        end
		end
endmodule
