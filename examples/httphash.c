/*
 * przyk�ad prostego programu ��cz�cego si� z serwerem i wysy�aj�cego
 * jedn� wiadomo��.
 */

#include <stdio.h>
#include <stdlib.h>
#include "libgg.h"

int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "u�ycie: %s <email> <has�o>\n", argv[0]);
		return 1;
	}

	printf("%u\n", gg_http_hash(argv[1], argv[2]));

	return 0;
}

