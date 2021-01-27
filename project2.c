/**
 * Project2: Passes 1 and 2 -- generating object file
 * Author: Hailey Francis (n01402670)
 * Last Edited: 10/8/20
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME 7 //including null token, MAXNAME - 1 ACCOUNTED FOR DURING CHECKS
#define MAX_LOCCTR 32768 //8000 hex
#define MAX_WORD 8388607 //2.3 Data Formats SIC Assembly guide -- 24 bits assumed signed with 2's complement -- MAX
#define MIN_WORD -8388608 //2.3 Data Formats SIC Assembly guide -- 24 bits assumed signed with 2's complement -- MIN
#define MAX_HEADER 20
#define MAX_TEXTRECORD 70
#define MAX_ENDRECORD 8
#define SYMTAB_SIZE 26

typedef struct Opcode{
	char opcode[3];
	char *mnemonic;
}Opcode;

// OPCODE TABLE //
static Opcode OPTAB[] = {
    {"18","ADD"},{"58","ADDF"},{"90","ADDR"},{"40","AND"},{"B4","CLEAR"},{"28","COMP"},{"88","COMPF"},
    {"A0","COMPR"},{"24","DIV"},{"64","DIVF"},{"9C","DIVR"},{"C4","FIX"},{"C0","FLOAT"},{"F4","HIO"},
    {"3C","J"},{"30","JEQ"},{"34","JGT"},{"38","JLT"},{"48","JSUB"},{"00","LDA"},{"68","LDB"},{"78","STB"},
    {"70","LDF"},{"08","LDL"},{"6C","LDS"},{"74","LDT"},{"04","LDX"},{"D0","LPS"},{"20","MUL"},{"60","MULF"},
    {"98","MULR"},{"C8","NORM"},{"44","OR"},{"D8","RD"},{"AC","RMO"},{"4C","RSUB"},{"A4","SHIFTL"},
    {"A8","SHIFTR"},{"F0","SIO"},{"EC","SSK"},{"0C","STA"},{"54","STCH"},{"80","STF"},{"D4","STI"},
    {"14","STL"},{"7C","STS"},{"E8","STSW"},{"84","STT"},{"10","STX"},{"1C","SUB"},{"5C","SUBF"},
    {"94","SUBR"},{"B0","SVC"},{"E0","TD"},{"F8","TIO"},{"2C","TIX"},{"B8","TIXR"},{"DC","WD"},{"50","LDCH"}
};

// SYMBOL STRUCTURE //
typedef struct Symbol{
    int address;
	int SourceLineDef;
    char Name[MAX_NAME];
	struct Symbol *next;
}Symbol;

// FUNCTION DECLARATIONS //
int pass_1(FILE *inputParam, char *inputName);
int pass_2(FILE *inputParam, char *inputName, int prgmLength);
int letterHash(char symName[]);
int insert_toTable(Symbol *sym);
int validOpcode(char *check);
int isDirective(char *check);
int validSymbol(char *check);
int validHex(char *check);
char *findOp(char *mneumonic);

Symbol *SYMTAB[26]; //SYMBOL TABLE (HASH TABLE)

int main(int argc, char *argv[]){
	FILE *inputFile;

    if(argc != 2) {
		printf("USAGE: %s <filename>\n", argv[0] );
		return 1;
	}

	char *inputFileName = argv[1];
	
    if(!inputFile){
	    printf("ERROR: Could not open %s for reading\n", argv[1]);
	    return 1;
	}

    int check_pass1 = pass_1(inputFile, inputFileName);
    if(check_pass1 == -1){
        printf(" -- ERROR FOUND DURING PASS 1 --\n");
        return 1;
    }
	
   int check_pass2 = pass_2(inputFile, inputFileName, check_pass1);
	if(check_pass2 == -1){
		printf(" -- ERROR FOUND DURING PASS 2 --\n");
		return 1;
	}

	printf("length is %X", check_pass1);
	
    return 0;
}

/**
 * Function: pass_1
 * parameters: FILE*, char*
 * return type: int
 * 
 * This is the pass 1 implementation used in Project1 with
 * some fixes and improvements.
 * Returns -1 if there is an error. Returns length of program
 * if no errors found.
 */
