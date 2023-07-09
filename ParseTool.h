#include <stdbool.h>

#define OPCODE_NUM 16
#define DIR_NUM 4
#define LABEL_SIZE 30
#define DATA 0
#define STRING 1
#define ENTRY 2
#define EXTERN 3
#define MAX_DATA 8191
#define MIN_DATA -8192

extern char OPCODE[OPCODE_NUM][5];
extern char DIRECTIVES[DIR_NUM][8];

bool inOP(char* name);
bool inDIR(char* name);
bool isReserved(char* name);
bool isName(char*,int);
int isDir(char*);
int isCmd(char*);
int spaceAdvance(char* line);
void getLabel(char* dest, char* src);