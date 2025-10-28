TOKENS.TXT follows format:
29 
2 x 
16 
2 y 
17 
20 
22 
2 y 
9 
2 x 
24 
31 
2 y 
23 
17 
31 
2 x 
4 
3 1 
17 
21 
18





































/*
Assignment:
HW3 - Parser and Code Generator for PL/0


Author: Jarielys Cruz Gomez, Daniel Rangosch Montero


Language: C (only)


To Compile:
   Scanner:
       gcc -O2 -std=c11 -o lex lex.c
   Parser/Code Generator:
       gcc -O2 -std=c11 -o parsercodegen parsercodegen.c


To Execute (on Eustis):
   ./lex <input_file.txt>
   ./parsercodegen


where:
   <input_file.txt> is the path to the PL/0 source program


Notes:
   - lex.c accepts ONE command-line argument (input PL/0 source file)
   - parsercodegen.c accepts NO command-line arguments
   - Input filename is hard-coded in parsercodegen.c
   - Implements recursive-descent parser for PL/0 grammar
   - Generates PM/0 assembly code (see Appendix A for ISA)
   - All development and testing performed on Eustis


Class: COP 3402 - System Software - Fall 2025


Instructor: Dr. Jie Lin


Due Date: Friday, October 31, 2025 at 11:59 PM ET
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define MAX_SYMBOL_TABLE_SIZE 500
#define MAX_STR_LEN 12


typedef enum {
   skipsym = 1,    // skip token
   identsym,       // identifier
   numbersym,      // number
   plussym,        // +
   minussym,       // -
   multsym,        // *
   slashsym,       // /
   eqsym,          // =
   neqsym,         // <>
   lessym,         // <
   leqsym,         // <=
   gtrsym,         // >
   geqsym,         // >=
   lparentsym,     // (
   rparentsym,     // )
   commasym,       // ,
   semicolonsym,   // ;
   periodsym,      // .
   becomessym,     // :=
   beginsym,       // being
   endsym,         // end
   ifsym,          // if
   fisym,          // fi
   thensym,        // then
   whilesym,       // while
   dosym,          // do
   callsym,        // call
   constsym,       // const
   varsym,         // var
   procsym,        // procedure
   writesym,       // write
   readsym,        // read
   elsesym,        // else
   evensym         // even
} token_type;
//Assembly Code Line Op L M
typedef struct {
   char OP_s[5];   // human-readable opcode format
   int OP;         // opcode
   int L;          // lexicographical level
   int M;          // m field
} instruction_register;
//Symbol table
typedef struct {
   int kind;       // const = 1, var = 2, proc = 3
   char name[12];  // name up to 11 chars
   int val;        // number (ASCII value)
   int level;      // L level
   int addr;       // M address
   int mark;       // to indicate unavailable or deleted
} symbol;


int symbolTableCheck(char id[], symbol table[], int size);
void initializeProgram(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code);
void block(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code);
void constantDeclaration(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code);
int variableDeclaration(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, FILE * assembly_code);
void statement(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code);
void condition(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code);
void expression(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code);
void term(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code);
void factor(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code);


int main() {


   FILE * tokens_file = fopen("tokens.txt", "r");
   FILE * assembly_code = fopen("elf.txt", "w");


   //checks if the files failed to be located or created
   if(tokens_file == NULL) {
       printf("Failed to locate \"tokens.txt\" file...\n");
       return 1;
   }
   else if(assembly_code == NULL) {
       printf("Failed to create \"elf.txt\" file...\n");
       return 1;
   }


   symbol symbol_table[MAX_SYMBOL_TABLE_SIZE];     //struct declaration to store symbol table
   instruction_register IR[MAX_SYMBOL_TABLE_SIZE]; //struct declaration to store instruction register


   //initializes starting size for struct declarations
   int table_size = 1;
   int IR_size = 0;  


   //2D array declaration to store tokens in input file
   char tokens[500][MAX_STR_LEN];


   int index = 0;
   int tokens_size = 0;


   //stores token list from input file to the 2D static array
   while(fscanf(tokens_file, "%s", tokens[tokens_size]) != EOF) {
       tokens_size++;
   }


   //function call to initialize the process of validation grammar from input file
   initializeProgram(tokens, &index, symbol_table, &table_size, IR, &IR_size, assembly_code);


   //for validating IR ONLY (for now)
   printf("OP\tL\tM\n");
   for(int i = 0; i <= IR_size; i++) {
       fprintf(assembly_code, "%d %d %d\n", IR[i].OP, IR[i].L, IR[i].M);
       printf("%s\t%d\t%d\n", IR[i].OP_s, IR[i].L, IR[i].M);


   }
  
   //for validating SYMBOL TABLE only (for now)
   printf("\nindex\tkind\tname\tvalue\tlevel\taddress\tmark\n");
   for(int i = 1; i <= table_size; i++) {
       if(symbol_table[i].kind == 1) { //constant variable - addr NOT used
           printf("%d\t%d\t%s\t%d\t%d\t-\t%d\n", i, symbol_table[i].kind, symbol_table[i].name, symbol_table[i].val, symbol_table[i].level, symbol_table[i].mark);
       }
       else if(symbol_table[i].kind == 2){ //variable - number NOT used
           printf("%d\t%d\t%s\t-\t%d\t%d\t%d\n", i, symbol_table[i].kind, symbol_table[i].name, symbol_table[i].level, symbol_table[i].addr, symbol_table[i].mark);
       }
   }


   fclose(tokens_file);
   fclose(assembly_code);


   return 0;
}


//helper function to locate repeated names
int symbolTableCheck(char id[], symbol table[], int size) {


   //traverses symbol table to return index of initial declaration of repeated name
   for(int i = 1; i <= size; i++) {
       if(strcmp(id, table[i].name) == 0) {
           return i;
       }
   }
   return 0; //returns 0 if name is not found
}


void initializeProgram(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code) {


   //JMP 0 3
   IR[*IR_size].OP = 7;
   IR[*IR_size].L = 0;
   IR[*IR_size].M = 3;


   strcpy(IR[*IR_size].OP_s, "JMP");


   (*IR_size)++;


   //function call for BLOCK procedure
   block(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);


   //converts string token to integer and checks if token matches periodsym
   if(atoi(tokens[*tokens_index]) != periodsym) {
       printf("Error: program must end with period\n");
       fprintf(assembly_code, "Error: program must end with period\n");
       exit(1);
   }


   //halts the successfully executed program
   //SYS 0 3
   IR[*IR_size].OP = 9;
   IR[*IR_size].L = 0;
   IR[*IR_size].M = 3;


   strcpy(IR[*IR_size].OP_s, "SYS");


   //sets all mark values to 1 to signify end of program
   for(int i = 1; i < *table_size; i++) {
       symbol_table[i].mark = 1;
   }
}


void block(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code) {
  
   //CONST-DECLARATION function call
   constantDeclaration(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);


   //stores returned integer from VAR-DECLARATION function call
   int numVars = 0;
   numVars = variableDeclaration(tokens, tokens_index, symbol_table, table_size, assembly_code);


   //INC 0 (3 + numVars)
   IR[*IR_size].OP = 6;
   IR[*IR_size].L = 0;
   IR[*IR_size].M = 3 + numVars;


   strcpy(IR[*IR_size].OP_s, "INC");


   (*IR_size)++;


   //STATEMENT function call


}


void constantDeclaration(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code) {
  
   //checks and enters do while loop if token is a constant token type
   if(atoi(tokens[*tokens_index]) == constsym) {
       do {
           (*tokens_index)++; //gets next token


           //checks for error of token not being an identifier token type
           if(atoi(tokens[*tokens_index]) != identsym && atoi(tokens[*tokens_index]) != skipsym) {
               printf("Error: const, var, and read keywords must be followed by identifier\n");
               fprintf(assembly_code, "Error: const, var, and read keywords must be followed by identifier\n");
               exit(1);
           }


           //checks for error of token representing lexical error token type, skipsym
           else if(atoi(tokens[*tokens_index]) == skipsym) {
               printf("Error: Scanning error detected by lexer (skipsym present)\n");
               fprintf(assembly_code, "Error: Scanning error detected by lexer (skipsym present)\n");
               exit(1);
           }


           //token == identsym
           else {
               (*tokens_index)++;


               //stores identifier name into temporary array
               char identifier[12];
               strcpy(identifier, tokens[*tokens_index]);


               //stores returned index from symbol table for identifier
               int table_index = symbolTableCheck(identifier, symbol_table, *table_size);


               //checks for error of multiple declarations of the same identifier
               if(table_index != 0) {
                   printf("Error: symbol name has already been declared\n");
                   fprintf(assembly_code, "Error: symbol name has already been declared\n");
                   exit(1);
               }


               (*tokens_index)++;


               //checks for error of missing "=" after constant declaration
               if(atoi(tokens[*tokens_index]) != eqsym) {
                   printf("Error: constants must be assigned with =\n");
                   fprintf(assembly_code, "Error: constants must be assigned with =\n");
                   exit(1);
               }


               (*tokens_index)++;


               //checks for error of constant declaration without assigned integer value
               if(atoi(tokens[*tokens_index]) != numbersym) {
                   printf("Error: constants must be assigned an integer value\n");
                   fprintf(assembly_code, "Error: constants must be assigned an integer value\n");
                   exit(1);
               }


               (*tokens_index)++;


               //stores the integer value of assigned to the identifier
               int number;
               number = atoi(tokens[*tokens_index]);


               //stores valid constant variable to symbol table
               symbol_table[*table_size].kind = 1;
               strcpy(symbol_table[*table_size].name, identifier);
               symbol_table[*table_size].val = number;
               symbol_table[*table_size].level = 0;
               symbol_table[*table_size].mark = 0;


               (*table_size)++; //increments symbol table size


               (*tokens_index)++;
           }
       } while(atoi(tokens[*tokens_index]) == commasym); //reads and stores all valid constant variables


       //checks for error of missing ";" at the end of constant variable declaration
       if(atoi(tokens[*tokens_index]) != semicolonsym && atoi(tokens[*tokens_index]) != identsym) {
           printf("Error: constant and variable declarations must be followed by a semicolon\n");
           fprintf(assembly_code, "Error: constant and variable declarations must be followed by a semicolon\n");
           exit(1);
       }


       //checks for error of missing "," between multiple constant variable declarations
       if(atoi(tokens[*tokens_index]) == identsym) {
           printf("Error: multiple constant and variable declarations must be followed by a comma\n");
           fprintf(assembly_code, "Error: multiple constant and variable declarations must be followed by a comma\n");
           exit(1);
       }


       (*tokens_index)++; //gets next token to check next portion of the grammar
   }
}


int variableDeclaration(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, FILE * assembly_code) {
 
   int numVars = 0; //initializes the number of variables to be stored in symbol table


   //checks and enters do while loop if token is a variable token type
   if(atoi(tokens[*tokens_index]) == varsym) {
       do {
           numVars++; //increments number of variables added


           (*tokens_index)++;
          
           if(atoi(tokens[*tokens_index]) != identsym && atoi(tokens[*tokens_index]) != skipsym) {
               printf("Error: const, var, and read keywords must be followed by identifier\n");
               fprintf(assembly_code, "Error: const, var, and read keywords must be followed by identifier\n");
               exit(1);
           }
           else if(atoi(tokens[*tokens_index]) == skipsym) {
               printf("Error: Scanning error detected by lexer (skipsym present)\n");
               fprintf(assembly_code, "Error: Scanning error detected by lexer (skipsym present)\n");
               exit(1);
           }
           else {
               (*tokens_index)++;


               char identifier[12];
               strcpy(identifier, tokens[*tokens_index]);


               int table_index = symbolTableCheck(identifier, symbol_table, *table_size);


               if(table_index != 0) {
                   printf("Error: symbol name has already been declared\n");
                   fprintf(assembly_code, "Error: symbol name has already been declared\n");
                   exit(1);
               }
              
               //stores valid variable to symbol table
               symbol_table[*table_size].kind = 2;
               strcpy(symbol_table[*table_size].name, identifier);
               symbol_table[*table_size].level = 0;
               symbol_table[*table_size].addr = numVars + 2;
               symbol_table[*table_size].mark = 0;


               (*table_size)++;


               (*tokens_index)++;


               //checks for error of variable being initialized with an integer value
               if(atoi(tokens[*tokens_index]) == eqsym) {
                   printf("Error: variables are not assigned with =\n");
                   fprintf(assembly_code, "Error: variables are not assigned with =\n");
                   exit(1);
               }
           }
       } while(atoi(tokens[*tokens_index]) == commasym);
      
       if(atoi(tokens[*tokens_index]) != semicolonsym && atoi(tokens[*tokens_index]) != identsym) {
           printf("Error: constant and variable declarations must be followed by a semicolon\n");
           fprintf(assembly_code, "Error: constant and variable declarations must be followed by a semicolon\n");
           exit(1);
       }


       if(atoi(tokens[*tokens_index]) == identsym) {
           printf("Error: multiple constant and variable declarations must be followed by a comma\n");
           fprintf(assembly_code, "Error: multiple constant and variable declarations must be followed by a comma\n");
           exit(1);
       }
      
       (*tokens_index)++;
   }
   return numVars; //returns total number of variables added
}


void statement(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code) {
  
   char identifier[MAX_STR_LEN];
   int table_index = 0;
   int jpc_index = 0;
   int loop_index = 0;


   switch(atoi(tokens[*tokens_index])) {
       case identsym:
  
           (*tokens_index)++;
  
           strcpy(identifier, tokens[*tokens_index]);
  
           table_index = symbolTableCheck(identifier, symbol_table, *table_size);
  
           //checks for error of identifier not being declared in symbol table
           if(table_index == 0) {
               printf("Error: undeclared identifier\n");
               fprintf(assembly_code, "Error: undeclared identifier\n");
               exit(1);
           }
  
           //checks for error of identifier not being of type "var"
           else if(symbol_table[table_index].kind != 2) {
               printf("Error: only variable values may be altered\n");
               fprintf(assembly_code, "Error: only variable values may be altered\n");
               exit(1);
           }
  
           (*tokens_index)++;
  
           //checks for error of token not being ":="
           if(atoi(tokens[*tokens_index]) != becomessym) {
               printf("Error: assignment statements must use :=\n");
               fprintf(assembly_code, "Error: assignment statements must use :=\n");
               exit(1);
           }
  
           (*tokens_index)++;
  
           //EXPRESSION function call
           expression(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);
  
           //STO 0 (symbol_table[table_index].addr)
           IR[*IR_size].OP = 4;
           IR[*IR_size].L = 0;
           IR[*IR_size].M = symbol_table[table_index].addr;
  
           strcpy(IR[*IR_size].OP_s, "STO");
  
           (*IR_size)++;
  
           break;
  
       case beginsym:
           do {
               (*tokens_index)++;
  
               statement(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);
  
           } while(atoi(tokens[*tokens_index]) == semicolonsym);
  
           //checks error of program starting with "begin" but not ending with "end"
           if(atoi(tokens[*tokens_index]) != endsym) {
               printf("Error: begin must be followed by end\n");
               fprintf(assembly_code, "Error: begin must be followed by end\n");
               exit(1);
           }
  
           (*tokens_index)++;


           break;
  
       case ifsym:
      
           (*tokens_index)++;
  
           //CONDITION function call
           condition(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);
  
           jpc_index = *tokens_index;
  
           //JPC 0 (jpc_index)
           IR[*IR_size].OP = 8;
           IR[*IR_size].L = 0;
           IR[*IR_size].M = jpc_index;
  
           strcpy(IR[*IR_size].OP_s, "JPC");
  
           (*IR_size)++;
  
           //checks for error of "if" not being followed by "then"
           if(atoi(tokens[*tokens_index]) != thensym) {
               printf("Error: if must be followed by then\n");
               fprintf(assembly_code, "Error: if must be followed by then\n");
               exit(1);
           }
  
           (*tokens_index)++;
  
           statement(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);
  
           IR[jpc_index].M = *tokens_index;
  
           //checks for error of "if" and "then" not being followed by "fi"
           if(atoi(tokens[*tokens_index]) != fisym) {
               printf("Error: if and then must be followed by fi\n");
               fprintf(assembly_code, "Error: if and then must be followed by fi\n");
               exit(1);
           }
 
           (*tokens_index)++;
          
           break;
      
       case whilesym:
           (*tokens_index)++;
          
           loop_index = (*tokens_index)++;
          
           condition(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);
         
           //checks for error of missing "do" after "while"
           if(atoi(tokens[*tokens_index]) != dosym) {
               printf("Error: while must be followed by do\n");
               fprintf(assembly_code, "Error: while must be followed by do\n");
               exit(1);
           }
          
           (*tokens_index)++;
          
           jpc_index = *tokens_index;
  
           //JPC 0 (jpc_index)
           IR[*IR_size].OP = 8;
           IR[*IR_size].L = 0;
           IR[*IR_size].M = jpc_index;
  
           strcpy(IR[*IR_size].OP_s, "JPC");
  
           (*IR_size)++;


           break;
         
       case readsym:
           (*tokens_index)++;
          
           if(atoi(tokens[*tokens_index]) != identsym) {
               printf("Error: const, var, and read keywords must be followed by identifier\n");
               fprintf(assembly_code, "Error: const, var, and read keywords must be followed by identifier\n");
               exit(1);
           }
          
           (*tokens_index)++;
          
           strcpy(identifier, tokens[*tokens_index]);
  
           table_index = symbolTableCheck(identifier, symbol_table, *table_size);
  
           //checks for error of identifier not being declared in symbol table
           if(table_index == 0) {
               printf("Error: undeclared identifier\n");
               fprintf(assembly_code, "Error: undeclared identifier\n");
               exit(1);
           }
  
           //checks for error of identifier not being of type "var"
           else if(symbol_table[table_index].kind != 2) {
               printf("Error: only variable values may be altered\n");
               fprintf(assembly_code, "Error: only variable values may be altered\n");
               exit(1);
           }
          
           (*tokens_index)++;
          
          
           //SYS 0 2
           IR[*IR_size].OP = 9;
           IR[*IR_size].L = 0;
           IR[*IR_size].M = 2;
  
           strcpy(IR[*IR_size].OP_s, "SYS");
  
           (*IR_size)++;
          
           //STO 0 (symbol_table[table_index].addr)
           IR[*IR_size].OP = 9;
           IR[*IR_size].L = 0;
           IR[*IR_size].M = symbol_table[table_index].addr;
  
           strcpy(IR[*IR_size].OP_s, "SYS");
  
           (*IR_size)++;
          
           break;


       case writesym:
      
           (*tokens_index)++;
          
           expression(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);
          
           //SYS 0 1
           IR[*IR_size].OP = 9;
           IR[*IR_size].L = 0;
           IR[*IR_size].M = 1;
  
           strcpy(IR[*IR_size].OP_s, "SYS");
  
           (*IR_size)++;
  
           break;
   }
}


//Working on this
void condition(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code) {
   /*
   if token == evensym
       get next token
       EXPRESSION
       emit EVEN
   else
       EXPRESSION
      
       if token == eqlsym
           get next token
           EXPRESSION
           emit EQL
       else if token == neqsym
           get next token
           EXPRESSION
           emit NEQ
       else if token == lessym
           get next token
           EXPRESSION
           emit LSS
       else if token == leqsym
           get next token
           EXPRESSION
           emit LEQ
       else if token == gtrsym
           get next token
           EXPRESSION
           emit GTR
       else if token == geqsym
           get next token
           EXPRESSION
           emit GEQ
       else
           error
   */
     //EXPRESSION function call
   //expression(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);
   
   //Need to check if statement logic
   //First case: odd. if(if odd x then ...)
   if(atoi(tokens[*tokens_index]) != evensym) {
    (*tokens_index)++;
    expression(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);
    //Emit ODD
    IR[*IR_size].OP = 2;
    IR[*IR_size].L = 0;
    IR[*IR_size].M = m; //Prefer to leave m blank for now.
   }


