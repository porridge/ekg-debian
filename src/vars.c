/* $Id$ */

/*
 *  (C) Copyright 2001-2002 Wojtek Kaniewski <wojtekka@irc.pl>
 *                          Robert J. Wo�ny <speedy@ziew.org>
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
#ifndef _AIX
#  include <string.h>
#endif
#include <stdarg.h>
#include <ctype.h>
#include "stuff.h"
#include "mail.h"
#include "vars.h"
#include "config.h"
#include "libgadu.h"
#include "dynstuff.h"
#include "xmalloc.h"
#include "ui.h"

list_t variables = NULL;

/*
 * dd_*()
 *
 * funkcje informuj�ce, czy dana grupa zmiennych ma zosta� wy�wietlona.
 * r�wnie dobrze mo�na by�o przekaza� wska�nik do zmiennej, kt�ra musi
 * by� r�na od zera, ale dzi�ki funkcjom nie trzeba b�dzie miesza� w 
 * przysz�o�ci.
 */
static int dd_beep(const char *name)
{
	return (config_beep);
}

static int dd_check_mail(const char *name)
{
	return (config_check_mail);
}

static int dd_dcc(const char *name)
{
	return (config_dcc);
}

static int dd_sound(const char *name)
{
	return (config_sound_app != NULL);
}

static int dd_sms(const char *name)
{
	return (config_sms_app != NULL);
}

static int dd_log(const char *name)
{
	return (config_log);
}

static int dd_color(const char *name)
{
	return (config_display_color);
}

static int dd_contacts(const char *name)
{
	return (config_contacts);
}

/*
 * variable_init()
 *
 * inicjuje list� zmiennych.
 */
