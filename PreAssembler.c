#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include "ParseTool.h"
#include "Allocator.h"

#define MACRO_START "mcr"
#define MACRO_END "endmcr"
#define SYNTAX_ERR -1
#define IN_MACRO 1
#define OUT_MACRO 0
#define ERR -1

typedef struct macroTable{
	char** table;
	size_t* size_table;
	size_t count;
	size_t space;
} macroTable;

unsigned linenum;
/*prototype block*/
int preAssemble(char* fname);
int parseMacro(char* line, FILE* out, int inMacro, macroTable* table);
size_t trimLine(char* line, size_t len);
size_t trimWhitespaces(char* line, size_t len);
void registerMacro(macroTable* table, char* name);
void putMacro(int index, macroTable* table, FILE* out);
int findMacro(char* name, macroTable* table);
void init_macroTable(macroTable* table);
void del_macroTable(macroTable* table);
void appendMacro(char* line, macroTable* table);
int startMacro(size_t len, int inMacro, macroTable* table, char* token);
int endMacro(size_t len, int inMacro, macroTable* table, char* token);
/*prototype block*/

/*reads lines from input file and sends them to be parsed*/
int preAssemble(char* fname)
{
	int i = 0;
	macroTable table;
	char line[81] = { '\0' };
	int inMacro = OUT_MACRO;
	bool error = false;
	FILE* in, * out;
	if ((in = fopen(fname, "r")) == NULL) /*open input file*/
	{
		printf("Error: couldn't open %s for reading, skipping\n", fname);
		return -1;
	}
	fname[strlen(fname) - 1] = 'm';
	if ((out = fopen(fname, "w")) == NULL) /*open macro output*/
	{
		printf("Error: couldn't create output file for pre-assembly of %s, skipping\n", fname);
		return -1;
	}
	fname[strlen(fname) - 1] = 's';
	init_macroTable(&table);
	linenum = 1;
	while ((fgets(line, 81, in)) != NULL)
	{
		i++;
		printf("%d\n", i);
		printf("%s", line);
		if ((inMacro = parseMacro(line, out, inMacro, &table)) == ERR) error = true;
		printf("finished\n");
		linenum++;
	}
	if (inMacro == IN_MACRO) error = true; /*number of macro closers doesn't match number of macro openers*/
	del_macroTable(&table);
	fclose(in);
	fclose(out);
	if (error) return ERR;
	return 0;
}

/*parses lines and pre-assembles them - trims, drops comments and empty lines, unpacks macros*/
int parseMacro(char* line, FILE* out, int inMacro, macroTable* table)
{
	char* token, *lineCpy;
	int index = -1;
	int len = strlen(line);
	len = trimLine(line, len);
	trimWhitespaces(line, len);
	line[len] = '\0'; /*null terminate - guaranteed to be allocated - because new len <= old len*/
	if (*line == ';' || *line == '\n') return inMacro;
	lineCpy = (char*)myMalloc(len + 2);
	strcpy(lineCpy, line);
	if (lineCpy[len - 1] == '\n') lineCpy[len - 1] = '\0';
	token = strtok(lineCpy, " ");
	if ((inMacro == OUT_MACRO && (inMacro = startMacro(len, inMacro, table, token)) == IN_MACRO) || inMacro == ERR)
	{
		free(lineCpy);
		return inMacro; /*if macro is started inMacro becomes true*/
	}
	else if ((inMacro == IN_MACRO && (inMacro = endMacro(len, inMacro, table, token)) == OUT_MACRO) || inMacro == ERR)
	{
		free(lineCpy);
		return inMacro; /*if macro is ended inMacro becomes false*/
	}
	else if (inMacro)
	{
		appendMacro(line, table); /*add line to macro definition*/
	}
	else if ((index = findMacro(token, table)) != -1) /*found macro name outside of macro*/
	{
		putMacro(index, table, out); /*unpack macro*/
	}
	else
	{
		fprintf(out, "%s", line); /*no macro write to output file as is*/
	}
	free(lineCpy);
	return inMacro;
}

/*trim leading and ending whitespaces*/
size_t trimLine(char* line, size_t len)
{
	int i = 0, start = 0, end = 0, newLen = 0;
	char* newLine;
	newLine = (char*)myMalloc(len + 1);
	while (i < len && isspace(line[i])) /*Get first index of non-whitespace*/
	{
		i++;
	}
	start = i;
	i = len - 1;
	while (i >= start && isspace(line[i])) /*Get last index on non-whitespace, if only whitespaces end is start-1*/
	{
		i--;
	}
	end = i;
	newLen = end - start + 1; /*length of trimmed string*/
	strncpy(newLine, line + start, newLen);
	newLine[newLen] = '\0';
	strncpy(line, newLine, newLen + 1);
	line[newLen] = '\n'; /*Put newline at new end of line*/
	line[newLen + 1] = '\0';
	free(newLine); /*de-allocate*/
	return newLen + 1; /*Ret length including newline char*/
}

