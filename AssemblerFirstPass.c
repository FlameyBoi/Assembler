#define _CRT_SECURE_NO_WARNINGS
#include <errno.h>
#include "Allocator.h"
#include "ParseTool.h"
#include <string.h>
#include <ctype.h>
#include "AssemblerSecondPass.h"
#include "defs.h"

bool error;
int linenum;
int cmdHandler(parsingTable* table, char* line, int index); /*from CommandHandler.c*/

/*prototype block*/
int firstPass(char* fname);
void parseLine(char* line, int len, parsingTable* table);
int findLabel(char* line, size_t len);
int parseDir(char* line, int index, char* label, parsingTable* table);
int parseData(char* lineCpy, parsingTable* table);
int parseString(char* lineCpy, parsingTable* table);
void insertData(int num, parsingTable* table);
int insertDataSymbol(char* label, parsingTable* table);
int insertCmdSymbol(char* label, parsingTable* table);
int insertExtSym(char* label, parsingTable* table);
int insertEntSym(char* label, parsingTable* table);
int parseCmd(char* line, int index, char* label, parsingTable* table);
/*prototype block*/

/*main function of first pass, creates parsing table, calls parseLine on each line and if no erros are encountered calls second pass*/
int firstPass(char* fname)
{
	char line[81] = { '\0' };
	char fullName[256] = { '\0' };
	size_t len = 0;
	FILE* obj, * ext, * ent, * input;
	parsingTable table;
	fname[strlen(fname) - 1] = 'm';
	if ((input = fopen(fname, "r")) == NULL) /*open input file*/
	{
		printf("Error: couldn't open %s for reading after pre-assembly\n", fname);
		return -1;
	}
	fname[strlen(fname) - 1] = 's';
	init_parsingTable(&table);
	error = false;
	linenum = 1;
	while ((fgets(line, 81, input)) != NULL)
	{
		parseLine(line, strlen(line), &table); /* len doesn't include the null terminator */
		linenum++;
	}
	if (error)
	{
		del_parsingTable(&table);
		return -1;
	}
	strcpy(fullName, fname);
	len = strlen(fullName) - 3; /*where extension will start*/
	fullName[len] = '\0';
	strcat(fullName, ".ob"); /*add ob extension*/
	obj = fopen(fullName, "w");
	fullName[len] = '\0';
	strcat(fullName, ".ent"); /*add ent extension*/                        
	ent = fopen(fullName, "w");
	fullName[len] = '\0';
	strcat(fullName, ".ext"); /*add ext extension*/
	ext = fopen(fullName, "w");
	if (!obj || !ent || !ext) /*if one of the files couldn't be opened/created*/
	{
		error = true;
		printf("Error: couldn't create one or more of the output files\n");
		return -1;
	}
	if (table.ccount + table.dcount > MEM_SIZE)
	{
		printf("Error: image size of resulting object file is greater than the allowed memory size\n");
		del_parsingTable(&table);
		error = true;
		return -1;
	}
	if (secondPass(&table, obj, ext, ent)) error = true; /*if second pass failed*/
	del_parsingTable(&table);
	if (!ftell(ext)) /*if .ext file is empty delete it*/
	{
		fullName[len] = '\0';
		strcat(fullName, ".ext");
		fclose(ext);
		remove(fullName);
	}
	else fclose(ext);
	if (!ftell(ent)) /*if .ent file is empty delete it*/
	{
		fullName[len] = '\0';
		strcat(fullName, ".ent");
		fclose(ent);
		remove(fullName);
	}
	else fclose(ent);
	fclose(obj);
	fclose(input);
	if (error) return -1;
	return 0;
}

