/* $Id$ */

/*
 *  (C) Copyright 2001-2003 Wojtek Kaniewski <wojtekka@irc.pl>
 *                          Robert J. Wo�ny <speedy@ziew.org>
 *                          Pawe� Maziarz <drg@o2.pl>
 *                          Dawid Jarosz <dawjar@poczta.onet.pl>
 *                          Piotr Domagalski <szalik@szalik.net>
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

#include "config.h"

#include <sys/types.h>

#include <stdio.h>
#include <string.h>

#include "dynstuff.h"
#include "emoticons.h"
#include "stuff.h"
#include "userlist.h"
#include "xmalloc.h"

list_t emoticons = NULL;

int config_emoticons = 1;

/*
 * emoticon_add()
 *
 * dodaje dany emoticon do listy.
 *
 *  - name - nazwa,
 *  - value - warto��.
 *
 * 0/-1
 */
int emoticon_add(const char *name, const char *value)
{
	struct emoticon e;
	list_t l;

	if (!name || !value)
		return -1;

	for (l = emoticons; l; l = l->next) {
		struct emoticon *g = l->data;

		if (!strcasecmp(name, g->name)) {
			xfree(g->value);
			g->value = xstrdup(value);
			return 0;
		}
	}

	e.name = xstrdup(name);
	e.value = xstrdup(value);

	return (list_add(&emoticons, &e, sizeof(e)) ? 0 : -1);
}

/*
 * emoticon_remove()
 *
 * usuwa emoticon o danej nazwie.
 *
 *  - name.
 *
 * 0/-1
 */
int emoticon_remove(const char *name)
{
	list_t l;

	for (l = emoticons; l; l = l->next) {
		struct emoticon *f = l->data;

		if (!strcasecmp(f->name, name)) {
			xfree(f->value);
			xfree(f->name);
			list_remove(&emoticons, f, 1);
			return 0;
		}
	}
	
	return -1;
}

/*
 * emoticon_read()
 *
 * �aduje do listy wszystkie makra z pliku ~/.gg/emoticons
 * format tego pliku w dokumentacji.
 *
 * 0/-1
 */
int emoticon_read()
{
	const char *filename;
	char *buf, **emot;
	FILE *f;

	if (!(filename = prepare_path("emoticons", 0)))
		return -1;
	
	if (!(f = fopen(filename, "r")))
		return -1;

	while ((buf = read_file(f))) {
	
		if (buf[0] == '#') {
			xfree(buf);
			continue;
		}

		emot = array_make(buf, "\t", 2, 1, 1);
	
		if (array_count(emot) == 2)
			emoticon_add(emot[0], emot[1]);

		array_free(emot);
		xfree(buf);
	}
	
	fclose(f);
	
	return 0;
}

/*
 * emoticon_expand()
 *
 * rozwija definicje makr (najcz�ciej b�d� to emoticony)
 *
 *  - s - string z makrami.
 *
 * zwraca zaalokowany, rozwini�ty string.
 */
char *emoticon_expand(const char *s)
{
	list_t l = NULL;
	const char *ss;
	char *ms;
	size_t n = 0;

	for (ss = s; *ss; ss++) {
		struct emoticon *e = NULL;
		size_t ns = strlen(ss);
		int ret = 1;

		for (l = emoticons; l && ret; l = (ret ? l->next : l)) {
			size_t nn;

			e = l->data;
			nn = strlen(e->name);
			if (ns >= nn)
				ret = strncmp(ss, e->name, nn);
		}

		if (l) {
			e = l->data;
			n += strlen(e->value);
			ss += strlen(e->name) - 1;
		} else
			n++;
	}

	ms = xcalloc(1, n + 1);

	for (ss = s; *ss; ss++) {
		struct emoticon *e = NULL;
		size_t ns = strlen(ss);
		int ret = 1;

		for (l = emoticons; l && ret; l = (ret ? l->next : l)) {
			size_t n;

			e = l->data;
			n = strlen(e->name);
			if (ns >= n)
				ret = strncmp(ss, e->name, n);
		}

		if (l) {
			e = l->data;
			strcat(ms, e->value);
			ss += strlen(e->name) - 1;
		} else
			ms[strlen(ms)] = *ss;
	}

	return ms;
}

/*
 * emoticon_free()
 *
 * usuwa pami�� zaj�t� przez emoticony.
 */
void emoticon_free()
{
	list_t l;

	if (!emoticons)
		return;

	for (l = emoticons; l; l = l->next) {
		struct emoticon *e = l->data;

		xfree(e->name);
		xfree(e->value);
	}

	list_destroy(emoticons, 1);
	emoticons = NULL;
}
