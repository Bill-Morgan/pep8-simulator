#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <bitset>

using namespace std;

unsigned char g_memory[0xFFFF];
bool g_statFlag[4];

const unsigned int a = 0, N = 0;
const unsigned int x = 1, Z = 1;
const unsigned int sp = 2, V = 2;
const unsigned int pc = 3, C = 3;

union registers
{
    short int intVal;
    unsigned short int indexVal;
    struct
    {
        unsigned int lo : 8;
        unsigned int hi : 8;
    } hilo;
} g_reg[4];

union instruction
{
    union
    {
        unsigned short int address;
        short int imedData;
        struct
        {
            unsigned int lo : 8;
            unsigned int hi : 8;
        } hilo;
    } opSpec;
    struct
    {
        unsigned int address : 16;
        unsigned int cmd : 8;
    } ins8;
    struct
    {
        unsigned int address : 16;
        unsigned int aaa : 3;
        unsigned int r : 1;
        unsigned int cmd : 4;
    } ins413;
    struct
    {
        unsigned int address : 16;
        unsigned int aaa : 3;
        unsigned int cmd : 5;
    } ins53;
    struct
    {
        unsigned int address : 16;
        unsigned int a : 1;
        unsigned int cmd : 7;
    } ins71;
};

int g_spStart = 0xFBCF;
int cmd4(instruction);
int cmd5(instruction);
int cmd7(instruction);
int cmd8(instruction);
char getCharFromInpStr();
short int addShorts(short int, short int);
short int twosComplement(short int, bool setStatusRegs = false);
void dumpRegs();
void dumpMem(string);
void printDebug(string);
int asciiCharHexToInt(char);
void help();
void setNZ(short int);
unsigned short int getIntFromInpStr();
void setAAAMode(int, instruction &);
short int asl(short int);
void spPushWord(registers);
string intToHexAscii(unsigned int, int);
void printSp();
void getInput();
void error(string);
string lTrim(string);

bool g_theEnd = false;
bool g_silent = false;
bool g_debugMode = false;
string g_batchInputStr;

int main(int argc, char *argv[])
{
    string cFile = "", iFile, tmp, dumpMemStartEnd = "";
    if (argc > 1)
    {
        if (argc == 2)
        {
            cFile = argv[1];
        }
        else
        {
            int i = 1;
            while (i < argc)
            {
                string tmp = argv[i];
                if (tmp == "-cf")
                {
                    cFile = argv[++i];
                }
                else if (tmp == "-if")
                {
                    iFile = argv[++i];
                }
                else if (tmp == "-i")
                {
                    g_batchInputStr = argv[++i];
                }
                else if (tmp == "-d")
                {
                    g_debugMode = true;
                }
                else if (tmp == "-s")
                {
                    g_silent = true;
                }
                else if (tmp == "-dm")
                {
                    dumpMemStartEnd = argv[++i];
                }
                i++;
            }
        }
        if (cFile.length() == 0)
        {
            help();
            return 1;
        }
        ifstream pep8File(cFile);
        if (pep8File.is_open())
        {
            string cmdStr = "";
            printDebug("\n\t********* input file **********\n");
            string line;
            while (getline(pep8File, line))
            {
                printDebug("\n\t" + line + "");
                cmdStr += line;
            }
            pep8File.close();
            union
            {
                struct
                {
                    int lo : 4;
                    int hi : 4;
                } hilo;
                char ch;
            } memByte;
            int index = 0;
            while (cmdStr.length() != 0)
            {
                if (cmdStr.at(0) == 'z')
                    break;
                if (cmdStr.at(0) != ' ')
                {
                    memByte.hilo.hi = asciiCharHexToInt(cmdStr.at(0));
                    cmdStr.erase(0, 1);
                    memByte.hilo.lo = asciiCharHexToInt(cmdStr.at(0));
                    g_memory[index++] = memByte.ch;
                }
                cmdStr.erase(0, 1);
            }
            if (g_debugMode)
            {
                dumpMem("0 " + to_string(index));
            }
        }
    }
    else
    {
        help();
        return 1;
    }
    if (iFile.length() != 0 & g_batchInputStr.length() == 0)
    {
        ifstream inpFile(iFile);
        if (inpFile.is_open())
        {
            string line;
            while (getline(inpFile, line))
            {
                if (g_batchInputStr.length() > 0)
                {
                    g_batchInputStr = g_batchInputStr + "\n";
                }
                g_batchInputStr = g_batchInputStr + line;
            }
            inpFile.close();
        }
    }
    printDebug("\n\n\t******* input string *********\n");
    printDebug("\n\t\"" + g_batchInputStr + "\"");

    instruction ins;

    g_reg[pc].indexVal = 0;
    g_reg[a].intVal = 0;
    g_reg[x].intVal = 0;
    g_reg[sp].indexVal = g_spStart;

    printDebug("\n\n******************  Start Execution  ******************\n\n");
    do
    {
        ins.ins8.cmd = g_memory[g_reg[pc].indexVal++];
        if (ins.ins71.cmd < 0b0000010)
        {
            cmd8(ins);
        }
        else if (ins.ins53.cmd < 0b00101)
        {
            cmd7(ins);
        }
        else if (ins.ins413.cmd < 0b0111)
        {
            cmd5(ins);
        }
        else
        {
            cmd4(ins);
        }
        dumpRegs();
    } while (!g_theEnd);
    printDebug("\n\n******************  End Execution  ******************\n\n");
    dumpMem(dumpMemStartEnd);
    return 0;
}