/*checks for label and calls directive/command handler according to first word after label if it exists*/
void parseLine(char* line, int len, parsingTable* table)
{
	char label[LABEL_SIZE + 1] = { '\0' };
	int offset = 0, index;
	offset = findLabel(line, len);
	if (offset == -1) return; /*error found move on to next line*/
	strncpy(label, line, offset);
	line = line + offset;
	if (label[0] != '\0') /*got label*/
	{
		label[offset] = '\0';
		offset = spaceAdvance(line + 1) + 1; /* line + offset points to ':' so at least 1 char is advanced*/
		if ((index = isDir(line + offset)) != -1) /*is directive*/
		{
			parseDir(line + offset, index, label, table);
		}
		else if ((index = isCmd(line + offset)) != -1) /*is command*/
		{
			parseCmd(line + offset, index, label, table);
		}
		else
		{
			printf("Error: unknown op in line %d\n", linenum);
			error = true;
		}
	}
	else /*no label*/
	{
		if ((index = isDir(line)) != -1) /*is directive*/
		{
			parseDir(line, index, "", table);
		}
		else if ((index = isCmd(line + offset)) != -1) /*is command*/
		{
			parseCmd(line, index, "", table);
		}
		else
		{
			printf("Error: unknown op in line %d\n", linenum);
			error = true;
		}
	}
}

/*checks for label, including legality check*/
int findLabel(char* line, size_t len)
{
	int i = 0;
	for (; i != len && line[i] != '\0'; i++)
	{
		if (line[i] == ':') break;
	}
	if (line[i] != ':') return 0; /*No label*/
	if (i > LABEL_SIZE || !isName(line,i)) /*if illegal label - too long or incorrectly formatted*/
	{
		printf("Error: illegal label in line %d\n", linenum);
		error = true;
		return -1;
	}
	return i;
}

/*handles parsing for directive line*/
int parseDir(char* line, int index, char* label, parsingTable* table) 
{
	char* lineCpy;
	char* token;
	lineCpy = (char*)myMalloc(strlen(line) + 1);
	strcpy(lineCpy, line);
	token = strtok(lineCpy, " ");
	if (strcmp(DIRECTIVES[index], token))
	{
		printf("Error: unrecognized token %s in line %d\n", token, linenum);
		error = true;
		free(lineCpy);
		return -1;
	}
	if (index == DATA)
	{
		if(insertDataSymbol(label, table)) return -1;
		if ((token = strtok(NULL, " ")) == NULL || *token == '\0')
		{
			printf("Error: no data to parse in line %d\n", linenum);
			error = true;
			free(lineCpy);
			return -1;
		}
		if (parseData(line, table)) 
		{
			error = true;
			free(lineCpy);
			return -1;
		}

	}
	else if (index == STRING)
	{
		if (insertDataSymbol(label, table)) return -1;
		table->symbols[table->scount - 1].t = s; /*change to string from regular data*/
		if ((token = strtok(NULL, " ")) == NULL)
		{
			printf("Error: no string to parse in line %d\n", linenum);
			error = true;
			free(lineCpy);
			return -1;
		}
		if (parseString(token, table))
		{
			error = true;
			free(lineCpy);
			return -1;
		}
	}
	else if (index == EXTERN)
	{
		token = strtok(NULL, " ");
		if ((strtok(NULL, " ")) != NULL)
		{
			printf("Error: unrecognized token in line %d\n", linenum);
			error = true;
			return -1;
		}
		if (insertExtSym(token, table))
		{
			free(lineCpy);
			return -1;
		}
	}
	else if (index == ENTRY)
	{
		token = strtok(NULL, " ");
		if ((strtok(NULL, " ")) != NULL)
		{
			printf("Error: unrecognized token in line %d\n", linenum);
			error = true;
			return -1;
		}
		if (insertEntSym(token, table))
		{
			free(lineCpy);
			return -1;
		}
	}
	else
	{
		printf("Fatal error: something went very wrong\n");
		free(lineCpy);
		exit(-1);
	}
	return 0;
}