if(atoi(tokens[*tokens_index]) == eqsym){
       
/* if token == eqlsym
get next token
EXPRESSION
emit EQL
OP L M (2, 0, m (8)? )
*/  
(*tokens_index)++;
expression(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);
//Emit eqsym
    IR[*IR_size].OP = 2;
    IR[*IR_size].L = 0;
    IR[*IR_size].M = 8;
}


else if(atoi(tokens[*tokens_index]) == neqsym){
/*else if token == neqsym
get next token
EXPRESSION
emit NEQ
OP L M (2, 0, m (9)? )
*/
(*tokens_index)++;
expression(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);
//Emit NEQ
   IR[*IR_size].OP = 2;
    IR[*IR_size].L = 0;
    IR[*IR_size].M = 9;
}
//<
else if(atoi(tokens[*tokens_index]) == lessym){
/*else if token == lessym
get next token
EXPRESSION
emit LSS
OP L M (2, 0, m (10)? )
*/
(*tokens_index)++;
expression(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);
//Emit LSS
    IR[*IR_size].OP = 2;
    IR[*IR_size].L = 0;
    IR[*IR_size].M = 10;
}
//<=
else if (atoi(tokens[*tokens_index]) ==  leqsym){
          /* get next token
           EXPRESSION
           emit LEQ
           OP L M (2,0, m (11)?)
           */
          (*tokens_index)++;
expression(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);
//Emit LEQ
    IR[*IR_size].OP = 2;
    IR[*IR_size].L = 0;
    IR[*IR_size].M = 11;


}
//>
else if(atoi(tokens[*tokens_index]) ==gtrsym){
/*
else if token == gtrsym
get next token
EXPRESSION
emit gtr
Op L m(2, 0, m (12) ?)
*/
(*tokens_index)++;
expression(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);
//Emit GTR
    IR[*IR_size].OP = 2;
    IR[*IR_size].L = 0;
    IR[*IR_size].M = 12;
}
//>=
else if (atoi(tokens[*tokens_index]) ==geqsym){
/*get next token
EXPRESSION
emit GEQ
OP L M (2, 0 , m (13)?)
*/(*tokens_index)++;
expression(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);
//Emit GEQ
    IR[*IR_size].OP = 2;
    IR[*IR_size].L = 0;
    IR[*IR_size].M = 13;
}
else{
printf("Error: arithmetic equations must contain operands, parentheses, numbers, or symbols\n");
       fprintf(assembly_code, "Error: arithmetic equations must contain operands, parentheses, numbers, or symbols\n");
       exit(1);
   }
}