int cmd8(instruction ins)
{
    switch (ins.ins8.cmd)
    {
    case 0b00000000: // Stop ececution   REQUIRED
        printDebug("\n0000 0000 | STOP  ");
        g_theEnd = true;
        return 0;
        break;
    case 0b00000001: // Return from trap
        printDebug("\n0000 0001 | RETTR  ");
        error("I don't know what return from trap does");
        break;
    case 0b00000010: // Move SP to A
        printDebug("\n0000 0010 | MOVSPA ");
        g_reg[a] = g_reg[sp];
        break;
    case 0b00000011: // Move NZVC flags to A
        printDebug("\n0000 0011 | MOVFLGA");
        g_reg[a].intVal = g_statFlag[N] * 0b1000 + g_statFlag[Z] * 0b100 + g_statFlag[V] * 0b10 + g_statFlag[C];
        break;
    }
    return 0;
}

int cmd7(instruction ins)
{
    registers temp;
    if (ins.ins71.cmd < 0b0001100)
    {
        if (ins.ins71.a == 0b0)
        {
            ins.ins71.address = g_memory[g_reg[pc].indexVal] * 0x100 + g_memory[g_reg[pc].indexVal + 1];
        }
        else
        {
            ins.ins71.address = g_reg[x].indexVal;
        }
        g_reg[pc].indexVal += 2;
    }
    switch (ins.ins71.cmd)
    {
    case 0b0000010: // Branch unconditional
        printDebug("\n0000 001a | BR     ");
        g_reg[pc].indexVal = ins.ins71.address;
        break;
    case 0b0000011: // Branch if less than or equal to
        printDebug("\n0000 011a | BRLE   ");
        if (g_statFlag[Z] | g_statFlag[N])
        {
            g_reg[pc].indexVal = ins.ins71.address;
        }
        break;
    case 0b0000100: // Branch if less than
        printDebug("\n0000 100a | BRLT   ");
        if (g_statFlag[N])
        {
            g_reg[pc].indexVal = ins.ins71.address;
        }
        break;
    case 0b0000101: // Branch if equal to
        printDebug("\n0000 101a | BREQ   ");
        if (g_statFlag[Z])
        {
            g_reg[pc].indexVal = ins.ins71.address;
        }
        break;
    case 0b0000110: // Branch if not equal to
        printDebug("\n0000 110a | BRNE   ");
        if (!g_statFlag[Z])
        {
            g_reg[pc].indexVal = ins.ins71.address;
        }
        break;
    case 0b0000111: // Branch if greater than or equal to
        printDebug("\n0000 111a | BRGE   ");
        if (!g_statFlag[N] | g_statFlag[Z])
        {
            g_reg[pc].indexVal = ins.ins71.address;
        }
        break;
    case 0b0001000: // Branch if greater than
        printDebug("\n0001 000a | BRGT   ");
        if (!g_statFlag[N] & !g_statFlag[Z])
        {
            g_reg[pc].indexVal = ins.ins71.address;
        }
        break;
    case 0b0001001: // Branch if V
        printDebug("\n0001 001a | BRV    ");
        if (g_statFlag[V])
        {
            g_reg[pc].indexVal = ins.ins71.address;
        }
        break;
    case 0b0001010: // Branch if C
        printDebug("\n0001 010a | BRC    ");
        if (g_statFlag[C])
        {
            g_reg[pc].indexVal = ins.ins71.address;
        }
        break;
    case 0b0001011: // Call subrotine
        printDebug("\n0001 011a | CALL   ");
        spPushWord(g_reg[pc]);
        g_reg[pc].indexVal = ins.ins71.address;
        break;
    case 0b0001100: // Bitwise invert r
        printDebug("\n0001 100r | NOTr   ");
        g_reg[ins.ins71.a].indexVal = ~g_reg[ins.ins71.a].indexVal;
        setNZ(g_reg[ins.ins71.a].intVal);
        break;
    case 0b0001101: // Negate r
        printDebug("\n0001 101r | NEGr   ");
        {
            /*
                I don't know if this is correct but it duplicates the pep8 simulator results for:
                    C0 ff ff
                    70 80 00
                    1A 1B zz
            */
            bool tempC = g_statFlag[C];
            g_reg[ins.ins71.a].intVal = twosComplement(g_reg[ins.ins71.a].intVal, true);
            g_statFlag[C] = tempC;
        }
        break;
    case 0b0001110: // Arithmetic shift left r
        printDebug("\n0001 110r | ASLr   ");
        g_reg[ins.ins71.a].intVal = asl(g_reg[ins.ins71.a].intVal);
        break;
    case 0b0001111: // Arithmetic shift right r
        printDebug("\n0001 111r | ASRr   ");
        g_statFlag[C] = g_reg[ins.ins71.a].intVal & 0b1 == 0b1;
        g_reg[ins.ins71.a].intVal = g_reg[ins.ins71.a].intVal >> 1;
        setNZ(g_reg[ins.ins71.a].intVal);
        break;
    case 0b0010000: // Rotate left r
    {
        printDebug("\n0010 000r | ROLr   ");
        bool newC = g_reg[ins.ins71.a].intVal < 0;
        g_reg[ins.ins71.a].indexVal = g_reg[ins.ins71.a].indexVal << 1;
        if (g_statFlag[C])
        {
            g_reg[ins.ins71.a].indexVal += 0x0001;
        }
        g_statFlag[C] = newC;
    }
    break;
    case 0b0010001: // Rotate right r
    {
        printDebug("\n0010 001r | RORr   ");
        bool newC = g_reg[ins.ins71.a].intVal & 0b1 == 0b1;
        g_reg[ins.ins71.a].indexVal = g_reg[ins.ins71.a].indexVal >> 1;
        if (g_statFlag[C])
        {
            g_reg[ins.ins71.a].indexVal += 0x8000;
        }
        g_statFlag[C] = newC;
    }
    break;
    }
    return 0;
}