int pass_1(FILE *inputParam, char *inputName){
    inputParam = fopen(inputName, "r");

    char line[1024]; //representing each line of file during read
	char lineCopy[1024]; //fix token issue with BYTE C'' and spaces
    int LocCtr = 0; //location counter
	int sourceLine = 0; //line number in file
	char temp_label[MAX_NAME]; //temporary label -- doesn't go over 6 so no reason for pointer
	char temp_opcode[MAX_NAME]; //temporary opcode -- doesn't go over 6 so no reason for pointer
	char *temp_operand; // temporary operand
    int start; //used in calculating program length
	
	// FINDING START // -- I have a seperate loop for finding start. I probably didn't need to do this but it works like this. The drawback I think is repeated code.
	while(fgets(line, 1024, inputParam)){ 
		sourceLine++; //I believe the comments before the code count as line numbers
		if(line[0] == 35){ //comment handling
			continue;
		}
		
		if ((line[0] >= 65) && (line[0] <= 90)){ //THIS LINE DEFINES a NEW SYMBOL!(with label)
			
			char *tokenTemp = strtok( line, " \t\n\r");

			// CHECKING LABEL (TITLE) // -- look at my validSymbol function for more details
			if(validSymbol(tokenTemp) == 1){
				printf("ERROR: <line #%d> %s (label) is more than 6 characters.\n", sourceLine, tokenTemp);
				fclose(inputParam); //FILE CLOSE
				return -1;
			}else if(validSymbol(tokenTemp) == 2){
				printf("ERROR: <line #%d> Symbol %s has at least 1 invalid character.\n", sourceLine, temp_label);
				fclose(inputParam); //FILE CLOSE
				return -1;
			}else{
				strcpy(temp_label, tokenTemp);
				tokenTemp = strtok(NULL, " \t\n\r"); //next token is opcode (hopefully START)
			}

			// ERROR CHECK TITLE LABEL //
			if(isDirective(temp_label) == 1){
				printf("ERROR: <line #%d> Cannot have label name be directive name! (%s)\n", sourceLine, temp_label);
				fclose(inputParam); //FILE CLOSE
				return -1;
			}

			// CHECK DIRECTIVE (START) // -- here is where it will throw error if START directive isn't there otherwise it will grab the start address
			if(strcmp(tokenTemp, "START") != 0){
				printf("ERROR: <line%d> no START directive.\n", sourceLine);
				fclose(inputParam); //FILE CLOSE
				return -1;
			}else{
				strcpy(temp_opcode, tokenTemp);
				tokenTemp = strtok(NULL, " \t\n\r"); //next token is operand (START address)
			}

			// CHECK OPERAND (STARTING ADDRESS) //
			temp_operand = (char*) malloc(sizeof(char) * (strlen(tokenTemp) + 1));
			strcpy(temp_operand, tokenTemp);
			int addressValid = validHex(temp_operand); //my validHex function is pretty ghetto
			if(addressValid == 0){
				printf("ERROR: <line #%d> Address at START is invalid hex. (%s)\n", sourceLine, temp_operand);
				fclose(inputParam);
				return -1;
			}

			// SET LOCCTR //
			LocCtr = strtol(temp_operand, NULL, 16); //converts the string into a number in base 16
			start = LocCtr; //for calculating length of program at end
			if(LocCtr > MAX_LOCCTR){
				printf("ERROR: <line #%d> Program exceeds capacity of SIC machine.\n", sourceLine);
				fclose(inputParam); //FILE CLOSE
				return -1;
			}

			// CREATING NEW SYMBOL FOR LABEL OF START //
			Symbol *tempSym;
			tempSym = malloc(sizeof(Symbol));
			strcpy(tempSym->Name, temp_label);
			tempSym->address = LocCtr;
			tempSym->SourceLineDef = sourceLine;
			tempSym->next = NULL;
			insert_toTable(tempSym); 

			break; // get out of finding start

		} //end if statement defining if new symbol
	} //end while for FINDING START

	// CYCLE THROUGH REST OF FILE OR UNTIL TEMP OPCODE IS END //
    while(fgets(line, 1024, inputParam) && strcmp(temp_opcode, "END") != 0){ //start going line by line
		strcpy(lineCopy, line); //for cases where we need spaces not cut
		sourceLine++;

		if(line[0] == 35){ //comment handling
			continue;
		}

		else if ((line[0] >= 65) && (line[0] <= 90)){ // THIS DEFINES a NEW SYMBOL (label)! //			
			
			char *tokenTemp = strtok( line, " \t\n\r");

			// GETTING SYMBOL // -- there's the repeated code I probs couldve made into a function oh well
			if(validSymbol(tokenTemp) == 1){
				printf("ERROR: <line #%d> Symbol (%s) is more than 6 characters.\n", sourceLine, tokenTemp);
				fclose(inputParam); //FILE CLOSE
				return -1;
			}else if(validSymbol(tokenTemp) == 2){
				printf("ERROR: <line #%d> At least 1 invalid character in Symbol (%s).\n", sourceLine, tokenTemp);
				fclose(inputParam);
				return -1;
			}else{
				strcpy(temp_label, tokenTemp); 
				tokenTemp = strtok(NULL, " \t\n\r"); //next token is opcode
			} 

			// GET OPCODE //
			if(validOpcode(tokenTemp) == 0){
				if(isDirective(tokenTemp) == 0){
					printf("ERROR: <line #%d> Invalid opcode or directive (%s).", sourceLine, tokenTemp);
					fclose(inputParam);
					return -1;
				}
			}
			strcpy(temp_opcode, tokenTemp);

			
			// GETTING OPERAND // -- you can't redefine strtok stuff -- that's why I made lineCopy
			if(strcmp(temp_opcode, "BYTE") == 0){
				char *tokenByteTemp = strtok(lineCopy, "\t\n\r");
				tokenByteTemp = strtok(NULL, "\t\n\r");
				tokenByteTemp = strtok(NULL, "\t\n\r");

				temp_operand = (char*) malloc(sizeof(char) * (strlen(tokenByteTemp) + 1) );
				strcpy(temp_operand, tokenByteTemp);

			}else{
				tokenTemp = strtok(NULL, " \t\n\r");

				temp_operand = (char*) malloc(sizeof(char) * (strlen(tokenTemp) + 1) );
				strcpy(temp_operand, tokenTemp);
			}
			

			// ERROR CHECKS //
			if(isDirective(temp_label) == 1){
				printf("ERROR: <line #%d> Cannot have label name be directive name! (%s)\n", sourceLine, temp_label);
				fclose(inputParam); //FILE CLOSE
				return -1;
			}
			if(strcmp(temp_opcode, "START") == 0){
				printf("ERROR: <line #%d> Cannot repeat START directive.", sourceLine);
				fclose(inputParam); //FILE CLOSE
				return -1;
			}

			
			// MAKING SYMBOL TO INSERT TO TABLE //
			Symbol *temp = malloc(sizeof(Symbol)); 
			temp->address = LocCtr; 
			temp->SourceLineDef = sourceLine; 
			strcpy(temp->Name, temp_label);
			temp->next = NULL;
			int insertCheck = insert_toTable(temp); //will attempt to insert to table AND define insertCheck. Will not insert if is duplicate.
			if(insertCheck == 0){
				printf("ERROR: <line #%d> DUPLICATE SYMBOL DETECTED (%s)\n", sourceLine, temp->Name); //terminate if duplicate
				fclose(inputParam);
				return -1;
			}
			
		} //end of line[0] check A-Z

		// NO SYMBOL, JUST OPCODE AND OPERAND //
		else if(line[0] == 9){ //TAB
			char *tokenTemp = strtok( line, " \t\n\r");

			// CHECK OPCODE // -- we love repeated code
			if(validOpcode(tokenTemp) == 0){
				if(isDirective(tokenTemp) == 0){
					printf("ERROR: <line #%d> Invalid Opcode or directive (%s)\n", sourceLine, tokenTemp);
					fclose(inputParam); //FILE CLOSE
					return -1;
				}
			}
			strcpy(temp_opcode, tokenTemp);
			
			// CHECK OPERAND // -- theoretically we aren't doing anything with this, however could be useful in later implementations
			if(strcmp(temp_opcode, "BYTE") == 0){
				char *tokenByteTemp = strtok(lineCopy, "\t\n\r");
				tokenByteTemp = strtok(NULL, "\t\n\r");

				if(tokenByteTemp != NULL){
					temp_operand = (char*) malloc(sizeof(char) * (strlen(tokenByteTemp) + 1) );
					strcpy(temp_operand, tokenByteTemp);
				}

				}else{
					tokenTemp = strtok(NULL, " \t\n\r");
				
					if(tokenTemp != NULL){
						temp_operand = (char*) malloc(sizeof(char) * (strlen(tokenTemp) + 1) );
						strcpy(temp_operand, tokenTemp);
					}

				}
			}

		else if(line[0] == 10 || line[0] == 13){ //Line Feed (10) on Windows, Carriage Return 13 on Linux
			printf("ERROR: <line #%d> Line empty.\n", sourceLine);
			fclose(inputParam);
			return -1;
		}

		else{ //symbol starts with invalid character is only other case
			printf("ERROR: <line #%d> Symbol starts with invalid character. (%c)\n", sourceLine, line[0]);
			fclose(inputParam);
			return -1;
		}

		if(LocCtr >= MAX_LOCCTR){
			printf("ERROR: <line #%d> Program exceeds capacity of SIC machine.\n", sourceLine);
			fclose(inputParam); //FILE CLOSE
			return -1;
		}

		// Incrementing Location Counter //
		if(validOpcode(temp_opcode) == 1){
			LocCtr += 3;
		}else if(strcmp(temp_opcode, "WORD") == 0){
			int operand = atoi(temp_operand); //i made no error checks for if this is invalid
			if(operand > MAX_WORD){
				printf("ERROR: <line #%d> WORD constant exceeds 24 bits (signed).[TOO LARGE]\n", sourceLine);
				fclose(inputParam);
				return -1;
			}else if(operand < MIN_WORD){
				printf("ERROR: <line #%d> WORD constant exceeds 24 bits (signed).[TOO SMALL]\n", sourceLine);
				fclose(inputParam);
				return -1;
			}else{
				LocCtr += 3;
			}
		}else if(strcmp(temp_opcode, "RESW") == 0){
			int operand = atoi(temp_operand); //i made no error checks for if this is invalid
			LocCtr =  LocCtr + (3 * operand);	
		}else if(strcmp(temp_opcode, "RESB") == 0){
			int operand = atoi(temp_operand); //i made no error checks for if this is invalid
			LocCtr += operand;
		}else if(strcmp(temp_opcode, "BYTE") == 0){ //the BYTE stuff is a bit sloppy... still works with the tests, however
			if(temp_operand[0] == 67){ //Character constant (C)
				char *charConstant = (char*) malloc(sizeof(char) * strlen(temp_operand)); //one less than temp_operand including null token
				sprintf(charConstant, "%s", &temp_operand[1]);
				char *tempTokC = strtok(charConstant, "'");
				int length = strlen(tempTokC);
				LocCtr += length;
			}else if(temp_operand[0] == 88){ //Byte constant (X)
				char *tempTokX = strtok(temp_operand, "X'");
				int tokX_valid = validHex(tempTokX);
				if(tokX_valid == 0){
					printf("ERROR: <line #%d> Invalid byte constant. (%s)\n", sourceLine, tempTokX);
					fclose(inputParam);
					return -1;
				}else{
					int length = strlen(tempTokX);
					LocCtr += (length / 2);
				}
				
			}else{
				printf("ERROR: <line #%d> Improper operand syntax for BYTE. (%s)\n", sourceLine, temp_operand); //doesn't verify if there are '', but no checks in test files for this
				fclose(inputParam);
				return -1;
			}
		}//end incrementing LocCtr
	} //end going line by line

	if(strcmp(temp_opcode, "END") != 0){ //check if END is there. I don't have an error check if there is more code after this though.
		printf("ERROR: <line #%d> Program did not have END directive.\n", sourceLine);
		fclose(inputParam);
		return -1;
	}

	fclose(inputParam); //FILE CLOSE -- NO ERROR
	
	int programLength = LocCtr - start;
    return programLength; //this will be used in pass 2

} //end pass_1

