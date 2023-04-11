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
#include <queue>
#include <boost/tokenizer.hpp>
using namespace std;


struct Latch
{
	int ALUSrc=2;
	int ALUOp=0;
	int RegDst=2;
	int MemWrite=2;
	int MemRead=2;
	int WriteBack=2;
	int MemtoReg=2;
	int ALUtoMem=2;
	int Branch=2;
	int TakeBranch=2;
	int destregister=-1;
};

void PassLatchValues(Latch* latchnext,Latch* latchprev)
{
	latchnext->ALUSrc=latchprev->ALUSrc;
	latchnext->ALUOp=latchprev->ALUOp;
	latchnext->RegDst=latchprev->RegDst;
	latchnext->MemWrite=latchprev->MemWrite;
	latchnext->MemRead=latchprev->MemRead;
	latchnext->WriteBack=latchprev->WriteBack;
	latchnext->MemtoReg=latchprev->MemtoReg;
	latchnext->ALUtoMem=latchprev->ALUtoMem;
	latchnext->Branch=latchprev->Branch;
	latchnext->TakeBranch=latchprev->TakeBranch;
	latchnext->destregister=latchprev->destregister;
}

void ClearLatchValues(Latch* L)
{
	L->ALUSrc=2;
	L->ALUOp=0;
	L->RegDst=2;
	L->MemWrite=2;
	L->MemRead=2;
	L->WriteBack=2;
	L->MemtoReg=2;
	L->ALUtoMem=2;
	L->Branch=2;
	L->TakeBranch=2;
	L->destregister=-1;
}

struct MIPS_Architecture
{
	int registers[32] = {0}, PCcurr = 0,PCnext=0;

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
		//The logic of the below code is based on the Figure 4.51 of the book Computer Organization and Design Edition 5

		bool RegWrite[32]={false};
		bool HaltPC=false;
		bool PCSrc=false;
		bool InstructionRemoved=false;

		Latch idwb,aluwb,memwb;
		Latch idmem,alumem;
		Latch idalu;

		int clockCycles=0;
		int FinalCount=0;

		int PCnew=0;

		queue<int> id_stage;

		//ports required in INSTRUCTION DECODE stage
		int data1=0,data2=0;
		int offset=0;
		int destregister0=-1,destregister1=-1;

		//ports required in ALU stage
		int aluinput1=0,aluinput2=0;
		int aluresult=0;
		int addresult=0;

		//ports required in MEM stage
		int memdata0=0,memdata1=0;