int cmd5(instruction ins)
{
    registers temp;
    setAAAMode(ins.ins53.aaa, ins);
    if (ins.ins53.aaa == 0)
    {
        temp.indexVal = ins.opSpec.address;
    }
    else
    {
        temp.indexVal = g_memory[ins.opSpec.address] * 0x100 + g_memory[ins.opSpec.address + 1];
    }
    switch (ins.ins53.cmd)
    {
    case 0b00100: // Unary no operation trap
        printDebug("\n0001 01nn | NOPn   ");
        error("I don't know what this command does");
        break;
    case 0b00101: // Nonunary no operation trap
        printDebug("\n0001 1aaa | NOP    ");
        error("I don't know what this command does");
        break;
    case 0b00110: // Decimal input trap
        printDebug("\n0011 0aaa | DECI   ");
        if (ins.ins53.aaa == 0b000)
        {
            error("Invalid trap addressing mode.");
            return 1;
        }
        temp.indexVal = getIntFromInpStr();
        g_memory[ins.ins53.address] = temp.hilo.hi;
        g_memory[ins.ins53.address + 1] = temp.hilo.lo;
        break;
    case 0b00111: // Decimal output trap
        printDebug("\n0011 1aaa | DECO   ");
        if (ins.ins53.aaa == 0b000)
        {
            error("Invalid trap addressing mode.");
            return 1;
        }
        temp.hilo.hi = g_memory[ins.ins53.address];
        temp.hilo.lo = g_memory[ins.ins53.address + 1];
        printDebug("\nDecimal output  [");
        cout << temp.intVal;
        printDebug("]");
        break;
    case 0b01000: // String output trap
        printDebug("\n0100 0aaa | STRO");
        printDebug("\nString output  [");
        if (ins.ins53.aaa == 0b000)
        {
            error("Invalid trap addressing mode.");
            return 1;
        }
        while (g_memory[ins.ins53.address] != 0)
        {
            cout << (g_memory[ins.ins53.address++]);
        }
        printDebug("]");
        break;
    case 0b01001: // Character input
        printDebug("\n0100 1aaa | CHARI  ");
        if (ins.ins53.aaa == 0b000)
        {
            error("Invalid trap addressing mode.");
            return 1;
        }
        g_memory[ins.ins53.address] = getCharFromInpStr();
        break;
    case 0b01010: // Character output
        printDebug("\n0101 0aaa | CHARO [");
        if (ins.ins53.aaa == 0)
        {
            cout << char(ins.opSpec.hilo.lo);
        }
        else
        {
            cout << char(g_memory[ins.ins8.address]);
        }
        printDebug("]");
        break;
    case 0b01011: // Return from call with n local bytes
        printDebug("\n0101 1aaa | RETn   ");
        g_reg[sp].indexVal += ins.ins53.aaa;
        g_reg[pc].hilo.hi = g_memory[g_reg[sp].indexVal++];
        g_reg[pc].hilo.lo = g_memory[g_reg[sp].indexVal++];
        break;
    case 0b01100: // Add to stack pointer (SP)
        printDebug("\n0110 0aaa | ADDSP  ");
        g_reg[sp].indexVal = addShorts(ins.opSpec.imedData, g_reg[sp].indexVal);
        break;
    case 0b01101: // Subtract from stack pointer (SP)
        printDebug("\n0110 1aaa | SUBSP  ");
        g_reg[sp].indexVal = addShorts(g_reg[sp].indexVal, twosComplement(ins.opSpec.imedData));
        break;
    default:
        break;
    }
    return 0;
}

