#include <stdio.h>
#include <stdlib.h>

#include "sha1.h"

int main()
{
	char *message = "abc";
	__uint8_t *hash = generate_hash(message);
	print_hash(hash);

	free(hash);
	return 0;
}