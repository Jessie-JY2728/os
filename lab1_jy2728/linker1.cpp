#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cstdio>
#include <vector>
using namespace std;

//string IAER = "IAER";
int linenum = 0;    //line number
int place = 1;  //offset in line
vector<int> modulelen;  //store length of each module
vector<int> bases;  // base-lengths for each module
//vector<int> ext_abs;    // abs addr that has appeared in E instr

int len = 0;    //length of the line s
int prev_len = 0;
string s;   //for getline from file
ifstream input; //input stream of the file
char* arr;  //something for getToken
int dif = 0;    //how many \n's are there at end
int flag = 1;  //flag, goes 1 if getToken gets null
bool synerr = false;    // true if there is syntax error

struct Symbol {
    string name;
    int value;
    int modu;
    int rep;
};

struct use_sym {
    string name;
    bool used;
};

vector<Symbol> deflist; //store all symbols in deflist
vector<use_sym> uselist; //store all symbols in uselist


int getToken(char** rest, char** prev, char** token)
{
    *token = strtok_r(*rest, "\\\t ", rest);
    if (*token == NULL)
        return 1;
    if (*prev == 0)
        place = 1 + *token - arr;
    else
        place += *token - *prev;
    *prev = *token;
    //cout<<linenum <<", "<<place<<": "<<(*token)<<endl;
    return 0;
}

// when getToken gets NULL, fetch new line until not empty
int linecoming(char** rest, char** prev, char** token) {
    if (arr != NULL) {
        //place = strlen(arr);
        delete[] arr;
    }
    while (true) {
        if (input.eof()) {
            return 1;   // eof
        }
        getline(input, s);
        prev_len = len;
        len = s.length();
        linenum++;
        if (len == 0) {
            dif++;
            continue;
        } else {
            break;
        }
    }
    // reset poiners
    dif = 0;
    arr = new char[len + 1];
    strcpy(arr, s.c_str());
    *rest = arr;
    *prev = 0;
    *token = 0;
    return 0;
}

int readInt(char** rest, char** prev, char** token)
{
    flag = getToken(rest, prev, token);
    if (flag == 1) {
        int eof = linecoming(rest, prev, token);
        if (eof) {
            //end_of = true;
            return -2;
        }
        flag = getToken(rest, prev, token);
    }
    int l = strlen(*token);
    for (int j = 0; j < l; j++) {
        if (!isdigit((*token)[j]))
            return -1;  // -1 for not a digit
    }
    int count = atoi(*token);
    return count;
}

char* readSymbol(char** rest, char** prev, char** token)
{
    flag = getToken(rest, prev, token);
    if (flag == 1) {
        int eof = linecoming(rest, prev, token);
        if (eof) {
            //end_of = true;
            return (char*) 0;
        }
        flag = getToken(rest, prev, token);
    }
    //cout<<"0: "<< *token[0]<<endl;
    if (!isalpha((*token)[0])) {
        // bad token name, not a valid symbol
        return (char*) 0;
    }
    int l = strlen(*token);
    for (int j = 1; j < l; j++) {
        //cout<<"j: "<< j <<" " << (*token)[j]<<endl;
        if (!isalnum((*token)[j]))
            return (char*) 0;
    }
    
    return *token;
}

char readIAER(char** rest, char** prev, char** token)
{
    flag = getToken(rest, prev, token);
    if (flag == 1) {
        int eof = linecoming(rest, prev, token);
        if (eof) {
            //end_of = true;
            return ' ';
        }
        flag = getToken(rest, prev, token);
    }
    int l = strlen(*token);
    if (l != 1) {
        // token is not 1 char of alphabet
        return ' ';
    }
    
    if ((**token != 'I') && (**token != 'A') && (**token != 'E') && (**token != 'R')) {
        // token is 1 char but not one of IAER
        return ' ';
    }
    return (*token)[0];
}

    
/* this method prints parser errors */
//TODO: deal with linenum and place when EOF
void __parseerror(int errcode) {
    static string errstr[] = {
        "NUM_EXPECTED", // 0
        "SYM_EXPECTED", // 1
        "ADDR_EXPECTED",    // 2
        "SYM_TOO_LONG", // 3
        "TOO_MANY_DEF_IN_MODULE",   //4
        "TOO_MANY_USE_IN_MODULE",   //5
        "TOO_MANY_INSTR"    //6
    };
    if (len == 0) {
        linenum --;
        place = dif > 1 ? 1 : prev_len + 1;
    }
    //cout<<"Parse Error line "<< linenum<<" offset "<<place<<": "<<errstr[errcode]<<endl;
    printf("Parse Error line %d offset %d: %s\n", linenum, place, errstr[errcode].c_str());
    synerr = true;
}

