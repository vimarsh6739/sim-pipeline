#include <bits/stdc++.h>
using namespace std;
#define NOOP 0x3f3f3f

struct PipelineRegister1
{
	int IR;
	PipelineRegister1()
	{
		IR = NOOP;
	}
};
struct PipelineRegister2
{
	int A;
	int B;
	int op;
	int destRgr;
	PipelineRegister2()
	{
		op = NOOP;
		A = B = destRgr = 0;
	}
};
struct PipelineRegister3
{
	int ALUOutput;
	int destRgr;
	int op;
	PipelineRegister3()
	{
		ALUOutput = destRgr = 0;
		op = NOOP;
	}
};
struct PipelineRegister4
{
	int LD;
	int destRgr;
	int op;
	int ALUOutput;
	PipelineRegister4()
	{
		destRgr = LD = ALUOutput = 0;
		op = NOOP ;
	}
};

class Processor
{
	private:

	//Defining the data and instruction cache
	vector<int> instCache;
	vector<int> dataCache;
	vector<int> R;
	vector<bool> flagR;
	vector<bool> flagWBR;
	
	//Defining the Pipeline Registers
	PipelineRegister1 IF_ID;
	PipelineRegister2 ID_EX;
	PipelineRegister3 EX_MEM;
	PipelineRegister4 MEM_WB;

	//Defining the parameters to be counted
	int cycleCount;
	int dataStall;
	int controlStall;
	int instructionCount;
	int arithmeticInstructionCount;
	int logicalInstructionCount;
	int dataTransferInstructionCount;
	int controlTransferInstructionCount;
	double cyclesPerInstruction;
	int totalStall;
	vector<string> stallType;

	//Defining parameters needed for running the processor
	int PC;
	int PCTemp;
	bool keepRunning;
	bool branchFlag;
	bool haltTriggerFlag;

	public:
	Processor()
	{
		instCache.resize(0);
		dataCache.resize(0);
		R.resize(0);
		flagR.resize(16, false);
		flagWBR.resize(16,false);//used for write back

		IF_ID = * new PipelineRegister1();
		ID_EX = * new PipelineRegister2();
		EX_MEM = * new PipelineRegister3();
		MEM_WB = * new PipelineRegister4();
		
		cycleCount = 0;
		dataStall = 0;
		controlStall = 0;
		instructionCount=0;
		arithmeticInstructionCount = 0;
		logicalInstructionCount = 0;
		dataTransferInstructionCount = 0;
		controlTransferInstructionCount = 0;
		cyclesPerInstruction = 0;
		totalStall = 0;
		stallType.resize(0);

		PC = 0;
		PCTemp = 0;
		keepRunning = true;
		branchFlag = false;
		haltTriggerFlag = false;
	}

	void InstructionFetch()
	{
		//check if the given instruction occurs after a branch instruction
		//if yes then dont fetch the instruction
		if(haltTriggerFlag)return;
		if(PC==NOOP)
		{
			//propogate the stall through the pipeline
			IF_ID.IR = NOOP;
			return;
		}
		if(branchFlag)
		{
			//reset the branch flag
			branchFlag = false;
			//propogate the stall through the pipeline
			IF_ID.IR = NOOP;
			return;
		} 

		int IR = instCache[PC];
		PC++;//instCache contains 128 entries of 2 bytes each
		IF_ID.IR = IR;
	}

