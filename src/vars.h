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

#ifndef __VARS_H
#define __VARS_H

#include "dynstuff.h"

enum {
	VAR_STR,		/* ci�g znak�w */
	VAR_INT,		/* liczba ca�kowita */
	VAR_BOOL,		/* 0/1, tak/nie, yes/no, on/off */
	VAR_FOREIGN,		/* nieznana zmienna */
	VAR_MAP,		/* bitmapa */
};

struct value_map {
	int value;		/* warto�� */
	int conflicts;		/* warto�ci, z kt�rymi koliduje */
	char *label;		/* nazwa warto�ci */
};

struct variable {
	char *name;		/* nazwa zmiennej */
	int name_hash;		/* hash nazwy zmiennej */
	int type;		/* rodzaj */
	int display;		/* 0 bez warto�ci, 1 pokazuje, 2 w og�le */
	void *ptr;		/* wska�nik do zmiennej */
	void (*notify)(const char*);	/* funkcja wywo�ywana przy zmianie */
	struct value_map *map;	/* mapa warto�ci i etykiet */
};

list_t variables;

void variable_init();
struct variable *variable_find(const char *name);
struct value_map *variable_map(int count, ...);
int variable_add(const char *name, int type, int display, void *ptr, void (*notify)(const char *name), struct value_map *map);
int variable_set(const char *name, const char *value, int allow_foreign);
void variable_free();

#endif /* __VARS_H */
