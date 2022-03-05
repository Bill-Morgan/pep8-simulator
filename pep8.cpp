#include<iostream>
#include<fstream>
#include<sstream>
#include<iomanip>
#include<string>
#include<bitset>

using namespace std;

unsigned char g_memory[0xFFFF];
bool g_statFlag[4];

const unsigned int a = 0, N = 0;
const unsigned int x = 1, Z = 1;
const unsigned int sp = 2, V = 2;
const unsigned int pc = 3, C = 3;

union registers {
    short int intVal;
    unsigned short int indexVal;
    struct {
        unsigned int lo: 8;
        unsigned int hi: 8;
    } hilo;
} g_reg[4];

union instruction
{
    union {
        unsigned short int address;
        short int imedData;
        struct
        {
            unsigned int lo: 8;
            unsigned int hi: 8;
        } hilo;
    } opSpec;
    struct  
    {
        unsigned int address: 16;
        unsigned int cmd: 8;
    } ins8;
    struct
    {
        unsigned int address: 16;
        unsigned int aaa: 3;
        unsigned int r: 1;
        unsigned int cmd: 4;
    } ins413;
    struct
    {
        unsigned int address: 16;
        unsigned int aaa: 3;
        unsigned int cmd: 5;
    } ins53;
    struct
    {
        unsigned int address: 16;
        unsigned int a: 1;
        unsigned int cmd: 7;
    } ins71;
};

int cmd4(instruction);
int cmd5(instruction);
int cmd7(instruction);
int cmd8(instruction);
void set_ins_address(instruction &);
char get_char_from_inpStr();
short int add_shorts(short int, short int);
short int twos_complement(short int);
void dump_regs();
void dump_mem(string);
void print_debug(string);
int ascii_char_hex_to_int(char);
void help();
void set_NZ(short int);
unsigned short int get_int_from_inpStr();
void set_aaa_mode(int, instruction&);
short int asl(short int);
void br(int);
void sp_push_word(registers);
unsigned short int reg_indexed(registers, int);
string int_to_hex_ascii(unsigned int, int);
void error(string);

bool the_end = false;
bool silent = false;
bool debugMode = false;
string dumpMem = "";
string g_inputStr;

int main(int argc, char *argv[]) {
    string cFile = "", iFile, tmp;
    if (argc > 1) {
        int i = 1;
        while (i < argc)
        {
            string tmp = argv[i];
            if (tmp == "-cf") 
            {
                cFile = argv[++i];
            } else if (tmp == "-if") 
            {
                iFile = argv[++i];
            } else if(tmp == "-i")
            {
                g_inputStr = argv[++i];
            } else if (tmp == "-d") 
            {
                debugMode = true;
            } else if (tmp == "-s")
            {
                silent = true;
            } else if (tmp == "-dm")
            {
                dumpMem = argv[++i];
            }
            i++;
        }
        if (cFile.length() == 0) 
        {
            help();
            return 1;
        }
        ifstream pep8file (cFile);
        if (pep8file.is_open()) {
            string line;
            int index = 0;
            int tmp;
            union {
                struct {
                    int lo: 4;
                    int hi: 4;
                } hilo;
                char ch;
            } memByte;
            string cmdStr = "";
            print_debug("\n\t********* input file **********\n");
            while (getline(pep8file, line)) {
                print_debug("\n\t" + line + "");
                cmdStr += line;
            }
            pep8file.close();
            int pos = 0;
            while (cmdStr.length() != 0) {
                if (cmdStr.at(0) == 'z') break;
                if (cmdStr.at(0) != ' ') {
                    memByte.hilo.hi = ascii_char_hex_to_int(cmdStr.at(0));
                    cmdStr.erase(0, 1);
                    memByte.hilo.lo = ascii_char_hex_to_int(cmdStr.at(0));
                    g_memory[index++] = memByte.ch;
                }
                cmdStr.erase(0, 1);
            }
            if (debugMode) {
                dump_mem("0 " + to_string(index));
            }
        }
    } else {
        help();
        return 1;
    }
    if (iFile.length() != 0 & g_inputStr.length() == 0) {
        ifstream inpFile (iFile);
        if (inpFile.is_open()) {
            string line;
            while (getline(inpFile, line)) {
                if (g_inputStr.length() > 0) {
                    g_inputStr = g_inputStr + "\n";
                }
                g_inputStr = g_inputStr + line;
            }
            inpFile.close();
        }
    }
    print_debug("\n\n\t******* input string *********\n");
    print_debug("\n\t\"" + g_inputStr + "\"");

    instruction ins;

    g_reg[pc].indexVal = 0;
    g_reg[a].intVal = 0;
    g_reg[x].intVal = 0;
    g_reg[sp].indexVal = 0xFBCF;

    // stringstream s;
    bitset<4> s(13);
    // s << bin << internal << setfill('0');
    // s << setw(4) << 25;
    cout << endl << s << endl;
    print_debug("\n\n******************  Start Execution  ******************\n\n");
    do
    {
        ins.ins8.cmd = g_memory[g_reg[pc].indexVal++];
        if (ins.ins71.cmd < 0b0000010) {
            cmd8(ins);
        } else if (ins.ins53.cmd < 0b00101) {
            cmd7(ins);
        } else if (ins.ins413.cmd < 0b0111) {
            cmd5(ins);
        } else {
            cmd4(ins);
        }
        dump_regs();
    } while (!the_end);
    print_debug("\n\n******************  End Execution  ******************\n\n");
    dump_mem(dumpMem);
    return 0;
}