int cmd4(instruction ins)
{
    registers temp;
    setAAAMode(ins.ins413.aaa, ins);
    if (ins.ins413.aaa == 0)
    {
        temp.indexVal = ins.opSpec.address;
    }
    else
    {
        temp.indexVal = g_memory[ins.opSpec.address] * 0x100 + g_memory[ins.opSpec.address + 1];
    }
    switch (ins.ins413.cmd)
    {
    case 0b0111: // ADD to r
        printDebug("\n0111 raaa | ADDr   ");
        g_reg[ins.ins413.r].intVal = addShorts(g_reg[ins.ins413.r].intVal, temp.intVal);
        break;
    case 0b1000: // Subtract from r
        printDebug("\n1000 raaa | SUBr   ");
        g_reg[ins.ins413.r].intVal = addShorts(twosComplement(temp.intVal), g_reg[ins.ins413.r].intVal);
        break;
    case 0b1001: // Bitwise AND to r
        printDebug("\n1001 raaa | ANDr   ");
        g_reg[ins.ins413.r].intVal = g_reg[ins.ins413.r].intVal & temp.intVal;
        setNZ(g_reg[ins.ins413.r].intVal);
        break;
    case 0b1010: // Bitwize OR to r
        printDebug("\n1010 raaa | ORr    ");
        g_reg[ins.ins413.r].intVal = g_reg[ins.ins413.r].intVal | temp.intVal;
        setNZ(g_reg[ins.ins413.r].intVal);
        break;
    case 0b1011: // Compare r
        printDebug("\n1011 raaa | CPr    ");
        addShorts(g_reg[ins.ins413.r].indexVal, twosComplement(temp.intVal));
        break;
    case 0b1100: // load r from memory
        printDebug("\n1100 raaa | LDr    ");
        g_reg[ins.ins413.r].intVal = temp.intVal;
        setNZ(g_reg[ins.ins413.r].intVal);
        break;
    case 0b1101: // load byte from memory
        printDebug("\n1101 raaa | LDBYTEr");
        g_reg[ins.ins413.r].hilo.lo = temp.indexVal / 0x100;
        setNZ(g_reg[ins.ins413.r].intVal);
        break;
    case 0b1110: // store r to memory
        printDebug("\n1110 raaa | STr    ");
        if (ins.ins413.aaa == 0)
        {
            error("Invalid Addressing Mode");
            return 1;
        }
        g_memory[ins.opSpec.address] = g_reg[ins.ins413.r].hilo.hi;
        g_memory[ins.opSpec.address + 1] = g_reg[ins.ins413.r].hilo.lo;
        break;
    case 0b1111: // store r byte to memory
        printDebug("\n1111 raaa | STBYTEr");
        if (ins.ins413.aaa == 0)
        {
            error("Invalid Addressing Mode");
            return 1;
        }
        g_memory[ins.opSpec.address] = g_reg[ins.ins413.r].hilo.lo;
        break;
    }
    return 0;
}

