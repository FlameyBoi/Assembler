/*specialized variant of in function found in ParseTool.c for symbols*/
#define _CRT_SECURE_NO_WARNINGS
#include "ParsingTable.h"
#include "Allocator.h"
#include <string.h>

bool insym(char* name, symbol* arr, size_t len) /*check existence of symbol name*/
{
	int i = 0;
	for (; i < len; i++)
	{
		if (!strcmp(name, arr[i].name) && arr[i].t != en && arr[i].t != ex)
			return true;
	}
	return false;
}

/*specialized variant of index function found in ParseTool.c for symbols*/
int symindex(char* name, parsingTable* table) /*check existence and ret index*/
{
	int i = 0;
	for (; i < table->scount; i++)
	{
		if (!strcmp(name, (table->symbols)[i].name) && (table->symbols)[i].t != en)
			return i;
	}
	return -1; /*doesn't exist*/
}

void init_parsingTable(parsingTable* table)
{
	table->cspace = 1;
	table->sspace = 1;
	table->dspace = 1;
	table->ccount = 0;
	table->scount = 0;
	table->dcount = 0;
	table->symbols = (symbol*)myMalloc(sizeof(symbol));
	table->data_array = (data*)myMalloc(sizeof(data));
	table->cmd_array = (command*)myMalloc(sizeof(command));
}

void del_parsingTable(parsingTable* table)
{
	int i;
	for (i = 0; i < table->scount; i++)
		free((table->symbols[i]).name);
	free(table->symbols);
	free(table->data_array);
	for (i = 0; i < table->scount; i++)
		free((table->cmd_array[i]).label);
	free(table->cmd_array);
}
#undef _CRT_SECURE_NO_WARNINGS