void variable_init()
{
	variable_add("uin", "ui", VAR_INT, 1, &config_uin, changed_uin, NULL, NULL);
	variable_add("password", "pa", VAR_STR, 0, &config_password, NULL, NULL, NULL);
	variable_add("email", "em", VAR_STR, 1, &config_email, NULL, NULL, NULL);

#ifdef HAVE_VOIP
	variable_add("audio_device", "ad", VAR_STR, 1, &config_audio_device, NULL, NULL, NULL);
#endif
	variable_add("auto_away", "aa", VAR_INT, 1, &config_auto_away, NULL, NULL, NULL);
	variable_add("auto_back", "ab", VAR_INT, 1, &config_auto_back, NULL, NULL, NULL);
	variable_add("auto_reconnect", "ac", VAR_INT, 1, &config_auto_reconnect, NULL, NULL, NULL);
	variable_add("auto_save", "as", VAR_INT, 1, &config_auto_save, NULL, NULL, NULL);
	variable_add("away_reason", "ar", VAR_STR, 1, &config_away_reason, changed_xxx_reason, NULL, NULL);
	variable_add("back_reason", "br", VAR_STR, 1, &config_back_reason, changed_xxx_reason, NULL, NULL);
#ifdef WITH_UI_NCURSES
	variable_add("backlog_size", "bs", VAR_INT, 1, &config_backlog_size, NULL, NULL, NULL);
#endif
	variable_add("beep", "be", VAR_BOOL, 1, &config_beep, NULL, NULL, NULL);
	variable_add("beep_msg", "bm", VAR_BOOL, 1, &config_beep_msg, NULL, NULL, dd_beep);
	variable_add("beep_chat", "bc", VAR_BOOL, 1, &config_beep_chat, NULL, NULL, dd_beep);
	variable_add("beep_notify", "bn", VAR_BOOL, 1, &config_beep_notify, NULL, NULL, dd_beep);
	variable_add("beep_mail", "bM", VAR_BOOL, 1, &config_beep_mail, NULL, NULL, dd_beep);
	variable_add("check_mail", "cm", VAR_MAP, 1, &config_check_mail, changed_check_mail, variable_map(4, 0, 0, "no", 1, 2, "mbox", 2, 1, "maildir", 4, 0, "notify"), NULL);
	variable_add("check_mail_frequency", "cf", VAR_INT, 1, &config_check_mail_frequency, changed_check_mail, NULL, dd_check_mail);
	variable_add("check_mail_folders", "cF", VAR_STR, 1, &config_check_mail_folders, changed_check_mail_folders, NULL, dd_check_mail);
	variable_add("completion_notify", "cn", VAR_MAP, 1, &config_completion_notify, NULL, variable_map(4, 0, 0, "none", 1, 2, "add", 2, 1, "addremove", 4, 0, "busy"), NULL);
	variable_add("ctrld_quits", "cq", VAR_BOOL, 1, &config_ctrld_quits, NULL, NULL, NULL);
#ifdef WITH_UI_NCURSES
	variable_add("contacts", "co", VAR_INT, 1, &config_contacts, contacts_rebuild, NULL, NULL);
	variable_add("contacts_descr", "cd", VAR_BOOL, 1, &config_contacts_descr, contacts_rebuild, NULL, NULL);
	variable_add("contacts_size", "cs", VAR_INT, 1, &config_contacts_size, contacts_rebuild, NULL, dd_contacts);
#endif
	variable_add("dcc", "dc", VAR_BOOL, 1, &config_dcc, changed_dcc, NULL, NULL);
	variable_add("dcc_ip", "di", VAR_STR, 1, &config_dcc_ip, changed_dcc, NULL, dd_dcc);
	variable_add("dcc_dir", "dd", VAR_STR, 1, &config_dcc_dir, NULL, NULL, dd_dcc);
	variable_add("display_ack", "da", VAR_INT, 1, &config_display_ack, NULL, variable_map(4, 0, 0, "none", 1, 0, "all", 2, 0, "delivered", 3, 0, "queued"), NULL);
	variable_add("display_color", "dC", VAR_BOOL, 1, &config_display_color, NULL, NULL, NULL);
	variable_add("display_color_map", "dm", VAR_STR, 1, &config_display_color_map, NULL, NULL, dd_color);
	variable_add("display_crap", "dr", VAR_BOOL, 1, &config_display_crap, NULL, NULL, NULL);
	variable_add("display_notify", "dn", VAR_INT, 1, &config_display_notify, NULL, variable_map(3, 0, 0, "none", 1, 0, "all", 2, 0, "significant"), NULL);
	variable_add("display_sent", "ds", VAR_BOOL, 1, &config_display_sent, NULL, NULL, NULL);
	variable_add("display_welcome", "dw", VAR_BOOL, 1, &config_display_welcome, NULL, NULL, NULL);
	variable_add("emoticons", "eM", VAR_BOOL, 1, &config_emoticons, NULL, NULL, NULL);
#ifdef HAVE_OPENSSL
	variable_add("encryption", "en", VAR_INT, 1, &config_encryption, NULL, variable_map(2, 0, 0, "none", 1, 0, "sim"), NULL);
#endif
	variable_add("enter_scrolls", "es", VAR_BOOL, 1, &config_enter_scrolls, NULL, NULL, NULL);
	variable_add("keep_reason", "kr", VAR_BOOL, 1, &config_keep_reason, NULL, NULL, NULL);
	variable_add("last", "la", VAR_MAP, 1, &config_last, NULL, variable_map(4, 0, 0, "none", 1, 2, "all", 2, 1, "separate", 4, 0, "sent"), NULL);
	variable_add("last_size", "ls", VAR_INT, 1, &config_last_size, NULL, NULL, NULL);
	variable_add("log", "lo", VAR_MAP, 1, &config_log, NULL, variable_map(4, 0, 0, "none", 1, 2, "file", 2, 1, "dir", 4, 0, "gzip"), NULL);
	variable_add("log_ignored", "li", VAR_INT, 1, &config_log_ignored, NULL, NULL, dd_log);
	variable_add("log_status", "lS", VAR_BOOL, 1, &config_log_status, NULL, NULL, dd_log);
	variable_add("log_path", "lp", VAR_STR, 1, &config_log_path, NULL, NULL, dd_log);
	variable_add("log_timestamp", "lt", VAR_STR, 1, &config_log_timestamp, NULL, NULL, dd_log);
	variable_add("make_window", "mw", VAR_INT, 1, &config_make_window, NULL, variable_map(3, 0, 0, "none", 1, 0, "usefree", 2, 0, "always"), NULL);
	variable_add("mesg_allow", "ma", VAR_INT, 1, &config_mesg_allow, mesg_changed, variable_map(3, 0, 0, "no", 1, 0, "yes", 2, 0, "default"), NULL);
	variable_add("proxy", "pr", VAR_STR, 1, &config_proxy, changed_proxy, NULL, NULL);
	variable_add("random_reason", "rr", VAR_MAP, 1, &config_random_reason, NULL, variable_map(5, 0, 0, "none", 1, 0, "away", 2, 0, "notavail", 4, 0, "avail", 8, 0, "invisible"), NULL);
	variable_add("quit_reason", "qr", VAR_STR, 1, &config_quit_reason, changed_xxx_reason, NULL, NULL);
	variable_add("query_commands", "qc", VAR_BOOL, 1, &config_query_commands, NULL, NULL, NULL);
	variable_add("save_password", "sp", VAR_BOOL, 1, &config_save_password, NULL, NULL, NULL);
	variable_add("server", "se", VAR_STR, 1, &config_server, NULL, NULL, NULL);
	variable_add("server_save", "ss", VAR_BOOL, 1, &config_server_save, NULL, NULL, NULL);
	variable_add("sms_away", "sa", VAR_MAP, 1, &config_sms_away, NULL, variable_map(3, 0, 0, "none", 1, 2, "all", 2, 1, "separate"), dd_sms);
	variable_add("sms_away_limit", "sl", VAR_INT, 1, &config_sms_away_limit, NULL, NULL, dd_sms);
	variable_add("sms_max_length", "sm", VAR_INT, 1, &config_sms_max_length, NULL, NULL, dd_sms);
	variable_add("sms_number", "sn", VAR_STR, 1, &config_sms_number, NULL, NULL, dd_sms);
	variable_add("sms_send_app", "sA", VAR_STR, 1, &config_sms_app, NULL, NULL, NULL);
	variable_add("sort_windows", "sw", VAR_BOOL, 1, &config_sort_windows, NULL, NULL, NULL);
	variable_add("sound_msg_file", "Sm", VAR_STR, 1, &config_sound_msg_file, NULL, NULL, dd_sound);
	variable_add("sound_chat_file", "Sc", VAR_STR, 1, &config_sound_chat_file, NULL, NULL, dd_sound);
	variable_add("sound_sysmsg_file", "Ss", VAR_STR, 1, &config_sound_sysmsg_file, NULL, NULL, dd_sound);
	variable_add("sound_notify_file", "Sn", VAR_STR, 1, &config_sound_notify_file, NULL, NULL, dd_sound);
	variable_add("sound_app", "Sa", VAR_STR, 1, &config_sound_app, NULL, NULL, NULL);
	variable_add("speech_app", "SA", VAR_STR, 1, &config_speech_app, NULL, NULL, NULL);
	variable_add("tab_command", "tc", VAR_STR, 1, &config_tab_command, NULL, NULL, NULL);
	variable_add("theme", "th", VAR_STR, 1, &config_theme, changed_theme, NULL, NULL);
	variable_add("time_deviation", "td", VAR_INT, 1, &config_time_deviation, NULL, NULL, NULL);
	variable_add("timestamp", "ts", VAR_STR, 1, &config_timestamp, NULL, NULL, NULL);
#ifdef WITH_UI_NCURSES
	variable_add("windows_save", "ws", VAR_BOOL, 1, &config_windows_save, NULL, NULL, NULL);
	variable_add("windows_layout", "wl", VAR_STR, 2, &config_windows_layout, NULL, NULL, NULL);
#endif

	variable_add("status", "st", VAR_INT, 2, &config_status, NULL, NULL, NULL);
	variable_add("debug", "de", VAR_BOOL, 2, &config_debug, changed_debug, NULL, NULL);
	variable_add("protocol", "pR", VAR_INT, 2, &config_protocol, NULL, NULL, NULL);
	variable_add("reason", "re", VAR_STR, 2, &config_reason, NULL, NULL, NULL);
}