void printsymtable() {
    int n = deflist.size();
    int m = modulelen.size();
    int prev_modu = 0;
    // check for warning of def val too big
    for (int i = 0; i < n; i++) {
        Symbol sym = deflist[i];
        int maxrel = modulelen[sym.modu] - 1;
        if (sym.value > maxrel) {
            //cout<<"Warning: Module "<< sym.modu + 1 <<": "<< sym.name 
             //   << " too big "<< sym.value << " (max=" << maxrel
             //   << ") assume zero relative"<< endl;
            printf("Warning: Module %d: %s too big %d (max=%d) assume zero relative\n", sym.modu+1, sym.name.c_str(), sym.value, maxrel);
            deflist[i].value = 0;
        }
	//if (sym.rep > 0) {
	    //cout<< "Warning: Module "<< sym.modu + 1<<": "
         //       << sym.name << " redefined and ignored"<<endl;
         //   printf("Warning: Module %d: %s redefined and ignored\n", sym.rep, sym.name.c_str());
        //}
    }
    bases.reserve(m);
    bases[0] = 0;
    for (int k = 1; k < m; k++) {
        bases[k] = bases[k-1] + modulelen[k-1];
    }
    // print it
    cout << "Symbol Table"<<endl;
    for (int i = 0; i < n; i++) {
        //Symbol sym = deflist[i];
        deflist[i].value = bases[deflist[i].modu] + deflist[i].value;
        if (deflist[i].rep > 0) {
            //cout << deflist[i].name << "="<< deflist[i].value
              //  << " Error: This variable is multiple times defined; first value used"
              //  << endl;
            printf("%s=%d Error: This variable is multiple times defined; first value used\n", deflist[i].name.c_str(), deflist[i].value);
        }
        else {
            //cout<< deflist[i].name << "="<< deflist[i].value << endl;
            printf("%s=%d\n", deflist[i].name.c_str(), deflist[i].value);
        }
    }
    printf("\nMemory Map\n");
}
    
void Pass1()
{
    modulelen.reserve(512);
    deflist.reserve(256);
    char* rest = 0;
    char* prev = 0;
    char* token = 0;
    int mod = 0;
    int sum_inscount = 0;
    
    while (!input.eof())
    {
        /* parsing deflist*/
        int defcount = readInt(&rest, &prev, &token);
        if (defcount == -2) {   // -2 for no more defs
            break;
        }
        if (defcount == -1) {
            __parseerror(0);
            return;
        }
        if (defcount > 16) {
            __parseerror(4);
            return;
        }
        
        //cout<<"defcount: "<< defcount<<endl;
        // TODO: check all integers, if exceed limit
        for (int j = 0; j < defcount; j++) {
            char* tok = readSymbol(&rest, &prev, &token);
            if (tok == NULL) {
                __parseerror(1);
                return;
            }
            /* check symbol name too long */
            if (strlen(tok) > 16) {
                __parseerror(3);
                return;
            }
            string stok(tok);
            
            //cout << "Symbol: "<< s <<"  ";
            int val = readInt(&rest, &prev, &token);
            if (val == -1) {
                __parseerror(0);
                return;
            }
            //cout<< "Value: "<<val<<endl;
            int n = deflist.size();
            bool isrepeat = false;
            for (int k = 0; k < n; k++) {
                if (deflist[k].name.compare(stok) == 0) {
                    deflist[k].rep = mod + 1;   // place for rep
                    isrepeat = true;
                    break;
                }
            }
            // never add repeated symbol
            if (isrepeat) {
                printf("Warning: Module %d: %s redefined and ignored\n", mod+1, stok.c_str());
                continue;
            }
            Symbol sym;
            sym.name = stok;
            sym.value = val;
            sym.rep = -1;
            sym.modu = mod;
            deflist.push_back(sym);
        }
        
        /* parsing uselist */
        int usecount = readInt(&rest, &prev, &token);
        if (usecount == -1) {
            __parseerror(0);
            return;
        }
        if (usecount > 16) {
            __parseerror(5);
            return;
        }
        
        //cout<<"usecount: "<<usecount<<endl;
        for (int j = 0; j < usecount; j++) {
            // TODO: nothing done here?
            char* tok = readSymbol(&rest, &prev, &token);
            if (tok == NULL) {
                __parseerror(1);
                return;
            }
            if (strlen(tok) >16) {
                __parseerror(3);
                return;
            }
            //cout<<"symbol: "<< tok<<endl;
        }
        
        /* parsing instr*/
        int inscount = readInt(&rest, &prev, &token);
        modulelen.push_back(inscount);
        sum_inscount += inscount;
        if (inscount == -1) {
            __parseerror(0);
            return;
        }
        if (sum_inscount > 512) {
            __parseerror(6);
            return;
        }
        for (int j = 0; j < inscount; j++) {
            char addrmode = readIAER(&rest, &prev, &token);
            if (addrmode == ' ') {
                __parseerror(2);
                return;
            }
            int oprand = readInt(&rest, &prev, &token);
            if (oprand == -1) {
                __parseerror(0);
                return;
            }
        }
        mod++;
    }
    modulelen.shrink_to_fit();
    deflist.shrink_to_fit();
    printsymtable();
    
    int n = deflist.size();
    for (int j = 0; j < n; j++) {
        // from here on, rep means if this has been used
        deflist[j].rep = -1;
    }
}

