/*
 * przyk�ad prostego programu ��cz�cego si� z serwerem i wysy�aj�cego
 * jedn� wiadomo��.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "libgadu.h"

int main(int argc, char **argv)
{
	struct gg_session *sess;
	struct gg_event *e;
	struct gg_login_params p;

	if (argc < 5) {
		fprintf(stderr, "u�ycie: %s <m�jnumerek> <mojehas�o> <numerek> <wiadomo��>\n", argv[0]);
		return 1;
	}

	gg_debug_level = 255;

	memset(&p, 0, sizeof(p));
	p.uin = atoi(argv[1]);
	p.password = argv[2];
	
	if (!(sess = gg_login(&p))) {
		printf("Nie uda�o si� po��czy�: %s\n", strerror(errno));
		gg_free_session(sess);
		return 1;
	}

	printf("Po��czono.\n");

	if (gg_send_message(sess, GG_CLASS_MSG, atoi(argv[3]), argv[4]) == -1) {
		printf("Po��czenie przerwane: %s\n", strerror(errno));
		gg_free_session(sess);
		return 1;
	}

	/* poni�sz� cz�� mo�na ola�, ale poczekajmy na potwierdzenie */

	while (1) {
		if (!(e = gg_watch_fd(sess))) {
			printf("Po��czenie przerwane: %s\n", strerror(errno));
			gg_logoff(sess);
			gg_free_session(sess);
			return 1;
		}

		if (e->type == GG_EVENT_ACK) {
			printf("Wys�ano.\n");
			gg_free_event(e);
			break;
		}

		gg_free_event(e);
	}

	gg_logoff(sess);
	gg_free_session(sess);

	return 0;
}