		while(true)
		{

			vector<pair<int,int>> modifiedMemory;
			//IF final count is = Extra Cycles then break the while loop

			//THIS IS THE WB STAGE

			if(memwb.WriteBack==1)
			{
				if(memwb.MemtoReg==1) registers[memwb.destregister]=memdata1;
				else registers[memwb.destregister]=memdata0;
				RegWrite[memwb.destregister]=false;
			}
			ClearLatchValues(&memwb);
			memdata0=0; memdata1=0;

			/*************************************************************************************************************************/

			//THIS IS THE MEM STAGE
			PassLatchValues(&memwb,&aluwb);

			//Implementing the branch control unit
			if(alumem.TakeBranch==1) 
			{
				PCnew=addresult;
				PCSrc=true;
				HaltPC=false;
				InstructionRemoved=false;
				addresult=-1;
			}
			alumem.TakeBranch=2; //reinitialising TakeBranch

			//passing the value of ALU/MEM latch to MEM/WB latch
			if(alumem.ALUtoMem==1)
			{
				memdata0=aluresult;
				memwb.WriteBack=1;
				aluresult=-1;
			}
			alumem.ALUtoMem=2;

			//if memory needs to be read
			if(alumem.MemRead==1)
			{
				//so the memory which needs to be read, its address is the result of ALU
				memdata1=data[aluresult];
				memwb.WriteBack=1;
				memwb.MemtoReg=1; // the data read from memory now needs 
				//to be written back to register
				aluresult=-1;
			}

			if(alumem.MemWrite==1)
			{
				//so what needs to be read in MemWrite is stored
				//in the register destregister 
				data[aluresult]=registers[aluwb.destregister];
				modifiedMemory.push_back({aluresult,data[aluresult]});
				aluresult=-1;
			}

			ClearLatchValues(&aluwb);
			ClearLatchValues(&alumem);

			/************************************************************************************************************************/

			//THIS IS THE ALU STAGE

			//transferring the contents of idmem to alumem
			PassLatchValues(&alumem,&idmem);
			PassLatchValues(&aluwb,&idwb);
			//Implementing the MUX controlled by RegDst
			if(idalu.RegDst==1) aluwb.destregister=destregister1;
			else if(idalu.RegDst==0) aluwb.destregister=destregister0;
			idalu.RegDst=2; //reinitialise value of RegDst
			//Implementing the MUX controlled by ALUSrc
			aluinput1=data1;
			if(idalu.ALUSrc==1) aluinput2=offset;
			else if(idalu.ALUSrc==0) aluinput2=data2;
			idalu.ALUSrc=2; //reinitialise value of ALUSrc
			//Implementing the ALU control unit
			//ALU control unit takes input as ALUOp control signal
			if(idalu.ALUOp<=4 && idalu.ALUOp>=1)
			{
				//so now the instruction is R type and we now check the value of rtype to get the actual instruction
				if(idalu.ALUOp==1) aluresult=aluinput1+aluinput2;
				else if(idalu.ALUOp==2) aluresult=aluinput1-aluinput2;
				else if(idalu.ALUOp==3) aluresult=aluinput1*aluinput2;
				else
				{
					if(aluinput1<aluinput2) aluresult=1;
					else aluresult=0;
				}
				alumem.ALUtoMem=1;
			}
			else if(idalu.ALUOp==5)
			{
				aluresult=aluinput1+aluinput2;
				alumem.ALUtoMem=1;
			}
			else if(idalu.ALUOp==6)
			{
				aluresult=(aluinput1+aluinput2)/4;
			}
			else if(idalu.ALUOp==7)
			{
				aluresult=(aluinput1+aluinput2)/4;
			}
			else if(idalu.ALUOp==8)
			{
				addresult=(PCnext+offset);
				if(aluinput1==aluinput2) alumem.TakeBranch=1;
			}
			else if(idalu.ALUOp==9)
			{
				addresult=(PCnext+offset);
				if(aluinput1!=aluinput2) alumem.TakeBranch=1;
			}

			ClearLatchValues(&idalu);
			ClearLatchValues(&idmem);
			ClearLatchValues(&idwb);
			aluinput1=0,aluinput2=0; //reinitialise aluinput1 and aluinput2
			destregister0=-1,destregister1=-1; //reinitialise destregister0 and destregister1

			/*********************************************************************************************************************/

			//THIS IS THE ID STAGE.

			if((!id_stage.empty()) && (!HaltPC)) 
			{
				int counter_id_stage=id_stage.front();
				vector<string> ins=commands[counter_id_stage];
				if((ins[0]=="add") || (ins[0]=="sub") || (ins[0]=="mul") || (ins[0]=="slt"))
				{
					//R type instructions : add,sub,mul,slt
					if((!RegWrite[registerMap[ins[2]]]) && (!RegWrite[registerMap[ins[3]]]))
					{
						data1=registers[registerMap[ins[2]]];
						data2=registers[registerMap[ins[3]]];
						destregister1=registerMap[ins[1]];
						RegWrite[destregister1]=true;
						idalu.RegDst=1;
						if(ins[0]=="add") idalu.ALUOp=1;
						else if(ins[0]=="sub") idalu.ALUOp=2;
						else if(ins[0]=="mul") idalu.ALUOp=3;
						else idalu.ALUOp=4;
						idalu.ALUSrc=0;
						id_stage.pop();
					}
					//else the ID stage is stuck at the instruction commands[counter_id_stage]
				}
				else if((ins[0]=="addi"))
				{
					if(!RegWrite[registerMap[ins[2]]])
					{
						offset=stoi(ins[3]);
						data1=registers[registerMap[ins[2]]];
						destregister0=registerMap[ins[1]];
						RegWrite[destregister0]=true;
						idalu.RegDst=0;
						idalu.ALUOp=5;
						idalu.ALUSrc=1;
						id_stage.pop();
					}
					//else do nothing, this instruction would remain at addi only
				}

				else if(ins[0]=="lw")
				{
					//need to load in register from memory and load in memory from registers
					pair<string,int> temp=LoadAndStore(ins[2]);
					if((!RegWrite[registerMap[temp.first]]))
					{
						offset=temp.second;
						data1=registers[registerMap[temp.first]];
						destregister0=registerMap[ins[1]];
						RegWrite[destregister0]=true;
						idalu.RegDst=0;
						idalu.ALUOp=6;
						idalu.ALUSrc=1;
						idmem.MemRead=1;
						id_stage.pop();
					}
				}

				else if(ins[0]=="sw")
				{
					//need to load in register from memory and load in memory from registers
					pair<string,int> temp=LoadAndStore(ins[2]);
					if((!RegWrite[registerMap[temp.first]]) && (!RegWrite[registerMap[ins[1]]]))
					{
						offset=temp.second;
						data1=registers[registerMap[temp.first]];
						destregister0=registerMap[ins[1]];
						idalu.RegDst=0;
						idalu.ALUOp=7;
						idalu.ALUSrc=1;
						idmem.MemWrite=1;
						id_stage.pop();
					}
				}

				else if((ins[0]=="beq") || (ins[0]=="bne"))
				{
					if(!(RegWrite[registerMap[ins[1]]]) && !(RegWrite[registerMap[ins[2]]]))
					{
						//here ins[3] is a label
						//I have been given the memory address to which the label points to
						//in the field address[ins[3]], I require the offset though
						offset=address[ins[3]]-PCnext; //so that PCnext+offset becomes equal to address[ins[3]]
						data1=registers[registerMap[ins[1]]];
						data2=registers[registerMap[ins[2]]];
						HaltPC=true;
						if(ins[0]=="beq") idalu.ALUOp=8;
						else idalu.ALUOp=9;
						id_stage.pop();
					}
				}
				//ID code for j instruction is still left
			}
			else if(!(id_stage.empty()) && (HaltPC))
			{
				if(!InstructionRemoved)
				{
					id_stage.pop();
					InstructionRemoved=true;
				}
			}
			/**************************************************************************************************************************/

			//THIS IS THE IF STAGE.

			//deciding the address of the next instruction to be executed
			if(PCSrc) 
			{
				PCcurr=PCnew;
				PCSrc=false;
			}
			else if(!HaltPC) PCcurr=PCnext;	
			if((!HaltPC) && (PCcurr<(int)commands.size())) 
			{
				id_stage.push(PCcurr);
				PCnext=PCcurr+1;
			}

			clockCycles++;

			//outputting values
			printRegisters(clockCycles);

			cout<<(int)modifiedMemory.size()<<" ";
			for(int i=0;i<(int)modifiedMemory.size();i++) cout<<modifiedMemory[i].first<<" "<<modifiedMemory[i].second<<" ";
			cout<<"\n";

			//Condition for exiting the while loop
			if(FinalCount==3) break;
			if(id_stage.empty()) FinalCount++;
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
