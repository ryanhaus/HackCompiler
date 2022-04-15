#pragma once

#include <cstdio>
#include <cassert>

#include <vector>
#include <map>
#include <bitset>

// all information taken from https://www.nand2tetris.org/_files/ugd/44046b_7ef1c00a714c46768f08c459a6cab45a.pdf

#pragma pack(push, 1) // pack tightly
struct general_instruction
{
	char bit[16]						/* 16-bit instruction */ = { '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0' };
};

struct a_type_instruction
{
	const char unused = '0';			// unused bit
	char value[15]						/* value being put into the A register */ = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
};

struct c_type_instruction
{
	const char unused[3]				/* unused bits */ = { '1', '1', '1' };
	char
		am = '0',						// A/M decider (0=A, 1=M)
		comp[6]							/* Computation bits(ALU inputs) */ = { '0', '0', '0', '0', '0', '0' },
		dest[3]							/* Destination bits */ = { '0', '0', '0' },
		jump[3]							/* Jump condition bits */ = { '0', '0', '0' };
};

struct c_instruction_components
{
	char
		am_decision = '0',				// A/M decision (0 = A, 1 = M)
		destination[3]					/* Destination string */ = { ' ', ' ', ' ' },
		value[3]						/* Value string */ = { ' ', ' ', ' ' },
		jump[3]							/* Jump condition string */ = { ' ', ' ', ' ' };
};

// for string comparison in accessing elements of the maps
template <int NCharacters>
struct string_comparison
{
	bool operator()(const char* str1, const char* str2) const
	{
		return std::strncmp(str1, str2, NCharacters) < 0; // compare the two strings up to their length
	}
};
#pragma pack(pop) // end pragma pack

std::map<std::string, uint16_t> variable_map = // variable map for the assembler, including the pre-defined variables
{
	{ "SP", 0 },
	{ "LCL", 1 },
	{ "ARG", 2 },
	{ "THIS", 3 },
	{ "THAT", 4 },
	{ "R0", 0 },
	{ "R1", 1 },
	{ "R2", 2 },
	{ "R3", 3 },
	{ "R4", 4 },
	{ "R5", 5 },
	{ "R6", 6 },
	{ "R7", 7 },
	{ "R8", 8 },
	{ "R9", 9 },
	{ "R10", 10 },
	{ "R11", 11 },
	{ "R12", 12 },
	{ "R13", 13 },
	{ "R14", 14 },
	{ "R15", 15 },
	{ "SCREEN", 16384 },
	{ "KBD", 24576 }
};

uint16_t variable_count = 0; // number of variables in the map (not labels), except for the default ones

// splits a c-type instruction into key components (destinations, value, and jump conditions)
inline c_instruction_components split_c_instruction(const char* instruction)
{
	c_instruction_components components;

	const auto semicolon_ptr = strchr(instruction, ';'); // search for semicolon in instruction
	if (semicolon_ptr != nullptr) // if there is a semicolon (indicating the presence of a jump condition)
		for (uint8_t i = 0; i < 3; i++)
		{
			const char c = *(semicolon_ptr + 1 + i); // get current character
			if (c == ' ') break; // if its a space we're done

			// if not:
			components.jump[i] = c; // set jump character
		}

	const auto equal_ptr = strchr(instruction, '='); // search for equal sign in instruction
	if (equal_ptr != nullptr) // if there is an equal sign in the instruction (indicating a destination)
	{
		// set value string
		for (uint8_t i = 0; i < 3; i++) // go through each character
		{
			char c = *(equal_ptr + 1 + i); // get current character
			if (c == ' ' || c == 0 || c == '\r' || c == '\n') break; // if its a space we're done

			if (c == 'M') // if M instead of A
			{
				components.am_decision = '1'; // set A/M decision flag
				c = 'A'; // replace character
			}

			// if not:
			components.value[i] = c; // set value character
		}

		// set destination string
		for (uint8_t i = 0; i < (equal_ptr - instruction); i++) // go through each character
			components.destination[i] = instruction[i]; // set destination character
	}
	else // no equal sign
		for (uint8_t i = 0; i < 3; i++) // go through each character
		{
			if (instruction[i] == ';') break; // if semicolon we're done

			// if not:
			char c = instruction[i]; // current character

			if (c == 'M') // if M instead of A
			{
				components.am_decision = '1'; // set A/M decision flag
				c = 'A'; // replace character
			}

			components.value[i] = c; // set value character
		}

	return components;
}