void expression(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code) {


   //TERM function call
   term(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);


   while(atoi(tokens[*tokens_index]) == plussym || atoi(tokens[*tokens_index]) == minussym) {
      
       if(atoi(tokens[*tokens_index]) == plussym) {
           (*tokens_index)++;
          
           term(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);
          
           //ADD 0 1
           IR[*IR_size].OP = 2;
           IR[*IR_size].L = 0;
           IR[*IR_size].M = 1;
      
           strcpy(IR[*IR_size].OP_s, "ADD");
      
           (*IR_size)++;


       }
       else {
           (*tokens_index)++;
          
           term(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);
          
           //SUB 0 2
           IR[*IR_size].OP = 2;
           IR[*IR_size].L = 0;
           IR[*IR_size].M = 2;
      
           strcpy(IR[*IR_size].OP_s, "SUB");
      
           (*IR_size)++;
       }
   }
}


void term(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code) {
  
   //FACTOR function call
   factor(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);


   while(atoi(tokens[*tokens_index]) == multsym || atoi(tokens[*tokens_index]) == slashsym) {
      
       if(atoi(tokens[*tokens_index]) == multsym) {
           (*tokens_index)++;
          
           factor(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);


           //MUL 0 3
           IR[*IR_size].OP = 2;
           IR[*IR_size].L = 0;
           IR[*IR_size].M = 3;
      
           strcpy(IR[*IR_size].OP_s, "MUL");
      
           (*IR_size)++;
       }
       else if(atoi(tokens[*tokens_index]) == slashsym) {
           (*tokens_index)++;
          
           factor(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);


           //DIV 0 4
           IR[*IR_size].OP = 2;
           IR[*IR_size].L = 0;
           IR[*IR_size].M = 4;
      
           strcpy(IR[*IR_size].OP_s, "DIV");
      
           (*IR_size)++;
       }
   }
}


