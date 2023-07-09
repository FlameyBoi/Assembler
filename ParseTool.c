#define _CRT_SECURE_NO_WARNINGS
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include "ParseTool.h"
#include "Allocator.h"
#include <string.h>

char OPCODE[OPCODE_NUM][5] = { "mov", "cmp", "add", "sub",
					"not", "clr", "lea", "inc",
					"dec", "jmp", "bne", "red",
					"prn", "jsr", "rts", "stop" };

char DIRECTIVES[DIR_NUM][8] = { ".data", ".string", ".entry", ".extern" };

bool inDIR(char* name) /*check existence*/
{
	int i = 0;
	char* nameCpy, * end;
	nameCpy = (char*)myMalloc(strlen(name) + 1);
	strcpy(nameCpy, name);
	end = nameCpy;
	while (!isspace(*end) && *end != '\0') end++;
	*end = '\0';
	for (; i < DIR_NUM; i++)
	{
		if (!strcmp(nameCpy, DIRECTIVES[i]))
		{
			free(nameCpy);
			return true;
		}
	}
	free(nameCpy);
	return false; /*doesn't exist*/
}

bool inOP(char* name) /*check existence*/
{
	int i = 0;
	char* nameCpy, * end;
	nameCpy = (char*)myMalloc(strlen(name) + 1);
	strcpy(nameCpy, name);
	end = nameCpy;
	while (!isspace(*end) && *end != '\0') end++;
	*end = '\0';
	for (; i < OPCODE_NUM; i++)
	{
		if (!strcmp(nameCpy, OPCODE[i]))
		{
			free(nameCpy);
			return true;
		}
	}
	free(nameCpy);
	return false; /*doesn't exist*/
}

bool inREG(char* name) /*check existence*/
{
	if (*name == 'r' && *(name + 1) >= '0' && *(name + 1) <= '7') return true;
	return false; 
}

int indexOP(char* name) /*check existence and ret index*/
{
	int i = 0;
	char* nameCpy, * end;
	nameCpy = (char*)myMalloc(strlen(name) + 1);
	strcpy(nameCpy, name);
	end = nameCpy;
	while (!isspace(*end) && *end != '\0') end++;
	*end = '\0';
	for (; i < OPCODE_NUM; i++)
	{
		if (!strcmp(nameCpy, OPCODE[i]))
		{
			free(nameCpy);
			return i;
		}
	}
	free(nameCpy);
	return -1; /*doesn't exist*/
}

int indexDIR(char* name) /*check existence and ret index*/
{
	int i = 0;
	char* nameCpy, * end;
	nameCpy = (char*)myMalloc(strlen(name) + 1);
	strcpy(nameCpy, name);
	end = nameCpy;
	while (!isspace(*end) && *end != '\0') end++;
	*end = '\0';
	for (; i < DIR_NUM; i++)
	{
		if (!strcmp(nameCpy, DIRECTIVES[i]))
		{
			free(nameCpy);
			return i;
		}
	}
	free(nameCpy);
	return -1; /*doesn't exist*/
}

bool isReserved(char* name)
{
	return (inOP(name) || inDIR(name) || inREG(name));
}

bool isName(char* name, int lim)
{
	int i = 0;
	if (isReserved(name)) return false;
	do
	{
		if (!isalnum(name[i])) return false;
		i++;
	} while (name[i] != '\0' && !isspace(name[i]) && i != lim);
	if (i == lim) return true;
	while (isspace(name[i])) i++;
	if (name[i] != '\0') return false;
	return true;
}

int spaceAdvance(char* line) /*return offset to next non-whitespace*/
{
	int i = 0;
	while (isspace(line[i]) && line[i] != '\0') i++;
	return i;
}

int isCmd(char* name)
{
	while (isspace(*name)) name++;
	return indexOP(name);
}

int isDir(char* name)
{
	while (isspace(*name)) name++;
	return indexDIR(name);
}

void getLabel(char* dest, char* src)
{
	while (isalnum(*src))
	{
		*dest = *src;
		dest++;
		src++;
	}
}
#undef _CRT_SECURE_NO_WARNINGS