	void InstructionDecode()
	{
		
		if(IF_ID.IR==NOOP)
		{
			//propogate the stall through the pipeline
			ID_EX.op = NOOP;
			return;
		}
		
		int IR = IF_ID.IR;
		int opcode = IR >>12;
		int op1 = (IR % (1<<12))>>8;
		int op2 = (IR % (1<<8))>>4;
		int op3 = IR % (1<<4);

		//cerr<<opcode<<" "<<op1<<" "<<op2<<" "<<op3<<"\n";
		//check for dependent instructions- if any
		switch(opcode)
		{
			//3 operand instructions
			case 0:
			case 1:
			case 2:
			case 4:
			case 5: 
			case 7:
				//if(PC==1)cerr<<(flagR[op2]||flagR[op3])<<"\n";
				if(flagR[op2] || flagR[op3])
				{
					dataStall++;
					PC--;
					IF_ID.IR = NOOP;
					stallType.push_back("RAW");
					return;
				}

				if(flagWBR[op2] || flagWBR[op3])
				{
					if(flagWBR[op2])flagWBR[op2] = false;
					if(flagWBR[op3])flagWBR[op3] = false;

					dataStall++;
					PC--;
					IF_ID.IR = NOOP;
					stallType.push_back("RAW");
					return;
				}
				break;
			
			//Load-Store Instructions
			case 8:
			case 9:
				if(flagR[op1] || flagR[op2])
				{
					dataStall++;
					PC--;
					IF_ID.IR = NOOP;
					stallType.push_back("RAW");
					return;
				}

				if(flagWBR[op1] || flagWBR[op2])
				{
					if(flagWBR[op1])flagWBR[op1] = false;
					if(flagWBR[op2])flagWBR[op2] = false;

					dataStall++;
					PC--;
					IF_ID.IR = NOOP;
					stallType.push_back("RAW");
					return;
				}
				break;

			//increment
			case 3:
				if(flagR[op1])
				{
					dataStall++;
					PC--;
					IF_ID.IR = NOOP;
					stallType.push_back("RAW");
					return;
				}

				if(flagWBR[op1])
				{
					flagWBR[op1] = false;

					dataStall++;
					PC--;
					IF_ID.IR = NOOP;
					stallType.push_back("RAW");
					return;
				}
				break;

			//logical NOT
			case 6:
				if (flagR[op2])
				{
					dataStall++;
					PC--;
					IF_ID.IR = NOOP;
					stallType.push_back("RAW");
					return;
				}

				if(flagWBR[op2])
				{
					flagWBR[op2] = false;

					dataStall++;
					PC--;
					IF_ID.IR = NOOP;
					stallType.push_back("RAW");
					return;
				}

				break;

			//conditional branching
			case 11:
				if (flagR[op1])
				{
					dataStall++;
					PC--;
					IF_ID.IR = NOOP;
					stallType.push_back("RAW");
					return;
				}

				if(flagWBR[op1])
				{
					flagWBR[op1] = false;

					dataStall++;
					PC--;
					IF_ID.IR = NOOP;
					stallType.push_back("RAW");
					return;
				}

				break;
		}
		//if(PC==1)cerr<<opcode<<"\n";
		//set appropriate flags and write contents to ID_EX
		switch(opcode)
		{
			case 0:
			case 1:
			case 2:
			case 4:
			case 5:
			case 7:
			case 8:
			case 9:
				ID_EX.op = opcode;
				flagR[op1] = true;
				ID_EX.A = op2;
				ID_EX.B = op3;
				ID_EX.destRgr = op1;
			break;

			case 6:
				//logical NOT
				flagR[op1] = true;
				ID_EX.A = op2;
				ID_EX.B = 0;
				ID_EX.op = opcode;	
				ID_EX.destRgr = op1;
			break;
			
			case 11:
				//conditional branching
				ID_EX.op = opcode;
				ID_EX.A = (IR % (1<<8));//the value of L
				ID_EX.B = NOOP;
				ID_EX.destRgr = op1;
				//save the current value in PC
				PCTemp = PC-1;
				//stall the program counter
				PC = NOOP;
				//increment the total number of control stalls
				controlStall+=2;
				stallType.push_back("Branch");
				stallType.push_back("Branch");
			break;
			
			case 10:
				//unconditional branching
				ID_EX.op = opcode;
				ID_EX.A = ((IR>>4) % (1<<8));
				ID_EX.B = NOOP;
				ID_EX.destRgr = NOOP;
				//save the current value of PC 
				PCTemp = PC - 1;
				//stall the program counter
				PC = NOOP;
				//increment the total number of control stalls
				controlStall+=2;
				stallType.push_back("Branch");
				stallType.push_back("Branch");
			break;
			
			case 3:
				//increment
				flagR[op1] = true;
				ID_EX.A = op1;
				ID_EX.B = 0;
				ID_EX.op = opcode;
				ID_EX.destRgr = op1;
			break;

			case 15:
				//exit the program
				ID_EX.op = opcode;
				ID_EX.A = 0;
				ID_EX.B = 0;
				ID_EX.destRgr = 0;
				haltTriggerFlag = true;
			break;
		}

		IF_ID.IR = NOOP;
	}