/*
 * variable_find()
 *
 * znajduje struktur� `variable' opisuj�c� zmienn� o podanej nazwie.
 *
 * - name.
 */
struct variable *variable_find(const char *name)
{
	list_t l;
	int hash;

	if (!name)
		return NULL;

	hash = ekg_hash(name);

	for (l = variables; l; l = l->next) {
		struct variable *v = l->data;

		if (v->name && v->name_hash == hash && !strcasecmp(v->name, name))
			return v;
	}

	return NULL;
}

/*
 * variable_map()
 *
 * tworzy now� map� warto�ci. je�li kt�ra� z warto�ci powinna wy��czy� inne
 * (na przyk�ad w ,,log'' 1 wy��cza 2, 2 wy��cza 1, ale nie maj� wp�ywu na 4)
 * nale�y doda� do ,,konflikt''.
 *
 *  - count - ilo��,
 *  - ... - warto��, konflikt, opis.
 *
 * zaalokowana tablica.
 */
struct value_map *variable_map(int count, ...)
{
	struct value_map *res;
	va_list ap;
	int i;

	res = xcalloc(count + 1, sizeof(struct value_map));
	
	va_start(ap, count);

	for (i = 0; i < count; i++) {
		res[i].value = va_arg(ap, int);
		res[i].conflicts = va_arg(ap, int);
		res[i].label = xstrdup(va_arg(ap, char*));
	}
	