/*parses a list of number following the .data directive*/
int parseData(char* lineCpy, parsingTable* table)
{
	bool negative = false;
	bool valid = false;
	int num = 0;
	while (*lineCpy != '\0' && !isdigit(*lineCpy) && *lineCpy != '-') lineCpy++; /*move to first number*/
	if (*lineCpy == '\0')
	{
		printf("Error: empty data directive in line %d\n", linenum);
		error = true;
		return -1;
	}
	lineCpy = strtok(lineCpy, ",");
	table->symbols[table->scount - 1].t = d; /*symbol of type data*/
	while (lineCpy != NULL) /*while not out of tokens*/
	{
		num = 0;
		while (isspace(*lineCpy)) lineCpy++;
		if (*lineCpy == '-')
		{
			negative = true;
			lineCpy++;
		}
		else if (*lineCpy == '+') lineCpy++;
		while (isdigit(*lineCpy)) /*this can be moved to seperate func*/
		{
			valid = true; /*at least 1 digit*/
			num *= 10; /*decimal <<*/
			num = num + (*lineCpy - '0'); /*translate to numeric value*/
			lineCpy++;
			if (num > MAX_DATA && !(num == (-1)*MIN_DATA && negative))
			{
				printf("Error: data can not be stored within 14 bits in line %d\n", linenum);
				error = true;
				return -1;
			}
		}
		while (isspace(*lineCpy)) lineCpy++;
		if (*lineCpy != '\0')
		{
			printf("Error: unrcognized token %s in line %d\n", lineCpy, linenum);
			error = true;
			return -1;
		}
		if (!valid)
		{
			printf("Error: empty initializer in line %d\n", linenum);
			error = true;
			return -1;
		}
		if (negative) num = (-1) * num;
		insertData(num, table);
		lineCpy = strtok(NULL, ",");
	}
	return 0;
}

/*parses string following .string directive*/
int parseString(char* lineCpy, parsingTable* table)
{
	char* token = lineCpy;
	if (*lineCpy != '"')
	{
		printf("Error: unrecognized token %s in line %d\n", lineCpy, linenum);
		error = true;
		return -1;
	}
	lineCpy++;
	while (*lineCpy != '"' && *lineCpy != '\0') /*while not out of tokens*/
	{
		insertData(*lineCpy, table); /*put char in data array*/
		lineCpy++;
	}
	if (*lineCpy == '\0' || (lineCpy = strtok(NULL, " ")) != NULL)
	{
		printf("Error: unrecognized token %s in line %d\n", token, linenum);
		error = true;
		return -1;
	}
	insertData(0, table); /*string null terminator*/
	return 0;
}

/*put data in data array*/
void insertData(int num, parsingTable* table)
{
	void* ptr;
	if (table->dcount == table->dspace) /*alloc if out of space*/
	{
		ptr = myRealloc(table->data_array, table->dspace * 2 * (sizeof(data)));
		table->data_array = (data*)ptr;
		table->dspace *= 2;
	}
	table->data_array[table->dcount].val = num;
	table->dcount++;
}

/*put symbol belonging to a line with a .data or .string directive*/
int insertDataSymbol(char* label, parsingTable* table)
{
	void* ptr;
	if (!strcmp(label, "")) return 0; /*no label*/
	if (insym(label, table->symbols, table->scount)) /*if this name exists already*/
	{
		printf("Error: label redeclared in line %d\n", linenum);
		error = true;
		return -1;
	}
	if (table->scount == table->sspace) /*alloc if out of space for symbols*/
	{
		ptr = myRealloc(table->symbols, table->sspace * 2 * (sizeof(symbol)));
		table->symbols = (symbol*)ptr;
		table->sspace *= 2;
	}
	table->symbols[table->scount].name = (char*)myMalloc(LABEL_SIZE + 1); /*alloc char array for symbol name*/
	strcpy(table->symbols[table->scount].name, label);
	table->symbols[table->scount].t = d; /*mark as type data*/
	table->symbols[table->scount].index = table->dcount; /*point symbol to next data*/
	table->scount++;
	return 0;
}

/*put symbol belonging to a line with a regular instruction*/
int insertCmdSymbol(char* label, parsingTable* table)
{
	void* ptr;
	if (!strcmp(label, "")) return 0; /*no label*/
	if (insym(label, table->symbols, table->scount)) /*if this name exists already*/
	{
		printf("Error: label redeclared in line %d\n", linenum);
		error = true;
		return -1;
	}
	if (table->scount == table->sspace) /*alloc if out of space*/
	{
		ptr = myRealloc(table->symbols, table->sspace * 2 * (sizeof(symbol)));
		table->symbols = (symbol*)ptr;
		table->sspace *= 2;
	}
	table->symbols[table->scount].name = (char*)myMalloc(LABEL_SIZE + 1);
	strcpy(table->symbols[table->scount].name, label);
	table->symbols[table->scount].t = cmd; /*mark symbol as cmd type*/
	table->symbols[table->scount].index = table->ccount; /*point symbol to next instruction*/
	table->scount++;
	return 0;
}

