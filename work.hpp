/**
 * @file MIPS_Processor.hpp
 * @author Mallika Prabhakar and Sayam Sethi
 * 
 */

#ifndef __MIPS_PROCESSOR_HPP__
#define __MIPS_PROCESSOR_HPP__

#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <fstream>
#include <exception>
#include <iostream>
#include <boost/tokenizer.hpp>
using namespace std;

struct MIPS_Architecture
{
	int registers[32] = {0}, PCcurr = 0, PCnext,PCnew;
	std::unordered_map<std::string, std::function<int(MIPS_Architecture &, std::string, std::string, std::string)>> instructions;
	std::unordered_map<std::string, int> registerMap, address;
	static const int MAX = (1 << 20);
	int data[MAX >> 2] = {0};
	std::vector<std::vector<std::string>> commands;
	std::vector<int> commandCount;
	enum exit_code
	{
		SUCCESS = 0,
		INVALID_REGISTER,
		INVALID_LABEL,
		INVALID_ADDRESS,
		SYNTAX_ERROR,
		MEMORY_ERROR
	};

	// constructor to initialise the instruction set
	MIPS_Architecture(std::ifstream &file)
	{
		instructions = {{"add", &MIPS_Architecture::add}, {"sub", &MIPS_Architecture::sub}, {"mul", &MIPS_Architecture::mul}, {"beq", &MIPS_Architecture::beq}, {"bne", &MIPS_Architecture::bne}, {"slt", &MIPS_Architecture::slt}, {"j", &MIPS_Architecture::j}, {"lw", &MIPS_Architecture::lw}, {"sw", &MIPS_Architecture::sw}, {"addi", &MIPS_Architecture::addi}};

		for (int i = 0; i < 32; ++i)
			registerMap["$" + std::to_string(i)] = i;
		registerMap["$zero"] = 0;
		registerMap["$at"] = 1;
		registerMap["$v0"] = 2;
		registerMap["$v1"] = 3;
		for (int i = 0; i < 4; ++i)
			registerMap["$a" + std::to_string(i)] = i + 4;
		for (int i = 0; i < 8; ++i)
			registerMap["$t" + std::to_string(i)] = i + 8, registerMap["$s" + std::to_string(i)] = i + 16;
		registerMap["$t8"] = 24;
		registerMap["$t9"] = 25;
		registerMap["$k0"] = 26;
		registerMap["$k1"] = 27;
		registerMap["$gp"] = 28;
		registerMap["$sp"] = 29;
		registerMap["$s8"] = 30;
		registerMap["$ra"] = 31;

		constructCommands(file);
		commandCount.assign(commands.size(), 0);
	}

