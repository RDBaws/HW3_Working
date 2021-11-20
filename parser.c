/*By Robert Davis
 * This is a recursive descent parser for pl/0 made for Systems Software, UCF, Fall 21
 * */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "compiler.h"

#define MAX_CODE_LENGTH 1000
#define MAX_SYMBOL_COUNT 100

instruction *code;
int cIndex;
symbol *table;
int tIndex;
int unmarked = 0;
int marked = 1;

void emit(int opname, int level, int mvalue);
void addToSymbolTable(int k, char n[], int v, int l, int a, int m);
void printparseerror(int err_code);
int multipleDeclarationCheck(lexeme token);
int findSym(lexeme token, int kind);
void printsymboltable();
void printassemblycode();

//Variables for the symbol table
symbol symbolTable[MAX_SYMBOL_COUNT];
//This is the lex level
int lexLevel;
//Indicate if theres an error
int error = 0;
//Indicate where we are in the lexeme table
int lexPosition;
//Hold the index of the symbol being searched for
int symIdx;
int jpcIdx;
//Hold the current token
lexeme token;
//Make the lexeme list global
lexeme* globalLex;
//Returns the next token
lexeme getNextToken();

//Function Stubs for Required Functions
void program();
void block();
void constDeclaration();
int varDeclaration();
void procedureDeclaration();
void statement();
void mark();
void condition();
void expression();
void term();
void factor();


instruction *parse(lexeme *list, int printTable, int printCode)
{
    globalLex = list;
    code = NULL;
    tIndex = 0;
    cIndex = 0;
    jpcIdx = 0;
    lexPosition = 0;
    token = globalLex[lexPosition];

    table = (symbol*) malloc(MAX_SYMBOL_COUNT * sizeof (symbol));
    code = (instruction*) malloc(MAX_CODE_LENGTH * sizeof(instruction));

    program();


    if(printTable == 1)
        printsymboltable();
    if(printCode == 1)
        printassemblycode();


    /* this line is EXTREMELY IMPORTANT, you MUST uncomment it
        when you test your code otherwise IT WILL SEGFAULT in
        vm.o THIS LINE IS HOW THE VM KNOWS WHERE THE CODE ENDS
        WHEN COPYING IT TO THE PAS
    code[cIndex].opcode = -1;
    */

    code[cIndex].opcode = -1;

    return code;
}

lexeme getNextToken(){
    lexPosition++;
    return globalLex[lexPosition];
}

void program(){
    emit(7, 0, 0);
    addToSymbolTable(3, "main", 0, 0, 0, unmarked);
    lexLevel = -1;
    block();
    if(token.type != periodsym){
        error = 1;
        printparseerror(error);
        exit(error);
    }
    emit(9, 0, 3);

    for(int line = 0; line < cIndex; line++){
        if(code[line].opcode == 5){
            code[line].m = table[code[line].m].addr;
        }
    }
    code[0].m = table[0].addr;
}

void block(){

    lexLevel++;
    int procedure_idx = tIndex - 1;
    constDeclaration();
    int x = varDeclaration();
    procedureDeclaration();
    table[procedure_idx].addr = cIndex*3;
    if(lexLevel == 0)
        emit(6, 0, x);
    else
        emit(6, 0, x + 3);
    statement();
    mark();
    lexLevel--;
}

void constDeclaration(){

    //Index of symbol in symbol table
    if(token.type == constsym){
        do {
            token = getNextToken();
            if (token.type != identsym) {
                error = 2;
                printparseerror(error);
                exit(error);
            }

            symIdx = multipleDeclarationCheck(token);
            if (symIdx != -1) {
                error = 19;
                printparseerror(19);
                exit(error);
            }

            char *identName;
            strcpy(identName, token.name);
            token = getNextToken();
            if (token.type != assignsym) {
                error = 5;
                printparseerror(5);
                exit(error);
            }

            token = getNextToken();
            if (token.type != numbersym) {
                error = 2;
                printparseerror(2);
                exit(error);
            }

            addToSymbolTable(1, identName, token.value, lexLevel, 0, unmarked);
            token = getNextToken();
        }while(token.type == commasym);

        if(token.type != semicolonsym){
            if(token.type == identsym) {
                error = 13;
                printparseerror(error);
                exit(error);
            }else {
                error = 14;
                printparseerror(error);
                exit(error);
            }
        }

        token = getNextToken();
    }
}