int cmd8(instruction ins) {
    switch (ins.ins8.cmd) {
        case 0b00000000: // Stop ececution   REQUIRED
            print_debug("\n0000 0000 | STOP  ");
            the_end = true;
            return 0;
            break;
        case 0b00000001: // Return from trap
            print_debug("\n0000 0001 | RETTR  ");
            error("I don't know what return from trap does");
            break;
        case 0b00000010: // Move SP to A
            print_debug("\n0000 0010 | MOVSPA ");
            g_reg[a] = g_reg[sp];
            break;
        case 0b00000011: // Move NZVC flags to A
            print_debug("\n0000 0011 | MOVFLGA");
            g_reg[a].intVal = g_statFlag[N] * 0b1000 + g_statFlag[Z] * 0b100 + g_statFlag[V] * 0b10 + g_statFlag[C];
            break;
    };
    return 0;
}

int cmd7(instruction ins) {
    registers temp;
    switch (ins.ins71.cmd) 
    {
    case 0b0000010: // Branch unconditional
        print_debug("\n0000 001a | BR     ");
        br(ins.ins71.a);
        break;
    case 0b0000011: // Branch if less than or equal to
        print_debug("\n0000 011a | BRLE   ");
        if (g_statFlag[Z] | g_statFlag[N]) {
            br(ins.ins71.a);
        }
        break;
    case 0b0000100: // Branch if less than
        print_debug("\n0000 100a | BRLT   ");
        if (g_statFlag[N]) {
            br(ins.ins71.a);
        }
        break;
    case 0b0000101: // Branch if equal to
        print_debug("\n0000 101a | BREQ   ");
        if (g_statFlag[Z]) {
            br(ins.ins71.a);
        }
        break;
    case 0b0000110: // Branch if not equal to
        print_debug("\n0000 110a | BRNE   ");
        if (!g_statFlag[Z]) {
            br(ins.ins71.a);
        }
        break;
    case 0b0000111: // Branch if greater than or equal to
        print_debug("\n0000 111a | BRGE   ");
        if (!g_statFlag[N] | g_statFlag[Z]) {
            br(ins.ins71.a);
        }
        break;
    case 0b0001000: // Branch if greater than
        print_debug("\n0001 000a | BRGT   ");
        if (!g_statFlag[N] & !g_statFlag[Z]) {
            br(ins.ins71.a);
        }
        break;
    case 0b0001001: // Branch if V
        print_debug("\n0001 001a | BRV    ");
        if (g_statFlag[V]) {
            br(ins.ins71.a);
        }
        break;
    case 0b0001010: // Branch if C
        print_debug("\n0001 010a | BRC    ");
        if (g_statFlag[C]) {
            br(ins.ins71.a);
        }
        break;
    case 0b0001011: // Call subrotine 
        print_debug("\n0001 011a | CALL   ");
        temp = g_reg[pc];
        g_reg[pc].indexVal += 2;
        sp_push_word(g_reg[pc]);
        if (ins.ins71.a == 0) {
            g_reg[pc].hilo.hi = g_memory[temp.indexVal++];
            g_reg[pc].hilo.lo = g_memory[temp.indexVal];
        } else {
            
            g_reg[pc].indexVal =  reg_indexed(temp, g_reg[x].intVal);
        }
        break;
    case 0b0001100: // Bitwise invert r
        print_debug("\n0001 100r | NOTr   ");
        g_reg[ins.ins71.a].indexVal = ~g_reg[ins.ins71.a].indexVal;
        set_NZ(g_reg[ins.ins71.a].intVal);
        break;
    case 0b0001101: // Negate r
        break;
    case 0b0001110: // Arithmetic shift left r
        print_debug("\n0001 110r | ASLr   ");
        g_reg[ins.ins71.a].intVal = asl(g_reg[ins.ins71.a].intVal);
        break;
    case 0b0001111: // Arithmetic shift right r
        print_debug("\n0001 111r | ASRr   ");
        g_statFlag[C] = g_reg[ins.ins71.a].intVal & 0x0001 == 0x0001;
        g_reg[ins.ins71.a].intVal = g_reg[ins.ins71.a].intVal >> 1;
        set_NZ(g_reg[ins.ins71.a].intVal);
        break;
    case 0b0010000: // Rotate left r
        {
            print_debug("\n0010 000r | ROLr   ");
            bool newC = g_reg[ins.ins71.a].intVal < 0;
            g_reg[ins.ins71.a].indexVal = g_reg[ins.ins71.a].indexVal << 1;
            if (g_statFlag[C]) {
                g_reg[ins.ins71.a].indexVal += 0x0001;
            }
            g_statFlag[C] = newC;
        }
        break;
    case 0b0010001: // Rotate right r
        {
            print_debug("\n0010 001r | RORr   ");
            bool newC = g_reg[ins.ins71.a].intVal % 0b10 == 0b1;
            g_reg[ins.ins71.a].indexVal = g_reg[ins.ins71.a].indexVal >> 1;
            if (g_statFlag[C]) {
                g_reg[ins.ins71.a].indexVal += 0x8000;
            }
            g_statFlag[C] = newC;
        }
        break;
    }
    return 0;
}