	// perform add operation
	int add(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a + b; });
	}

	// perform subtraction operation
	int sub(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a - b; });
	}

	// perform multiplication operation
	int mul(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a * b; });
	}

	// perform the binary operation
	int op(std::string r1, std::string r2, std::string r3, std::function<int(int, int)> operation)
	{
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		registers[registerMap[r1]] = operation(registers[registerMap[r2]], registers[registerMap[r3]]);
		PCnext = PCcurr + 1;
		return 0;
	}

	// perform the beq operation
	int beq(std::string r1, std::string r2, std::string label)
	{
		return bOP(r1, r2, label, [](int a, int b)
				   { return a == b; });
	}

	// perform the bne operation
	int bne(std::string r1, std::string r2, std::string label)
	{
		return bOP(r1, r2, label, [](int a, int b)
				   { return a != b; });
	}

	// implements beq and bne by taking the comparator
	int bOP(std::string r1, std::string r2, std::string label, std::function<bool(int, int)> comp)
	{
		if (!checkLabel(label))
			return 4;
		if (address.find(label) == address.end() || address[label] == -1)
			return 2;
		if (!checkRegisters({r1, r2}))
			return 1;
		PCnext = comp(registers[registerMap[r1]], registers[registerMap[r2]]) ? address[label] : PCcurr + 1;
		return 0;
	}

	// implements slt operation
	int slt(std::string r1, std::string r2, std::string r3)
	{
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		registers[registerMap[r1]] = registers[registerMap[r2]] < registers[registerMap[r3]];
		PCnext = PCcurr + 1;
		return 0;
	}

	// perform the jump operation
	int j(std::string label, std::string unused1 = "", std::string unused2 = "")
	{
		if (!checkLabel(label))
			return 4;
		if (address.find(label) == address.end() || address[label] == -1)
			return 2;
		PCnext = address[label];
		return 0;
	}

	// perform load word operation
	int lw(std::string r, std::string location, std::string unused1 = "")
	{
		if (!checkRegister(r) || registerMap[r] == 0)
			return 1;
		int address = locateAddress(location);
		if (address < 0)
			return abs(address);
		registers[registerMap[r]] = data[address];
		PCnext = PCcurr + 1;
		return 0;
	}

	// perform store word operation
	int sw(std::string r, std::string location, std::string unused1 = "")
	{
		if (!checkRegister(r))
			return 1;
		int address = locateAddress(location);
		if (address < 0)
			return abs(address);
		data[address] = registers[registerMap[r]];
		PCnext = PCcurr + 1;
		return 0;
	}

	int locateAddress(std::string location)
	{
		if (location.back() == ')')
		{
			try
			{
				int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
				std::string reg = location.substr(lparen + 1);
				reg.pop_back();
				if (!checkRegister(reg))
					return -3;
				int address = registers[registerMap[reg]] + offset;
				if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
					return -3;
				return address / 4;
			}
			catch (std::exception &e)
			{
				return -4;
			}
		}
		try
		{
			int address = stoi(location);
			if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
				return -3;
			return address / 4;
		}
		catch (std::exception &e)
		{
			return -4;
		}
	}

	// perform add immediate operation
	int addi(std::string r1, std::string r2, std::string num)
	{
		if (!checkRegisters({r1, r2}) || registerMap[r1] == 0)
			return 1;
		try
		{
			registers[registerMap[r1]] = registers[registerMap[r2]] + stoi(num);
			PCnext = PCcurr + 1;
			return 0;
		}
		catch (std::exception &e)
		{
			return 4;
		}
	}

	// checks if label is valid
	inline bool checkLabel(std::string str)
	{
		return str.size() > 0 && isalpha(str[0]) && all_of(++str.begin(), str.end(), [](char c)
														   { return (bool)isalnum(c); }) &&
			   instructions.find(str) == instructions.end();
	}

	// checks if the register is a valid one
	inline bool checkRegister(std::string r)
	{
		return registerMap.find(r) != registerMap.end();
	}

	// checks if all of the registers are valid or not
	bool checkRegisters(std::vector<std::string> regs)
	{
		return std::all_of(regs.begin(), regs.end(), [&](std::string r)
						   { return checkRegister(r); });
	}

	/*
		handle all exit codes:
		0: correct execution
		1: register provided is incorrect
		2: invalid label
		3: unaligned or invalid address
		4: syntax error
		5: commands exceed memory limit
	*/
	void handleExit(exit_code code, int cycleCount)
	{
		std::cout << '\n';
		switch (code)
		{
		case 1:
			std::cerr << "Invalid register provided or syntax error in providing register\n";
			break;
		case 2:
			std::cerr << "Label used not defined or defined too many times\n";
			break;
		case 3:
			std::cerr << "Unaligned or invalid memory address specified\n";
			break;
		case 4:
			std::cerr << "Syntax error encountered\n";
			break;
		case 5:
			std::cerr << "Memory limit exceeded\n";
			break;
		default:
			break;
		}
		if (code != 0)
		{
			std::cerr << "Error encountered at:\n";
			for (auto &s : commands[PCcurr])
				std::cerr << s << ' ';
			std::cerr << '\n';
		}
		std::cout << "\nFollowing are the non-zero data values:\n";
		for (int i = 0; i < MAX / 4; ++i)
			if (data[i] != 0)
				std::cout << 4 * i << '-' << 4 * i + 3 << std::hex << ": " << data[i] << '\n'
						  << std::dec;
		std::cout << "\nTotal number of cycles: " << cycleCount << '\n';
		std::cout << "Count of instructions executed:\n";
		for (int i = 0; i < (int)commands.size(); ++i)
		{
			std::cout << commandCount[i] << " times:\t";
			for (auto &s : commands[i])
				std::cout << s << ' ';
			std::cout << '\n';
		}
	}

	// parse the command assuming correctly formatted MIPS instruction (or label)
	void parseCommand(std::string line)
	{
		// strip until before the comment begins
		line = line.substr(0, line.find('#'));
		std::vector<std::string> command;
		boost::tokenizer<boost::char_separator<char>> tokens(line, boost::char_separator<char>(", \t"));
		for (auto &s : tokens)
			command.push_back(s);
		// empty line or a comment only line
		if (command.empty())
			return;
		else if (command.size() == 1)
		{
			std::string label = command[0].back() == ':' ? command[0].substr(0, command[0].size() - 1) : "?";
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command.clear();
		}
		else if (command[0].back() == ':')
		{
			std::string label = command[0].substr(0, command[0].size() - 1);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command = std::vector<std::string>(command.begin() + 1, command.end());
		}
		else if (command[0].find(':') != std::string::npos)
		{
			int idx = command[0].find(':');
			std::string label = command[0].substr(0, idx);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command[0] = command[0].substr(idx + 1);
		}
		else if (command[1][0] == ':')
		{
			if (address.find(command[0]) == address.end())
				address[command[0]] = commands.size();
			else
				address[command[0]] = -1;
			command[1] = command[1].substr(1);
			if (command[1] == "")
				command.erase(command.begin(), command.begin() + 2);
			else
				command.erase(command.begin(), command.begin() + 1);
		}
		if (command.empty())
			return;
		if (command.size() > 4)
			for (int i = 4; i < (int)command.size(); ++i)
				command[3] += " " + command[i];
		command.resize(4);
		commands.push_back(command);
	}

	// construct the commands vector from the input file
	void constructCommands(std::ifstream &file)
	{
		std::string line;
		while (getline(file, line))
			parseCommand(line);
		file.close();
	}

	//function written by us to find the offset and source register separately for load and store instructions.
	pair<string,int> LoadAndStore(string location)
	{
		int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
		std::string reg = location.substr(lparen + 1);
		reg.pop_back();
		return {reg,offset};
	}

	// execute the commands sequentially (no pipelining)
	// void executeCommandsUnpipelined()
	// {
	// 	if (commands.size() >= MAX / 4)
	// 	{
	// 		handleExit(MEMORY_ERROR, 0);
	// 		return;
	// 	}

	// 	int clockCycles = 0;
	// 	while (PCcurr < commands.size())
	// 	{
	// 		++clockCycles;
	// 		std::vector<std::string> &command = commands[PCcurr];
	// 		if (instructions.find(command[0]) == instructions.end())
	// 		{
	// 			handleExit(SYNTAX_ERROR, clockCycles);
	// 			return;
	// 		}
	// 		exit_code ret = (exit_code) instructions[command[0]](*this, command[1], command[2], command[3]);
	// 		if (ret != SUCCESS)
	// 		{
	// 			handleExit(ret, clockCycles);
	// 			return;
	// 		}
	// 		++commandCount[PCcurr];
	// 		PCcurr = PCnext;
	// 		printRegisters(clockCycles);
	// 	}
	// 	handleExit(SUCCESS, clockCycles);
	// }


	void executeCommandPipelined()
	{
		//CONTROL SIGNALS
		bool PCSrc=false;
		bool RegWrite=false; //this control signal denotes 
		string regwrite; //this will store the register in which 
		//value needs to be written during WB stage
		bool ALUSrc=false;
		bool ALUOp0=false;
		bool ALUOp1=false;
		bool RegDst=false;
		bool MemWrite=false;
		bool MemRead=false;
		bool MemtoReg=false;
		bool Branch=false;

		queue<int> wb_stage,mem_stage,alu_stage,id_stage,if_stage;

		//ports required in INSTRUCTION DECODE stage
		int data1,data2;
		int offset;
		int destregister0,destregister1;

		while(true)
		{

			/************************************************************************************************************************/

			//THIS IS THE ALU STAGE

			if(!alu_stage.empty())
			{
				int counter_alu_stage=alu_stage.front();

			}

			/*********************************************************************************************************************/

			//THIS IS THE INSTRUCTION DECODE STAGE.

			if(!id_stage.empty()) 
			{
				int counter_id_stage=id_stage.front();
				vector<string> ins=commands[counter_id_stage];
				if((ins[0]=="add") || (ins[0]=="sub") || (ins[0]=="mul") || (ins[0]=="slt"))
				{
					//R type instructions : add,sub,mul
					if(!RegWrite || ((RegWrite) && (ins[2]!=regwrite) && (ins[3]!=regwrite)))
					{
						data1=registers[registerMap[ins[2]]];
						data2=registers[registerMap[ins[3]]];
						destregister1=registerMap[ins[1]];
						RegDst=true;
						ALUOp0=false; ALUOp1=true;
						ALUSrc=true;
						id_stage.pop(); //ID stage of this instruction is done
						alu_stage.push(counter_id_stage); //Prepare it for ALU stage
					}
					//else the ID stage is stuck at the instruction commands[counter_id_stage]
				}
				else if((ins[0]=="addi"))
				{
					if(!RegWrite || ((RegWrite) && (ins[2]!=regWrite)))
					{
						offset=stoi(ins[3]);
						data1=registers[registerMap[ins[2]]];
						destregister0=registerMap[ins[1]];
						RegDst=false;
						ALUSrc=false;
						id_stage.pop();
						alu_stage.push(counter_id_stage);
					}
					//else do nothing, this instruction would remain at addi only
				}

				else if((ins[0]=="lw") || (ins[0]=="sw"))
				{
					//need to load in register from memory and load in memory from registers
					pair<string,int> temp=LoadAndStore(ins[2]);
					if(!RegWrite || ((RegWrite) && (temp.first!=regWrite)))
					{
						offset=temp.second;
						data1=registers[registerMap[temp.first]];
						destregister0=registerMap[ins[1]];
						RegDst=false;
						ALUOp0=false; ALUOp1=false;
						ALUSrc=false;
						id_stage.pop();
						alu_stage.push(counter_id_stage);
					}
				}

				else if((ins[0]=="beq") || (ins[0]=="bne"))
				{
					if(!RegWrite || ((RegWrite) && (ins[1]!=regWrite) && (ins[2]!=regWrite)))
					{
						//here ins[3] is a label
						//I have been given the memory address to which the label points to
						//in the field address[ins[3]], I require the offset though
						offset=address[ins[3]]-PCnext;
						data1=registers[registerMap[ins[1]]];
						data2=registers[registerMap[ins[2]]];
						ALUOp0=true; ALUOp1=false;
						id_stage.pop();
						alu_stage.push(counter_id_stage);
					}
				}
				//ID code for j instruction is still left
			}
			/**************************************************************************************************************************/

			//THIS IS THE INSTRUCTION FETCH STAGE.

			//the MUX in the IF stage is implemented as an if else statement
			if(! PCSrc) PCcurr=PCnext;
			else 
			{
				PCcurr=PCnew;
				PCSrc=false;
			}		
			if_stage.push(PCcurr);
			PCnext=PCcurr+1;

		}
	}

	// print the register data in hexadecimal
	void printRegisters(int clockCycle)
	{
		std::cout << "Cycle number: " << clockCycle << '\n'
				  << std::hex;
		for (int i = 0; i < 32; ++i)
			std::cout << registers[i] << ' ';
		std::cout << std::dec << '\n';
	}
};

#endif