void error(string message)
{
    cout << "\nERROR: " << message;
    g_theEnd = true;
}

int asciiCharHexToInt(char ch)
{
    int tmp;
    stringstream ss;
    ss << hex << ch;
    ss >> tmp;
    tmp -= 48;
    if (tmp > 9)
    {
        tmp -= 49;
    }
    return tmp;
}

string intToHexAscii(unsigned int i, int numChars)
{
    stringstream s;
    s << uppercase << hex << internal << setfill('0');
    s << setw(numChars) << i;
    return s.str();
}

void setAAAMode(int aaa, instruction &ins)
{
    instruction temp;
    registers tempReg;
    switch (aaa)
    {
    case 0x000: // imediate aaa mode
    case 0b001: // direct aaa mode
        ins.opSpec.hilo.hi = g_memory[g_reg[pc].indexVal++];
        ins.opSpec.hilo.lo = g_memory[g_reg[pc].indexVal++];
        break;
    case 0b010: // Indirect aaa mode
        setAAAMode(0b001, temp);
        ins.opSpec.hilo.hi = g_memory[temp.ins8.address++];
        ins.opSpec.hilo.lo = g_memory[temp.ins8.address];
        break;
    case 0b011: // Stack-relative
        setAAAMode(0b000, temp);
        ins.opSpec.address = g_reg[sp].indexVal + temp.opSpec.imedData;
        break;
    case 0b100: // Stack-relative deferred
        error("address mode not implemented");
        return;
        setAAAMode(0b011, temp);
        tempReg.indexVal = g_memory[ins.opSpec.address] * 0x100 + g_memory[ins.opSpec.address + 1];
        ins.opSpec.hilo.hi = g_memory[tempReg.indexVal];
        ins.opSpec.hilo.lo = g_memory[tempReg.indexVal + 1];
        break;
    case 0b101: // Indexed
        error("address mode not implemented");
        return;
        setAAAMode(0b000, temp);
        ins.opSpec.hilo.hi = g_memory[g_reg[x].indexVal + temp.ins8.address++];
        ins.opSpec.hilo.lo = g_memory[g_reg[x].indexVal + temp.ins8.address];
        break;
    case 0b110: // Stack-indexed
        error("address mode not implemented");
        return;
        setAAAMode(0b000, temp);
        ins.opSpec.hilo.hi = g_memory[g_reg[sp].indexVal + temp.ins8.address++ + g_reg[x].indexVal];
        ins.opSpec.hilo.lo = g_memory[g_reg[sp].indexVal + temp.ins8.address + g_reg[x].indexVal];
        break;
    case 0b111: // Stack-indexed deferred
        error("address mode not implemented");
        return;
        setAAAMode(0b110, temp);
        ins.opSpec.hilo.hi = g_memory[temp.ins8.address++];
        ins.opSpec.hilo.lo = g_memory[temp.ins8.address];
        break;
    }
}