int cmd5(instruction ins) {
    // instruction temp;           ********************* SET UP ADDRESSING MODE
    switch (ins.ins53.cmd)
    {
    case 0b00100: // Unary no operation trap
        print_debug("\nUnary no operation trap");
        break;
    case 0b00101: // Nonunary no operation trap
        print_debug("\nNounary no operation trap");
        break;
    case 0b00110: // Decimal input trap         N Z V                   
        print_debug("\n0011 0aaa | DECI   ");
        switch (ins.ins53.aaa) 
        {
        case 0b000:
            error("Invalid trap addressing mode.");
            return 1;
        case 0b001:
            set_aaa_mode(0, ins);
            break;
        }
        registers temp;
        temp.indexVal = get_int_from_inpStr();
        g_memory[ins.ins53.address] = temp.hilo.hi;
        g_memory[ins.ins53.address + 1] = temp.hilo.lo;
        break;
    case 0b00111: // Decimal output trap                           
        print_debug("\n0011 1aaa | DECO   ");
        switch (ins.ins53.aaa) 
        {
        case 0b000:
            error("Invalid trap addressing mode.");
            return 1;
        case 0b001:
            set_aaa_mode(0, ins);
            break;
        }
        temp.hilo.hi = g_memory[ins.ins53.address];
        temp.hilo.lo = g_memory[ins.ins53.address + 1];
        print_debug("\nDecimal output  [");
        cout << temp.intVal;
        print_debug("]");
        break;
    case 0b01000: // String output trap
        print_debug("\n0100 0aaa | STRO");
        print_debug("\nString output  [");
        switch (ins.ins53.aaa) 
        {
        case 0b000:
            error("Invalid trap addressing mode.");
            return 1;
        case 0b001:
            set_ins_address(ins);
            break;
        }
        while (g_memory[ins.ins53.address] != 0) {
            cout << (g_memory[ins.ins53.address++]);
        };
        print_debug("]");
        break;
    case 0b01001: // Character input
        print_debug("\n0100 1aaa | CHARI  ");
        switch (ins.ins53.aaa) 
        {
        case 0b000:
            error("Invalid trap addressing mode.");
            return 1;
        case 0b001:
            set_ins_address(ins);
            break;
        }
        g_memory[ins.ins8.address] = get_char_from_inpStr();
        break;
    case 0b01010: // Character output
        print_debug("\n0101 0aaa | CHARO [");
        set_aaa_mode(ins.ins53.aaa, ins);
        if (ins.ins53.aaa == 0) {
            // cout << to_string(ins.hiLoImediate.lo);
            cout << to_string(ins.opSpec.hilo.lo);
        } else {
            cout << g_memory[ins.ins8.address];
        }
        print_debug("]");
        break;
    case 0b01011: // Return from call with n local bytes
        print_debug("\n0101 1aaa | RETn   ");
        g_reg[sp].indexVal += ins.ins53.aaa;
        g_reg[pc].hilo.hi = g_memory[g_reg[sp].indexVal++];
        g_reg[pc].hilo.lo = g_memory[g_reg[sp].indexVal++];
        break;
    case 0b01100: // Add to stack pointer (SP)
        print_debug("\n0110 0aaa | ADDSP  ");
        g_reg[sp].indexVal = add_shorts(ins.opSpec.imedData, g_reg[sp].indexVal);
        break;
    case 0b01101: // Subtract from stack pointer (SP)
        print_debug("\n0110 1aaa | SUBSP  ");
        g_reg[sp].indexVal = add_shorts(ins.opSpec.imedData, twos_complement(g_reg[sp].indexVal));
        break;
    default:
        break;
    }
    return 0;
}

