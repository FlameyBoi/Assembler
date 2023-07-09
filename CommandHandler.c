#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "ParsingTable.h"
#include "ParseTool.h"
#include <stdbool.h>
#include "defs.h"
#include "Allocator.h"
extern int linenum;

/*prototype block*/
void insertOp(int op, int encoding, int addressing, int jmpAdd, parsingTable* table);
void insertParam(int encoding, int param, char* label, parsingTable* table);
int twoArgs(parsingTable* table, char* line, int index);
int twoArgsNoDest(parsingTable* table, char* line, int index);
int oneArg(parsingTable* table, char* line, int index);
int oneArgNoDest(parsingTable* table, char* line, int index);
int noArg(parsingTable* table, char* line, int index);
int jmpOps(parsingTable* table, char* line, int index);
int parseArg(char* line);
int parseImmediate(char* line);
/*prototype block*/

/*insert Op word into instruction array*/
void insertOp(int op, int encoding, int addressing, int jmpAdd, parsingTable* table)
{
	void* ptr;
	if (table->ccount == table->cspace) /*alloc if out of space*/
	{
		ptr = myRealloc(table->cmd_array, table->cspace * 2 * (sizeof(command)));
		table->cmd_array = (command*)ptr;
		table->cspace *= 2;
	}
	/*aligning values into proper bits*/
	table->cmd_array[table->ccount].val = jmpAdd;
	table->cmd_array[table->ccount].val <<= 4;
	table->cmd_array[table->ccount].val += op;
	table->cmd_array[table->ccount].val <<= 4;
	table->cmd_array[table->ccount].val += addressing;
	table->cmd_array[table->ccount].val <<= 2;
	table->cmd_array[table->ccount].val += encoding;
	table->cmd_array[table->ccount].label = (char*)myMalloc(1);
	strcpy(table->cmd_array[table->ccount].label, "");
	table->cmd_array[table->ccount].t = cmd;
	table->ccount++;
}

/*insert arg word into instruction array*/
void insertParam(int encoding, int param, char* label, parsingTable* table)
{
	void* ptr;
	if (table->ccount == table->cspace) /*alloc if out of space*/
	{
		ptr = myRealloc(table->cmd_array, table->cspace * 2 * (sizeof(command)));
		table->cmd_array = (command*)ptr;
		table->cspace *= 2;
	}
	/*aligning values into proper bits*/
	table->cmd_array[table->ccount].val = param;
	table->cmd_array[table->ccount].val <<= 2;
	table->cmd_array[table->ccount].val += encoding;
	table->cmd_array[table->ccount].label = (char*)myMalloc(LABEL_LEN + 1);
	strcpy(table->cmd_array[table->ccount].label, label);
	table->cmd_array[table->ccount].t = d;
	table->ccount++;
}

/*handler for all two arg ops other than cmp, parses line, extracts args and checks validity - finally adds values to instruction array*/
int twoArgs(parsingTable* table, char* line, int index)
{
	int operand1, operand2, encoding1 = ABS, encoding2 = ABS, addressing;
	char label1[LABEL_LEN + 1] = {'\0'}, label2[LABEL_LEN + 1] = { '\0' };
	while (isspace(*line)) line++;
	line = strtok(line, ","); /*split into tokesn using comma*/
	if ((operand1 = parseArg(line)) == ERR) return -1; /*parse first arg*/
	if (operand1 == LABEL) getLabel(label1, line); /*if operand1 is a label save it*/
	line = strtok(NULL, ","); /*move to next arg*/
	if (line == NULL)
	{
		printf("Error: missing second operand in line %d\n", linenum);
		return -1;
	}
	while (isspace(*line)) line++;
	if ((operand2 = parseArg(line)) == ERR) return -1; /*parse arg 2*/
	if (operand2 == LABEL) getLabel(label2, line); /*if arg 2 is a label save it*/
	if ((line = strtok(NULL, ",")) != NULL)
	{
		printf("Error: unrecognized token %s found in line %d\n", line, linenum);
		return -1;
	}
	/*special handling for 2 reg case*/
	if (operand1 >= r0 && operand1 <= r7 && operand2 >= r0 && operand2 <= r7)
	{
		insertOp(index, ABS, (REGISTER << 2) + REGISTER, 0, table);
		insertParam(ABS, ((operand1 - r0) << 6) + (operand2 - r0), "", table);
		return 0;
	}
	if (operand1 == LABEL) /*if operand1 is label*/
	{
		addressing = DIRECT;
		encoding1 = UNKNOWN;
	}
	else if (operand1 >= r0 && operand1 <= r7) /*if operand1 is reg*/
	{
		addressing = REGISTER;
		operand1 = (operand1 - r0) << 6; /*source register appears in bits 2-7 of additional word (0-5 if we ignore encoding bits)*/
	}
	else addressing = IMMEDIATE; /*if operand1 is immediate*/
	addressing <<= 2; /*lower 2 bits of addressing belong to dest arg*/
	if (operand2 == LABEL) /*if operand2 is a label save it*/
	{
		addressing += DIRECT;
		encoding2 = UNKNOWN;
	}
	else if (operand2 >= r0 && operand2 <= r7) /*if operand2 is reg*/
	{
		addressing += REGISTER;
		operand2 = (operand2 - r0); /*destination register appears in bits 8-13 of additional word (6-11 if we ignore encoding bits)*/
	}
	else /*if operand2 is immediate*/
	{
		printf("Error: immediate value used as a destination in line %d\n", linenum);
		return -1;
	}
	/*insert op and args*/
	insertOp(index, ABS, addressing, 0, table);
	insertParam(encoding1, operand1, label1, table);
	insertParam(encoding2, operand2, label2, table);
	return 0;
}