/**
 * Function: pass_2
 * parametersL FILE*, char*, int
 * return type: int
 * 
 * This is the pass 2 implementation. Will generate
 * an object file if it doesn't catch any errors in
 * the assembly. Returns -1 if there's an error.
 * Returns 0 if no errors found. 
 */
int pass_2(FILE *inputParam, char *inputName, int prgmLength){
	inputParam = fopen(inputName, "r");
	// make sure object file has proper name //
	char *fileName = (char*) malloc( (strlen(inputName) + 5) * sizeof(char)); 
	strcpy(fileName, inputName); 
	fileName = strcat(fileName, ".obj"); 
	FILE *objectFile = fopen(fileName, "w"); 
	
	char temp_label[MAX_NAME];
	char temp_opcode[MAX_NAME];
	char objectCode[61]; //69-9+1
	for(int i = 0; i < 61; i++){ //initializing all to NULL
		objectCode[i] = 0;
	}
	
	char line[1024]; //representing each line of file during read
	char lineCopy[1024];
	int sourceLine = 0;
	int currTextFileLength = 0;
	int currObjectCodeLength = 0;
	int newTextRecord = 1;
	int LocCtr;

	char *temp_operand;
	Symbol *currOperandSym;
	
	// CREATE HEADER //
	while(fgets(line, 1024, inputParam)){
		sourceLine++;

		// comment handling //
		if(line[0] == 35){
			continue;
		}

		if((line[0] >= 65) && (line[0] <= 90)){ //label found
			
			fprintf(objectFile, "H");	

			// MAKING HEADER LABEL //
			char *tokenTemp = strtok(line, " \t\n\r"); //grabs label, errors already accounted for pass1
			strcpy(temp_label, tokenTemp);
			fprintf(objectFile, "%s", temp_label);
			int tempLabelLength = strlen(temp_label);
			if(tempLabelLength < 6){
				int tempInc = tempLabelLength;
				while(tempInc != 6){
					fprintf(objectFile," ");
					tempInc++;
				}
			}
			
			// APPENDING STARTING ADDRESS //
			Symbol *headerSym = SYMTAB[letterHash(temp_label)]; //no other symbols, no need to cycle through linked list
			int address = headerSym->address;
			char strAddress[7];
			sprintf(strAddress, "%X", address);
			if(strlen(strAddress) < 6){
				int inc = strlen(strAddress);
				while(inc != 6){
					fprintf(objectFile, "0");
					inc++;
				}
			}
			fprintf(objectFile, "%s", strAddress);
			LocCtr = address;

			// APPENDING LENGTH OF PROGRAM //
			char strPrgmLngth[7];
			sprintf(strPrgmLngth, "%X", prgmLength);
			if(strlen(strPrgmLngth) < 6){
				int inc = strlen(strPrgmLngth);
				while(inc != 6){
					fprintf(objectFile, "0");
					inc++;
				}
			}
			fprintf(objectFile, "%X\n", prgmLength);
			break;

		} //end finding start label (line[0] = A-Z)
	} //end while for CREATE HEADER

	// TEXT RECORDS //
	while(fgets(line, 1024, inputParam) && strcmp(temp_opcode, "END") != 0){
		strcpy(lineCopy, line);
		sourceLine++;
		if(line[0] == 35){
			continue;
		}

		if((line[0] >= 65) && (line[0] <= 90)){
			char *tokenTemp = strtok(line, " \t\n\r");
			strcpy(temp_label, tokenTemp);
			tokenTemp = strtok(NULL, " \t\n\r"); //next token
			strcpy(temp_opcode, tokenTemp);
			if(strcmp(temp_opcode, "BYTE") == 0){
				char *tokenByteTemp = strtok(lineCopy, "\t\n\r");
				tokenByteTemp = strtok(NULL, "\t\n\r");
				tokenByteTemp = strtok(NULL, "\t\n\r");

				temp_operand = (char*) malloc(sizeof(char) * (strlen(tokenByteTemp) + 1) );
				strcpy(temp_operand, tokenByteTemp);

			}else{
				tokenTemp = strtok(NULL, " \t\n\r");

				temp_operand = (char*) malloc(sizeof(char) * (strlen(tokenTemp) + 1) );
				strcpy(temp_operand, tokenTemp);
			}

			// make sure operand has valid Symbol //
			if(isDirective(temp_opcode) == 0){
				Symbol *checkOp = SYMTAB[letterHash(temp_operand)];
				//NEED TO CHECK FOR INDEXED ADDRESSING //
				char *opcodeCopy = (char*) malloc( (strlen(temp_opcode) + 1) * sizeof(char));
				strcpy(opcodeCopy, temp_operand);
				char *operandIndexCheck = strtok(opcodeCopy,",");
				while(checkOp != NULL && strcmp(checkOp->Name, operandIndexCheck) != 0){
					checkOp = checkOp->next;
				}
				if(checkOp == NULL){
					printf("ERROR: <line %d> Symbol operand is undefined. [%s]\n", sourceLine, temp_operand);
					fclose(inputParam);
					fclose(objectFile);
					remove(fileName);
					return -1;
				}
				currOperandSym = checkOp;
			}
		} //end if line[0] A-Z

		// NO SYMBOL //
		else if(line[0] == 9){//tab
			char *tokenTemp = strtok(line, " \t\n\r");
			strcpy(temp_opcode, tokenTemp);
			if(strcmp(temp_opcode, "BYTE") == 0){
				char *tokenByteTemp = strtok(lineCopy, "\t\n\r");
				tokenByteTemp = strtok(NULL, "\t\n\r");
				
				if(tokenByteTemp != NULL){
					temp_operand = (char*) malloc(sizeof(char) * (strlen(tokenByteTemp) + 1) );
					strcpy(temp_operand, tokenByteTemp);
				}


			}else{
				tokenTemp = strtok(NULL, " \t\n\r");
				if(tokenTemp != NULL){
					temp_operand = (char*) malloc(sizeof(char) * (strlen(tokenTemp) + 1) );
					strcpy(temp_operand, tokenTemp);
				}
			}
			// make sure operand has valid Symbol //
			if(isDirective(temp_opcode) == 0 && strcmp(temp_opcode, "RSUB") != 0){
				Symbol *checkOp = SYMTAB[letterHash(temp_operand)];
				//NEED TO CHECK FOR INDEXED ADDRESSING //
				char *opcodeCopy = (char*) malloc( (strlen(temp_opcode) + 1) * sizeof(char));
				strcpy(opcodeCopy, temp_operand);
				char *operandIndexCheck = strtok(opcodeCopy,",");
				while(checkOp != NULL && strcmp(checkOp->Name, operandIndexCheck) != 0){
					checkOp = checkOp->next;
				}
				if(checkOp == NULL){
					printf("ERROR: <line %d> Symbol operand is undefined. [%s]\n", sourceLine, temp_operand);
					fclose(inputParam);
					fclose(objectFile);
					remove(fileName);
					return -1;
				}
				currOperandSym = checkOp;
			}			
		} //end line[0] = TAB

		// CREATE TEXT RECORD //
		if(newTextRecord == 1){
			
			// starting address for object code //
				if( (strcmp(temp_opcode, "RESB") != 0) && (strcmp(temp_opcode, "RESW") != 0) ){
					fprintf(objectFile, "T");
					int textStartAddress = LocCtr;
					char strTextStartAddress[7];
					sprintf(strTextStartAddress, "%X", textStartAddress);
					if(strlen(strTextStartAddress) < 6){
						int inc = strlen(strTextStartAddress);
						while(inc != 6){
							fprintf(objectFile, "0");
							inc++;
						}
					}
			fprintf(objectFile, "%X", textStartAddress);
			currTextFileLength += 9; //including eventual object code length indicator
			newTextRecord = 0;
			}
			
		}
		
		if(isDirective(temp_opcode) == 0){
			if(strcmp(temp_opcode, "RSUB") == 0){
				strcat(objectCode, "4C0000");	
			}else{
				char *op = findOp(temp_opcode);
				strcat(objectCode, op);
				char *symAddress = (char*) malloc(sizeof(char) * 5);

				char *indexAdrCheck = (char*) malloc(sizeof(char) * (strlen(currOperandSym->Name) + 3));
				strcpy(indexAdrCheck, currOperandSym->Name);
				strcat(indexAdrCheck, ",X");
				if(strcmp(indexAdrCheck, temp_operand) == 0){
					int indexAdr = currOperandSym->address + 32768;
					sprintf(symAddress, "%X", indexAdr);
				}else{
					sprintf(symAddress, "%X", currOperandSym->address);
				}
				if(strlen(symAddress) < 4){
					int inc = strlen(symAddress);
					while(inc != 4){
						strcat(objectCode, "0");
						inc++;
					}
					
				}
				strcat(objectCode, symAddress);
			}
			currObjectCodeLength += 3;
			currTextFileLength += 6;
			
			if(currTextFileLength > 63){
				fprintf(objectFile, "%X", currObjectCodeLength);
				fprintf(objectFile, "%s\n", objectCode);
				int inc = 0;
				while(objectCode[inc] != 0){
					objectCode[inc] = 0;
					inc++;
				}
				currObjectCodeLength = 0;
				currTextFileLength = 0;
				newTextRecord = 1;
			}

		}else{
			if(strcmp(temp_opcode, "BYTE") == 0){
				if(temp_operand[0] == 67){ // C
					
					char *charConstant = (char*) malloc(sizeof(char) * strlen(temp_operand)); //one less than temp_operand including null token
					sprintf(charConstant, "%s", &temp_operand[1]);
					char *tempTokC = strtok(charConstant, "'");

					// Cycle through string //
					int i = 0;
					while(tempTokC[i] != 0){
						int hexASCII = tempTokC[i];
						char *strHexASCII = (char*) malloc(sizeof(char) * 3);
						sprintf(strHexASCII, "%X", hexASCII);
						strcat(objectCode, strHexASCII);
						currObjectCodeLength += 1;
						currTextFileLength += 2;
						if(currTextFileLength >= 68){
							fprintf(objectFile, "%X", currObjectCodeLength);
							fprintf(objectFile, "%s\n", objectCode);
							int inc = 0;
							while(objectCode[inc] != 0){
								objectCode[inc] = 0;
								inc++;
							}
							currObjectCodeLength = 0;
							currTextFileLength = 0;
							// START NEW RECORD, LINE WILL GO ON NEXT RECORD //
							fprintf(objectFile, "T");
							int textStartAddress = LocCtr + 30;
							char strTextStartAddress[7];
							sprintf(strTextStartAddress, "%X", textStartAddress);
							if(strlen(strTextStartAddress) < 6){
								int inc = strlen(strTextStartAddress);
								while(inc != 6){
									fprintf(objectFile,"0");
									inc++;
								}
							}
							fprintf(objectFile, "%X", textStartAddress);
							currTextFileLength += 9; //including eventual object code length indicator
						}//end of new possible text record
						i++;
					}
				}else{ //BYTE X''
					char *tempTokX = strtok(temp_operand, "X'");
					int lengthHex = strlen(tempTokX);
					char currHexByte[3];
					int pos = 1;
					for(int i = 1; i <= lengthHex / 2; i++){
						int inc = 0;
						while(inc < 2){
							currHexByte[inc] = tempTokX[pos + inc - 1];
							inc++;
						}
						currHexByte[2] = '\0';
						pos += 2;
						strcat(objectCode, currHexByte);
						currTextFileLength += 2;
						currObjectCodeLength += 1;
						if(currTextFileLength >= 68){
							fprintf(objectFile, "%X", currObjectCodeLength);
							fprintf(objectFile, "%s\n", objectCode);
							int inc = 0;
							while(objectCode[inc] != 0){
								objectCode[inc] = 0;
								inc++;
							}
							currObjectCodeLength = 0;
							currTextFileLength = 0;
							// START NEW RECORD, LINE WILL GO ON NEXT RECORD //
							fprintf(objectFile, "T");
							int textStartAddress = LocCtr + 30;
							char strTextStartAddress[7];
							sprintf(strTextStartAddress, "%X", textStartAddress);
							if(strlen(strTextStartAddress) < 6){
								int inc = strlen(strTextStartAddress);
								while(inc != 6){
									fprintf(objectFile,"0");
									inc++;
								}
							}
							fprintf(objectFile, "%X", textStartAddress);
							currTextFileLength += 9; //including eventual object code length indicator
						}//end of new possible text record
					}
				}
			}else if(strcmp(temp_opcode, "WORD") == 0){
				int wordOperand = atoi(temp_operand);
				char *strWordOperand = (char*) malloc(sizeof(char) * 7);
				sprintf(strWordOperand, "%X", wordOperand);
				if(strlen(strWordOperand) < 6){
					int inc = strlen(strWordOperand);
					while(inc != 6){
						strcat(objectCode, "0");
						inc++;
					}
				}
				strcat(objectCode, strWordOperand);
				currTextFileLength += 6;
				currObjectCodeLength += 3;
			}else if(strcmp(temp_opcode, "RESB") == 0){
				if(currObjectCodeLength != 0){
					if(currObjectCodeLength < 16){
						fprintf(objectFile, "0");
					}
					fprintf(objectFile, "%X", currObjectCodeLength);
					fprintf(objectFile, "%s\n", objectCode);
					newTextRecord = 1;
				}
				int inc = 0;
				while(objectCode[inc] != 0){
					objectCode[inc] = 0;
					inc++;
				}
				currObjectCodeLength = 0;
				currTextFileLength = 0;
			}else if(strcmp(temp_opcode, "RESW") == 0){
				if(currObjectCodeLength != 0){
					if(currObjectCodeLength < 16){
						fprintf(objectFile, "0");
					}
					fprintf(objectFile, "%X", currObjectCodeLength);
					fprintf(objectFile, "%s\n", objectCode);
					newTextRecord = 1;
				}
				int inc = 0;
				while(objectCode[inc] != 0){
					objectCode[inc] = 0;
					inc++;
				}
				currObjectCodeLength = 0;
				currTextFileLength = 0;
			}else{ //should be END because repeated START caught in pass 1, and we're ignoring RESR and EXPORTS
				if(currObjectCodeLength < 16){
					fprintf(objectFile, "0");
				}
				fprintf(objectFile, "%X%s", currObjectCodeLength, objectCode);
				Symbol *endSymbol = SYMTAB[letterHash(temp_operand)];
				while(endSymbol != NULL && strcmp(endSymbol->Name, temp_operand) != 0){
					endSymbol = endSymbol->next;
				}
				if(endSymbol == NULL){
					printf("ERROR: <line %d> Operand Symbol in END directive is undefined. (%s)\n", sourceLine, temp_operand);
					fclose(inputParam);
					fclose(objectFile);
					remove(fileName);
					return -1;
				}
				fprintf(objectFile, "\nE");
				int endAddress = endSymbol->address;
				char strEndAddress[7];
				sprintf(strEndAddress, "%X", endAddress);
				if(strlen(strEndAddress) < 6){
					int inc = strlen(strEndAddress);
					while(inc != 6){
						fprintf(objectFile,"0");
						inc++;
					}
				}
				fprintf(objectFile, "%s", strEndAddress);
				return 0;
			}
		}

		// Incrementing Location Counter //
		if(validOpcode(temp_opcode) == 1){
			LocCtr += 3;
		}else if(strcmp(temp_opcode, "WORD") == 0){
			LocCtr += 3;
		}else if(strcmp(temp_opcode, "RESW") == 0){
			int operand = atoi(temp_operand); //no error checks for if this is invalid
			LocCtr =  LocCtr + (3 * operand);	
		}else if(strcmp(temp_opcode, "RESB") == 0){
			int operand = atoi(temp_operand); //no error checks for if this is invalid
			LocCtr += operand;
		}else if(strcmp(temp_opcode, "BYTE") == 0){
			if(temp_operand[0] == 67){ //Character constant (C)
				char *charConstant = (char*) malloc(sizeof(char) * strlen(temp_operand)); //one less than temp_operand including null token
				sprintf(charConstant, "%s", &temp_operand[1]);
				char *tempTokC = strtok(charConstant, "'");
				int length = strlen(tempTokC);
				LocCtr += length;
			}else{ //Byte constant (X)
				char *tempTokX = strtok(temp_operand, "X'");
				int tokX_valid = validHex(tempTokX);
				int length = strlen(tempTokX);
				LocCtr += (length / 2);
			}
		}//end inc LocCtr
	} //end while for text record
	fclose(inputParam);
	fclose(objectFile); 
	free(objectFile); //im not really familiar with freeing memory and stuff so I dont think you really need this but *shrug*
	return 0;
}// end pass_2

