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

#ifndef __VARS_H
#define __VARS_H

enum {
	VAR_STR,
	VAR_INT,
	VAR_BOOL,
};

struct variable {
	char *name;
	int type;
	int display; /* 0 ukrywa warto��, 1 pokazuje, 2 ukrywa ca�� zmienn� */
	void *ptr;
};

#define MAX_VARS 50	/* �eby si� g�upi gcc nie plu� */

struct variable variables[MAX_VARS];

struct variable *find_variable(char *name);
int set_variable(char *name, char *value);

#endif