/*Works the same as regular 2arg parser but lets dest arg be immediate*/
int twoArgsNoDest(parsingTable* table, char* line, int index)
{
	int operand1, operand2, encoding1 = ABS, encoding2 = ABS, addressing;
	char label1[LABEL_LEN + 1] = { '\0' }, label2[LABEL_LEN + 1] = { '\0' };
	line = strtok(line, ",");
	while (isspace(*line)) line++;
	if ((operand1 = parseArg(line)) == ERR) return -1;
	if (operand1 == LABEL) getLabel(label1, line);
	line = strtok(NULL, ",");
	if (line == NULL)
	{
		printf("Error: missing second operand in line %d\n", linenum);
		return -1;
	}
	while (isspace(*line)) line++;
	if ((operand2 = parseArg(line)) == ERR) return -1;
	if (operand2 == LABEL) getLabel(label2, line);
	if ((line = strtok(NULL, ",")) != NULL)
	{
		printf("Error: unrecognized token %s found in line %d\n", line, linenum);
		return -1;
	}
	/*2 regs*/
	if (operand1 >= r0 && operand1 <= r7 && operand2 >= r0 && operand2 <= r7)
	{
		insertOp(index, ABS, (REGISTER << 2) + REGISTER, 0, table);
		insertParam(ABS, ((operand1 - r0) << 6) + (operand2 - r0), "", table);
		return 0;
	}
	if (operand1 == LABEL)
	{
		addressing = DIRECT;
		encoding1 = UNKNOWN;
	}
	else if (operand1 >= r0 && operand1 <= r7)
	{
		addressing = REGISTER;
		operand1 = (operand1 - r0) << 6; /*source register appears in bits 8-13 of additional word (6-11 if we ignore encoding bits)*/
	}
	else addressing = IMMEDIATE;
	addressing <<= 2;
	if (operand2 == LABEL)
	{
		addressing += DIRECT;
		encoding2 = UNKNOWN;
	}
	else if (operand2 >= r0 && operand2 <= r7)
	{
		addressing += REGISTER;
		operand2 = (operand2 - r0); /*destination register appears in bits 2-7 of additional word (0-5 if we ignore encoding bits)*/
	}
	else addressing += IMMEDIATE;
	insertOp(index, ABS, addressing, 0, table);
	insertParam(encoding1, operand1, label1, table);
	insertParam(encoding2, operand2, label2, table);
	return 0;
}