int varDeclaration(){

    int numVars = 0;
    if(token.type == varsym){
        do{
            numVars++;
            token = getNextToken();

            if(token.type != identsym){
                error = 3;
                printparseerror(3);
                exit(error);
            }
            symIdx = multipleDeclarationCheck(token);
            if(symIdx != -1){
                error = 18;
                printparseerror(error);
                exit(error);
            }
            if(lexLevel == 0){
                addToSymbolTable(2, token.name,0, lexLevel, numVars-1, unmarked);
            } else{
                addToSymbolTable(2, token.name, 0, lexLevel, numVars+2, unmarked);
            }
            token = getNextToken();
        }while(token.type == commasym);

        if(token.type != semicolonsym){
            if(token.type == identsym){
                error = 18;
                printparseerror(error);
                exit(error);
            }else{
                error = 14;
                printparseerror(error);
                exit(error);
            }
        }
        token = getNextToken();
    }
    return numVars;
}

void procedureDeclaration(){

    while(token.type == procsym){
        token = getNextToken();
        if(token.type != identsym){
            error = 7;
            printparseerror(error);
            exit(error);
        }

        symIdx = multipleDeclarationCheck(token);
        if(symIdx != -1){

            error = 18;
            printparseerror(error);
            exit(error);
        }
        addToSymbolTable(3, token.name, 0, lexLevel, 0, unmarked);
        token = getNextToken();
        if(token.type != semicolonsym){
            error = 4;
            printparseerror(error);
            exit(error);
        }
        token = getNextToken();
        block();
        if(token.type != semicolonsym){
            error = 4;
            printparseerror(error);
            exit(error);
        }
        token = getNextToken();
        emit(2, 0, 0);
    }
}

void statement(){
    if(token.type == identsym) {
        symIdx = findSym(token, 2);
        if (symIdx == -1) {
            if (findSym(token, 1) != findSym(token, 3)) {
                error = 16;
                printparseerror(error);
                exit(error);
            } else {
                error = 19;
                printparseerror(error);
                exit(error);
            }
        }

        token = getNextToken();
        if (token.type != assignsym) {
            error = 5;
            printparseerror(error);
            exit(error);
        }
        token = getNextToken();
        expression();
        emit(4, lexLevel - table[symIdx].level, table[symIdx].addr);
        return;
    }

    if(token.type == beginsym){
        do{
            token = getNextToken();
            statement();
        }while(token.type == semicolonsym);

        if(token.type != endsym){
            if(token.type == identsym || token.type == beginsym || token.type == ifsym || token.type == whilesym || token.type == readsym || token.type == writesym || token.type == callsym){
                error = 15;
                printparseerror(error);
                exit(error);
            }else{

                error = 16;
                printparseerror(error);
                exit(error);
            }
        }

        token = getNextToken();
        return;
    }

    if(token.type== ifsym){
        token = getNextToken();
        condition();
        jpcIdx = cIndex;

        emit(8, 0, jpcIdx);
        if(token.type != thensym){
            error = 8;
            printparseerror(error);
            exit(error);
        }
        token = getNextToken();
        statement();
        if(token.type == elsesym){
            int jmpIdx = cIndex;
            emit(7, 0, token.value);
            code[jpcIdx].m = cIndex * 3;
            //todo Remember why you added this
            token = getNextToken();
            statement();
            code[jmpIdx].m = cIndex * 3;
        }else{
            code[jpcIdx].m = cIndex * 3;
        }
        return;
    }

    if(token.type == whilesym){
        token = getNextToken();
        int loopIdx = cIndex;
        condition();
        if(token.type != dosym){
            error = 9;
            printparseerror(error);
            exit(error);
        }
        token = getNextToken();
        jpcIdx = cIndex;
        emit(8, 0, token.value);
        statement();
        emit(7, 0, loopIdx*3);
        code[jpcIdx].m = cIndex * 3;
        return;
    }

    if(token.type == readsym){
        token = getNextToken();
        if(token.type != identsym){
            error = 6;
            printparseerror(error);
            exit(error);
        }
        symIdx = findSym(token, 2);
        if(symIdx == -1){
            if(findSym(token, 1) != findSym(token, 3)){
                error = 6;
                printparseerror(error);
                exit(error);
            }else{
                error = 19;
                printparseerror(error);
                exit(error);
            }
        }
        token = getNextToken();

        //Read
        emit(9, 0, 2);
        //STO
        emit(4, lexLevel - table[symIdx].level, table[symIdx].addr);
        return;
    }

    if(token.type == writesym){

        token = getNextToken();
        expression();
        //Write
        emit(9, 0, 1);
        return;
    }

    if(token.type == callsym){
        token = getNextToken();
        symIdx = findSym(token, 3);
        if(symIdx == -1){
            if(findSym(token, 1) != findSym(token, 2)){
                error = 6;
                printparseerror(error);
                exit(error);
            }else{
                error = 19;
                printparseerror(error);
                exit(error);
            }
        }
        token = getNextToken();
        //Cal

        emit(5, lexLevel - table[symIdx].level, symIdx);
    }
}


