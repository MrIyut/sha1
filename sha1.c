#include "sha1.h"

__uint8_t *hex_alphabet = "0123456789ABCDEF";

void char_to_hex(__uint8_t number, __uint8_t *string)
{
	string[0] = hex_alphabet[number >> 4];
	string[1] = hex_alphabet[number & 0x0F];
}

__uint64_t make_congruent_to_modulo(__uint64_t number, __uint64_t congruent_to, __uint64_t modulo)
{
	// return how much to add to the current number so the number % modulo = congruent_to
	return number % modulo <= congruent_to ? (congruent_to - number % modulo) : (modulo - number % modulo + congruent_to);
}

__uint32_t left_rotate(__uint32_t number, __uint8_t bits)
{
	return ((number << bits) | (number >> (32 - bits))) & 0xFFFFFFFF;
}

__uint32_t format_string_to_int(__uint8_t *string)
{
	return string[0] << 24 | string[1] << 16 | string[2] << 8 | string[3];
}

void format_int_to_string(__uint32_t number, __uint8_t *string)
{
	string[0] = (number & 0xFF000000) >> 24;
	string[1] = (number & 0x00FF0000) >> 16;
	string[2] = (number & 0x0000FF00) >> 8;
	string[3] = number & 0x000000FF;
}

__uint8_t *convert_string_to_hex(__uint8_t *string, __uint64_t string_len)
{
	__uint8_t *new_string = calloc(string_len << 1, sizeof(__uint8_t));
	for (__uint32_t i = 0; i < string_len; i++)
		char_to_hex(string[i], new_string + (i << 1));

	return new_string;
}

void print_string_as_hex(__uint8_t *string, __uint64_t string_len)
{
	for (__uint32_t i = 0; i < string_len; i++)
		printf("%02x", (__uint8_t)string[i]);
	printf("\n");
}

void print_hash(__uint8_t *hash)
{
	for (__uint32_t i = 0; i < (HASH_LENGTH << 1); i++)
		printf("%c", hash[i]);
	printf("\n");
}

paddedMessage *padd_message(char *message)
{
	__uint64_t original_length = strlen(message);

	// take into account the bit 1 that needs to be appended
	__uint64_t padding_length = make_congruent_to_modulo(original_length + 1, 56, 64);
	// + 8 because the original length will need to be appended as well as an 64-bit integer
	__uint64_t total_length = original_length + 1 + padding_length + 8;

	__uint8_t *padded_message = calloc(total_length, sizeof(__uint8_t));
	memcpy(padded_message, message, original_length);
	padded_message[original_length] = 0x80; // append the bit 1 to the end of the message, 0x80 = 1000 0000b

	// add 0 until the message is 8 bytes short of being a 64 multiple
	for (__uint64_t i = 0; i < padding_length; i++)
		padded_message[original_length + i + 1] = 0x0;

	// append the original length (in bits, not bytes) to the message
	__uint64_t original_length_in_bits = original_length << 3;
	for (__uint64_t i = 0; i < 8; i++)
		padded_message[original_length + 1 + padding_length + i] = (original_length_in_bits >> ((7 - i) << 3)) & 0xFF;

	paddedMessage *new_message = calloc(1, sizeof(paddedMessage));
	new_message->message = padded_message;
	new_message->length = total_length;

	return new_message;
}

__uint8_t **string_split(__uint8_t *string, __uint64_t string_len, __uint32_t split_size)
{
	__uint64_t total_splits = string_len / split_size;
	__uint8_t **splits = calloc(total_splits, sizeof(__uint8_t *));

	for (__uint64_t split = 0; split < total_splits; split++)
	{
		splits[split] = calloc(split_size, sizeof(__uint8_t));
		memcpy(splits[split], string + split * split_size, split_size);
	}

	return splits;
}