/*handler for all one arg ops other than prn*/
int oneArg(parsingTable* table, char* line, int index)
{
	int operand, encoding = ABS, addressing;
	char label[LABEL_LEN + 1] = { '\0' };
	line = strtok(line, " ");
	while (isspace(*line)) line++;
	if ((operand = parseArg(line)) == ERR) return -1;
	if (operand == LABEL) getLabel(label, line);
	line = strtok(NULL, " ");
	if (line != NULL && *line != '\n') /*extra token after arg*/
	{
		printf("Error: unrecognized token %s found in line %d\n",line, linenum);
		return -1;
	}
	if (operand == LABEL) /*operand is label*/
	{
		addressing = DIRECT;
		encoding = UNKNOWN;
	}
	else if (operand >= r0 && operand <= r7) /*operand is reg*/
	{
		addressing = REGISTER;
		operand = (operand - r0); /*destination register appears in bits 2-7 of additional word (0-5 if we ignore encoding bits)*/
	}
	else /*operand is immediate*/
	{
		printf("Error: immediate value used as a destination in line %d\n", linenum);
		return -1;
	}
	/*insert op and arg*/
	insertOp(index, ABS, addressing, 0, table);
	insertParam(encoding, operand, label, table);
	return 0;
}

/*same as oneArg but can take immediate as destination*/
int oneArgNoDest(parsingTable* table, char* line, int index)
{
	int operand, encoding = ABS, addressing;
	char label[LABEL_LEN + 1] = { '\0' };
	line = strtok(line, " ");
	while (isspace(*line)) line++;
	if ((operand = parseArg(line)) == ERR) return -1;
	if (operand == LABEL) getLabel(label, line);
	line = strtok(NULL, " ");
	if (line != NULL)
	{
		printf("Error: unrecognized token %s found in line %d\n", line, linenum);
		return -1;
	}
	if (operand == LABEL)
	{
		addressing = DIRECT;
		encoding = UNKNOWN;
	}
	else if (operand >= r0 && operand <= r7)
	{
		addressing = REGISTER;
		operand = (operand - r0); /*destination register appears in bits 2-7 of additional word (0-5 if we ignore encoding bits)*/
	}
	else addressing = IMMEDIATE;
	insertOp(index, ABS, addressing, 0, table);
	insertParam(encoding, operand, label,table);
	return 0;
}

/*handler for no arg instructions*/
int noArg(parsingTable* table, char* line, int index)
{
	while (isspace(*line)) line++; /*check that there's no token after op name*/
	if (*line != '\0')
	{
		printf("Error: unrecognized token %s found in line %d\n", line, linenum);
		return -1;
	}
	insertOp(index, ABS, 0, 0, table); /*no arg commands have no addressing information nor any additional words*/
	return 0;
}

/*handles all jmp ops - either single label arg or label with 2 extra args*/
int jmpOps(parsingTable* table, char* line, int index)
{
	int operand1, operand2, operand3, encoding1 = ABS, encoding2 = ABS, addressing;
	char label1[LABEL_LEN + 1] = { '\0' }, label2[LABEL_LEN + 1] = { '\0' }, label3[LABEL_LEN + 1] = { '\0' };
	char* ptr;
	bool switched = false;
	while (isspace(*line)) line++;
	ptr = line;
	while (*ptr != '(' && *ptr != '\0') ptr++;
	if (*ptr == '(') /* make jump label into it's own token*/
	{
		*ptr = '\0';
		switched = true;
	}
	if ((operand1 = parseArg(line)) == ERR) return -1; /*parse jmp label*/
	if (switched) *ptr = '('; /*revert string*/
	if (operand1 != LABEL) /*first jump arg is not a label*/
	{
		printf("Error: attempted to perform jump to a non-label in line %d\n", linenum);
		return -1;
	}
	getLabel(label1, line); /*save name of jump label*/
	if (!switched) /*no switch means no extra 2 args, so only op and jump label arg*/
	{
		insertOp(index, ABS, DIRECT, 0, table);
		insertParam(UNKNOWN, 0, label1, table);
		return 0;
	}
	switched = false;
	line = ptr + 1; /*move to first of 2 extra args*/
	while (isspace(*line)) line++;
	line = strtok(line, ",");
	if ((operand2 = parseArg(line)) == ERR) return -1; /*from here it is basically the same as 2args*/
	if (operand2 == LABEL) getLabel(label2, line);
	line = strtok(NULL, ",");
	if (line == NULL)
	{
		printf("Error: missing second jmp operand in line %d\n", linenum);
		return -1;
	}
	ptr = line;
	while (*ptr != ')' && *ptr != '\0') ptr++;
	if (*ptr == '\0')
	{
		printf("Error: no closing bracket found for jmp with params in line %d\n", linenum);
		return -1;
	}
	if (*ptr == ')')
	{
		*ptr = '\0';
		switched = true;
	}
	if ((operand3 = parseArg(line)) == ERR) return -1;
	if (operand3 == LABEL) getLabel(label3, line);
	if ((line = strtok(NULL, ",")) != NULL)
	{
		printf("Error: unrecognized token %s found in line %d\n", line, linenum);
		return -1;
	}
	while (isspace(*(++ptr)));
	if (*ptr != '\0' && switched)
	{
		printf("Error: unrecognized token %s found in line %d\n", ptr, linenum);
		return -1;
	}
	if (operand2 >= r0 && operand2 <= r7 && operand3 >= r0 && operand3 <= r7)
	{
		insertOp(index, ABS, JMP_PARAMS, (REGISTER << 2) + REGISTER, table);
		insertParam(UNKNOWN, 0, label1, table);
		insertParam(ABS, ((operand2 - r0) << 6) + (operand3 - r0), "", table);
		return 0;
	}
	if (operand2 == LABEL)
	{
		addressing = DIRECT;
		encoding1 = UNKNOWN;
	}
	else if (operand2 >= r0 && operand2 <= r7)
	{
		addressing = REGISTER;
		operand2 = (operand2 - r0) << 6; /*source register appears in bits 8-13 of additional word (6-11 if we ignore encoding bits)*/
	}
	else addressing = IMMEDIATE;
	addressing <<= 2;
	if (operand3 == LABEL)
	{
		addressing += DIRECT;
		encoding2 = UNKNOWN;
	}
	else if (operand3 >= r0 && operand3 <= r7)
	{
		addressing += REGISTER;
		operand3 = (operand3 - r0); /*destination register appears in bits 2-7 of additional word (0-5 if we ignore encoding bits)*/
	}
	else addressing += IMMEDIATE;
	insertOp(index, ABS, JMP_PARAMS, addressing, table);
	insertParam(UNKNOWN, 0, label1, table);
	insertParam(encoding1, operand2, label2, table);
	insertParam(encoding2, operand3, label3, table);
	return 0;
}