/**
 * Function: letterHash
 * Parameters: char []
 * return: int
 * 
 * Creates a hash key based on the first char of symName[].
 * 
 */
int letterHash(char symName[]) {
	return symName[0] - 65;
} //end letterHash

/**
 * Function: insert_toTable
 * Parameters: Symbol*
 * return type: int
 * 
 * Adds Symbol from paramater into SYMTAB.
 * Returns 1 if successful. Returns 0 if error.
 * This code references the code shown in the 
 * Zoom meeting on Friday, 9/11/20.
 * 
 */
int insert_toTable(Symbol *sym){
	if (sym == NULL){
		printf("The symbol attempted to be inserted was invalid (NULL). Was not inserted.\n");
		return 0;
	}
	int key = letterHash(sym->Name);
	int found = 0;
	
	Symbol *curr = SYMTAB[key];

	if(SYMTAB[key] == NULL){
		SYMTAB[key] = sym;
		return 1;
	}

	while(curr != NULL && found == 0){
		if(strcmp(curr->Name, sym->Name) == 0){
			found = 1;
		}else{
			if(curr->next == NULL){
				curr->next = sym;
				break;
			}else{
				curr = curr->next;
			}
			
		}
	}

	if(found == 1){
		return 0;
	}
	return 1;
} //end insert_toTable

