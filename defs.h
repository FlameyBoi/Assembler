#define MAX_NUM 2047
#define MIN_NUM -2048
#define LABEL 2048
#define r0 2049
#define r1 2050
#define r2 2051
#define r3 2052
#define r4 2053
#define r5 2054
#define r6 2055
#define r7 2056
#define ERR 2057
/*registers signified by values 2048-2055*/
/*encodings*/
#define ABS 0
#define EXT 1
#define REL 2
#define UNKNOWN 3 /*this value is used on first pass to mark labels*/
/*addressing schemes*/
#define IMMEDIATE 0
#define DIRECT 1
#define JMP_PARAMS 2
#define REGISTER 3

#define LABEL_LEN 30
#define WORD_SIZE 14
#define ADD_SIZE 9 /*since memory is 256 but base address is 100 9 bits are enough*/
#define BASE_ADD 100
#define MEM_SIZE 256