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
#include <errno.h>
#ifndef _AIX
#  include <string.h>
#endif
#include "dynstuff.h"

/*
 * list_add_sorted()
 *
 * dodaje do listy dany element. przy okazji mo�e te� skopiowa� zawarto��.
 * je�li poda si� jako ostatni parametr funkcj� por�wnuj�c� zawarto��
 * element�w, mo�e posortowa� od razu.
 *
 *  - list - wska�nik do listy,
 *  - data - wska�nik do elementu,
 *  - alloc_size - rozmiar elementu, je�li chcemy go skopiowa�.
 *
 * zwraca wska�nik zaalokowanego elementu lub NULL w przpadku b��du.
 */
void *list_add_sorted(struct list **list, void *data, int alloc_size, int (*comparision)(void *, void *))
{
	struct list *new, *tmp;

	if (!list) {
		errno = EFAULT;
		return NULL;
	}

	if (!(new = malloc(sizeof(struct list))))
		return NULL;

	new->data = data;
	new->next = NULL;

	if (alloc_size) {
		if (!(new->data = malloc(alloc_size))) { 
			free(new);
			return NULL;
		}
		memcpy(new->data, data, alloc_size);
	}

	if (!(tmp = *list)) {
		*list = new;
	} else {
		if (!comparision) {
			while (tmp->next)
				tmp = tmp->next;
			tmp->next = new;
		} else {
			struct list *prev = NULL;
			
			while (comparision(new->data, tmp->data) > 0) {
				prev = tmp;
				tmp = tmp->next;
				if (!tmp)
					break;
			}
			
			if (!prev) {
				tmp = *list;
				*list = new;
				new->next = tmp;
			} else {
				prev->next = new;
				new->next = tmp;
			}
		}
	}

	return new->data;
}

/*
 * list_add()
 *
 * wrapper do list_add_sorted(), kt�ry zachowuje poprzedni� sk�adni�.
 */
void *list_add(struct list **list, void *data, int alloc_size)
{
	return list_add_sorted(list, data, alloc_size, NULL);
}

/*
 * list_remove()
 *
 * usuwa z listy wpis z podanym elementem.
 *
 *  - list - wska�nik do listy,
 *  - data - element,
 *  - free_data - zwolni� pami�� po elemencie.
 */
int list_remove(struct list **list, void *data, int free_data)
{
	struct list *tmp, *last = NULL;

	if (!list) {
		errno = EFAULT;
		return -1;
	}

	tmp = *list;
	if (tmp->data == data) {
		*list = tmp->next;
	} else {
		for (; tmp && tmp->data != data; tmp = tmp->next)
			last = tmp;
		if (!tmp) {
			errno = ENOENT;
			return -1;
		}
		last->next = tmp->next;
	}

	if (free_data)
		free(tmp->data);
	free(tmp);

	return 0;
}

/*
 * list_count()
 *
 * zwraca ilo�� element�w w danej li�cie.
 *
 *  - list - lista.
 */
int list_count(struct list *list)
{
	int count = 0;

	for (; list; list = list->next)
		count++;

	return count;
}

/*
 * string_append_c()
 *
 * dodaje do danego ci�gu jeden znak, alokuj�c przy tym odpowiedni� ilo��
 * pami�ci.
 *
 *  - s - wska�nik do `struct string',
 *  - c - znaczek do dopisania.
 */
int string_append_c(struct string *s, char c)
{
	char *new;

	if (!s) {
		errno = EFAULT;
		return -1;
	}
	
	if (!s->str || strlen(s->str) + 2 > s->size) {
		if (!(new = realloc(s->str, s->size + 80)))
			return -1;
		if (!s->str)
			*new = 0;
		s->size += 80;
		s->str = new;
	}

	s->str[strlen(s->str) + 1] = 0;
	s->str[strlen(s->str)] = c;

	return 0;
}

/*
 * string_append_n()
 *
 * dodaje tekst do bufora alokuj�c odpowiedni� ilo�� pami�ci.
 *
 *  - s - wska�nik `struct string',
 *  - str - tekst do dopisania,
 *  - count - ile znak�w tego tekstu dopisa�? (-1 znaczy, �e ca�y).
 */
int string_append_n(struct string *s, char *str, int count)
{
	char *new;

	if (!s) {
		errno = EFAULT;
		return -1;
	}

	if (count == -1)
		count = strlen(str);

	if (!s->str || strlen(s->str) + count + 1 > s->size) {
		if (!(new = realloc(s->str, s->size + count + 80)))
			return -1;
		if (!s->str)
			*new = 0;
		s->size += count + 80;
		s->str = new;
	}

	s->str[strlen(s->str) + count] = 0;
	strncpy(s->str + strlen(s->str), str, count);

	return 0;
}

int string_append(struct string *s, char *str)
{
	return string_append_n(s, str, -1);
}

/*
 * string_init()
 *
 * inicjuje struktur� string. alokuje pami�� i przypisuje pierwsz� warto��.
 *
 *  - value - je�li NULL, ci�g jest pusty, inaczej kopiuje tam.
 *
 * zwraca zaalokowan� struktur� `string' lub NULL je�li pami�ci brak�o.
 */
struct string *string_init(char *value)
{
	struct string *tmp = malloc(sizeof(struct string));

	if (!tmp)
		return NULL;

	if (!value)
		value = "";

	tmp->str = strdup(value);
	tmp->size = strlen(value) + 1;

	return tmp;
}

/*
 * string_free()
 *
 * zwalnia pami�� po strukturze string i mo�e te� zwolni� pami�� po samym
 * ci�gu znak�w.
 *
 *  - s - struktura, kt�r� wycinamy,
 *  - free_string - zwolni� pami�� po ci�gu znak�w?
 *
 * je�li free_string=0 zwraca wska�nik do ci�gu, inaczej NULL.
 */
char *string_free(struct string *s, int free_string)
{
	char *tmp = NULL;

	if (!s)
		return NULL;

	if (free_string)
		free(s->str);
	else
		tmp = s->str;

	free(s);

	return tmp;
}