	va_end(ap);

	return res;
}

/*
 * variable_add()
 *
 * dodaje zmienn� do listy zmiennych.
 *
 *  - name - nazwa,
 *  - short_name - kr�tka nazwa,
 *  - type - typ zmiennej,
 *  - display - czy i jak ma wy�wietla�,
 *  - ptr - wska�nik do zmiennej,
 *  - notify - funkcja powiadomienia,
 *  - map - mapa warto�ci,
 *  - dyndisplay - funkcja sprawdzaj�ca czy wy�wietli� zmienn�.
 *
 * zwraca 0 je�li si� uda�o, je�li nie to -1.
 */
int variable_add(const char *name, const char *short_name, int type, int display, void *ptr, void (*notify)(const char*), struct value_map *map, int (*dyndisplay)(const char *name))
{
	struct variable v;
	list_t l;

	if (type != VAR_FOREIGN) {
		for (l = variables; l; l = l->next) {
			struct variable *v = l->data;

			if (!strcmp(v->short_name, short_name)) {
				fprintf(stderr, "Error! Variable short name conflict:\n- short name: \"%s\"\n- existing variable: \"%s\"\n- conflicting variable: \"%s\"\n\nPress any key to continue...\n", short_name, v->name, name);
				getchar();
			}
		}
	}

	v.name = xstrdup(name);
	v.name_hash = ekg_hash(name);
	v.type = type;
	strcpy(v.short_name, short_name);
	v.display = display;
	v.ptr = ptr;
	v.notify = notify;
	v.map = map;
	v.dyndisplay = dyndisplay;

	return (list_add(&variables, &v, sizeof(v)) != NULL) ? 0 : -1;
}

/*
 * variable_set()
 *
 * ustawia warto�� podanej zmiennej. je�li to zmienna liczbowa lub boolowska,
 * zmienia ci�g na liczb�. w przypadku boolowskich, rozumie zwroty typu `on',
 * `off', `yes', `no' itp. je�li dana zmienna jest bitmap�, akceptuje warto��
 * w postaci listy flag oraz konstrukcje `+flaga' i `-flaga'.
 *
 *  - name - nazwa zmiennej,
 *  - value - nowa warto��,
 *  - allow_foreign - czy ma pozwala� dopisywa� obce zmienne.
 */
int variable_set(const char *name, const char *value, int allow_foreign)
{
	struct variable *v = variable_find(name);

	if (!v && allow_foreign) {
		variable_add(name, "##", VAR_FOREIGN, 2, xstrdup(value), NULL, NULL, NULL);
		return -1;
	}

	if (!v && !allow_foreign)
		return -1;

	switch (v->type) {
		case VAR_INT:
		case VAR_MAP:
		{
			const char *p = value;

			if (!value)
				return -2;

			if (v->map && v->type == VAR_INT && !isdigit(*p)) {
				int i;

				for (i = 0; v->map[i].label; i++)
					if (!strcasecmp(v->map[i].label, value))
						value = itoa(v->map[i].value);
			}

			if (v->map && v->type == VAR_MAP && !isdigit(*p)) {
				int i, k = *(int*)(v->ptr);
				int mode = 0; /* 0 set, 1 add, 2 remove */
				char **args;

				if (*p == '+') {
					mode = 1;
					p++;
				} else if (*p == '-') {
					mode = 2;
					p++;
				}

				if (!mode)
					k = 0;

				args = array_make(p, ",", 0, 1, 0);

				for (i = 0; args[i]; i++) {
					int j, found = 0;

					for (j = 0; v->map[j].label; j++) {
						if (!strcasecmp(args[i], v->map[j].label)) {
							found = 1;

							if (mode == 2)
								k &= ~(v->map[j].value);
							if (mode == 1)
								k &= ~(v->map[j].conflicts);
							if (mode == 1 || !mode)
								k |= v->map[j].value;
						}
					}

					if (!found) {
						array_free(args);
						return -2;
					}
				}

				array_free(args);

				value = itoa(k);
			}

			p = value;
				
			if (!strncmp(p, "0x", 2))
				p += 2;

			while (*p && *p != ' ') {
				if (!isdigit(*p))
					return -2;
				p++;
			}

			*(int*)(v->ptr) = strtol(value, NULL, 0);

			if (v->notify)
				(v->notify)(v->name);

			ui_event("variable_changed", v->name);
			
			return 0;
		}

		case VAR_BOOL:
		{
			int tmp;
		
			if (!value)
				return -2;
		
			if ((tmp = on_off(value)) == -1)
				return -2;

			*(int*)(v->ptr) = tmp;

			if (v->notify)
				(v->notify)(v->name);

			ui_event("variable_changed", v->name);
		
			return 0;
		}

		case VAR_STR:
		{
			char **tmp = (char**)(v->ptr);
			
			xfree(*tmp);
			
			if (value) {
				if (*value == 1)
					*tmp = base64_decode(value + 1);
				else
					*tmp = xstrdup(value);

				if (!*tmp)
					return -3;
			} else
				*tmp = NULL;
	
			if (v->notify)
				(v->notify)(v->name);

			ui_event("variable_changed", v->name);

			return 0;
		}
	}

	return -1;
}

