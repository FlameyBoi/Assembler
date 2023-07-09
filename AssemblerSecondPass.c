#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include "ParsingTable.h"
#include "defs.h"

bool error;

/*prototype block*/
int secondPass(parsingTable* table, FILE* obj, FILE* externals, FILE* entries);
void toUniqueBin(int val, char* arr, int bits);
int printInstructions(parsingTable* table, FILE* obj, FILE* externals);
/*prototype block*/

/*handles everything related to second pass*/
int secondPass(parsingTable* table, FILE* obj, FILE* externals, FILE* entries)
{
	int i, index;
	char line[WORD_SIZE + 1] = { '\0' };
	error = false;
	toUniqueBin(table->ccount, line, 8);
	fprintf(obj, "%s ", line);
	toUniqueBin(table->dcount, line, 8);
	fprintf(obj, "%s\n", line);

	/*resolve symbols, print instructions and arguments ,print uses of external symbols in .ext*/
	if (printInstructions(table, obj, externals) == -1) return -1;
	for (i = 0; i < table->dcount; i++) /*print all data*/
	{
		toUniqueBin(BASE_ADD + i + table->ccount, line, ADD_SIZE); /*address would be 100 + IC + (index of data)*/
		fprintf(obj, "%s", line);
		toUniqueBin(table->data_array[i].val, line, WORD_SIZE);
		fprintf(obj, "\t%s\n", line);
	}

	for (i = 0; i < table->scount; i++) /*check that all entry directives use declared symbols and print them into .ent*/
	{
		if (table->symbols[i].t == en)
		{
			index = symindex(table->symbols[i].name, table);
			if (index == -1)
			{
				printf("Error: encountered entry of undeclared label %s\n", table->symbols[i].name);
				error = true;
				continue;
			}
			if(table->symbols[index].t == cmd) toUniqueBin(table->symbols[index].index + BASE_ADD, line, ADD_SIZE);
			else toUniqueBin(table->symbols[index].index + table->ccount + BASE_ADD, line, ADD_SIZE);
			fprintf(entries, "%s\t%s\n", table->symbols[i].name, line);
		}
	}
	if (error) return -1;
	return 0;
}

/*translate nunmber with specified number of bits to uniqueBinary string*/
void toUniqueBin(int val, char* arr, int bits)
{
	int i = 0;
	for (; i < bits; i++)
	{
		if (val & 1)
			arr[bits - i - 1] = '/'; /*/ is 1*/
		else
			arr[bits - i - 1] = '.'; /*. is 0*/
		val >>= 1;
	}
	arr[bits] = '\0'; /*make sure only relevant part is printed*/
}

/*resolve symbols, print instruction segment to obj and externals to ext*/
int printInstructions(parsingTable* table, FILE* obj, FILE* externals)
{
	int i, index;
	char line[15] = { '\0' };
	for (i = 0; i < table->ccount; i++)
	{
		if (strcmp(table->cmd_array[i].label, "")) /*if instruction has associated symbol*/
		{
			index = symindex(table->cmd_array[i].label, table); /*find symbol*/
			if (index == -1) /*no symbol found*/
			{
				printf("Error: unresolved symbol %s found\n", table->cmd_array[i].label);
				error = true;
			}
			else if (table->symbols[index].t == ex) /*external symbol found*/
			{
				table->cmd_array[i].val = EXT;
				toUniqueBin(BASE_ADD + i, line, ADD_SIZE);
				fprintf(externals, "%s\t%s\n", table->cmd_array[i].label, line);
				fprintf(obj, "%s", line);
				toUniqueBin(table->cmd_array[i].val, line, WORD_SIZE);
				fprintf(obj, "\t%s\n", line);
			}
			else /*non-external symbol*/
			{
				if (table->symbols[index].t == d || table->symbols[index].t == s) index = table->symbols[index].index + table->ccount;
				else index = table->symbols[index].index; /*value to replace placeholder*/
				table->cmd_array[i].val = ((index + BASE_ADD)<< 2) + REL;
				toUniqueBin(BASE_ADD + i, line, ADD_SIZE);
				fprintf(obj, "%s", line);
				toUniqueBin(table->cmd_array[i].val, line, WORD_SIZE);
				fprintf(obj, "\t%s\n", line);
			}
		}
		else /*no associated symbol*/
		{
			toUniqueBin(BASE_ADD + i, line, ADD_SIZE);
			fprintf(obj, "%s", line);
			toUniqueBin(table->cmd_array[i].val, line, WORD_SIZE);
			fprintf(obj, "\t%s\n", line);
		}
	}
	if (error) return -1;
	return 0;
}
#undef _CRT_SECURE_NO_WARNINGS