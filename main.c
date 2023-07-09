#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "PreAssembler.h"
#include "AssemblerFirstPass.h"

int main(int argc, char* argv[])
{
	int i = 1; /*first arg isn't user supplied*/
	char name[256] = { '\0' }; /*file names are 255 chars long in linux systems*/
	for (; i < argc; i++)
	{
		printf("Alert: beginning assembly of");
		printf("%s", argv[i]);
		strcpy(name, argv[i]);
		if (strlen(name) <= 3 || strcmp(name + strlen(name) - 3, ".as")) /*check proper file extension*/
		{
			printf("Error: file %s has improper name format, skipping\n", argv[i]);
			printf("Alert: assembly of file %s failed\n", argv[i]);
			continue;
		}
		if (preAssemble(name) == -1)
		{
			printf("Alert: assembly of file %s failed\n", argv[i]);
			continue;
		}
		if (firstPass(name) == -1) /*begin assembler*/
		{
			printf("Alert: assembly of file %s failed\n", argv[i]);
			continue;
		}
		printf("Alert: assembly of file %s has completed\n", argv[i]);
	}
	printf("Finished all jobs, exitting\n");
	return 0;
}
#undef _CRT_SECURE_NO_WARNINGS