int cmd4(instruction ins) {
    registers temp;
    set_aaa_mode(ins.ins413.aaa, ins);
    if (ins.ins413.aaa == 0) {
        temp.indexVal = ins.opSpec.address;
    } else {
        temp.indexVal = g_memory[ins.opSpec.address] * 0x100 + g_memory[ins.opSpec.address + 1];
    }
    switch (ins.ins413.cmd)
    {
        case 0b0111: // ADD to r
            print_debug("\n0111 raaa | ADDr   ");
            g_reg[ins.ins413.r].intVal = add_shorts(g_reg[ins.ins413.r].intVal, temp.intVal);
            break;
        case 0b1000: // Subtract from r
            print_debug("\n1000 raaa | SUBr   ");
            g_reg[ins.ins413.r].intVal = add_shorts(twos_complement(temp.intVal), g_reg[ins.ins413.r].intVal);
            break;
        case 0b1001:  // Bitwise AND to r
            print_debug("\n1001 raaa | ANDr   ");
            g_reg[ins.ins413.r].intVal = g_reg[ins.ins413.r].intVal & temp.intVal;
            set_NZ(g_reg[ins.ins413.r].intVal);
            break;    
        case 0b1010:  // Bitwize OR to r            
            print_debug("\n1010 raaa | ORr    ");
            g_reg[ins.ins413.r].intVal = g_reg[ins.ins413.r].intVal | temp.intVal;
            set_NZ(g_reg[ins.ins413.r].intVal);
            break;
        case 0b1011: // Compare r                                                               XXXXXXXX           
            print_debug("\n1011 raaa | CPr    ");
            break;
        case 0b1100: // load r from memory          
            print_debug("\n1100 raaa | LDr    ");
            g_reg[ins.ins413.r].intVal = temp.intVal;
            set_NZ(g_reg[ins.ins413.r].intVal);
            break;
        case 0b1101: // load byte from memory       
            print_debug("\n1101 raaa | LDBYTEr");
            g_reg[ins.ins413.r].hilo.lo = temp.indexVal / 0x100;
            set_NZ(g_reg[ins.ins413.r].intVal);
            break;
        case 0b1110: // store r to memory                               
            print_debug("\n1110 raaa | STr    ");
            if (ins.ins413.aaa == 0) {
                error("Invalid Addressing Mode");
                return 1;
            }
            g_memory[ins.opSpec.address] = g_reg[ins.ins413.r].hilo.hi;
            g_memory[ins.opSpec.address + 1] = g_reg[ins.ins413.r].hilo.lo;
            break;
        case 0b1111: // store r byte to memory                          
            print_debug("\n1111 raaa | STBYTEr");
            if (ins.ins413.aaa == 0) {
                error("Invalid Addressing Mode");
                return 1;
            }
            g_memory[ins.opSpec.address] = g_reg[ins.ins413.r].hilo.lo;
            break;
    }
    return 0;
}

