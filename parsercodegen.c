/*
Assignment:
HW3 - Parser and Code Generator for PL/0


Author: Jarielys Cruz Ggcccomez, Daniel Rangosch Montero


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
#define MAX_CODE_SIZE 500
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


typedef struct {
   char OP_s[4];   // human-readable opcode format
   int OP;         // opcode
   int L;          // lexicographical level
   int M;          // m field
} instruction_list;


typedef struct {
   int kind;       // const = 1, var = 2, proc = 3
   char name[12];  // name up to 11 chars
   int val;        // number (ASCII value)
   int level;      // L level
   int addr;       // M address
   int mark;       // to indicate unavailable or deleted
} symbol;


int symbolTableCheck(char id[], symbol table[], int size);
void initializeProgram(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_list instructions[], int * instructions_size, FILE * assembly_code);
void block(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_list instructions[], int * instructions_size, FILE * assembly_code);
void constantDeclaration(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_list instructions[], int * instructions_size, FILE * assembly_code);
int variableDeclaration(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, FILE * assembly_code);
void statement(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_list instructions[], int * instructions_size, FILE * assembly_code);
void condition(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_list instructions[], int * instructions_size, FILE * assembly_code);
void expression(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_list instructions[], int * instructions_size, FILE * assembly_code);
void term(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_list instructions[], int * instructions_size, FILE * assembly_code);
void factor(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_list instructions[], int * instructions_size, FILE * assembly_code);
void emit(int op, int l, int m, instruction_list instructions[], int * instructions_size);


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
   instruction_list instructions[MAX_CODE_SIZE];   //struct declaration to store instruction register


   //initializes starting size for struct declarations
   int table_size = 1;
   int instructions_size = 0;


   //2D array declaration to store tokens in input file
   char tokens[MAX_CODE_SIZE][MAX_STR_LEN];


   int index = 0;
   int tokens_size = 0;


   //stores token list from input file to the 2D static array
   while(fscanf(tokens_file, "%s", tokens[tokens_size]) != EOF) {
       tokens_size++;
   }
   strcpy(tokens[tokens_size], "\0");


   //function call to initialize the process of validation grammar from input file
   initializeProgram(tokens, &index, symbol_table, &table_size, instructions, &instructions_size, assembly_code);


   printf("Assembly Code:\n\n");
   printf("Line\t OP  L\t M\n");


   //displays assembly code in terminal
   for(int i = 0; i <= instructions_size; i++) {
       printf("%3d\t%s  %d %3d\n", i, instructions[i].OP_s, instructions[i].L, instructions[i].M);
   }


   printf("\nSymbol Table:\n\n");
   printf("Kind | Name\t| Value | Level | Address | Mark\n");
   printf("---------------------------------------------------\n");


   //displays symbol table in terminal
   for(int i = 1 ; i < table_size; i++) {
       if(symbol_table[i].kind == 1) {
           printf("%4d | \t%7s | %5d | %5d | \t- | %5d\n", symbol_table[i].kind, symbol_table[i].name, symbol_table[i].val, symbol_table[i].level, symbol_table[i].mark);
       }
       else {
           printf("%4d | \t%7s | %5d | %5d | %7d | %5d\n", symbol_table[i].kind, symbol_table[i].name, symbol_table[i].val, symbol_table[i].level, symbol_table[i].addr, symbol_table[i].mark);
       }
   }


   //writes the PM/0 code in format OP L M in output file, elf.txt
   for(int i = 0; i <= instructions_size; i++) {
       fprintf(assembly_code, "%d %d %d\n", instructions[i].OP, instructions[i].L, instructions[i].M);
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


void initializeProgram(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_list instructions[], int * instructions_size, FILE * assembly_code) {


   int op, l, m; //declares variables for PM/0 instructions (OP, L, M)


   //JMP 0 3
   op = 7;
   l = 0;
   m = 3;


   //function call to emit/store instruction into an instructions struct array
   emit(op, l, m, instructions, instructions_size);


   //function call for BLOCK procedure
   block(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);


   //converts string token to integer and checks if token matches periodsym
   if(atoi(tokens[*tokens_index]) != periodsym) {
       printf("Error: program must end with period\n");
       fprintf(assembly_code, "Error: program must end with period\n");
       exit(0);
   }


   //checks for error of continued code after "."
   if(atoi(tokens[*tokens_index]) == periodsym && strcmp(tokens[*tokens_index + 1], "\0") != 0) {
       printf("Error: program cannot continue after period\n");
       fprintf(assembly_code, "Error: program cannot continue after period\n");
       exit(0);
   }


   //halts the successfully executed program
   //SYS 0 3
   op = 9;
   l = 0;
   m = 3;


   emit(op, l, m, instructions, instructions_size);


   //sets all mark values to 1 to signify end of program
   for(int i = 1; i < *table_size; i++) {
       symbol_table[i].mark = 1;
   }
}


void block(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_list instructions[], int * instructions_size, FILE * assembly_code) {


   int op, l, m;


   //CONST-DECLARATION function call
   constantDeclaration(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);


   //stores returned integer from VAR-DECLARATION function call
   int numVars = 0;
   numVars = variableDeclaration(tokens, tokens_index, symbol_table, table_size, assembly_code);


   //INC 0 (3 + numVars)
   op = 6;
   l = 0;
   m = 3 + numVars;


   emit(op, l, m, instructions, instructions_size);


   //STATEMENT function call
   statement(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);
}


void constantDeclaration(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_list instructions[], int * instructions_size, FILE * assembly_code) {


   //checks and enters do while loop if token is a constant token type
   if(atoi(tokens[*tokens_index]) == constsym) {
       do {
           (*tokens_index)++; //gets next token


           //checks for error of token not being an identifier token type
           if(atoi(tokens[*tokens_index]) != identsym && atoi(tokens[*tokens_index]) != skipsym) {
               printf("Error: const, var, and read keywords must be followed by identifier\n");
               fprintf(assembly_code, "Error: const, var, and read keywords must be followed by identifier\n");
               exit(0);
           }


           //checks for error of token representing lexical error token type, skipsym
           else if(atoi(tokens[*tokens_index]) == skipsym) {
               printf("Error: Scanning error detected by lexer (skipsym present)\n");
               fprintf(assembly_code, "Error: Scanning error detected by lexer (skipsym present)\n");
               exit(0);
           }


           //token == identsym
           else {
               (*tokens_index)++;


               //stores identifier name into temporary array
               char identifier[MAX_STR_LEN];
               strcpy(identifier, tokens[*tokens_index]);


               //stores returned index from symbol table for identifier
               int table_index = symbolTableCheck(identifier, symbol_table, *table_size);


               //checks for error of multiple declarations of the same identifier
               if(table_index != 0) {
                   printf("Error: symbol name has already been declared\n");
                   fprintf(assembly_code, "Error: symbol name has already been declared\n");
                   exit(0);
               }


               (*tokens_index)++;


               //checks for error of missing "=" after constant declaration
               if(atoi(tokens[*tokens_index]) != eqsym) {
                   printf("Error: constants must be assigned with =\n");
                   fprintf(assembly_code, "Error: constants must be assigned with =\n");
                   exit(0);
               }


               (*tokens_index)++;


               //checks for error of constant declaration without assigned integer value
               //checks for error of constant declaration without assigned integer value
               if(atoi(tokens[*tokens_index]) != numbersym && atoi(tokens[*tokens_index]) != skipsym) {
                   printf("Error: constants must be assigned an integer value\n");
                   fprintf(assembly_code, "Error: constants must be assigned an integer value\n");
                   exit(0);
               }


               //checks for error of token representing lexical error token type, skipsym
               else if(atoi(tokens[*tokens_index]) == skipsym) {
                   printf("Error: Scanning error detected by lexer (skipsym present)\n");
                   fprintf(assembly_code, "Error: Scanning error detected by lexer (skipsym present)\n");
                   exit(0);
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
           exit(0);
       }


       //checks for error of missing "," between multiple constant variable declarations
       if(atoi(tokens[*tokens_index]) == identsym) {
           printf("Error: multiple constant and variable declarations must be followed by a comma\n");
           fprintf(assembly_code, "Error: multiple constant and variable declarations must be followed by a comma\n");
           exit(0);
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
               exit(0);
           }
           else if(atoi(tokens[*tokens_index]) == skipsym) {
               printf("Error: Scanning error detected by lexer (skipsym present)\n");
               fprintf(assembly_code, "Error: Scanning error detected by lexer (skipsym present)\n");
               exit(0);
           }
           else {
               (*tokens_index)++;


               char identifier[MAX_STR_LEN];
               strcpy(identifier, tokens[*tokens_index]);


               int table_index = symbolTableCheck(identifier, symbol_table, *table_size);


               if(table_index != 0) {
                   printf("Error: symbol name has already been declared\n");
                   fprintf(assembly_code, "Error: symbol name has already been declared\n");
                   exit(0);
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
                   exit(0);
               }
           }
       } while(atoi(tokens[*tokens_index]) == commasym);


       if(atoi(tokens[*tokens_index]) != semicolonsym && atoi(tokens[*tokens_index]) != identsym) {
           printf("Error: constant and variable declarations must be followed by a semicolon\n");
           fprintf(assembly_code, "Error: constant and variable declarations must be followed by a semicolon\n");
           exit(0);
       }


       if(atoi(tokens[*tokens_index]) == identsym) {
           printf("Error: multiple constant and variable declarations must be followed by a comma\n");
           fprintf(assembly_code, "Error: multiple constant and variable declarations must be followed by a comma\n");
           exit(0);
       }


       (*tokens_index)++;
   }
   return numVars; //returns total number of variables added
}


void statement(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_list instructions[], int * instructions_size, FILE * assembly_code) {
  
   int op, l, m;


   int table_index, jpc_index, loop_index; //declares variables to store index of symbol table, and M value of JPC and JMP instructions


   char identifier[MAX_STR_LEN]; //declares temporary array to store identifiers


   switch(atoi(tokens[*tokens_index])) {
       case identsym:
  
           (*tokens_index)++;
  
           strcpy(identifier, tokens[*tokens_index]);
  
           table_index = symbolTableCheck(identifier, symbol_table, *table_size);
  
           //checks for error of identifier not being declared in symbol table
           if(table_index == 0) {
               printf("Error: undeclared identifier\n");
               fprintf(assembly_code, "Error: undeclared identifier\n");
               exit(0);
           }
  
           //checks for error of identifier not being of type "var"
           else if(symbol_table[table_index].kind != 2) {
               printf("Error: only variable values may be altered\n");
               fprintf(assembly_code, "Error: only variable values may be altered\n");
               exit(0);
           }
  
           (*tokens_index)++;
  
           //checks for error of token not being ":="
           if(atoi(tokens[*tokens_index]) != becomessym) {
               printf("Error: assignment statements must use :=\n");
               fprintf(assembly_code, "Error: assignment statements must use :=\n");
               exit(0);
           }
  
           (*tokens_index)++;
  
           //EXPRESSION function call
           expression(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);
  
           //STO 0 (symbol_table[table_index].addr)
           op = 4;
           l = 0;
           m = symbol_table[table_index].addr;


           emit(op, l, m, instructions, instructions_size);
  
           break;
  
       case beginsym:
          
           do {
               (*tokens_index)++;
  
               statement(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);
  
           } while(atoi(tokens[*tokens_index]) == semicolonsym);
  
           //checks error of program starting with "begin" but not ending with "end"
           if(atoi(tokens[*tokens_index]) != endsym) {
               printf("Error: begin must be followed by end\n");
               fprintf(assembly_code, "Error: begin must be followed by end\n");
               exit(0);
           }
  
           (*tokens_index)++;


           break;
  
       case ifsym:
      
           (*tokens_index)++;
  
           //CONDITION function call
           condition(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);
  
           jpc_index = *instructions_size;
          
           //JPC 0 (jpc_index * 3)
           op = 8;
           l = 0;
           m = jpc_index * 3;


           emit(op, l, m, instructions, instructions_size);
  
           //checks for error of "if" not being followed by "then"
           if(atoi(tokens[*tokens_index]) != thensym) {
               printf("Error: if must be followed by then\n");
               fprintf(assembly_code, "Error: if must be followed by then\n");
               exit(0);
           }
  
           (*tokens_index)++;
  
           statement(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);


           instructions[jpc_index].M = *instructions_size * 3;


           //checks for error of "if" and "then" not being followed by "fi"
           if(atoi(tokens[*tokens_index]) != fisym) {
               printf("Error: if and then must be followed by fi\n");
               fprintf(assembly_code, "Error: if and then must be followed by fi\n");
               exit(0);
           }
 
           (*tokens_index)++;
          
           break;
      
       case whilesym:
           (*tokens_index)++;
          
           loop_index = *instructions_size;
          
           condition(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);
         
           //checks for error of missing "do" after "while"
           if(atoi(tokens[*tokens_index]) != dosym) {
               printf("Error: while must be followed by do\n");
               fprintf(assembly_code, "Error: while must be followed by do\n");
               exit(0);
           }
          
           (*tokens_index)++;
          
           jpc_index = *instructions_size;
          
           //JPC 0 (jpc_index * 3)
           op = 8;
           l = 0;
           m = jpc_index * 3;


           emit(op, l, m, instructions, instructions_size);
          
           statement(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);


           //JMP 0 (loop_index)
           op = 7;
           l = 0;
           m = loop_index * 3;


           emit(op, l, m, instructions, instructions_size);
          
           instructions[jpc_index].M = *instructions_size * 3;


           break;
         
       case readsym:
           (*tokens_index)++;
          
           if(atoi(tokens[*tokens_index]) != identsym) {
               printf("Error: const, var, and read keywords must be followed by identifier\n");
               fprintf(assembly_code, "Error: const, var, and read keywords must be followed by identifier\n");
               exit(0);
           }
          
           (*tokens_index)++;
          
           strcpy(identifier, tokens[*tokens_index]);
  
           table_index = symbolTableCheck(identifier, symbol_table, *table_size);
  
           //checks for error of identifier not being declared in symbol table
           if(table_index == 0) {
               printf("Error: undeclared identifier\n");
               fprintf(assembly_code, "Error: undeclared identifier\n");
               exit(0);
           }
  
           //checks for error of identifier not being of type "var"
           else if(symbol_table[table_index].kind != 2) {
               printf("Error: only variable values may be altered\n");
               fprintf(assembly_code, "Error: only variable values may be altered\n");
               exit(0);
           }
          
           (*tokens_index)++;
          
           //SYS 0 2
           op = 9;
           l = 0;
           m = 2;


           emit(op, l, m, instructions, instructions_size);


          
           //STO 0 (symbol_table[table_index].addr)
           op = 4;
           l = 0;
           m = symbol_table[table_index].addr;


           emit(op, l, m, instructions, instructions_size);
          
           break;


       case writesym:
      
           (*tokens_index)++;
          
           expression(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);
          
           //SYS 0 1
           op = 9;
           l = 0;
           m = 1;


           emit(op, l, m, instructions, instructions_size);
  
           break;
      
       //checks if token is empty
       default:
           break;
   }
}


void condition(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_list instructions[], int * instructions_size, FILE * assembly_code) {
  
   int op, l, m;


   //checks for even operator
   if(atoi(tokens[*tokens_index]) == evensym) {
      
       (*tokens_index)++;
      
       expression(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);
      
       //EVEN 0 11
       op = 2;
       l = 0;
       m = 11;


       emit(op, l, m, instructions, instructions_size);
   }


   //checks for every valid comparison operator
   else {
      
       expression(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);
      
       if(atoi(tokens[*tokens_index]) == eqsym) {
          
           (*tokens_index)++;
          
           expression(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);
          
           //EQL 0 5
           op = 2;
           l = 0;
           m = 5;


           emit(op, l, m, instructions, instructions_size);
       }
       else if(atoi(tokens[*tokens_index]) == neqsym) {
          
           (*tokens_index)++;
          
           expression(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);
          
           //NEQ 0 6
           op = 2;
           l = 0;
           m = 6;


           emit(op, l, m, instructions, instructions_size);
       }
       else if(atoi(tokens[*tokens_index]) == lessym) {
          
           (*tokens_index)++;
          
           expression(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);
          
           //LSS 0 7
           op = 2;
           l = 0;
           m = 7;


           emit(op, l, m, instructions, instructions_size);
       }
       else if(atoi(tokens[*tokens_index]) == leqsym) {
          
           (*tokens_index)++;
          
           expression(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);
          
           //LEQ 0 8
           op = 2;
           l = 0;
           m = 8;


           emit(op, l, m, instructions, instructions_size);
       }
       else if(atoi(tokens[*tokens_index]) == gtrsym) {
          
           (*tokens_index)++;
          
           expression(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);
          
           //GTR 0 9
           op = 2;
           l = 0;
           m = 9;


           emit(op, l, m, instructions, instructions_size);
       }
       else if(atoi(tokens[*tokens_index]) == geqsym) {
          
           (*tokens_index)++;
          
           expression(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);
          
           //GEQ 0 10
           op = 2;
           l = 0;
           m = 10;


           emit(op, l, m, instructions, instructions_size);
       }
       else {
          
           //checks for error of program missing an operator in CONDITION function  
           printf("Error: condition must contain comparison operator\n");
           fprintf(assembly_code, "Error: condition must contain comparison operator\n");
           exit(0);
       }
   }
}


void expression(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_list instructions[], int * instructions_size, FILE * assembly_code) {
  
   int op, l, m;


   //TERM function call
   term(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);


   while(atoi(tokens[*tokens_index]) == plussym || atoi(tokens[*tokens_index]) == minussym) {
      
       //checks if current token is "+"
       if(atoi(tokens[*tokens_index]) == plussym) {
           (*tokens_index)++;
          
           term(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);
          
           //ADD 0 1
           op = 2;
           l = 0;
           m = 1;


           emit(op, l, m, instructions, instructions_size);
       }


       //checks if current token is "-"
       else {
           (*tokens_index)++;
          
           term(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);
          
           //SUB 0 2
           op = 2;
           l = 0;
           m = 2;


           emit(op, l, m, instructions, instructions_size);
       }
   }
}


void term(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_list instructions[], int * instructions_size, FILE * assembly_code) {
  
   int op, l, m;


   //FACTOR function call
   factor(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);


   while(atoi(tokens[*tokens_index]) == multsym || atoi(tokens[*tokens_index]) == slashsym) {
      
       //checks if current token is "*"
       if(atoi(tokens[*tokens_index]) == multsym) {
           (*tokens_index)++;
          
           factor(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);


           //MUL 0 3
           op = 2;
           l = 0;
           m = 3;


           emit(op, l, m, instructions, instructions_size);
       }


       //checks if current token is "/"
       else if(atoi(tokens[*tokens_index]) == slashsym) {
           (*tokens_index)++;
          
           factor(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);


           //DIV 0 4
           op = 2;
           l = 0;
           m = 4;


           emit(op, l, m, instructions, instructions_size);
       }
   }
}


void factor(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_list instructions[], int * instructions_size, FILE * assembly_code) {
  
   int op, l, m;


   if(atoi(tokens[*tokens_index]) == skipsym) {
       printf("Error: Scanning error detected by lexer (skipsym present)\n");
       fprintf(assembly_code, "Error: Scanning error detected by lexer (skipsym present)\n");
       exit(0);
   }




   if(atoi(tokens[*tokens_index]) == identsym) {
      
       (*tokens_index)++;
      
       char identifier[MAX_STR_LEN];
       strcpy(identifier, tokens[*tokens_index]);
  
       int table_index = symbolTableCheck(identifier, symbol_table, *table_size);
  
       if(table_index == 0) {
           printf("Error: undeclared identifier\n");
           fprintf(assembly_code, "Error: undeclared identifier\n");
           exit(0);
       }
      
       //checks if identifier is a constant and pushes its literal value
       if(symbol_table[table_index].kind == 1) {
          
           //LIT 0 (symbol_table[table_index].val)
           op = 1;
           l = 0;
           m = symbol_table[table_index].val;


           emit(op, l, m, instructions, instructions_size);
       }


       //checks if identifier is a variable and loads its value through memory lookup
       else {


           //LOD 0 (symbol_table[table_index].addr)
           op = 3;
           l = 0;
           m = symbol_table[table_index].addr;


           emit(op, l, m, instructions, instructions_size);
       }
      
       (*tokens_index)++;
   }


   //checks if token is a number and pushes the literal value
   else if(atoi(tokens[*tokens_index]) == numbersym) {
       (*tokens_index)++;
      
       int number = atoi(tokens[*tokens_index]);
      
       //LIT 0 (number)
       op = 1;
       l = 0;
       m = number;


       emit(op, l, m, instructions, instructions_size);
      
       (*tokens_index)++;
   }
   else if(atoi(tokens[*tokens_index]) == lparentsym){
       (*tokens_index)++;
      
       expression(tokens, tokens_index, symbol_table, table_size, instructions, instructions_size, assembly_code);
      
       if(atoi(tokens[*tokens_index]) != rparentsym) {
          
           //checks for error of program missing ")" after "("
           printf("Error: right parenthesis must follow left parenthesis\n");
           fprintf(assembly_code, "Error: right parenthesis must follow left parenthesis\n");
           exit(0);
       }
      
       (*tokens_index)++;
   }
   else {
      
       //checks for error of missing identifiers, numbers, parentheses, or operands in FACTOR function
       printf("Error: arithmetic equations must contain operands, parentheses, numbers, or symbols\n");
       fprintf(assembly_code, "Error: arithmetic equations must contain operands, parentheses, numbers, or symbols\n");
       exit(0);
   }
}


void emit(int op, int l, int m, instruction_list instructions[], int * instructions_size) {
  
   //checks if current index of instructions struct array is out of bounds
   if(*instructions_size >= MAX_CODE_SIZE) {
       printf("Index for instructions struct array is out of bounds...\n");
       exit(0);
   }
   else {
       switch(op) {
           case 1:
               instructions[*instructions_size].OP = op;
               instructions[*instructions_size].L = l;
               instructions[*instructions_size].M = m;
              
               strcpy(instructions[*instructions_size].OP_s, "LIT");


               (*instructions_size)++;


               break;
          
           case 2:
               instructions[*instructions_size].OP = op;
               instructions[*instructions_size].L = l;
               instructions[*instructions_size].M = m;
              
               strcpy(instructions[*instructions_size].OP_s, "OPR");


               (*instructions_size)++;


               break;


           case 3:
               instructions[*instructions_size].OP = op;
               instructions[*instructions_size].L = l;
               instructions[*instructions_size].M = m;
              
               strcpy(instructions[*instructions_size].OP_s, "LOD");


               (*instructions_size)++;


               break;
          
           case 4:
               instructions[*instructions_size].OP = op;
               instructions[*instructions_size].L = l;
               instructions[*instructions_size].M = m;
              
               strcpy(instructions[*instructions_size].OP_s, "STO");


               (*instructions_size)++;


               break;


           case 5:
               instructions[*instructions_size].OP = op;
               instructions[*instructions_size].L = l;
               instructions[*instructions_size].M = m;
              
               strcpy(instructions[*instructions_size].OP_s, "CAL");


               (*instructions_size)++;


               break;
          
           case 6:
               instructions[*instructions_size].OP = op;
               instructions[*instructions_size].L = l;
               instructions[*instructions_size].M = m;
              
               strcpy(instructions[*instructions_size].OP_s, "INC");


               (*instructions_size)++;


               break;


           case 7:
               instructions[*instructions_size].OP = op;
               instructions[*instructions_size].L = l;
               instructions[*instructions_size].M = m;
              
               strcpy(instructions[*instructions_size].OP_s, "JMP");


               (*instructions_size)++;


               break;
          
           case 8:
               instructions[*instructions_size].OP = op;
               instructions[*instructions_size].L = l;
               instructions[*instructions_size].M = m;
              
               strcpy(instructions[*instructions_size].OP_s, "JPC");


               (*instructions_size)++;


               break;


           case 9:
               instructions[*instructions_size].OP = op;
               instructions[*instructions_size].L = l;
               instructions[*instructions_size].M = m;
              
               strcpy(instructions[*instructions_size].OP_s, "SYS");


               //checks that instruction is not HALT to increment size of instructions struct array
               if(m != 3) {
                   (*instructions_size)++;
               }


               break;
          
           default:
               printf("Opcode is not within the PM/0 ISA bounds, 1 - 9...\n");
               exit(0);
       }
   }
}