/*put symbol belonging to a line with the extern directive*/
int insertExtSym(char* label, parsingTable* table)
{
	void* ptr;
	int index;
	if (label == NULL || !strcmp(label, ""))
	{
		printf("Error: external directive without identifier in line %d\n", linenum);
		error = true;
		return -1;
	}
	index = symindex(label, table);
	if (!isName(label, 0)) 
	{
		printf("Error: illegal name in line %d\n", linenum);
		error = true;
		return -1;
	}
	if (index != -1) /*if this name already exists*/
	{
		printf("Error: label %s declared multiple times\n", label);
		error = true;
		return -1;

	}
	if (index != -1 && table->symbols[index].t == ex) return 0; /*ignore repeated extern directives*/
	if (table->scount == table->sspace) /*alloc if out of space*/
	{
		ptr = myRealloc(table->symbols, table->sspace * 2 * (sizeof(symbol)));
		table->symbols = (symbol*)ptr;
		table->sspace *= 2;
	}
	table->symbols[table->scount].name = (char*)myMalloc(LABEL_SIZE + 1);
	if (label[strlen(label) - 1] == '\n') label[strlen(label) - 1] = '\0';
	strcpy(table->symbols[table->scount].name, label);
	table->symbols[table->scount].t = ex; /*mark symbol as type extern*/
	table->symbols[table->scount].index = -1; /*index is meaningless for extern symbols*/
	table->scount++;
	return 0;
}

/*put symbol belonging to a line with the entry directive*/
int insertEntSym(char* label, parsingTable* table)
{
	void* ptr;
	int index; 
	if (label == NULL || !strcmp(label, ""))
	{
		printf("Error: entry directive without identifier in line %d\n", linenum);
		error = true;
		return -1;
	}
	index = symindex(label, table);
	if (!isName(label, 0))
	{
		printf("Error: illegal name in line %d\n", linenum);
		error = true;
		return -1;
	}
	if (index != -1 && table->symbols[index].t == ex) /*if this name already exists as an extern symbol*/
	{
		printf("Error: label %s declared as both entry and extern\n", label);
		error = true;
		return -1;

	}
	if (index != -1 && table->symbols[index].t == en) return 0; /*ignore repeated entry directives*/
	if (table->scount == table->sspace) /*alloc if out of space*/
	{
		ptr = myRealloc(table->symbols, table->sspace * 2 * (sizeof(symbol)));
		table->symbols = (symbol*)ptr;
		table->sspace *= 2;
	}
	table->symbols[table->scount].name = (char*)myMalloc(LABEL_SIZE + 1);
	if (label[strlen(label) - 1] == '\n') label[strlen(label) - 1] = '\0';
	strcpy(table->symbols[table->scount].name, label);
	table->symbols[table->scount].t = en; /*mark symbol as type entry*/
	table->symbols[table->scount].index = -1; /*index is meaningless for entry symbols*/
	table->scount++;
	return 0;
}

/*handles parsing of lines with regular instructions*/
int parseCmd(char* line, int index, char* label, parsingTable* table)
{
	char* lineCpy, * end;
	lineCpy = (char*)myMalloc(strlen(line) + 1); /*make copy of line*/
	strcpy(lineCpy, line);
	end = lineCpy;
	while (!isspace(*end) && *end != '\0') end++;
	*end = '\0';
	if (strcmp(OPCODE[index], lineCpy)) /*check that first token matches a command*/
	{
		printf("Error: unrecognized token %s in line %d\n", lineCpy, linenum);
		error = true;
		free(lineCpy);
		return -1;
	}
	insertCmdSymbol(label, table); /*insert related symbol*/
	while (isspace(*line)) line++;
	line = line + strlen(OPCODE[index]); /*skip over op name*/
	if (cmdHandler(table, line, index) == -1) /*call op handlers - checks proper syntax - arg types, number and makes entries in data array*/
	{
		error = true;
		free(lineCpy);
		return -1;
	}
	free(lineCpy);
	return 0;
}
#undef _CRT_SECURE_NO_WARNINGS