int parseArg(char* line)
{
	char* ptr = line;
	while (!isspace(*ptr)) ptr++;
	while (isspace(*ptr)) ptr++;
	if (*ptr != '\0')
	{
		printf("Error: unrecognized token %s in line %d\n", ptr, linenum);
		return ERR;
	}
	if (*line == '#') return parseImmediate(++line);
	if (*line == 'r' && isReserved(line) && isdigit(*(line + 1)))
	{
		return r0 + *(line + 1) - '0';
	}
	if (isName(line,0))
	{
		return LABEL;
	}
	printf("Error: invalid argument %s in line %d\n", line, linenum);
	return ERR;
}

int parseImmediate(char* line)
{
	bool negative = false;
	bool valid = false;
	int num = 0;
	if (*line == '-')
	{
		line++;
		negative = true; /*found minus sign mark as negative*/
	}
	else if (*line == '+') line++; /*skip plus sign*/
	while (isdigit(*line)) /*this can be moved to seperate func*/
	{
		valid = true; /*at least 1 digit*/
		num *= 10; /*decimal <<*/
		num = num + (*line - '0'); /*translate to numeric value*/
		line++;
		if (num > MAX_NUM && !(num == (-1)*MIN_NUM && negative))
		{
			printf("Error: immediate can not be stored within 12 bits in line %d\n", linenum);
			return ERR; /*LABEL code functions as error value*/
		}
	}
	while (isspace(*line)) line++;
	if (*line != '\0' || !valid)
	{
		printf("Error: unrecognized token %s in line %d\n", line, linenum);
		return ERR;
	}
	if (negative) num = (-1) * num;
	return num;
}

int cmdHandler(parsingTable* table, char* line, int index)
{
	if (index == 0 || index == 2 || index == 3 || index == 6) return twoArgs(table, line, index);
	else if (index == 1) return twoArgsNoDest(table, line, index);
	else if (index == 12) return oneArgNoDest(table, line, index);
	else if (index == 4 || index == 5 || index == 7 || index == 8 || index == 11) return oneArg(table, line, index);
	else if (index == 14 || index == 15) return noArg(table, line, index);
	else if (index == 9 || index == 10 || index == 13) return jmpOps(table, line, index);
	else
		return ERR;
}

typedef int (*f)(parsingTable, char*, int);
#undef _CRT_SECURE_NO_WARNINGS