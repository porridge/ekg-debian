/* $Id$ */

/*
 *  (C) Copyright 2002 Wojtek Kaniewski <wojtekka@irc.pl>
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

#ifndef __UI_H
#define __UI_H

#include "config.h"

void (*ui_loop)(void);
void (*ui_print)(const char *target, const char *line);
void (*ui_beep)(void);
void (*ui_new_target)(const char *target);
void (*ui_query)(const char *target);
void (*ui_deinit)(void);

#include "ui-batch.h"

#ifdef WITH_UI_READLINE
#include "ui-readline.h"
#endif

#ifdef WITH_UI_NCURSES
#include "ui-ncurses.h"
#endif

#endif /* __UI_H */