void compress(__uint32_t word, __int32_t word_number, __uint32_t *a, __uint32_t *b, __uint32_t *c, __uint32_t *d, __uint32_t *e)
{
	__uint32_t f = 0, k = 0;
	if (word_number >= 0 && word_number < 20)
	{
		f = ((*b) & (*c)) | (~(*b) & (*d));
		k = 0x5A827999;
	}
	else if (word_number >= 20 && word_number < 40)
	{
		f = (*b) ^ (*c) ^ (*d);
		k = 0x6ED9EBA1;
	}
	else if (word_number >= 40 && word_number < 60)
	{
		f = ((*b) & (*c)) | ((*b) & (*d)) | ((*c) & (*d));
		k = 0x8F1BBCDC;
	}
	else if (word_number >= 60 && word_number < 80)
	{
		f = (*b) ^ (*c) ^ (*d);
		k = 0xCA62C1D6;
	}

	__uint32_t temp = left_rotate((*a), 5) + f + (*e) + k + word;
	*e = *d;
	*d = *c;
	*c = left_rotate(*b, 30);
	*b = *a;
	*a = temp;
}

__uint8_t *concatenate_hash_values(__uint32_t h0, __uint32_t h1, __uint32_t h2, __uint32_t h3, __uint32_t h4)
{
	__uint8_t *hash = calloc(HASH_LENGTH, sizeof(__uint8_t));
	format_int_to_string(h0, hash);
	format_int_to_string(h1, hash + 4);
	format_int_to_string(h2, hash + 8);
	format_int_to_string(h3, hash + 12);
	format_int_to_string(h4, hash + 16);

	return hash;
}

__uint8_t *process_hash(processingBlocks *processing_blocks)
{
	__uint32_t H0 = 0x67452301;
	__uint32_t H1 = 0xEFCDAB89;
	__uint32_t H2 = 0x98BADCFE;
	__uint32_t H3 = 0x10325476;
	__uint32_t H4 = 0xC3D2E1F0;

	for (__uint64_t block = 0; block < processing_blocks->total_blocks; block++)
	{
		__uint32_t *working_words = calloc(WORKING_WORDS_NUMBER, sizeof(__uint32_t));
		__uint8_t **original_words = string_split(processing_blocks->blocks[block], BLOCK_SIZE, WORD_SIZE);

		// convert to unsigned int for easier processing
		for (__int32_t i = 0; i < BLOCK_SIZE / WORD_SIZE; i++)
			working_words[i] = format_string_to_int(original_words[i]);

		for (__int32_t i = BLOCK_SIZE / WORD_SIZE; i < WORKING_WORDS_NUMBER; i++)
			working_words[i] = left_rotate((working_words[i - 3] ^ working_words[i - 8] ^ working_words[i - 14] ^ working_words[i - 16]), 1);

		__uint32_t a = H0, b = H1, c = H2, d = H3, e = H4;
		for (__int32_t word = 0; word < WORKING_WORDS_NUMBER; word++)
			compress(working_words[word], word, &a, &b, &c, &d, &e);

		H0 += a;
		H1 += b;
		H2 += c;
		H3 += d;
		H4 += e;

		free(working_words);
		for (__int32_t word = 0; word < BLOCK_SIZE / WORD_SIZE; word++)
			free(original_words[word]);
		free(original_words);
	}

	return concatenate_hash_values(H0, H1, H2, H3, H4);
}

processingBlocks *split_message_in_blocks(paddedMessage *message)
{
	// split the message into blocks of 64 bytes each
	__uint64_t total_blocks = message->length / BLOCK_SIZE;
	__uint8_t **blocks = string_split(message->message, message->length, BLOCK_SIZE);

	processingBlocks *processing_blocks = calloc(1, sizeof(processingBlocks));
	processing_blocks->blocks = blocks;
	processing_blocks->total_blocks = total_blocks;

	return processing_blocks;
}

__uint8_t *generate_hash(__uint8_t *message)
{
	paddedMessage *padded_message = padd_message(message);
	processingBlocks *processing_blocks = split_message_in_blocks(padded_message);
	__uint8_t *hash = process_hash(processing_blocks);
	__uint8_t *display_hash = convert_string_to_hex(hash, HASH_LENGTH);

	free(padded_message->message);
	free(padded_message);

	for (__uint64_t block = 0; block < processing_blocks->total_blocks; block++)
		free(processing_blocks->blocks[block]);
	free(processing_blocks->blocks);
	free(processing_blocks);

	free(hash);

	return display_hash;
}