/* $Id$ */

/*
 *  (C) Copyright 2001 Wojtek Kaniewski <wojtekka@irc.pl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License Version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#ifndef _AIX
#  include <string.h>
#endif
#include <stdarg.h>
#include <ctype.h>
#include "libgg.h"
#include "http.h"

/*
 * gg_register()
 *
 * pr�buje zarejestrowa� u�ytkownika. W TEJ CHWILI JU� DZIA�A, 
 *
 *  - email, password - informacja rejestracyjne,
 *  - async - ma by� asynchronicznie?
 *
 * zwraca zaalokowan� struktur� `gg_register', kt�r� po�niej nale�y zwolni�
 * funkcj� gg_free_register(), albo NULL je�li wyst�pi� b��d.
 */
struct gg_register *gg_register(char *email, char *password, int async)
{
	struct gg_register *r;
	char *__pwd, *__email, *form, *query;

	if (!email | !password) {
		errno = EFAULT;
		return NULL;
	}

	__pwd = gg_urlencode(password);
	__email = gg_urlencode(email);

	if (!__pwd || !__email) {
		gg_debug(GG_DEBUG_MISC, "=> register, not enough memory for form fields\n");
		free(__pwd);
		free(__email);
		return NULL;
	}

	form = gg_alloc_sprintf("pwd=%s&email=%s&code=%u", __pwd, __email,
			gg_http_hash(email, password));

	free(__pwd);
	free(__email);

	if (!form) {
		gg_debug(GG_DEBUG_MISC, "=> register, not enough memory for form query\n");
		return NULL;
	}

	gg_debug(GG_DEBUG_MISC, "=> register, %s\n", form);

	query = gg_alloc_sprintf(
		"Host: " GG_REGISTER_HOST "\r\n"
		"Content-Type: application/x-www-form-urlencoded\r\n"
		"User-Agent: " GG_HTTP_USERAGENT "\r\n"
		"Content-Length: %d\r\n"
		"Pragma: no-cache\r\n"
		"\r\n"
		"%s",
		strlen(form), form);

	free(form);

	if (!(r = malloc(sizeof(*r)))) {
		free(query);
		return NULL;
	}

	memset(r, 0, sizeof(*r));

	r->done = 0;
	r->uin = 0;
	r->password = password;
	
	if (!(r->http = gg_http_connect(GG_REGISTER_HOST, GG_REGISTER_PORT, async, "POST", "/appsvc/fmregister.asp", query))) {
		gg_debug(GG_DEBUG_MISC, "=> register, gg_http_connect() failed mysteriously\n");
		free(query);
		free(r);
		return NULL;
	}

	free(query);

	gg_http_copy_vars(r);
	
	if (!async)
		gg_register_watch_fd(r);
	
	return r;
}

/*
 * gg_register_watch_fd()
 *
 * przy asynchronicznym zak�adaniu wypada�oby wywo�a� t� funkcj� przy
 * jaki� zmianach na gg_register->fd.
 *
 *  - r - to co�, co zwr�ci�o gg_register()
 *
 * je�li wszystko posz�o dobrze to 0, inaczej -1. przeszukiwanie b�dzie
 * zako�czone, je�li r->state == GG_STATE_FINISHED. je�li wyst�pi jaki�
 * b��d, to b�dzie tam GG_STATE_IDLE i odpowiedni kod b��du w r->error.
 */
int gg_register_watch_fd(struct gg_register *r)
{
	int res;

	if (!r || !r->http) {
		errno = EINVAL;
		return -1;
	}
	
	if (r->http->state != GG_STATE_FINISHED) {
		if ((res = gg_http_watch_fd(r->http)) == -1) {
			gg_debug(GG_DEBUG_MISC, "=> register, http failure\n");
			return -1;
		}
		gg_http_copy_vars(r);
	}
	
	if (r->state == GG_STATE_FINISHED) {
		r->done = 1;
		gg_debug(GG_DEBUG_MISC, "=> register, let's parse...\n");
		gg_debug(GG_DEBUG_MISC, "=> register, \"%s\"\n", r->http->data);

		if (strncasecmp(r->http->data, "reg_success:", 12)) {
			gg_debug(GG_DEBUG_MISC, "=> register, failed.\n");
        		r->uin = 0;
		} else {
			r->uin = strtol(r->http->data + 12, NULL, 0);
			gg_debug(GG_DEBUG_MISC, "=> register, done (uin=%ld)\n", r->uin);
		}

		return 0;
	}

	return 0;
}

/*
 * gg_register_cancel()
 *
 * je�li rejestracja jest w trakcie, przerywa.
 *
 *  - r - to co�, co zwr�ci�o gg_register().
 *
 * UWAGA! funkcja potencjalnie niebezpieczna, bo mo�e pozwalnia� bufory
 * i pozamyka� sockety, kiedy co� si� dzieje. ale to ju� nie m�j problem ;)
 */
void gg_register_cancel(struct gg_register *r)
{
	if (!r || !r->http)
		return;

	gg_http_stop(r->http);
}

/*
 * gg_free_register()
 *
 * zwalnia pami�� po efektach rejestracji.
 *
 *  - r - to co�, co nie jest ju� nam potrzebne.
 *
 * nie zwraca niczego. najwy�ej segfaultnie.
 */
void gg_free_register(struct gg_register *r)
{
	if (!r)
		return;

	gg_free_http(r->http);

	free(r);
}

/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: notnil
 * End:
 *
 * vim: expandtab shiftwidth=8:
 */