void error(string message) {
    cout << "\nERROR: " << message;
    the_end = true;
}

void br(int r) {
    switch (r)
    {
    case 0:
        {
            int address = g_reg[pc].indexVal;
            g_reg[pc].hilo.hi = g_memory[address++];
            g_reg[pc].hilo.lo = g_memory[address];
        }
        break;
    case 1:
        g_reg[pc].indexVal = g_reg[x].intVal;
        break;
    default:
        break;
    }
}

int ascii_char_hex_to_int(char ch) {
    int tmp;
    stringstream ss;
    ss << hex << ch;
    ss >> tmp;
    tmp -= 48;
    if (tmp > 9) {
        tmp -= 49;
    }
    return tmp;
}

string int_to_hex_ascii(unsigned int i, int numChars)
{
    stringstream s;
    s << uppercase << hex << internal << setfill('0');
    s << setw(numChars) << i;
    return s.str();
}

void set_ins_address(instruction &ins) {
    ins.opSpec.hilo.hi = g_memory[g_reg[pc].indexVal++];
    ins.opSpec.hilo.lo = g_memory[g_reg[pc].indexVal++];
}

void set_aaa_mode(int aaa, instruction &ins) {
    instruction temp;
    switch (aaa) {
    case 0x000: // imediate aaa mode
    case 0b001: // direct aaa mode
        set_ins_address(ins);
        break;
    case 0b010: // Indirect aaa mode
        set_aaa_mode(0b001, temp);
        ins.opSpec.hilo.hi = g_memory[temp.ins8.address++];
        ins.opSpec.hilo.lo = g_memory[temp.ins8.address];
        break;
    case 0b011: // Stack-relative
        set_aaa_mode(0b000, temp);
        ins.opSpec.hilo.hi = g_memory[g_reg[sp].indexVal + temp.ins8.address++];
        ins.opSpec.hilo.lo = g_memory[g_reg[sp].indexVal + temp.ins8.address];
        break;
    case 0b100: // Stack-relative deferred
        set_aaa_mode(0b011, temp);
        ins.opSpec.hilo.hi = g_memory[g_reg[sp].indexVal + temp.ins8.address++];
        ins.opSpec.hilo.lo = g_memory[g_reg[sp].indexVal + temp.ins8.address];
        break;
    case 0b101: // Indexed
        set_aaa_mode(0b000, temp);
        ins.opSpec.hilo.hi = g_memory[g_reg[x].indexVal + temp.ins8.address++];
        ins.opSpec.hilo.lo = g_memory[g_reg[x].indexVal + temp.ins8.address];
        break;
    case 0b110: // Stack-indexed
        set_aaa_mode(0b000, temp);
        ins.opSpec.hilo.hi = g_memory[g_reg[sp].indexVal + temp.ins8.address++ + g_reg[x].indexVal];
        ins.opSpec.hilo.lo = g_memory[g_reg[sp].indexVal + temp.ins8.address + g_reg[x].indexVal];
        break;
    case 0b111: // Stack-indexed deferred
        set_aaa_mode(0b110, temp);
        ins.opSpec.hilo.hi = g_memory[temp.ins8.address++];
        ins.opSpec.hilo.lo = g_memory[temp.ins8.address];
        break;
    }
}

short int asl(short int shortVal) {
    int intVal = (unsigned short int) shortVal;
    intVal  = intVal << 1;
    short int newShortVal = intVal % 0x10000;
    bool a = (intVal & 0x10000 == 0x10000);
    bool c = shortVal < 0;
    bool d = newShortVal < 0;
    g_statFlag[C] = a;
    g_statFlag[V] = c != d;
    set_NZ(newShortVal);
    return newShortVal;
}

short int add_shorts(short int short1, short int short2){
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
    set_NZ(shortSum);
    return shortSum;
}

void sp_push_word(registers data){
    g_memory[--g_reg[sp].indexVal] = data.hilo.lo;
    g_memory[--g_reg[sp].indexVal] = data.hilo.hi;
}