// generate a-type instruction from an integer
inline a_type_instruction generate_a_type(const uint16_t n)
{
	const auto str = std::bitset<15>(n).to_string();

	a_type_instruction instruction;
	memcpy(instruction.value, str.c_str(), 15);

	return instruction;
}

// generate c-type instruction
inline c_type_instruction generate_c_type(const char am_decision, const char* alu_input, const char* dest_input, const char* jump_input)
{
	// ALU input values for all possible expressions
	const std::map<const char*, const char*, string_comparison<3>> alu_inputs =
	{
		{ "0  ", "101010" },		// ALU inputs for 0
		{ "1  ", "111111" },		// ALU inputs for 1
		{ "-1 ", "111010" },		// ALU inputs for -1
		{ "D  ", "001100" },		// ALU inputs for D value
		{ "A  ", "110000" },		// ALU inputs for A/M value
		{ "!D ", "001101" },		// ALU inputs for ~D
		{ "!A ", "110001" },		// ALU inputs for ~A/M
		{ "-D ", "001111" },		// ALU inputs for -D
		{ "-A ", "110011" },		// ALU inputs for -A/M
		{ "D+1", "011111" },		// ALU inputs for D+1
		{ "A+1", "110111" },		// ALU inputs for A/M+1
		{ "D-1", "001110" },		// ALU inputs for D-1
		{ "A-1", "110010" },		// ALU inputs for A/M-1
		{ "D+A", "000010" },		// ALU inputs for D+A/M
		{ "D-A", "010011" },		// ALU inputs for D-A/M
		{ "A-D", "000111" },		// ALU inputs for A/M-D
		{ "D&A", "000000" },		// ALU inputs for D&A/M
		{ "D|A", "010101" }			// ALU inputs for D|A/M
	};

	// destination values for all possible expressions
	const std::map<const char*, const char*, string_comparison<3>> jump_inputs =
	{
		{ "   ", "000" },			// bits for never jump
		{ "JGT", "001" },			// bits for jump if greater than zero
		{ "JEQ", "010" },			// bits for jump if equal to zero
		{ "JGE", "011" },			// bits for jump if greater than or equal to zero
		{ "JLT", "100" },			// bits for jump if less than zero
		{ "JNE", "101" },			// bits for jump if not equal to zero
		{ "JLE", "110" },			// bits for jump if less than or equal to zero
		{ "JMP", "111" }			// bits for jump unconditionally
	};

	c_type_instruction instr; // blank instruction
	instr.am = am_decision; // set A/M decision bit
	memcpy(instr.comp, alu_inputs.at(alu_input), 6); // copy computation bits

	// copy destination bits
	for (uint8_t i = 0; i < 3; i++) // go through each character
	{
		if (dest_input[i] == 'A')
			instr.dest[0] = '1'; // set A destination flag
		else if (dest_input[i] == 'D')
			instr.dest[1] = '1'; // set M destination flag
		else if (dest_input[i] == 'M')
			instr.dest[2] = '1'; // set D destination flag
		else break; // if not stop looking
	}

	memcpy(instr.jump, jump_inputs.at(jump_input), 3); // copy jump condition bits

	return instr; // instruction is ready
}