	void Execute()
	{

		//read values from ID_EX and start execution
		if(ID_EX.op == NOOP)
		{
			//propogate the stall through the pipeline
			EX_MEM.op = NOOP;
			return;
		}

		int opcode = ID_EX.op;
		int a = ID_EX.A;
		int b = ID_EX.B;
		int dest = ID_EX.destRgr;
		int alOp = 0;

		instructionCount++;
		switch(opcode)
		{
			case 0:
				//add 
				alOp = R[a] + R[b];
				EX_MEM.ALUOutput = alOp;
				EX_MEM.destRgr = dest;
				EX_MEM.op = opcode;
				arithmeticInstructionCount++;
				break;
			
			case 1:
				//subtract
				alOp = R[a]-R[b];
				EX_MEM.ALUOutput = alOp;
				EX_MEM.destRgr = dest;
				EX_MEM.op = opcode;
				arithmeticInstructionCount++;
				break;
			
			case 2:
				//multiply
				alOp = R[a]*R[b];
				EX_MEM.ALUOutput = alOp;
				EX_MEM.destRgr = dest;
				EX_MEM.op = opcode;
				arithmeticInstructionCount++;
				break;
			
			case 3:
				//increment
				alOp = R[a] + 1;
				EX_MEM.ALUOutput = alOp;
				EX_MEM.destRgr = dest;
				EX_MEM.op = opcode;
				arithmeticInstructionCount++;
				break;
			
			case 4:
				//bitwise and
				alOp = R[a] & R[b];
				EX_MEM.ALUOutput = alOp;
				EX_MEM.destRgr = dest;
				EX_MEM.op = opcode;
				logicalInstructionCount++;
				break;
			
			case 5:
				//bitwise or
				alOp = R[a] | R[b];
				EX_MEM.ALUOutput = alOp;
				EX_MEM.destRgr = dest;
				EX_MEM.op = opcode;
				logicalInstructionCount++;
				break;
			
			case 6:
				//bitwise not
				alOp = (1<<8) - R[a];//register contains only 8 bit value
				EX_MEM.ALUOutput = alOp;
				EX_MEM.destRgr = dest;
				EX_MEM.op = opcode;
				logicalInstructionCount++;
				break;
			
			case 7:
				//bitwise xor
				alOp = R[a]^R[b];
				EX_MEM.ALUOutput = alOp;
				EX_MEM.destRgr = dest;
				EX_MEM.op = opcode;
				logicalInstructionCount++;
				break;
			
			case 8:
				//calculate address value for load
				alOp = R[a] + b;
				EX_MEM.ALUOutput = alOp;
				EX_MEM.destRgr = dest;
				EX_MEM.op = opcode;
				dataTransferInstructionCount++;
				break;
			
			case 9:
				//calculate address value for store [r2+x] = r1(r1 is present in destRgr)
				alOp = R[a] + b;
				EX_MEM.ALUOutput = alOp;
				EX_MEM.destRgr = dest;
				EX_MEM.op = opcode;
				dataTransferInstructionCount++;
				break;
			
			case 10:
				//unconditional jump
				//set the new value of the program counter
				PC = PCTemp + a;
				if(a>127) PC = PC - 256;
				//reset value of PCTemp
				PCTemp = PC;
				//set branchFlag 
				branchFlag = true;
				//execute MEM and WB stages as stalls
				EX_MEM.op = NOOP;
				controlTransferInstructionCount++;
			break;
			
			case 11:
				//conditional jump
				if(R[dest]==0)
				{
					PC = PCTemp + a;
					if(a>127)PC = PC -256;
				}
				else
				{
					PC = PCTemp + 1;
				}
				//reset the value of PCTemp
				PCTemp = PC;
				//set branchFlag for 2nd stall
				branchFlag = true;
				//execute MEM and WB stages as stalls
				EX_MEM.op = NOOP;
				controlTransferInstructionCount++;
				break;
			
			case 15:
				//halt the program
				EX_MEM.op = opcode;
			break;
		}

		ID_EX.op = NOOP;

	}

