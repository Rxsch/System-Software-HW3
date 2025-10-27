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


typedef struct {
   char OP_s[3];   // human-readable opcode format
   int OP;         // opcode
   int L;          // lexicographical level
   int M;          // m field
} instruction_register;


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


int main() {
   FILE * tokens_file = fopen("tokens.txt", "r");
   FILE * assembly_code = fopen("elf.txt", "w");


   //checks if files failed to be located or created
   if(tokens_file == NULL) {
       printf("Failed to locate \"tokens.txt\" file...\n");
       return 1;
   }
   else if(assembly_code == NULL) {
       printf("Failed to create \"elf.txt\" file...\n");
       return 1;
   }


   symbol symbol_table[MAX_SYMBOL_TABLE_SIZE];
   instruction_register IR[MAX_SYMBOL_TABLE_SIZE];


   int table_size = 1;
   int IR_size = 0;


   char tokens[500][MAX_STR_LEN];
   int index = 0;
   int tokens_size = 0;


   //stores token list from input file in a 2D static array
   while(fscanf(tokens_file, "%s", tokens[tokens_size]) != EOF) {
       tokens_size++;
   }


   //for checking purposes ONLY
   printf("Token List: \n");
   for(int i = 0; i < tokens_size; i++) {
       printf("%s\n", tokens[i]);
   }
   printf("\n");


   initializeProgram(tokens, &index, symbol_table, &table_size, IR, &IR_size, assembly_code);


   //for checking purposes ONLY (for now)
   for(int i = 0; i <= IR_size; i++) {
       printf("%s %d %d\n", IR[i].OP_s, IR[i].L, IR[i].M);
       fprintf(assembly_code, "%d %d %d\n", IR[i].OP, IR[i].L, IR[i].M);
   }
  
   //for checking purposes ONLY (for now)
   printf("\nindex\tkind\tname\tvalue\tlevel\taddress\tmark\n");
   for(int i = 1; i < table_size; i++) {
       if(symbol_table[i].kind == 1) {
           printf("%d\t%d\t%s\t%d\t%d\t-\t%d\n", i, symbol_table[i].kind, symbol_table[i].name, symbol_table[i].val, symbol_table[i].level, symbol_table[i].mark);
       }
       else {
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
   for(int i = 1; i < size; i++) {
       if(strcmp(id, table[i].name) == 0) {
           return i;
       }
   }
   return 0; //returns 0 if name is not found
}


void initializeProgram(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code) {


   //JMP 0 3
   strcpy(IR[*IR_size].OP_s, "JMP");


   IR[*IR_size].OP = 7;
   IR[*IR_size].L = 0;
   IR[*IR_size].M = 3;


   (*IR_size)++;


   //block function call
   block(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);


   //converts string to integer and checks if token matches periodsym
   if(atoi(tokens[*tokens_index]) != periodsym) {
       printf("Error: program must end with period\n");
       fprintf(assembly_code, "Error: program must end with period\n");
       exit(1);
   }


   //halt successfully executed program
   strcpy(IR[*IR_size].OP_s, "SYS");


   IR[*IR_size].OP = 9;
   IR[*IR_size].L = 0;
   IR[*IR_size].M = 3;


   //sets all mark values to 1
   for(int i = 1; i < *table_size; i++) {
       symbol_table[i].mark = 1;
   }
}


void block(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code) {
   //const-declaration function call
   constantDeclaration(tokens, tokens_index, symbol_table, table_size, IR, IR_size, assembly_code);


   int numVars = 0;
   // numVars = function call for var-declaration
   numVars = variableDeclaration(tokens, tokens_index, symbol_table, table_size, assembly_code);




       //emit INC (M = 3 + numVars)
       strcpy(IR[*IR_size].OP_s, "INC");


   IR[*IR_size].OP = 6;
   IR[*IR_size].L = 0;
   IR[*IR_size].M = 3 + numVars;


   (*IR_size)++;


   //statement function call


}


void constantDeclaration(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, instruction_register IR[], int * IR_size, FILE * assembly_code) {
   if(atoi(tokens[*tokens_index]) == constsym) {
       do {
           (*tokens_index)++;


           if(atoi(tokens[*tokens_index]) != identsym && atoi(tokens[*tokens_index]) != skipsym) {
               //error: identifier not found
               printf("Error: const, var, and read keywords must be followed by identifier\n");
               fprintf(assembly_code, "Error: const, var, and read keywords must be followed by identifier\n");
               exit(1);
           }
           else if(atoi(tokens[*tokens_index]) == skipsym) {
               printf("Error: Scanning error detected by lexer (skipsym present)\n");
               fprintf(assembly_code, "Error: Scanning error detected by lexer (skipsym present)\n");
               exit(1);
           }
           else { //token == identsym
               (*tokens_index)++;


               //storing identifier into temporary array
               char identifier[12];
               strcpy(identifier, tokens[*tokens_index]);


               int table_index = symbolTableCheck(identifier, symbol_table, *table_size);


               //check if identifier name has already been declared
               if(table_index != 0) {
                   printf("Error: symbol name has already been declared\n");
                   fprintf(assembly_code, "Error: symbol name has already been declared\n");
                   exit(1);
               }


               (*tokens_index)++;


               //check if token != eqsym
               if(atoi(tokens[*tokens_index]) != eqsym) {
                   printf("Error: constants must be assigned with =\n");
                   fprintf(assembly_code, "Error: constants must be assigned with =\n");
                   exit(1);
               }


               (*tokens_index)++;


               //check if token != numbersym
               if(atoi(tokens[*tokens_index]) != numbersym) {
                   printf("Error: constants must be assigned an integer value\n");
                   fprintf(assembly_code, "Error: constants must be assigned an integer value\n");
                   exit(1);
               }


               (*tokens_index)++; //get number


               //stores and converts string token to an integer value
               int number;
               number = atoi(tokens[*tokens_index]);


               //add valid const identifier to symbol table
               //const does NOT use addr
               symbol_table[*table_size].kind = 1;
               strcpy(symbol_table[*table_size].name, identifier);
               symbol_table[*table_size].val = number;
               symbol_table[*table_size].level = 0;
               symbol_table[*table_size].mark = 0;


               (*table_size)++;
               (*tokens_index)++;
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
}


int variableDeclaration(char tokens[][MAX_STR_LEN], int * tokens_index, symbol symbol_table[], int * table_size, FILE * assembly_code) {
   int numVars = 0;
  
   if(atoi(tokens[*tokens_index]) == varsym) {
       do {
           numVars++;
           (*tokens_index)++;
          
           if(atoi(tokens[*tokens_index]) != identsym && atoi(tokens[*tokens_index]) != skipsym) {
               //error: identifier not found
               printf("Error: const, var, and read keywords must be followed by identifier\n");
               fprintf(assembly_code, "Error: const, var, and read keywords must be followed by identifier\n");
               exit(1);
           }
           else if(atoi(tokens[*tokens_index]) == skipsym) {
               printf("Error: Scanning error detected by lexer (skipsym present)\n");
               fprintf(assembly_code, "Error: Scanning error detected by lexer (skipsym present)\n");
               exit(1);
           }
           else { //token == identsym
               (*tokens_index)++;


               //storing identifier into temporary array
               char identifier[12];
               strcpy(identifier, tokens[*tokens_index]);


               int table_index = symbolTableCheck(identifier, symbol_table, *table_size);


               //check if identifier name has already been declared
               if(table_index != 0) {
                   printf("Error: symbol name has already been declared\n");
                   fprintf(assembly_code, "Error: symbol name has already been declared\n");
                   exit(1);
               }
              
               //assign var to symbol table
               //var does NOT use number
               symbol_table[*table_size].kind = 2;
               strcpy(symbol_table[*table_size].name, identifier);
               symbol_table[*table_size].level = 0;
               symbol_table[*table_size].addr = numVars + 2;
               symbol_table[*table_size].mark = 0;


               (*table_size)++;
               (*tokens_index)++;


               //check if token == eqsym
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
   return numVars;
}
