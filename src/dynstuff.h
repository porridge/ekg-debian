/* $Id$ */

/*
 *  (C) Copyright 2001-2002 Wojtek Kaniewski <wojtekka@irc.pl>
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

#ifndef __DYNSTUFF_H
#define __DYNSTUFF_H

struct list {
	void *data;
	struct list *next;
};

void *list_add(struct list **list, void *data, int alloc_size);
void *list_add_sorted(struct list **list, void *data, int alloc_size, int (*comparision)(void *, void *));
int list_remove(struct list **list, void *data, int free_data);
int list_count(struct list *list);
int list_destroy(struct list *list, int free_data);

struct string {
	char *str;
	int size;
};

struct string *string_init(char *str);
int string_append(struct string *s, char *str);
int string_append_n(struct string *s, char *str, int count);
int string_append_c(struct string *s, char ch);
char *string_free(struct string *s, int free_string);

char *itoa(long int i);

#endif
