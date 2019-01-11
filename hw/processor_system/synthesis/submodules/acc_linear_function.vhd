-- acc_linear_function.vhd

-- This file was auto-generated as a prototype implementation of a module
-- created in component editor.  It ties off all outputs to ground and
-- ignores all inputs.  It needs to be edited to make it do something
-- useful.
-- 
-- This file will not be automatically regenerated.  You should check it in
-- to your version control system if you want to keep it.

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity acc_linear_function is
	port (
		reset                  : in  std_logic                     := '0';             --  reset.reset
		avs_params_address     : in  std_logic                     := '0';             -- params.address
		avs_params_read        : in  std_logic                     := '0';             --       .read
		avs_params_readdata    : out std_logic_vector(7 downto 0);                     --       .readdata
		avs_params_write       : in  std_logic                     := '0';             --       .write
		avs_params_writedata   : in  std_logic_vector(7 downto 0)  := (others => '0'); --       .writedata
		avs_params_waitrequest : out std_logic;                                        --       .waitrequest
		clk                    : in  std_logic                     := '0';             --  clock.clk
		asi_in_data            : in  std_logic_vector(7 downto 0)  := (others => '0'); --     in.data
		asi_in_ready           : out std_logic;                                        --       .ready
		asi_in_valid           : in  std_logic                     := '0';             --       .valid
		asi_in_sop             : in  std_logic                     := '0';             --       .startofpacket
		asi_in_eop             : in  std_logic                     := '0';             --       .endofpacket
		aso_out_data           : out std_logic_vector(15 downto 0);                    --    out.data
		aso_out_ready          : in  std_logic                     := '0';             --       .ready
		aso_out_valid          : out std_logic;                                        --       .valid
		aso_out_sop            : out std_logic;                                        --       .startofpacket
		aso_out_eop            : out std_logic;                                        --       .endofpacket
		aso_out_empty          : out std_logic                                         --       .empty
	);
end entity acc_linear_function;

architecture rtl of acc_linear_function is
	signal a_reg : std_logic_vector(7 downto 0); --pixel counter
	signal b_reg : std_logic_vector(7 downto 0);
	
	constant A_ADDR : std_logic := '0';
	constant B_ADDR : std_logic := '1';
	
	signal a_strobe : std_logic;
	signal b_strobe : std_logic;
	
	signal read_out_mux : std_logic_vector(7 downto 0);
	
	type state is (config, hist_calc, finished);
	
	signal current_state, next_state : state;

	signal input_sample : integer;
	signal output_sample : signed(15 downto 0);
	
	signal int_asi_in_ready : std_logic;

	--type mem_t is array (0 to 255) of std_logic_vector(31 downto 0);
	type mem_t is array (0 to 255) of integer range 0 to 2**32 - 1;
	signal ram_memory : mem_t;
	signal memory_read : integer range 0 to 2**32 - 1;
--	signal memory_read : std_logic_vector(31 downto 0);
	
	signal pixel_cnt : integer;
	
begin

	a_strobe <= '1' when (avs_params_write = '1') and (avs_params_address = A_ADDR) else '0';
	b_strobe <= '1' when (avs_params_write = '1') and (avs_params_address = B_ADDR) else '0';
	
	read_out_mux <= a_reg when (avs_params_address = A_ADDR) else
						 b_reg when (avs_params_address = B_ADDR) else
						 "00000000";
	
	write_reg_a: process(clk, reset)
	begin
		if (reset = '1') then
			a_reg <= "00000010";
		elsif (rising_edge(clk)) then
			if (a_strobe = '1') then
				a_reg <= avs_params_writedata;
			end if;
		end if;
	end process;

	write_reg_b: process(clk, reset)
	begin
		if (reset = '1') then
			b_reg <= "00000011";
		elsif (rising_edge(clk)) then
			if (b_strobe = '1') then
				b_reg <= avs_params_writedata;
			end if;
		end if;
	end process;

	read_regs: process(clk, reset)
	begin
		if (reset = '1') then
			avs_params_readdata <= "00000000";
		elsif (rising_edge(clk)) then
			avs_params_readdata <= read_out_mux;
		end if;
	end process;
	
--	process_sample : process(clk, reset)
--	begin
--		if (reset = '1') then
--			output_sample <= x"BEEF";
--		elsif (rising_edge(clk)) then
--			if ((current_state = process_input) or ((current_state = full_process or current_state = wait_output_and_process) and aso_out_ready = '1')) then
--				output_sample <= signed(a_reg)*input_sample + signed(b_reg);
--			end if;
--		end if;
--	end process;
	
	aso_out_data <= std_logic_vector(to_unsigned(ram_memory(pixel_cnt),16));
	
--	memory_read <= ram_memory(input_sample);
	
	process_sample : process(clk, reset)
	begin
		if (reset = '1') then
			output_sample <= x"BEEF";
		elsif (rising_edge(clk)) then
			if (current_state = hist_calc) then
				ram_memory(input_sample) <= ram_memory(input_sample) + 1;
			end if;
		end if;
	end process;
	
	pixel_cnt_update : process(clk, reset)
	begin
		if (reset = '1') then
			pixel_cnt <= 0;
		elsif (rising_edge(clk)) then
			case current_state is
				when config =>
					pixel_cnt <= to_integer(unsigned(a_reg));
				when hist_calc =>
					if (pixel_cnt /= 0) then
						pixel_cnt <= pixel_cnt - 1;
					else
						pixel_cnt <= to_integer(unsigned(a_reg));
					end if;
				when finished =>
					pixel_cnt <= pixel_cnt - 1;
					--pixel_cnt <= pixel_cnt + 1;
			end case;
		end if;
	end process;
	
	read_sample : process(clk, reset)
	begin
		if (reset = '1') then
			input_sample <= 0;
		elsif (rising_edge(clk)) then
			if (int_asi_in_ready = '1' and asi_in_valid = '1') then
				input_sample <= to_integer(unsigned(asi_in_data));
			end if;
		end if;
	end process;
	
	control_fsm: process(clk, reset)
	begin
		if (reset = '1') then
			current_state <= config;
		elsif (rising_edge(clk)) then
			current_state <= next_state;
		end if;
	end process;
	
	
	streaming_protocol: process(current_state, asi_in_valid, aso_out_ready)
	begin
		case current_state is
			when config =>
				int_asi_in_ready <= '0';
				aso_out_valid <= '0';
				if (b_reg = x"01") then
				--if (asi_in_valid = '1') then --neki config registar
					next_state <= hist_calc;
				else
					next_state <= config;
				end if;				
		
			when hist_calc =>
				int_asi_in_ready <= '1';
				aso_out_valid <= '0';
				if (pixel_cnt = 0) then
					next_state <= finished;
				else
					next_state <= hist_calc;
				end if;				
			
			when finished =>
				int_asi_in_ready <= '0';
				aso_out_valid <= '1';
				if (pixel_cnt = 0) then
					next_state <= config;
				else
					next_state <= finished;
				end if;
				
		end case;
				
	
	end process;
	
	asi_in_ready <= int_asi_in_ready;
	
	aso_out_eop <= '0';
	aso_out_sop <= '0';
	aso_out_empty <= '0';

	avs_params_waitrequest <= '0';
end architecture rtl; -- of acc_linear_function
