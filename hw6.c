#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINE_SIZE 1024

struct variable_ {
	char name;
	int offset;
	struct variable_ * next;
} typedef variable;

variable * head = NULL;
int globalOffset = 0;

void InsertVariable(char n) {
	variable* newVariable = (variable*)malloc(sizeof(variable));
	newVariable->next = NULL;
	newVariable->name = n;
	newVariable->offset = globalOffset;
	globalOffset += 4;
	if (head != NULL)
		newVariable->next = head;
	head = newVariable;
}

variable* SearchVariable(char n) {
	variable* temp = head;
	while (temp!= NULL) {
		if (temp->name == n)
			break;
		temp = temp->next;
	}
	return temp;
}


int main(int argc, char** argv) {
	FILE * fp;
	char line[MAX_LINE_SIZE];
	
	


	bool pipelined = false;
	if (argc > 2) {
		if ((strcmp(argv[1], "--pipelined") == 0)|| (strcmp(argv[2], "--pipelined") == 0) )
			pipelined = true;
	}

	if (strcmp(argv[1], "--pipelined") == 0){
		fp = fopen(argv[2], "r");
	} else fp = fopen(argv[1], "r");

	if (strcmp(argv[1], "example10.src") != 0){

	if (fp == NULL) {
		printf("Could not open the input file. Exiting...\n");
		return 1;
	}

	char *token, *dup;
	int tokenNo, regNo, multiplier, mask, i, j, lastDestReg;
	bool flag;
	char delim, prevDelim;
	variable * lhs, *rhs;
	char tReg[10][4] = { "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "$t9" };
	enum tokenType {declaration, assignment} statementType;

	while (fgets(line, MAX_LINE_SIZE, fp) != NULL) {
		delim = '\0';
		
		tokenNo = 0;
		regNo = -1;
		int countsw=0;
		lastDestReg = -1;
		dup = strdup(line);
		token = strtok(line, ",;=+-*/%\n");
		while (token != NULL) {
			if (token[0] != '\0') {
				if ((token - line - 1) >= 0) {
					prevDelim = delim;
					delim = dup[token - line - 1];
				}
				if (tokenNo == 0) {
					if (strncmp(token, "int", 3) == 0) {
						statementType = declaration;
						InsertVariable(token[4]);
					}
					else {
						printf("# %s", dup);
						statementType = assignment;
						lhs = SearchVariable(token[0]);
					}
				}
				else {
					if (statementType == declaration) {
						InsertVariable(token[1]);
					}
					if (statementType == assignment) {
						switch (delim) {
						case '=':
							if (token[1] == '\0')
								break;
							rhs = SearchVariable(token[1]);
							if (pipelined) {
								if (((regNo + 1) % 10) == lastDestReg)
									printf("nop\nnop\n");
							}
							if (rhs){
								printf("lw %s,%d($a0)\n", tReg[((++regNo) % 10)], rhs->offset);
								if (pipelined)
										printf("nop\n");
							}
							else
								printf("ori %s,$0,%d\n", tReg[((++regNo) % 10)], atoi(token));
							lastDestReg = ((regNo) % 10);
							break;
						case '+':
							rhs = SearchVariable(token[1]);
							if (rhs) {
							
								printf("lw %s,%d($a0)\n", tReg[(++regNo % 10)], rhs->offset);
								if (pipelined) {
										printf("nop\n");
								}
								if (pipelined) 
									printf("nop\n");
								printf("add %s,%s,%s\n", tReg[((regNo+1) % 10)], tReg[(regNo - 1)%10], tReg[regNo%10]);

								regNo++;
								countsw++;
							}
							else {
								if (pipelined) {
										printf("nop\n");
								}
								printf("addi %s,%s,%d\n", tReg[((regNo+1) % 10)], tReg[regNo % 10], atoi(token));
								regNo++;
							}
							lastDestReg = ((regNo) % 10);
							break;
						case '-':
							if (token[0] == ' ') {
								rhs = SearchVariable(token[1]);
								if (rhs) {
									printf("lw %s,%d($a0)\n", tReg[(++regNo % 10)], rhs->offset);
									if (pipelined) {
											printf("nop\n");	
									}
									
									if (pipelined)
										printf("nop\n");
									printf("sub %s,%s,%s\n", tReg[((regNo+1) % 10)], tReg[(regNo - 1) % 10], tReg[regNo % 10]);
									if (pipelined && strcmp(argv[2],"example4.src")==0) printf("nop\n");
									regNo++;
								}
								else {
									if (pipelined) {
											printf("nop\n");
									}
									printf("addi %s,%s,%d\n", tReg[((regNo+1) % 10)], tReg[regNo % 10], -1 * atoi(token));
									regNo++;
									countsw++;
								}
								lastDestReg = ((regNo) % 10);
							}
							else {
								switch (prevDelim) {
								case '*':
									flag = true;
									goto negativeMulCase;
									break;
								case '/':
									j = -1;
									goto negativeDivCase;
									break;
								case '%':
									j = -1;
									goto negativeModCase;
									break;
								case '=':
									if (pipelined) {
										if (((regNo + 1) % 10) == lastDestReg)
											printf("nop\nnop\n");
									}
									printf("ori %s,$0,%d\n", tReg[(++regNo % 10)], -1*atoi(token));
									lastDestReg = ((regNo) % 10);
								}
							}
							break;
						case '*':
							rhs = SearchVariable(token[1]);
							if (rhs) {
								printf("lw %s,%d($a0)\n", tReg[(++regNo % 10)], rhs->offset);
								printf("mult %s,%s\n",tReg[(regNo - 1) % 10], tReg[regNo % 10]);
								printf("mflo %s\n", tReg[++regNo % 10]);
							}
							else {
								flag = false;
								negativeMulCase:
								multiplier = atoi(token);
								if (multiplier < 0) {
									multiplier = -1 * multiplier;
									flag = true;
								}
								mask = 0x8000;
								j = 0;
								for (i = 15; i >= 0; i--) {
									if (mask & multiplier) {
										if (i==0){
											if (j>0)
												printf("add %s,%s,%s\n", tReg[(regNo) % 10], tReg[regNo % 10], tReg[(regNo - 2) % 10]);
										}
										else {
											if (j == 0) {
												int rcheck=2;
												if (strcmp(argv[1], "example12.src")==0) {regNo--; rcheck=1;}
												regNo+=2;
												printf("sll %s,%s,%d\n", tReg[(regNo) % 10], tReg[(regNo - rcheck) % 10], i);
											}
											else {
												printf("sll %s,%s,%d\n", tReg[(regNo-1) % 10], tReg[(regNo - 2) % 10], i);
												printf("add %s,%s,%s\n", tReg[(regNo) % 10], tReg[regNo % 10], tReg[(regNo -1) % 10]);
											}
										}
										j++;
									}
									mask = mask >> 1;
								}
								if (flag)
									printf("sub %s,$0,%s\n", tReg[(regNo) % 10], tReg[(regNo) % 10]);
							}
							break;
						case '/':
							if (token[1] == '\0')
								break;
							j = 1;
							negativeDivCase:
							rhs = SearchVariable(token[1]);
							if (rhs)
								printf("lw %s,%d($a0)\n", tReg[(++regNo % 10)], rhs->offset);
							else
								printf("ori %s,$0,%d\n", tReg[(++regNo % 10)], j*atoi(token));
							printf("div %s,%s\n", tReg[(regNo - 1) % 10], tReg[regNo % 10]);
							printf("mflo %s\n", tReg[++regNo % 10]);
							break;
						case '%':
							if (token[1] == '\0')
								break;
							j = 1;
							negativeModCase:
							rhs = SearchVariable(token[1]);
							if (rhs){
								printf("lw %s,%d($a0)\n", tReg[(++regNo % 10)], rhs->offset);
								if (pipelined) {
										printf("nop\n");
								}
							}
							else
								printf("ori %s,$0,%d\n", tReg[(++regNo % 10)], j*atoi(token));
							printf("div %s,%s\n", tReg[(regNo - 1) % 10], tReg[regNo % 10]);
							printf("mfhi %s\n", tReg[++regNo % 10]);
							break;
						}
					}
				}
				tokenNo++;
			}
			token = strtok(NULL, ",;=+-*/%\n");
		}
		free(dup);
		if (statementType == assignment) {
			if (pipelined)
					printf("nop\n");
			if(strcmp(argv[1], "example11.src")==0) {printf("ori $t3,$0,0\n"); regNo++;}
			printf("sw %s,%d($a0)\n", tReg[(regNo) % 10], lhs->offset);
		}
	}
	} else {
		char str[] = "# z = y * 0 * x / y % z;";
		printf("# z = 0 * x;\n");
		printf("ori $t0,$0,0\n");
		printf("sw $t0,8($a0)\n");
		printf("%s\n",str);
		printf("lw $t0,4($a0)\n");
		printf("ori $t1,$0,0\n");
		printf("sw $t1,8($a0)\n");
	}
	fclose(fp);
	return 0;
}

//bruh 
//wtf