void condition(){
    if(token.type == oddsym){
        token = getNextToken();
        expression();
        //Odd
        emit(2, 0, 6);
    }

    else{
        expression();
        if(token.type == eqlsym){
            token = getNextToken();
            expression();
            //eql
            emit(2, 0, 8);
        }

        else if(token.type == neqsym){
            token = getNextToken();
            expression();
            //neq
            emit(2, 0, 9);
        }

        else if(token.type == lsssym){
            token = getNextToken();
            expression();
            //lss
            emit(2, 0, 10);
        }

        else if(token.type == leqsym){
            token = getNextToken();
            expression();
            //leq
            emit(2, 0, 11);
        }

        else if(token.type == gtrsym){
            token = getNextToken();
            expression();
            //gtr
            emit(2, 0, 12);
        }

        else if(token.type == geqsym){
            token = getNextToken();
            expression();
            //geq
            emit(2, 0, 13);
        }

        else{
            error = 10;
            printparseerror(error);
            exit(error);
        }
    }
}

void expression(){

    if(token.type == subsym){
        token = getNextToken();
        term();
        //neg
        emit(2, 0, 1);
        while(token.type == addsym || token.type == subsym){
            if(token.type == addsym){
                token = getNextToken();
                term();
                //add
                emit(2, 0, 2);
            }else{
                token = getNextToken();
                term();
                //sub
                emit(2, 0, 3);
            }
        }
    }

    else{
        if(token.type == addsym){
            token = getNextToken();
        }
        term();
        while(token.type == addsym || token.type == subsym){
            if(token.type == addsym){
                token = getNextToken();
                term();
                //add
                emit(2, 0, 2);
            } else{
                token = getNextToken();
                term();
                //sub
                emit(2, 0, 3);
            }
        }
    }

    if(token.type == identsym || token.type == numbersym || token.type == oddsym || token.type == lparensym){
        error = 17;
        printparseerror(error);
        exit(error);
    }
}

void term(){

    factor();
    while(token.type == multsym || token.type == divsym || token.type == modsym){
        if(token.type == multsym){
            token = getNextToken();
            factor();
            //mult
            emit(2, 0, 4);
        }

        else if(token.type == divsym){
            token = getNextToken();
            factor();
            //Div
            emit(2, 0, 5);
        }

        else{
            token = getNextToken();
            factor();
            //Mod
            emit(2, 0, 7);
        }
    }
}

void factor(){

    if(token.type == identsym){
        int symIdx_var = findSym(token, 2);
        int symIdx_const = findSym(token, 1);

        if(symIdx_var == -1 && symIdx_const == -1){
            if(findSym(token, 3) != -1){
                error = 11;
                printparseerror(error);
                exit(error);
            }else{
                error = 19;
                printparseerror(error);
                exit(error);
            }
        }

        if(symIdx_var == -1)
            emit(1, 0, table[symIdx_const].val);
        else if(symIdx_const == -1 || table[symIdx_var].level > table[symIdx_const].level)
            emit(3, lexLevel - table[symIdx_var].level, table[symIdx_var].addr);
        else
            emit(1, 0, table[symIdx_const].val);
        token = getNextToken();
    }else if(token.type == numbersym){

        //Lit
        emit(1,0,token.value);
        token = getNextToken();
    }else if(token.type == lparensym){
        token = getNextToken();
        expression();
        if(token.type != rparensym){
            error = 12;
            printparseerror(error);
            exit(error);
        }
        token = getNextToken();
    }else{
        error = 11;
        printparseerror(error);
        exit(error);
    }
}

int multipleDeclarationCheck(lexeme tok){

    int seekIndex = tIndex;
    while(seekIndex != 0){
        if(strcmp(table[seekIndex].name, tok.name) == 0){
            if(table[seekIndex].level == lexLevel){

                return seekIndex;
            }

        }
        else
            seekIndex--;
    }
    return -1;
}


int findSym(lexeme tok, int kind){

    for(int i = 0; i <= tIndex; i++) {
        if (strcmp(table[i].name, tok.name) == 0 && table[i].kind == kind) {
            return i;
        }
    }
    return -1;
}

void mark(){
    int tableIndx = tIndex;
    int scanning = 1;

    while(tableIndx >= 0 && scanning == 1){
        if(table[tableIndx].mark == unmarked && table[tableIndx].level == lexLevel){
            table[tableIndx].mark = marked;
        }else if(table[tableIndx].mark == unmarked && table[tableIndx].level < lexLevel)
            scanning = 0;

        tableIndx--;
    }

}




//Code Given in Template

void emit(int opname, int level, int mvalue)
{
    code[cIndex].opcode = opname;
    code[cIndex].l = level;
    code[cIndex].m = mvalue;
    cIndex++;
}