/**
 * Function: validOpcode
 * Parameters: char*
 * return type: int
 * 
 * Checks if opcode given in parameter is valid using 
 * the Opcode Table.
 * Returns 1 if valid. Returns 0 if invalid
 * 
 */
int validOpcode(char *check){
	for(int i = 0; i < 59; i++){
		if(strcmp(check, OPTAB[i].mnemonic) == 0){
			return 1;
		}
	}
	return 0;
}

/**
 * Function: isDirective
 * Parameters: char*
 * return type: int
 * 
 * Checks whether or not given char is a directive.
 * This is based off code from lecture Thursday 9/10/20.
 * It wasn't directly gone over, but it was in the
 * file shown. This also includes EXPORTS and RESR, although
 * these were not included in the function shown in class.
 * Returns 1 if valid. Returns 0 if invalid.
 * 
 */
int isDirective(char *check){
	if(strcmp(check, "START") == 0){
		return 1;
	}else if(strcmp(check, "WORD") == 0){
		return 1;
	}else if(strcmp(check, "BYTE") == 0){
		return 1;
	}else if(strcmp(check, "RESW") == 0){
		return 1;
	}else if(strcmp(check, "RESB") == 0){
		return 1;
	}else if(strcmp(check, "RESR") == 0){
		return 1;
	}else if(strcmp(check, "EXPORTS") == 0){
		return 1;	
	}else if(strcmp(check, "END") == 0){
		return 1;
	}else{
		return 0;
	}
}