inline void parse_file(const char* file_name)
{
	FILE* f;
	fopen_s(&f, file_name, "rb"); // open file

	assert(f != nullptr); // make sure that file is opened correctly

	std::vector<general_instruction> instructions; // vector of instructions generated
	char line_buffer[0x80]; // buffer of line being read from file

	uint16_t command_count = 0; // keeping track of the number of commands parsed
	while (fgets(line_buffer, sizeof(line_buffer), f) != nullptr) // initial scan of the file for labels
	{
		char* line_buffer_ptr = line_buffer; // pointer to new start of line (for ignoring whitespace)
		while (line_buffer_ptr[0] == ' ') // while the starting character is whitespace
			line_buffer_ptr++; // increase the pointer

		if (line_buffer_ptr[0] < ' ' || line_buffer_ptr[0] > 'z' || line_buffer_ptr[0] == '/') continue; // filter out lines that aren't commands

		if (line_buffer_ptr[0] == '(') // if the line is a label
		{
			char label_name[0x40] = { 0 };
			uint8_t current_label_position = 0;

			for (uint8_t i = 0; line_buffer_ptr[i] != ')' && i < 0x40; i++) // go through each character until buffer is full or were at the end of the label name
				if (line_buffer_ptr[i] != '(')
					label_name[current_label_position++] = line_buffer_ptr[i]; // copy character
			label_name[current_label_position] = 0; // terminating character

			variable_map[label_name] = command_count;
		}
		else
			command_count++; // increase the command counter
	}

	fseek(f, 0, SEEK_SET); // seek back to beginning of the file
	while (fgets(line_buffer, sizeof(line_buffer), f) != nullptr) // read each line of file for parsing of commands
	{
		char* line_buffer_ptr = line_buffer; // pointer to new start of line (for ignoring whitespace)
		while (line_buffer_ptr[0] == ' ') // while the starting character is whitespace
			line_buffer_ptr++; // increase the pointer

		if (line_buffer_ptr[0] < ' ' || line_buffer_ptr[0] > 'z' || line_buffer_ptr[0] == '/' || line_buffer_ptr[0] == '(') continue; // filter out lines that aren't commands

		general_instruction instruction; // the instruction being generated
		if (line_buffer_ptr[0] == '@') // a-type instruction
		{
			if (line_buffer_ptr[1] >= '0' && line_buffer_ptr[1] <= '9') // if proceeding statement is a number (not a variable)
			{
				int num;
				sscanf_s(line_buffer_ptr, "@%i", &num); // extract number from expression

				a_type_instruction instr = generate_a_type(static_cast<uint16_t>(num)); // generate a-type instruction
				memcpy(&instruction, &instr, sizeof(a_type_instruction)); // copy memory
			}
			else // is a variable
			{
				char var_name[65]; // variable name
				sscanf_s(line_buffer_ptr, "@%s", var_name, 64); // extract variable name from expression
				var_name[64] = 0; // terminating character

				uint16_t variable_ptr; // pointer for variable in memory

				if (variable_map.find(var_name) == variable_map.end()) // if variable doesn't already exist in map
				{
					variable_ptr = 16 + variable_count++; // get new pointer for variable
					variable_map.insert_or_assign(var_name, variable_ptr); // set in map
				}
				else
					variable_ptr = variable_map.at(var_name); // get variable pointer

				a_type_instruction instr = generate_a_type(variable_ptr); // generate a-type instruction
				memcpy(&instruction, &instr, sizeof(a_type_instruction)); // copy memory
			}
		}
		else // c-instruction
		{
			const auto components = split_c_instruction(line_buffer_ptr); // get instruction components
			auto instr = generate_c_type(components.am_decision, components.value, components.destination, components.jump); // generate c-instruction
			
			memcpy(&instruction, &instr, sizeof(general_instruction)); // copy memory
		}

		instructions.push_back(instruction); // log instruction
	}

	fclose(f);

	FILE* output_f; // compiler output file
	fopen_s(&output_f, "out.hack", "wb"); // open stream

	assert(output_f != nullptr); // make sure it is opened correctly
	
	for (general_instruction instruction : instructions) // go through each compiled instruction
		fprintf(output_f, "%.16s\n", instruction.bit); // write to file

	fclose(output_f); // close stream
}