/*
 * variable_free()
 *
 * zwalnia pami�� u�ywan� przez zmienne.
 */
void variable_free()
{
	list_t l;

	for (l = variables; l; l = l->next) {
		struct variable *v = l->data;

		xfree(v->name);

		if (v->type == VAR_STR) {
			xfree(*((char**) v->ptr));
			*((char**) v->ptr) = NULL;
		}

		if (v->type == VAR_FOREIGN)
			xfree((char*) v->ptr);

		if (v->map) {
			int i;

			for (i = 0; v->map[i].label; i++)
				xfree(v->map[i].label);

			xfree(v->map);
		}
	}

	list_destroy(variables, 1);
	variables = NULL;
}

/*
 * variable_digest()
 *
 * tworzy skr�con� wersj� listy zmiennej do zachowania w li�cie kontakt�w
 * na serwerze.
 */
char *variable_digest()
{
	string_t s = string_init(NULL);
	list_t l;

	for (l = variables; l; l = l->next) {
		struct variable *v = l->data;

		if ((v->type == VAR_INT || v->type == VAR_BOOL) && strcmp(v->name, "uin")) {
			string_append(s, v->short_name);
			string_append(s, itoa(*(int*)(v->ptr)));
		}
	}

	for (l = variables; l; l = l->next) {
		struct variable *v = l->data;
		char *val;

		if (!v->type == VAR_STR || !strcmp(v->name, "password"))
			continue;
	
		val = *((char**)v->ptr);

		string_append(s, v->short_name);

		if (!val) {
			string_append_c(s, '-');
			continue;
		}

		if (strchr(val, ':') || val[0] == '+') {
			char *tmp = base64_encode(val);
			string_append_c(s, '+');
			string_append(s, tmp);
			string_append_c(s, ':');
			xfree(tmp);
		} else {
			string_append(s, val);
			string_append_c(s, ':');
		}
	}
	
	return string_free(s, 0);
}

/*
 * variable_undigest()
 *
 * rozszyfrowuje skr�t zmiennych z listy kontakt�w i ustawia wszystko
 * co trzeba.
 *
 *  - str - ci�g znak�w.
 *
 * 0/-1
 */
int variable_undigest(const char *digest)
{
	const char *p = digest;

	while (*p) {
		struct variable *v;
		list_t l;

		for (v = NULL, l = variables; l; l = l->next) {
			struct variable *w = l->data;

			if (w && !strncmp(p, w->short_name, 2)) {
				v = w;
				break;
			}
		}

		if (!v) {
			gg_debug(GG_DEBUG_MISC, "// unknown short \"%c%c\"\n", p[0], p[1]);
			return -1;
		}

		p += 2;

		if (v->type == VAR_INT || v->type == VAR_BOOL) {
			char *end;
			int val;
			
			val = strtol(p, &end, 10);

			variable_set(v->name, itoa(val), 0);

			p = end;
		}

		if (v->type == VAR_STR) {
			char *val = NULL;
			
			if (*p == '-') {
				val = NULL;
				p++;
			} else {
				const char *q;
				char *tmp;
				int len = 0, base64 = 0;

				if (*p == '+') {
					base64 = 1;
					p++;
				}
				
				for (q = p; *q && *q != ':'; q++, len++)
					;

				tmp = xstrmid(p, 0, len);

				gg_debug(GG_DEBUG_MISC, "// got string variable \"%s\"\n", tmp);

				if (base64) {
					val = base64_decode(tmp);
					xfree(tmp);
				} else
					val = tmp;

				p += len + 1;
			}

			gg_debug(GG_DEBUG_MISC, "// setting variable %s = \"%s\"\n", v->name, val);

			variable_set(v->name, val, 0);
			
			xfree(val);
		}
	}

	return 0;
}