short int asl(short int shortVal)
{
    int intVal = (unsigned short int)shortVal;
    intVal = intVal << 1;
    short int newShortVal = intVal % 0x10000;
    bool a = (intVal & 0x10000 == 0x10000);
    bool c = shortVal < 0;
    bool d = newShortVal < 0;
    g_statFlag[C] = a;
    g_statFlag[V] = c != d;
    setNZ(newShortVal);
    return newShortVal;
}

short int addShorts(short int short1, short int short2)
{
    int val1 = (unsigned short int)short1;
    int val2 = (unsigned short int)short2;
    int sum = val1 + val2;
    short int shortSum = sum % 0x10000;
    bool a = ((sum & 0x10000) != 0);
    bool b = short1 < 0;
    bool c = short2 < 0;
    bool d = shortSum < 0;
    g_statFlag[C] = a;
    g_statFlag[V] = (b == c) & (d != b);
    setNZ(shortSum);
    return shortSum;
}

void spPushWord(registers data)
{
    g_memory[--g_reg[sp].indexVal] = data.hilo.lo;
    g_memory[--g_reg[sp].indexVal] = data.hilo.hi;
}

void setNZ(short int short1)
{
    g_statFlag[N] = short1 < 0;
    g_statFlag[Z] = short1 == 0;
}

short int twosComplement(short int short1, bool setStatusRegs)
{
    if (setStatusRegs)
    {
        return (addShorts(~short1, 1));
    }
    else
    {
        return (~short1 + 1);
    }
}

void printDebug(string str)
{
    if (!g_debugMode)
        return;
    if (str.length() == 0)
    {
        str = "[debug] no string \n";
    }
    cout << str;
}

void dumpMem(string startEnd)
{
    if (startEnd.length() == 0)
        return;
    cout << "\n\n\t******** memory dump *********\n";
    string temp = startEnd.substr(0, startEnd.find(" "));
    int start = stoi(temp);
    if ((start % 8) != 0)
    {
        start = (start / 8) * 8;
    }
    temp = startEnd.substr(temp.length());
    int end = stoi(temp);
    if ((end % 8) != 0)
    {
        end = (((end / 8) + 1) * 8) - 1;
    }
    for (unsigned int i = start; i <= end; i++)
    {
        if ((i % 8) == 0)
        {
            cout << "\n\t" << intToHexAscii(i, 4) << " |";
        }
        cout << " " << intToHexAscii(g_memory[i], 2);
    }
    cout << endl
         << endl;
}