	void MemoryAccess()
	{
		if(EX_MEM.op == NOOP)
		{
			MEM_WB.op = NOOP;
			return;
		}

		int opcode = EX_MEM.op;
		int alOp = EX_MEM.ALUOutput;
		int dest = EX_MEM.destRgr;

		//Carry out Data Memory access operations
		switch(opcode)
		{
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
				//normalInsstruction
				MEM_WB.ALUOutput = alOp;
				MEM_WB.op = opcode;
				MEM_WB.destRgr = dest;
				break;

			case 8:
				//Load instruction
				MEM_WB.LD = dataCache[alOp];
				MEM_WB.destRgr = dest;
				MEM_WB.op = opcode;
				break;

			case 9:
				//Store Instruction-  store the value of r1 at address 
				//specified by ALUOutput-store instruction ends here
				dataCache[alOp] = R[dest];
				MEM_WB.LD = 0;
				MEM_WB.destRgr = 0;
				MEM_WB.op = opcode;
				break;
			
			case 15:
				MEM_WB.op = opcode;
				break;
		}

		EX_MEM.op = NOOP;
	}

	void WriteBack()
	{
		if(MEM_WB.op==NOOP)
		{
			//cout<<(PC)<<" ";
			return;
		}

		//cout<<(PC)<<" ";
		int dest = MEM_WB.destRgr;
		int opcode = MEM_WB.op;
		int aluOutput = MEM_WB.ALUOutput;
		int loadRegister = MEM_WB.LD;

		//Perform operations and reset the flags set in Instruction Decode
		switch(opcode)
		{
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
				//write contents to register values
				//and set the flag variable of current register to false
				R[dest] = aluOutput;
				flagR[dest] = false;
				flagWBR[dest] = true;
				break;

			case 8:
				R[dest] = loadRegister;
				flagR[dest] = false;
				flagWBR[dest] = true;
			break;

			case 15:
				keepRunning = false;
				return;
			break;
		}

		MEM_WB.op = NOOP;
	}

	void getFileContents(ifstream& fin, vector<int> &arr)
	{
		int address;
		do
    	{
    		//read address as a hexadecimal integer
    		fin>>hex>>address;
    		arr.push_back(address);
    	}while(fin.good());
	}

	void initializeFiles()
	{
		ifstream fin;

		fin.open("ICache.txt");
		getFileContents(fin, instCache);
		fin.close();

		fin.open("DCache.txt");
		getFileContents(fin, dataCache);
		fin.close();

		fin.open("RF.txt");
		getFileContents(fin, R);
		fin.close();
	}

	void startProcessor()
	{
		while(keepRunning)
		{
			cycleCount++;
			WriteBack();
			MemoryAccess();
			Execute();
			InstructionDecode();
			InstructionFetch();
		}
		totalStall = dataStall + controlStall;
	}
	
	void getStatistics()
	{
		ofstream fout;
		
		fout.open("output.txt");

		fout<<instructionCount<<"\n";
		fout<<arithmeticInstructionCount<<"\n";
		fout<<logicalInstructionCount<<"\n";
		fout<<dataTransferInstructionCount<<"\n";
		fout<<controlTransferInstructionCount<<"\n";
		cyclesPerInstruction = 1.0 * cycleCount/instructionCount;
		fout<<cyclesPerInstruction<<"\n";
		fout<<totalStall<<"\n";

		for(int i=0;i<totalStall;++i)
		{
			fout<<"S"<<(i+1)<<" : "<<stallType[i]<<"\n";
		}
		fout<<"RAW: "<<dataStall<<"\n";
		fout<<"Control: "<<controlStall<<"\n";
		
		fout.close();

		fout.open("RF.txt");
		for(int i=0;i<15;++i)
		{
			fout<<R[i]<<"\n";
		}
		fout.close();

		fout.open("DCache.txt");
		for(int i=0;i<dataCache.size();++i)
		{
			fout<<dataCache[i]<<"\n";
		}
		fout.close();
	}

	void startAll()
	{
		initializeFiles();
		startProcessor();
		getStatistics();
	}
};

int main()
{
	Processor obj;
	obj.startAll();
	return 0;
}