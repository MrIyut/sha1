This is a small implementation of **SHA-1** in C.

Useful functions:

- void print_hash(__uint8_t *hash) -- prints to the console the 160-bit hash as a regular string
- void print_string_as_hex(__uint8_t *string, __uint64_tstring_len) -- prints to the console a regular string as a hex string
- __uint8_t *convert_string_to_hex(__uint8_t *string, __uint64_t string_len) -- converts a regular string to a hex string