void printSp()
{
    int start = g_reg[sp].indexVal;
    if ((start % 8) != 0)
    {
        start = (start / 8) * 8;
    }
    int end = g_spStart;
    if ((end % 8) != 0)
    {
        end = (((end / 8) + 1) * 8);
    }
    for (unsigned int i = start; i < end; i++)
    {
        if ((i % 8) == 0)
        {
            if (i != start)
            {
                cout << " \n\t\t\t\t\t";
            }
            cout << intToHexAscii(i, 4) << " |";
        }
        if (i == g_reg[sp].indexVal)
        {
            cout << " \033[30;47m" << intToHexAscii(g_memory[i], 2) << "\033[0m";
        }
        else
        {
            cout << " " << intToHexAscii(g_memory[i], 2);
        }
    }
    cout << endl;
}

void dumpRegs()
{
    if (g_silent)
        return;
    if (!g_debugMode)
    {
        cout << "\t\t";
    }
    cout << "\t|| Registers:"
         << "\ta = 0x" << intToHexAscii(g_reg[a].indexVal, 4)
         << "  x = 0x" << intToHexAscii(g_reg[x].indexVal, 4)
         << " sp = 0x" << intToHexAscii(g_reg[sp].indexVal, 4)
         << " pc = 0x" << intToHexAscii(g_reg[pc].indexVal, 4)
         << " | Status bits:"
         << " N = " << g_statFlag[N]
         << "  Z = " << g_statFlag[Z]
         << "  V = " << g_statFlag[V]
         << "  C = " << g_statFlag[C]
         << "\n\t\t\t|| Stack:\t";
    printSp();
    cout << endl;
}

char getCharFromInpStr()
{
    getInput();
    char retVal = g_batchInputStr.at(0);
    g_batchInputStr.erase(0, 1);
    return retVal;
}

unsigned short int getIntFromInpStr()
{
    getInput();
    long long value = 0;
    try
    {
        value = stoi(g_batchInputStr);
        g_batchInputStr = g_batchInputStr.substr(to_string(value).length(), string::npos);
        g_statFlag[N] = (value & 0x8000) == 0x8000;
        g_statFlag[V] = (value < 0) ^ g_statFlag[N];
        printDebug("\nDecimal input  [" + to_string(value) + "]");
    }
    catch (invalid_argument err)
    {
        error("Invalid DECI input");
    }
    return value % 0x10000;
}

void getInput()
{
    g_batchInputStr = lTrim(g_batchInputStr);
    int pos = g_batchInputStr.find_first_not_of(" \n\r\t\f\v");
    while (pos == -1 || g_batchInputStr.length() == 0)
    {
        cin >> g_batchInputStr;
        pos = g_batchInputStr.find_first_not_of(" \n\r\t\f\v");
    }
}

string lTrim(string data)
{
    int pos = data.find_first_not_of(" \n\r\t\f\v");
    if (pos > 0)
    {
        data = data.substr(pos);
    }
    return data;
}

void help()
{
    cout << "\npep8"
         << "\nWilliam Morgan"
         << "\nUIS wmorga4"
         << "\nSP22 CSC376"
         << "\nMid-term Project"
         << "\n\n\n"
         << "Usage:"
         << "\n\npep8.exe [args]"
         << "\n     Args:"
         << "\n              If there is only a single argument, it is expected to be the command file file name"
         << "\n        -cf   Command file flag.  -cf followed by file name.  REQUIRED"
         << "\n        -if   Input file flag.  -if followed by file name.  Optional"
         << "\n        -i    Input flag. -i followed by a input string.  Optional"
         << "\n        -d    Turn on debug mode.  Provides more debugging output. Optional. If omitted, debugging mode is off"
         << "\n        -s    Turn on silent mode.  Turn off Register/Status dump after each command. If omitted, silent mode is off"
         << "\n        -dm   Dump memory flag.  -dm followed by a string with memory start address and end address separated by a space. Optional. If included, the memory from start to end will be dumped to screen when the program terminates."
         << "\n\n Example:"
         << "\nC:\\pep8> ./pep8.exe -cf cmds.pepo -d -i \"ABCD1234\" -dm \"25 50\"";
}