/*trim whitespace sequences into a single regular space*/
size_t trimWhitespaces(char* line, size_t len)
{
	int i = 0, j = 0;
	bool inSeq = false;
	char* newLine;
	newLine = (char*)myMalloc(len + 1);
	for (; i < len; i++)
	{
		if (line[i] == '\n') break;
		else if (isspace(line[i]) && !inSeq)
		{
			inSeq = true;
			newLine[j] = ' ';
			j++;
		}
		else if (isspace(line[i]))
		{
			continue;
		}
		else if (inSeq)
		{
			inSeq = false;
			newLine[j] = line[i];
			j++;
		}
		else
		{
			newLine[j] = line[i];
			j++;
		}
	}
	newLine[j] = '\n'; /*add newline*/
	newLine[j + 1] = '\0'; /*add null terminator*/
	strncpy(line, newLine, j + 2); /*copy including null terminator*/
	free(newLine); /*de-allocate*/
	return j + 1; /*Ret length including newline char*/
}

/*add macro into macro table*/
void registerMacro(macroTable* table, char* name)
{
	char** newTable;
	if (table->space == table->count)
	{
		newTable = (char**)myRealloc(table->table, table->space * 2 * sizeof(char*));
		table->table = newTable;
		table->size_table = (size_t*)myRealloc(table->size_table, table->space * 2 * sizeof(size_t));
		table->space = table->space * 2;
	}
	/*strlen + 2 is used to place 2 null chars at the end of the string
	 *the logic being that the macro name is delimited from the macro itself by a null
	 * and the macro string itself is null terminated as well*/
	table->size_table[table->count] = strlen(name) + 2;
	table->table[table->count] = (char*)myMalloc(strlen(name) + 2);
	strncpy(table->table[table->count], name, strlen(name) + 2);
}

/*add line to registered macro*/
void appendMacro(char* line, macroTable* table)
{
	size_t len = strlen(line);
	size_t arr_index = table->count;
	int count = 0;
	int i = 0;
	table->table[arr_index] = (char*)myRealloc(table->table[arr_index], table->size_table[arr_index] + len);
	table->size_table[arr_index] += len;
	while (count != 2) /*look for second null terminator*/
	{
		if (table->table[arr_index][i] == '\0') count++; 
		i++;
	}
	i--;
	strcpy(table->table[arr_index] + i, line); /*copy over second null terminator re-adding a new one at the end*/
}

/*print macro to file instead of it's name*/
void putMacro(int index, macroTable* table, FILE* out)
{
	int i = 0;
	while ((table->table[index][i] != '\0')) /*find delimiter null terminator*/
	{
		i++;
	}
	i++;
	fprintf(out, "%s", table->table[index] + i);
}

/*search for macro name in table by going through every entry in it*/
/*since the macro name is delimited from the macro itself using a null terminator, strcmp can safely be used to check the name*/
int findMacro(char* name, macroTable* table)
{
	int i = 0;
	for (; i < table->count; i++)
	{
		if (!strcmp(table->table[i], name))
		{
			return i;
		}
	}
	return -1;
}

/*int table with space for 1 macro*/
void init_macroTable(macroTable* table)
{
	table->count = 0;
	table->space = 1;
	table->size_table = (size_t*)myMalloc(sizeof(size_t));
	table->table = (char**)myMalloc(sizeof(char*));
}

/*free all table dynamic alloc*/
void del_macroTable(macroTable* table)
{
	int i = 0;
	free(table->size_table);
	for (; i < table->count; i++)
	{
		free(table->table[i]);
	}
	free(table->table);
}

/*check for macro start*/
int startMacro(size_t len, int inMacro, macroTable* table, char* token)
{
	if (len > strlen(MACRO_START) && !strcmp(token, MACRO_START)) /*check for macro start*/
	{
		if (inMacro) /*macro start within a macro definition*/
		{
			printf("Error: found nested macro definition in line %d\n", linenum);
			return ERR;
		}
		inMacro = IN_MACRO;
		token = strtok(NULL, " ");
		if (token == NULL) return inMacro;
		if (!isName(token,0))
		{
			printf("Error: illegal macro name %s used in line %d\n", token, linenum);
			return ERR;
		}
		else registerMacro(table, token);
		if (token != NULL && strtok(NULL, " ") != NULL) /*if there is an extra token other than macro name*/
		{
			printf("Error: unrecognized token found in macro definition in line %d\n", linenum);
			return ERR;
		}
	}
	return inMacro;
}

/*check for macro end*/
int endMacro(size_t len, int inMacro, macroTable* table, char* token)
{
	if (len > strlen(MACRO_END) && !strcmp(token, MACRO_END)) /*check for macro end*/
	{
		if (!inMacro)
		{
			printf("Fatal error: found macro end outside of macro in line %d\n", linenum);
			return ERR;
		}
		inMacro = OUT_MACRO; /*macro end*/
		token = strtok(NULL, " ");
		if (token != NULL) /*since the line was trimmed no more tokens should be found*/
		{
			printf("Fatal error: unrecognized token found in macro definition in line %d\n", linenum);
			return ERR;
		}
		table->count++; /*macro definition finished inc macro count*/
	}
	return inMacro;
}
#undef _CRT_SECURE_NO_WARNINGS