unsigned short int reg_indexed(registers temp, int offset) {
    registers index;
    index.hilo.hi = g_memory[temp.indexVal++];
    index.hilo.lo = g_memory[temp.indexVal];
    cout << endl << to_string(g_memory[index.indexVal + offset] * 0x100 + g_memory[index.indexVal + offset + 1]);
    cout << endl << to_string(g_memory[index.indexVal + offset + 1]);
    return g_memory[index.indexVal + offset] * 0x100 + g_memory[index.indexVal + offset + 1];
}

void set_NZ(short int short1) {
    g_statFlag[N] = short1 < 0;
    g_statFlag[Z] = short1 == 0;
}

short int twos_complement(short int short1) {
    short int retVal = ~short1 + 1;
    return retVal;
}

void print_debug(string str) {
    if (!debugMode) return;
    if (str.length() == 0) {
        str = "[debug] no string \n";
    }
    cout << str;
}

void dump_mem(string startEnd) {
    if (startEnd.length() == 0) return;
    cout << "\n\n\t******** memory dump *********\n";
    string temp = startEnd.substr(0, dumpMem.find(" "));
    int start = stoi(temp);
    if ((start % 8) != 0) {
        start = (start / 8) * 8;
    }
    temp = startEnd.substr(temp.length());
    int end = stoi(temp);
    if ((end % 8) != 0) {
        end = (((end / 8) + 1) * 8) - 1;
    }
    for (unsigned int i = start; i <= end; i++) {
        if ((i % 8) == 0) {
            cout << "\n\t" << int_to_hex_ascii(i, 4) << " |";
        }
        cout <<  " " << int_to_hex_ascii(g_memory[i],2);
    }
    cout << endl << endl;
}

void dump_regs() {
    if (silent) return;
    cout << "\t|| Registers:"
    << "  a = 0x" << int_to_hex_ascii(g_reg[a].indexVal, 4)
    << "  x = 0x" << int_to_hex_ascii(g_reg[x].indexVal, 4)
    << " sp = 0x" << int_to_hex_ascii(g_reg[sp].indexVal, 4)
    << " pc = 0x" << int_to_hex_ascii(g_reg[pc].indexVal, 4)
    << " | Status bits:"
    << " N = " << g_statFlag[N]
    << "  Z = " << g_statFlag[Z]
    << "  V = " << g_statFlag[V]
    << "  C = " << g_statFlag[C]
    << endl;
}

char get_char_from_inpStr() { 
    char retVal = g_inputStr.at(0);
    g_inputStr.erase(0, 1);
    return retVal;
}

unsigned short int get_int_from_inpStr() {
    int pos = g_inputStr.length();
    if (g_inputStr.find(' ') != std::string::npos) {
        pos = g_inputStr.find(' ');
    }
    string temp = g_inputStr.substr(0, pos);
    long long value = stoi(g_inputStr.substr(0, pos));
    if (g_inputStr.length() == pos) {
        g_inputStr = "";
    } else {
        g_inputStr = g_inputStr.substr(pos);
    }
    g_statFlag[V] = (value > 0x10000);
    g_statFlag[N] = (value & 0x8000) == 0x8000;
    return value % 0x10000;
}

void help() {
    cout << "\npep8"
    << "\nWilliam Morgan"
    << "\nUIS wmorga4"
    << "\nSP22 CSC376"
    << "\nMid-term Project"
    << "\n\n\n"
    << "Usage:"
    << "\n\npep8.exe [args]"
    << "\n     Args:"
    << "\n        -cf   Command file flag.  -cf followed by file name.  REQUIRED"
    << "\n        -if   Input file flag.  -if followed by file name.  Optional"
    << "\n        -i    Input flag. -i followed by a input string.  Optional"
    << "\n        -d    Turn on debug mode.  Provides more debugging output. Optional. If omitted, debugging mode is off"
    << "\n        -s    Turn on silent mode.  Turn off Register/Status dump after each command. If omitted, silent mode is off"
    << "\n        -dm   Dump memory flag.  -dm followed by a string with memory start address and end address separated by a space. Optional. If included, the memory from start to end will be dumped to screen when the program terminates."
    << "\n\n Example:"
    << "\nC:\\pep8> ./pep8.exe -cf cmds.pepo -d -i \"ABCD1234\" -dm \"25 50\"";
}