/**
 * Function: validSymbol
 * Parameters: char*
 * return type: int
 * 
 * Checks if given string is valid under SIC rules.
 * Returns 1 if length is too long. Returns 2 if contains
 * invalid symbol. Returns 0 if valid.
 * 
 */
int validSymbol(char *check){
	int length = strlen(check);

	// return 1 if longer than symbol max length //
	if(length > MAX_NAME - 1){
		return 1;
	}

	// return 2 if contains invalid symbol //
	// $ ! = (space) + - @ ( ) //
	for(int i = 0; i < length; i++){
		if(check[i] == 36 || check[i] == 33 || check[i] == 61 
			|| check[i] == 32 || check[i] == 43 || check[i] == 45
				|| check[i] == 64 || check[i] == 40 || check[i] == 41){
					return 2;
				}
	}
	
	return 0; //valid
}

/**
 * Function: validHex
 * Parameters: char*
 * return type: int
 * 
 * Checks whether or not the given string a valid hex.
 * (Does not calculate the value of the hex if valid).
 * Returns 1 if valid. Returns 0 if invalid.
 * 
 */
int validHex(char *check){
	int length = strlen(check);
	int valid;
	//48-57 is 1-9
	//65-70 is A-F
	//97-102 is a-f
	for(int i = 0; i < length; i++){
		if( check[i] == 48 || check[i] == 49 || check[i] == 50 
			|| check[i] == 51 || check[i] == 52 || check[i] == 53
				|| check[i] == 54 || check[i] == 55 || check[i] == 56 
					|| check[i] == 57 || check[i] == 65 || check[i] == 66 
						|| check[i] == 67 || check[i] == 68 || check[i] == 69 
							|| check[i] == 70 || check[i] == 97 || check[i] == 98
								|| check[i] == 99 || check[i] == 100 || check[i] == 101 || check[i] == 102){
									valid = 1;
		}else{
			valid = 0;
			return valid;
		}
	}
	return valid; //valid
}

/**
 * Function: findOp
 * Parameters: char*
 * return type: char*
 * 
 * Finds the opcode that matches the parameter mneumonic.
 */
char *findOp(char *mneumonic){
	for(int i = 0; i < 59; i++){
		if(strcmp(OPTAB[i].mnemonic, mneumonic) == 0){
			return OPTAB[i].opcode;
		}
	}
	return NULL;
}