void checkdeflist() {
    printf("\n");
    int defsize = deflist.size();
    for (int i = 0; i < defsize; i++) {
        if (deflist[i].rep != 0)
            printf("Warning: Module %d: %s was defined but never used\n", deflist[i].modu + 1, deflist[i].name.c_str());
    }
    printf("\n");
}

void Pass2() {
    char* rest = 0;
    char* prev = 0;
    char* token = 0;
    int mod = 0;
    int defsize = deflist.size();
    
    while (!input.eof()) {
        int defcount = readInt(&rest, &prev, &token);
        if (defcount == -2)
            break;
        for (int j = 0; j < defcount; j++) {
            char* tok = readSymbol(&rest, &prev, &token);
            int val = readInt(&rest, &prev, &token);
        }
        
        int usecount = readInt(&rest, &prev, &token);
        uselist.reserve(usecount);
        for (int j = 0; j < usecount; j++) {
            char* tok = readSymbol(&rest, &prev, &token);
            string stok(tok);
            use_sym usym;
            usym.name = stok;
            usym.used = false;
            uselist.push_back(usym);
        }
        
        int inscount = readInt(&rest, &prev, &token);
        for (int j = 0; j < inscount; j++) {
            char addrmode = readIAER(&rest, &prev, &token);
            int oprand = readInt(&rest, &prev, &token);
            printf("%03d: ", j + bases[mod]);
            if (addrmode == 'I') {
                if (oprand >= 10000)
                    printf("9999 Error: Illegal immediate value; treated as 9999\n");
                else
                    printf("%04d\n", oprand);
            } else if (addrmode == 'A') {
                if (oprand >= 10000)
                    printf("9999 Error: Illegal opcode; treated as 9999\n");
                else if (oprand % 1000 >= 512)
                    printf("%04d Error: Absolute address exceeds machine size; zero used\n", oprand - (oprand % 1000));
                else
                    printf("%04d\n", oprand);
            } else if (addrmode == 'R') {
                if (oprand >= 10000)
                    printf("9999 Error: Illegal opcode; treated as 9999\n");
                else if (oprand % 1000 >= inscount)
                    printf("%04d Error: Relative address exceeds module size; zero used\n", oprand - (oprand % 1000) + bases[mod]);
                else
                    printf("%04d\n", oprand + bases[mod]);
            } else { // 'E'
                int index = oprand % 1000;
                if (index >= usecount)
                    printf("%04d Error: External address exceeds length of uselist; treated as immediate\n", oprand);
                else  {
                    string used = uselist[index].name;
                    int index_abs = -1;
                    // look for value of this external reference
                    uselist[index].used = true;
                    for (int i = 0; i < defsize; i++) {
                        if (deflist[i].name.compare(used) == 0) {
                            deflist[i].rep = 0;
                            index_abs = deflist[i].value;
                        }
                    }
        
                    if (index_abs == -1)
                        printf("%04d Error: %s is not defined; zero used\n", oprand - index, used.c_str());
                    else
                        printf("%04d\n", oprand - index + index_abs);
                }
            }
        }
        // print warning for in uselist but not used
        for (int j = 0; j < usecount; j++) {
            if (!uselist[j].used)
                printf("Warning: Module %d: %s appeared in the uselist but was not actually used\n", mod+1, uselist[j].name.c_str());
        }
        uselist.clear();
        mod++;
    }
    checkdeflist();
}


int main(int argc, char* argv[]) {
    if (argc != 2)
        return 1;
    char* filename = argv[1];
    input.open(filename);
    if (!input.is_open())
        return 1;
    if (input.eof())
        return 1;
    Pass1();
    if (synerr)
        return 0;
    input.close();
    input.clear();
    input.open(filename);
    arr = 0;
    Pass2();
    return 0;
}