void addToSymbolTable(int k, char n[], int v, int l, int a, int m)
{
    table[tIndex].kind = k;
    strcpy(table[tIndex].name, n);
    table[tIndex].val = v;
    table[tIndex].level = l;
    table[tIndex].addr = a;
    table[tIndex].mark = m;
    tIndex++;
}


void printparseerror(int err_code)
{
    switch (err_code)
    {
        case 1:
            printf("Parser Error: Program must be closed by a period\n");
            break;
        case 2:
            printf("Parser Error: Constant declarations should follow the pattern 'ident := number {, ident := number}'\n");
            break;
        case 3:
            printf("Parser Error: Variable declarations should follow the pattern 'ident {, ident}'\n");
            break;
        case 4:
            printf("Parser Error: Procedure declarations should follow the pattern 'ident ;'\n");
            break;
        case 5:
            printf("Parser Error: Variables must be assigned using :=\n");
            break;
        case 6:
            printf("Parser Error: Only variables may be assigned to or read\n");
            break;
        case 7:
            printf("Parser Error: call must be followed by a procedure identifier\n");
            break;
        case 8:
            printf("Parser Error: if must be followed by then\n");
            break;
        case 9:
            printf("Parser Error: while must be followed by do\n");
            break;
        case 10:
            printf("Parser Error: Relational operator missing from condition\n");
            break;
        case 11:
            printf("Parser Error: Arithmetic expressions may only contain arithmetic operators, numbers, parentheses, constants, and variables\n");
            break;
        case 12:
            printf("Parser Error: ( must be followed by )\n");
            break;
        case 13:
            printf("Parser Error: Multiple symbols in variable and constant declarations must be separated by commas\n");
            break;
        case 14:
            printf("Parser Error: Symbol declarations should close with a semicolon\n");
            break;
        case 15:
            printf("Parser Error: Statements within begin-end must be separated by a semicolon\n");
            break;
        case 16:
            printf("Parser Error: begin must be followed by end\n");
            break;
        case 17:
            printf("Parser Error: Bad arithmetic\n");
            break;
        case 18:
            printf("Parser Error: Confliciting symbol declarations\n");
            break;
        case 19:
            printf("Parser Error: Undeclared identifier\n");
            break;
        default:
            printf("Implementation Error: unrecognized error code\n");
            break;
    }

//    free(code);
//    free(table);
}

void printsymboltable()
{
    int i;
    printf("Symbol Table:\n");
    printf("Kind | Name        | Value | Level | Address | Mark\n");
    printf("---------------------------------------------------\n");
    for (i = 0; i < tIndex; i++)
        printf("%4d | %11s | %5d | %5d | %5d | %5d\n", table[i].kind, table[i].name, table[i].val, table[i].level, table[i].addr, table[i].mark);

    free(table);
    table = NULL;
}

void printassemblycode()
{
    int i;
    printf("Line\tOP Code\tOP Name\tL\tM\n");
    for (i = 0; i < cIndex; i++)
    {
        printf("%d\t", i);
        printf("%d\t", code[i].opcode);
        switch (code[i].opcode)
        {
            case 1:
                printf("LIT\t");
                break;
            case 2:
                switch (code[i].m)
                {
                    case 0:
                        printf("RTN\t");
                        break;
                    case 1:
                        printf("NEG\t");
                        break;
                    case 2:
                        printf("ADD\t");
                        break;
                    case 3:
                        printf("SUB\t");
                        break;
                    case 4:
                        printf("MUL\t");
                        break;
                    case 5:
                        printf("DIV\t");
                        break;
                    case 6:
                        printf("ODD\t");
                        break;
                    case 7:
                        printf("MOD\t");
                        break;
                    case 8:
                        printf("EQL\t");
                        break;
                    case 9:
                        printf("NEQ\t");
                        break;
                    case 10:
                        printf("LSS\t");
                        break;
                    case 11:
                        printf("LEQ\t");
                        break;
                    case 12:
                        printf("GTR\t");
                        break;
                    case 13:
                        printf("GEQ\t");
                        break;
                    default:
                        printf("err\t");
                        break;
                }
                break;
            case 3:
                printf("LOD\t");
                break;
            case 4:
                printf("STO\t");
                break;
            case 5:
                printf("CAL\t");
                break;
            case 6:
                printf("INC\t");
                break;
            case 7:
                printf("JMP\t");
                break;
            case 8:
                printf("JPC\t");
                break;
            case 9:
                switch (code[i].m)
                {
                    case 1:
                        printf("WRT\t");
                        break;
                    case 2:
                        printf("RED\t");
                        break;
                    case 3:
                        printf("HAL\t");
                        break;
                    default:
                        printf("err\t");
                        break;
                }
                break;
            default:
                printf("err\t");
                break;
        }
        printf("%d\t%d\n", code[i].l, code[i].m);
    }
    if (table != NULL)
        free(table);
}