void factor(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code) {
  
   if(atoi(tokens[*tokens_index]) == identsym) {
      
       (*tokens_index)++;
      
       char identifier[MAX_STR_LEN];
       strcpy(identifier, tokens[*tokens_index]);
  
       int table_index = symbolTableCheck(identifier, symbol_table, *table_size);
  
       if(table_index == 0) {
           printf("Error: undeclared identifier\n");
           fprintf(assembly_code, "Error: undeclared identifier\n");
           exit(1);
       }
      
       if(symbol_table[table_index].kind == 1) {
          
           //LIT 0 (symbol_table[table_index].value)
           IR[*IR_size].OP = 1;
           IR[*IR_size].L = 0;
           IR[*IR_size].M = symbol_table[table_index].val;
      
           strcpy(IR[*IR_size].OP_s, "LIT");
      
           (*IR_size)++;
       }
       else {
           //LOD 0 (symbol_table[table_index].addr)
           IR[*IR_size].OP = 3;
           IR[*IR_size].L = 0;
           IR[*IR_size].M = symbol_table[table_index].addr;
      
           strcpy(IR[*IR_size].OP_s, "LOD");
      
           (*IR_size)++;
       }
      
       (*tokens_index)++;
   }
   else if(atoi(tokens[*tokens_index]) == numbersym) {
       (*tokens_index)++;
      
       int number = atoi(tokens[*tokens_index]);
      
       //LIT 0 (number)
       IR[*IR_size].OP = 1;
       IR[*IR_size].L = 0;
       IR[*IR_size].M = number;
      
       strcpy(IR[*IR_size].OP_s, "LIT");
      
       (*tokens_index)++;
   }
   else if(atoi(tokens[*tokens_index]) == lparentsym){
       (*tokens_index)++;
      
       expression(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);
      
       if(atoi(tokens[*tokens_index]) != rparentsym) {
          
           //checks for error of program missing ")" after "("
           printf("Error: right parenthesis must follow left parenthesis\n");
           fprintf(assembly_code, "Error: right parenthesis must follow left parenthesis\n");
           exit(1);
       }
      
       (*tokens_index)++;


   }
   else {
      
       //checks for error of missing identifiers, numbers, parentheses, or operands in FACTOR function
       printf("Error: arithmetic equations must contain operands, parentheses, numbers, or symbols\n");
       fprintf(assembly_code, "Error: arithmetic equations must contain operands, parentheses, numbers, or symbols\n");
       exit(1);
   }
}