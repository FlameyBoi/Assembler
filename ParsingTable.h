#include <stdlib.h>
#include <stdbool.h>

typedef struct data {
	int val : 14;
} data;

typedef enum type { d, s, en, ex, cmd, na } type; /* data, string, entry, extern, command, n/a (place holder) */

typedef struct command {
	int val : 14;
	type t;
	char* label;
} command;

typedef struct symbol {
	char* name;
	type t;
	int index; /*depending on type this points to either an index in data array or cmd_array*/
} symbol;

typedef struct parsingTable {
	symbol* symbols; /*symbols*/
	size_t scount;
	size_t sspace;
	data* data_array; /*data*/
	size_t dcount; /*DC*/
	size_t dspace;
	command* cmd_array; /*commands*/
	size_t ccount; /*IC*/
	size_t cspace;
} parsingTable;

/*specialized variant of in function found in ParseTool.c for symbols*/
bool insym(char* name, symbol* arr, size_t len); /*check existence of symbol name*/

/*specialized variant of index function found in ParseTool.c for symbols*/
int symindex(char* name, parsingTable* table); /*check existence and ret index*/

void init_parsingTable(parsingTable* table);

void del_parsingTable(parsingTable* table);