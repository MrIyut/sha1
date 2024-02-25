#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 64
#define WORD_SIZE 4
#define WORKING_WORDS_NUMBER 80
#define HASH_LENGTH 20

typedef struct
{
	__uint8_t *message;
	__uint64_t length;
} paddedMessage;

typedef struct
{
	__uint8_t **blocks;
	__uint64_t total_blocks;
} processingBlocks;

__uint8_t *generate_hash(__uint8_t *message);
void print_hash(__uint8_t *hash);