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

/*
 * XXX TODO:
 * - escapowanie w put_log().
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pwd.h>
#include <limits.h>
#ifndef _AIX
#  include <string.h>
#endif
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef WITH_IOCTLD
#  include <sys/un.h>
#  include "ioctld.h"
#endif
#include <ctype.h>
#ifdef HAVE_OPENSSL
#  include "sim.h"
#  include "simlite.h"
#endif
#ifdef HAVE_ZLIB_H
#  include <zlib.h>
#endif
#include "compat.h"
#include "libgadu.h"
#include "stuff.h"
#include "dynstuff.h"
#include "themes.h"
#include "commands.h"
#include "vars.h"
#include "userlist.h"
#include "xmalloc.h"
#include "ui.h"

#ifndef PATH_MAX
#  define PATH_MAX _POSIX_PATH_MAX
#endif

struct gg_session *sess = NULL;
list_t children = NULL;
list_t aliases = NULL;
list_t watches = NULL;
list_t transfers = NULL;
list_t events = NULL;
list_t emoticons = NULL;
list_t bindings = NULL;
list_t timers = NULL;
list_t lasts = NULL;
list_t conferences = NULL;
list_t sms_away = NULL;
list_t buffers = NULL;
list_t searches = NULL;

int in_autoexec = 0;
int in_auto_away = 0;
int config_auto_reconnect = 10;
int reconnect_timer = 0;
int config_auto_away = 600;
int config_auto_save = 0;
time_t last_save = 0;
int config_log = 0;
int config_log_ignored = 0;
int config_log_status = 0;
char *config_log_path = NULL;
int config_display_color = 1;
int config_beep = 1;
int config_beep_msg = 1;
int config_beep_chat = 1;
int config_beep_notify = 1;
int config_beep_mail = 1;
int config_display_pl_chars = 1;
int config_events_delay = 3;
char *config_sound_msg_file = NULL;
char *config_sound_chat_file = NULL;
char *config_sound_sysmsg_file = NULL;
char *config_sound_notify_file = NULL;
char *config_sound_mail_file = NULL;
char *config_sound_app = NULL;
int config_uin = 0;
int last_sysmsg = 0;
char *config_password = NULL;
int config_sms_away = 0;
int config_sms_away_limit = 0;
char *config_sms_number = NULL;
char *config_sms_app = NULL;
int config_sms_max_length = 100;
int search_type = 0;
int config_changed = 0;
int config_display_ack = 1;
int config_completion_notify = 1;
int connecting = 0;
time_t last_conn_event = 0;
int config_display_notify = 1;
char *config_theme = NULL;
int config_status = GG_STATUS_AVAIL;
char *reg_password = NULL;
char *reg_email = NULL;
int config_dcc = 0;
char *config_dcc_ip = NULL;
char *config_dcc_dir = NULL;
char *config_reason = NULL;
char *home_dir = NULL;
char *config_quit_reason = NULL;
char *config_away_reason = NULL;
char *config_back_reason = NULL;
int config_random_reason = 0;
int config_query_commands = 0;
char *config_proxy = NULL;
char *config_server = NULL;
int quit_message_send = 0;
int registered_today = 0;
int config_protocol = 0;
int pipe_fd = -1;
int batch_mode = 0;
char *batch_line = NULL;
int immediately_quit = 0;
int config_emoticons = 1;
int config_make_window = 0;
int ekg_segv_handler = 0;
char *config_tab_command = NULL;
int ioctld_sock = -1;
int config_ctrld_quits = 1;
int config_save_password = 1;
char *config_timestamp = NULL;
int config_display_sent = 0;
int config_sort_windows = 0;
int config_last_size = 10;
int config_last = 0;
int config_keep_reason = 0;
int config_enter_scrolls = 0;
int server_index = 0;
char *config_audio_device = NULL;
char *config_speech_app = NULL;
int config_encryption = 0;
char *config_log_timestamp = NULL;
int config_server_save = 0;
char *config_email = NULL;
int config_time_deviation = 300;
int config_mesg_allow = 2;
int config_display_welcome = 1;
int config_auto_back = 0;
int config_display_crap = 1;
char *config_display_color_map = NULL;
int config_windows_save = 0;
char *config_windows_layout = NULL;
char *config_profile = NULL;
int config_header_size = 0;
int config_statusbar_size = 1;
char *config_proxy_forwarding = NULL;
int config_password_cp1250 = 0;
char *config_interface = NULL;

char *last_search_first_name = NULL;
char *last_search_last_name = NULL;
char *last_search_nickname = NULL;
uin_t last_search_uin = 0;

struct event_label event_labels[] = {
	{ EVENT_MSG, "msg" },
	{ EVENT_CHAT, "chat" },
	{ EVENT_AVAIL, "avail" },
	{ EVENT_NOT_AVAIL, "notavail" },
	{ EVENT_AWAY, "away" },
	{ EVENT_DCC, "dcc" },
	{ EVENT_INVISIBLE, "invisible" },
	{ EVENT_DESCR, "descr" },
	{ EVENT_ONLINE, "online" },
	{ EVENT_SIGUSR1, "sigusr1" },
	{ EVENT_SIGUSR2, "sigusr2" },
	{ EVENT_DELIVERED, "delivered" },
	{ EVENT_QUEUED, "queued" },
	{ EVENT_NEW_MAIL, "newmail" },
	{ EVENT_QUERY, "query" },
	{ EVENT_BLOCKED, "blocked" },
	{ INACTIVE_EVENT, NULL },
	{ 0, NULL }
};

/*
 * alias_add()
 *
 * dopisuje alias do listy alias�w.
 *
 *  - string - linia w formacie 'alias cmd',
 *  - quiet - czy wypluwa� mesgi na stdout,
 *  - append - czy dodajemy kolejn� komend�?
 *
 * 0/-1
 */
int alias_add(const char *string, int quiet, int append)
{
	char *cmd;
	list_t l;
	struct alias a;
	char *params = NULL;

	if (!string || !(cmd = strchr(string, ' ')))
		return -1;

	*cmd++ = 0;

	for (l = aliases; l; l = l->next) {
		struct alias *j = l->data;

		if (!strcasecmp(string, j->name)) {
			if (!append) {
				printq("aliases_exist", string);
				return -1;
			} else {
				list_t l;

				list_add(&j->commands, cmd, strlen(cmd) + 1);
				
				/* przy wielu komendach trudno dope�nia�, bo wg. kt�rej? */
				for (l = commands; l; l = l->next) {
					struct command *c = l->data;

					if (!strcasecmp(c->name, j->name)) {
						xfree(c->params);
						c->params = xstrdup("?");
						break;
					}
				}
			
				printq("aliases_append", string);

				return 0;
			}
		}
	}

	for (l = commands; l; l = l->next) {
		struct command *c = l->data;

		if (!strcasecmp(string, c->name) && !c->alias) {
			printq("aliases_command", string);
			return -1;
		}

		if (!strcasecmp(cmd, c->name))
			params = xstrdup(c->params);
	}

	a.name = xstrdup(string);
	a.commands = NULL;
	list_add(&a.commands, cmd, strlen(cmd) + 1);
	list_add(&aliases, &a, sizeof(a));

	command_add(a.name, (params) ? params: "?", cmd_alias_exec, 1, "", "", "");
	
	xfree(params);

	printq("aliases_add", a.name, "");

	return 0;
}

/*
 * alias_remove()
 *
 * usuwa alias z listy alias�w.
 *
 *  - name - alias lub NULL,
 *  - quiet.
 *
 * 0/-1
 */
int alias_remove(const char *name, int quiet)
{
	list_t l;
	int removed = 0;

	for (l = aliases; l; ) {
		struct alias *a = l->data;

		l = l->next;

		if (!name || !strcasecmp(a->name, name)) {
			if (name)
				printq("aliases_del", name);
			command_remove(a->name);
			xfree(a->name);
			list_destroy(a->commands, 1);
			list_remove(&aliases, a, 1);
			removed = 1;
		}
	}

	if (!removed) {
		if (name) {
			printq("aliases_noexist", name);
		} else {
			printq("aliases_list_empty");
		}

		return -1;
	}

	if (removed && !name)
		printq("aliases_del_all");

	return 0;
}

/*
 * alias_free()
 *
 * usuwa pami�� zaj�t� przez aliasy.
 */
void alias_free()
{
	list_t l;

	if (!aliases)
		return;

	for (l = aliases; l; l = l->next) {
		struct alias *a = l->data;
		
		xfree(a->name);
		list_destroy(a->commands, 1);
	}

	list_destroy(aliases, 1);
	aliases = NULL;
}

/*
 * base64_encode()
 *
 * zapisuje ci�g znak�w w base64. alokuje pami��. 
 */
char *base64_encode(const char *buf)
{
	char *tmp = gg_base64_encode(buf);

	if (!buf)
		return xstrdup("");

	if (!tmp)
		ekg_oom_handler();

	return tmp;
}

/*
 * base64_decode()
 *
 * wczytuje ci�g znak�w base64, zwraca zaalokowany buforek.
 */
char *base64_decode(const char *buf)
{
	char *tmp = gg_base64_decode(buf);

	if (!buf)
		return xstrdup("");

	if (!tmp)
		ekg_oom_handler();

	return tmp;
}

/*
 * binding_list()
 *
 * wy�wietla list� przypisanych komend.
 */
void binding_list(int quiet, const char *name, int all) 
{
	list_t l;

	if (!bindings)
		printq("bind_seq_list_empty");

	for (l = bindings; l; l = l->next) {
		struct binding *b = l->data;

		if (name) {
			if (strcasestr(b->key, name))
				printq("bind_seq_list", b->key, b->action);
			continue;
		}

		if (!b->internal || (all && b->internal))
			printq("bind_seq_list", b->key, b->action);
	}
}

/*
 * binding_free()
 *
 * zwalnia pami�� po li�cie przypisanych klawiszy.
 */
void binding_free() 
{
	list_t l;

	if (!bindings)
		return;

	for (l = bindings; l; l = l->next) {
		struct binding *b = l->data;

		xfree(b->key);
		xfree(b->action);
		xfree(b->arg);
		xfree(b->default_action);
		xfree(b->default_arg);
	}

	list_destroy(bindings, 1);
	bindings = NULL;
}

/*
 * buffer_add()
 *
 * dodaje linijk� do danego typu bufora. je�li max_lines > 0
 * to pilnuje, aby w buforze by�o maksymalnie tyle linii.
 *
 *  - type,
 *  - line,
 *  - max_lines.
 *
 * 0/-1
 */
int buffer_add(int type, const char *target, const char *line, int max_lines)
{
	struct buffer b;

	if (max_lines && buffer_count(type) >= max_lines) {
		struct buffer *foo = buffers->data;

		xfree(foo->line);
		list_remove(&buffers, foo, 1);
	}

	b.type = type;
	b.target = xstrdup(target);
	b.line = xstrdup(line);

	return ((list_add(&buffers, &b, sizeof(b)) ? 0 : -1));
}

/* 
 * buffer_flush()
 *
 * zwraca zaalokowany �ancuch zawieraj�cy wszystkie linie
 * z bufora danego typu.
 *
 *  - type,
 *  - target - dla kogo by� bufor? NULL, je�li olewamy.
 */
char *buffer_flush(int type, const char *target)
{
	string_t str = string_init(NULL);
	list_t l;

	for (l = buffers; l; ) {
		struct buffer *b = l->data;

		l = l->next;

		if (type != b->type)
			continue;

		if (target && b->target && strcmp(target, b->target))
			continue;

		string_append(str, b->line);
		string_append_c(str, '\n');

		xfree(b->line);
		xfree(b->target);
		list_remove(&buffers, b, 1);
	}

	return string_free(str, 0);
}

/*
 * buffer_count()
 *
 * zwraca liczb� linii w buforze danego typu.
 */
int buffer_count(int type)
{
	list_t l;
	int count = 0;

	for (l = buffers; l; l = l->next) {
		struct buffer *b = l->data;

		if (b->type == type)
			count++;
	}	

	return count;
}

/*
 * buffer_free()
 * 
 * czy�ci pami�� po buforach.
 */
void buffer_free()
{
	list_t l;

	if (!buffers)
		return;

	for (l = buffers; l; l = l->next) {
		struct buffer *b = l->data;

		xfree(b->line);
		xfree(b->target);
	}

	list_destroy(buffers, 1);
	buffers = NULL;
}

/*
 * changed_backlog_size()
 *
 * wywo�ywane po zmianie warto�ci zmiennej ,,backlog_size''.
 */
void changed_backlog_size(const char *var)
{
#ifdef WITH_UI_NCURSES
	int min = ui_screen_height - 1 - config_statusbar_size - config_header_size;

	if (config_backlog_size < min)
		config_backlog_size = min;
#endif
}

/*
 * changed_dcc()
 *
 * funkcja wywo�ywana przy zmianie warto�ci zmiennej ,,dcc''.
 */
void changed_dcc(const char *var)
{
	struct userlist *u = userlist_find(config_uin, NULL);
	struct gg_dcc *dcc = NULL;
	list_t l;
	
	if (!config_uin)
		return;
	
	if (!strcmp(var, "dcc")) {
		for (l = watches; l; l = l->next) {
			struct gg_common *c = l->data;
	
			if (c->type == GG_SESSION_DCC_SOCKET)
				dcc = l->data;
		}
	
		if (!config_dcc && dcc) {
			list_remove(&watches, dcc, 0);
			gg_free_dcc(dcc);
		}
	
		if (config_dcc && !dcc) {
			if (!(dcc = gg_dcc_socket_create(config_uin, 0))) {
				print("dcc_create_error", strerror(errno));
			} else {
				list_add(&watches, dcc, 0);
			}
		}

		if (u && dcc)
			u->port = dcc->port;
	}

	if (!strcmp(var, "dcc_ip")) {
		if (config_dcc_ip) {
			if (!strcasecmp(config_dcc_ip, "auto")) {
				gg_dcc_ip = inet_addr("255.255.255.255");
			} else {
				if (inet_addr(config_dcc_ip) != INADDR_NONE)
					gg_dcc_ip = inet_addr(config_dcc_ip);
				else {
					print("dcc_invalid_ip");
					config_dcc_ip = NULL;
					gg_dcc_ip = 0;
				}
			}
		} else
			gg_dcc_ip = 0;

		if (u)
			u->ip.s_addr = gg_dcc_ip;
	}

	update_status_myip();
}
	
/*
 * changed_proxy()
 *
 * funkcja wywo�ywana przy zmianie warto�ci zmiennej ,,proxy''.
 */
void changed_proxy(const char *var)
{
	char **auth, **userpass = NULL, **hostport = NULL;
	
	gg_proxy_port = 0;
	xfree(gg_proxy_host);
	gg_proxy_host = NULL;
	xfree(gg_proxy_username);
	gg_proxy_username = NULL;
	xfree(gg_proxy_password);
	gg_proxy_password = NULL;

	if (!config_proxy)
		return;

	auth = array_make(config_proxy, "@", 0, 0, 0);

	if (!auth[0] || !strcmp(auth[0], ""))
		return; 
	
	gg_proxy_enabled = 1;

	if (auth[0] && auth[1]) {
		userpass = array_make(auth[0], ":", 0, 0, 0);
		hostport = array_make(auth[1], ":", 0, 0, 0);
	} else
		hostport = array_make(auth[0], ":", 0, 0, 0);
	
	if (userpass && userpass[0] && userpass[1]) {
		gg_proxy_username = xstrdup(userpass[0]);
		gg_proxy_password = xstrdup(userpass[1]);
	}

	gg_proxy_host = xstrdup(hostport[0]);
	gg_proxy_port = (hostport[1]) ? atoi(hostport[1]) : 8080;

	array_free(hostport);
	array_free(userpass);
	array_free(auth);
}

/*
 * changed_theme()
 *
 * funkcja wywo�ywana przy zmianie warto�ci zmiennej ,,theme''.
 */
void changed_theme(const char *var)
{
	if (!config_theme) {
		theme_init();
		ui_event("theme_init");
	} else {
		if (!theme_read(config_theme, 1)) {
			theme_cache_reset();
			if (!in_autoexec)
				print("theme_loaded", config_theme);
		} else
			if (!in_autoexec)
				print("error_loading_theme", strerror(errno));
	}
}

/*
 * changed_uin()
 *
 * funkcja wywo�ywana przy zmianie zmiennej uin.
 */
void changed_uin(const char *var)
{
	ui_event("xterm_update");
}

/*
 * changed_xxx_reason()
 *
 * funkcja wywo�ywana przy zmianie domy�lnych powod�w.
 */
void changed_xxx_reason(const char *var)
{
	char *tmp = NULL;

	if (!strcmp(var, "away_reason"))
		tmp = config_away_reason;
	if (!strcmp(var, "back_reason"))
		tmp = config_back_reason;
	if (!strcmp(var, "quit_reason"))
		tmp = config_quit_reason;

	if (!tmp)
		return;

	if (strlen(tmp) > GG_STATUS_DESCR_MAXSIZE)
		print("descr_too_long", itoa(strlen(tmp) - GG_STATUS_DESCR_MAXSIZE));
}

const char *compile_time()
{
	return __DATE__ " " __TIME__;
}

/*
 * conference_add()
 *
 * dopisuje konferencje do listy konferencji.
 *
 *  - name - nazwa konferencji,
 *  - nicklist - lista nick�w, grup, czegokolwiek,
 *  - quiet - czy wypluwa� mesgi na stdout.
 *
 * zaalokowan� struct conference lub NULL w przypadku b��du.
 */
struct conference *conference_add(const char *name, const char *nicklist, int quiet)
{
	struct conference c;
	char **nicks, **p;
	char *buf, *tmp;
	list_t l;
	int i, count;

	memset(&c, 0, sizeof(c));

	if (!name || !nicklist)
		return NULL;

	buf = xstrdup(nicklist);
	tmp = buf;
	buf = strip_spaces(buf);
	
	if (buf[0] == ',' || buf[strlen(buf) - 1] == ',') {
		printq("invalid_params", "chat");
		xfree(tmp);
		return NULL;
	}

	xfree(tmp);

	nicks = array_make(nicklist, " ,", 0, 1, 0);

	/* grupy zamieniamy na niki */
	for (i = 0; nicks[i]; i++) {
		if (nicks[i][0] == '@') {
			char *gname = xstrdup(nicks[i] + 1);
			int first = 0;
			int nig = 0; /* nicks in group */

		        for (l = userlist; l; l = l->next) {
				struct userlist *u = l->data;
				list_t m;

				for (m = u->groups; m; m = m->next) {
					struct group *g = m->data;

					if (!strcasecmp(gname, g->name)) {
						if (first++)
							array_add(&nicks, xstrdup(u->display));
						else {
							xfree(nicks[i]);
							nicks[i] = xstrdup(u->display);
						}

						nig++;

						break;
					}
				}
			}

			if (!nig) {
				printq("group_empty", gname);
				printq("conferences_not_added", name);

				xfree(gname);

				return NULL;
			}

			xfree(gname);
		}
	}

	count = array_count(nicks);

	for (l = conferences; l; l = l->next) {
		struct conference *cf = l->data;
		
		if (!strcasecmp(name, cf->name)) {
			printq("conferences_exist", name);

			array_free(nicks);

			return NULL;
		}
	}

	for (p = nicks, i = 0; *p; p++) {
		uin_t uin;

		if (!strcmp(*p, ""))
		        continue;

		if (!(uin = get_uin(*p))) {
			printq("user_not_found", *p);
			continue;
		}


		list_add(&(c.recipients), &uin, sizeof(uin));
		i++;
	}

	array_free(nicks);

	if (i != count) {
		printq("conferences_not_added", name);
		return NULL;
	}

	printq("conferences_add", name);

	c.name = xstrdup(name);

	add_send_nick(name);

	return list_add(&conferences, &c, sizeof(c));
}

/*
 * conference_remove()
 *
 * usuwa konferencje z listy konferencji.
 *
 *  - name - konferencja lub NULL dla wszystkich,
 *  - quiet.
 *
 * 0/-1
 */
int conference_remove(const char *name, int quiet)
{
	list_t l;
	int removed = 0;

	for (l = conferences; l; ) {
		struct conference *c = l->data;

		l = l->next;

		if (!name || !strcasecmp(c->name, name)) {
			if (name)
				printq("conferences_del", name);
			remove_send_nick(c->name);
			xfree(c->name);
			list_destroy(c->recipients, 1);
			list_remove(&conferences, c, 1);
			removed = 1;
		}
	}

	if (!removed) {
		if (name) {
			printq("conferences_noexist", name);
		} else {
			printq("conferences_list_empty");
		}
		
		return -1;
	}

	if (removed && !name)
		printq("conferences_del_all");

	return 0;
}

/*
 * conference_create()
 *
 * tworzy nowa konferencje z wygenerowana nazwa.
 *
 *  - nicks - lista nikow tak, jak dla polecenia conference.
 */
struct conference *conference_create(const char *nicks)
{
	struct conference *c;
	static int count = 1;
	char *name = saprintf("#conf%d", count);

	if ((c = conference_add(name, nicks, 0)))
		count++;

	xfree(name);

	return c;
}

/*
 * conference_find()
 *
 * znajduje i zwraca wska�nik do konferencji lub NULL.
 *
 *  - name - nazwa konferencji.
 */
struct conference *conference_find(const char *name) 
{
	list_t l;

	for (l = conferences; l; l = l->next) {
		struct conference *c = l->data;

		if (!strcmp(c->name, name))
			return c;
	}
	
	return NULL;
}

/*
 * conference_find_by_uins()
 *
 * znajduje konferencj�, do kt�rej nale�� podane uiny. je�eli nie znaleziono,
 * zwracany jest NULL.
 * 
 *  - from - kto jest nadawc� wiadomo�ci,
 *  - recipients - tablica numer�w nale��cych do konferencji,
 *  - count - ilo�� numer�w.
 */
struct conference *conference_find_by_uins(uin_t from, uin_t *recipients, int count) 
{
	int i;
	list_t l, r;

	for (l = conferences; l; l = l->next) {
		struct conference *c = l->data;
		int matched = 0;

		for (r = c->recipients; r; r = r->next) {
			for (i = 0; i <= count; i++) {
				uin_t uin = (i == count) ? from : recipients[i];
				
				if (uin == *((uin_t *) (r->data))) {
					matched++;
					break;
				}
			}
		}

		if (matched == list_count(c->recipients) && matched == (from == config_uin ? count : count + 1))
			return l->data;
	}

	return NULL;
}

/*
 * conference_set_ignore()
 *
 * ustawia stan konferencji na ignorowany lub nie.
 *
 *  - name - nazwa konferencji,
 *  - flag - 1 ignorowa�, 0 nie ignorowa�,
 *  - quiet.
 *
 * 0/-1
 */
int conference_set_ignore(const char *name, int flag, int quiet)
{
	struct conference *c = NULL;

	c = conference_find(name);

	if (!c) {
		printq("conferences_noexist", name);
		return -1;
	}

	c->ignore = flag ? 1 : 0;
	printq(flag ? "conferences_ignore" : "conferences_unignore", name);

	return 0;
}

/*
 * conference_rename()
 *
 * zmienia nazw� instniej�cej konferencji.
 * 
 *  - oldname - stara nazwa,
 *  - newname - nowa nazwa,
 *  - quiet.
 *
 * 0/-1
 */
int conference_rename(const char *oldname, const char *newname, int quiet)
{
	struct conference *c;
	
	if (conference_find(newname)) {
		printq("conferences_exist", newname);
		return -1;
	}

	if (!(c = conference_find(oldname))) {
		printq("conference_noexist", oldname);
		return -1;
	}

	xfree(c->name);
	c->name = xstrdup(newname);
	remove_send_nick(oldname);
	add_send_nick(newname);
	
	printq("conferences_rename", oldname, newname);

	ui_event("conference_rename", oldname, newname, NULL);
	
	return 0;
}

/*
 * conference_free()
 *
 * usuwa pami�� zaj�t� przez konferencje.
 */
void conference_free()
{
	list_t l;

	if (!conferences)
		return;

	for (l = conferences; l; l = l->next) {
		struct conference *c = l->data;
		
		xfree(c->name);
		list_destroy(c->recipients, 1);
	}

	list_destroy(conferences, 1);
	conferences = NULL;
}

/*
 * config_read()
 *
 * czyta z pliku ~/.gg/config lub podanego konfiguracj�.
 *
 *  - filename,
 *  - var - zmienna lub NULL, je�li wszystkie.
 *
 * 0/-1
 */
int config_read(const char *filename, const char *var)
{
	char *buf, *foo;
	FILE *f;
	int even_started = 0, home = ((filename) ? 0 : 1);
	struct stat st;

	if (!filename && !(filename = prepare_path("config", 0)))
		return -1;

	if (!(f = fopen(filename, "r")))
		return -1;

	if (stat(filename, &st) || !S_ISREG(st.st_mode)) {
		errno = EINVAL;
		fclose(f);
		return -1;
	}

	gg_debug(GG_DEBUG_MISC, "// config_read(%s);\n", ((var) ? var : ""));

	if (!in_autoexec && !var) {
		list_t l;

		for (l = userlist; l; l = l->next) {
			struct userlist *u = l->data;

			if (ignored_check(u->uin))
				ignored_remove(u->uin);
		}

		for (l = bindings; l; ) {
			struct binding *b = l->data;

			l = l->next;

			if (!b->internal)
				ui_event("command", 1, "bind", "--del", b->key, NULL, NULL);
		}

		alias_free();
		timer_remove_user(-1);
		event_free();
		variable_free();
		variable_init();
		variable_set_default();

		gg_debug(GG_DEBUG_MISC, "\tflushed previous config\n");
	}

	while ((buf = read_file(f))) {
		if (buf[0] == '#' || buf[0] == ';' || (buf[0] == '/' && buf[1] == '/')) {
			xfree(buf);
			continue;
		}

		if (!(foo = strchr(buf, ' '))) {
			xfree(buf);
			continue;
		}

		even_started = 1;
		*foo++ = 0;

		if (var && strcmp(buf, var)) {
			xfree(buf);
			continue;
		}

		if (!strcasecmp(buf, "set")) {
			char *bar;

			if (!(bar = strchr(foo, ' ')))
				variable_set(foo, NULL, 1);
			else {
				*bar++ = 0;
				variable_set(foo, bar, 1);
			}
		} else if (!strcasecmp(buf, "ignore")) {
			if (atoi(foo))
				ignored_add(atoi(foo), IGNORE_ALL);
		} else if (!strcasecmp(buf, "alias")) {
			gg_debug(GG_DEBUG_MISC, "\talias %s\n", foo);
			alias_add(foo, 1, 1);
		} else if (!strcasecmp(buf, "on")) {
                        int flags;
                        char **pms = array_make(foo, " \t", 3, 1, 0);

                        if (pms && pms[0] && pms[1] && pms[2] && (flags = event_flags(pms[0]))) {
                                event_add(flags, pms[1], pms[2], 1);
				gg_debug(GG_DEBUG_MISC, "\ton %s %s %s\n", pms[0], pms[1], pms[2]);
			}

			array_free(pms);
		} else if (!strcasecmp(buf, "bind")) {
			char **pms = array_make(foo, " \t", 2, 1, 0);

			if (pms && pms[0] && pms[1]) {
				ui_event("command", 1, "bind", "--add", pms[0], pms[1], NULL);
				gg_debug(GG_DEBUG_MISC, "\tbind %s %s\n", pms[0], pms[1]);
			}

			array_free(pms);
		} else if (!strcasecmp(buf, "timer") || !strcasecmp(buf, "at")) {
			char **p = array_make(foo, " \t", 3, 1, 0);
			char *tmp = NULL, *period_str = NULL, *name = NULL;
			time_t period;
			int at = !strcasecmp(buf, "at");

			if (p && p[0] && p[1] && p[2]) {
				gg_debug(GG_DEBUG_MISC, "\t%s %s %s %s\n", ((at) ? "at" : "timer"), p[0], p[1], p[2]);

				if (strcmp(p[0], "(null)"))
					name = p[0];

				if (!strncmp(p[1], "*/", 2)) {
					period = atoi(p[1] + 2);
					period_str = saprintf("*/%d", period);
				} else
					if (at)	 {
						struct tm *t;

						period = atoi(p[1]);
						t = localtime(&period);
						period_str = xmalloc(100);
						strftime(period_str, 100, "%Y%m%d%H%M.%S", t);
					} else {
						period = atoi(p[1]) - time(NULL);
						period_str = saprintf("%d", period);
					}
		
				if (period > 0) {
					tmp = saprintf("^%s --add %s %s %s", (at) ? "at" : "timer", (name) ? name : "", period_str, p[2]);
					command_exec(NULL, tmp);
					xfree(tmp);
				}

				xfree(period_str);
			}
				array_free(p);
                } else {
			int wrong = variable_set(buf, foo, 1);

			if (wrong && !home && !in_autoexec && !var) {
				xfree(buf);
				fclose(f);
				config_read(NULL, NULL);
				errno = EINVAL;
				return -1;
			}	
		}

		xfree(buf);
	}
	
	fclose(f);

	if (!even_started && !home && !in_autoexec && !var) {
		config_read(NULL, NULL);
		errno = EINVAL;
		return -1;
	}
	
	return 0;
}

/*
 * config_write_variable()
 *
 * zapisuje jedn� zmienn� do pliku konfiguracyjnego.
 *
 *  - f - otwarty plik konfiguracji,
 *  - v - wpis zmiennej,
 *  - base64 - czy wolno nam u�ywa� base64 i zajmowa� pami��?
 */
void config_write_variable(FILE *f, struct variable *v, int base64)
{
	if (!f || !v)
		return;

	if (v->type == VAR_STR) {
		if (*(char**)(v->ptr)) {
			if (!v->display && base64) {
				char *tmp = base64_encode(*(char**)(v->ptr));
				if (config_save_password)
					fprintf(f, "%s \001%s\n", v->name, tmp);
				xfree(tmp);
			} else 	
				fprintf(f, "%s %s\n", v->name, *(char**)(v->ptr));
		}
	} else if (v->type == VAR_FOREIGN)
		fprintf(f, "%s %s\n", v->name, (char*) v->ptr);
	else
		fprintf(f, "%s %d\n", v->name, *(int*)(v->ptr));
}

/*
 * config_write_main()
 *
 * w�a�ciwa funkcja zapisuj�ca konfiguracj� do podanego pliku.
 *
 *  - f - plik, do kt�rego piszemy,
 *  - base64 - czy kodowa� ukryte pola?
 */
void config_write_main(FILE *f, int base64)
{
	list_t l;

	if (!f)
		return;

	for (l = variables; l; l = l->next)
		config_write_variable(f, l->data, base64);

	for (l = aliases; l; l = l->next) {
		struct alias *a = l->data;
		list_t m;

		for (m = a->commands; m; m = m->next)
			fprintf(f, "alias %s %s\n", a->name, (char*) m->data);
	}

        for (l = events; l; l = l->next) {
                struct event *e = l->data;

                fprintf(f, "on %s %s %s\n", event_format(e->flags), e->target, e->action);
        }

	for (l = bindings; l; l = l->next) {
		struct binding *b = l->data;

		if (b->internal)
			continue;

		fprintf(f, "bind %s %s\n", b->key, b->action);
	}

	for (l = timers; l; l = l->next) {
		struct timer *t = l->data;
		const char *name = NULL;

		if (t->type != TIMER_COMMAND)
			continue;

		if (!t->persistent && t->ends.tv_sec - time(NULL) < 5)	/* nie ma sensu zapisywa� */
			continue;

		if (t->name && !isdigit(t->name[0]))
			name = t->name;
		else
			name = "(null)";

		if (t->at)
			fprintf(f, "at %s %s %s\n", name, itoa(t->ends.tv_sec), t->command);
		else {
			char *foo;

			if (t->persistent)
				foo = saprintf("*/%s", itoa(t->period));
			else
				foo = saprintf("%s", itoa(t->ends.tv_sec));

			fprintf(f, "timer %s %s %s\n", name, foo, t->command);

			xfree(foo);
		}
	}

}

/*
 * config_write()
 *
 * zapisuje aktualn� konfiguracj� -- zmienne i list� ignorowanych do pliku
 * ~/.gg/config lub podanego.
 *
 * 0/-1
 */
int config_write(const char *filename)
{
	FILE *f;

	if (!filename && !(filename = prepare_path("config", 1)))
		return -1;
	
	if (!(f = fopen(filename, "w")))
		return -1;
	
	fchmod(fileno(f), 0600);

	config_write_main(f, 1);

	fclose(f);
	
	return 0;
}

/*
 * config_write_partly()
 *
 * zapisuje podane zmienne, nie zmieniaj�c reszty konfiguracji.
 *  
 *  - vars - tablica z nazwami zmiennych do zapisania.
 * 
 * 0/-1.
 */
int config_write_partly(char **vars)
{
	const char *filename;
	char *newfn, *line;
	FILE *fi, *fo;
	int *wrote, i;

	if (!vars)
		return -1;

	if (!(filename = prepare_path("config", 1)))
		return -1;
	
	if (!(fi = fopen(filename, "r")))
		return -1;

	newfn = saprintf("%s.%d.%d", filename, getpid(), time(NULL));

	if (!(fo = fopen(newfn, "w"))) {
		xfree(newfn);
		fclose(fi);
		return -1;
	}
	
	wrote = xcalloc(array_count(vars) + 1, sizeof(int));
	
	fchmod(fileno(fo), 0600);

	while ((line = read_file(fi))) {
		char *tmp;

		if (line[0] == '#' || line[0] == ';' || (line[0] == '/' && line[1] == '/'))
			goto pass;

		if (!strchr(line, ' '))
			goto pass;

		if (!strncasecmp(line, "ignore ", 7))
			goto pass;

		if (!strncasecmp(line, "alias ", 6))
			goto pass;

		if (!strncasecmp(line, "on ", 3))
			goto pass;

		if (!strncasecmp(line, "bind ", 5))
			goto pass;

		tmp = line;

		if (!strncasecmp(tmp, "set ", 4))
			tmp += 4;
		
		for (i = 0; vars[i]; i++) {
			int len = strlen(vars[i]);

			if (strlen(tmp) < len + 1)
				continue;

			if (strncasecmp(tmp, vars[i], len) || tmp[len] != ' ')
				continue;
			
			config_write_variable(fo, variable_find(vars[i]), 1);

			wrote[i] = 1;
			
			xfree(line);
			line = NULL;
			
			break;
		}

		if (!line)
			continue;

pass:
		fprintf(fo, "%s\n", line);
		xfree(line);
	}

	for (i = 0; vars[i]; i++) {
		if (wrote[i])
			continue;

		config_write_variable(fo, variable_find(vars[i]), 1);
	}

	xfree(wrote);
	
	fclose(fi);
	fclose(fo);
	
	rename(newfn, filename);

	xfree(newfn);

	return 0;
}

/*
 * config_write_crash()
 *
 * funkcja zapisuj�ca awaryjnie konfiguracj�. nie powinna alokowa� �adnej
 * pami�ci.
 */
void config_write_crash()
{
	char name[32];
	char path[PATH_MAX];
	FILE *f;

	if (config_profile)
		snprintf(path, sizeof(path), "%s/%s", config_dir, config_profile);
	else
		snprintf(path, sizeof(path), "%s", config_dir);

	chdir(path);

	snprintf(name, sizeof(name), "config.%d", getpid());
	if (!(f = fopen(name, "w")))
		return;

	chmod(name, 0400);
	
	config_write_main(f, 0);
	
	fclose(f);
}

/*
 * debug_write_crash()
 *
 * zapisuje ostatnie linie z debug.
 */
void debug_write_crash()
{
	char name[32];
	FILE *f;
	char path[PATH_MAX];
	list_t l;

	if (config_profile)
		snprintf(path, sizeof(path), "%s/%s", config_dir, config_profile);
	else
		snprintf(path, sizeof(path), "%s", config_dir);

	chdir(path);

	snprintf(name, sizeof(name), "debug.%d", getpid());
	if (!(f = fopen(name, "w")))
		return;

	chmod(name, 0400);
	
	for (l = buffers; l; l = l->next) {
		struct buffer *b = l->data;

		if (b->type == BUFFER_DEBUG)
			fprintf(f, "%s\n", b->line);
	}
	
	fclose(f);
}

/*
 * sysmsg_read()
 *
 * 0/-1
 */
int sysmsg_read()
{
	const char *filename;
	char *buf, *foo;
	FILE *f;

	if (!(filename = prepare_path("sysmsg", 0)))
		return -1;
	
	if (!(f = fopen(filename, "r")))
		return -1;

	while ((buf = read_file(f))) {
		if (buf[0] == '#') {
			xfree(buf);
			continue;
		}

		if (!(foo = strchr(buf, ' '))) {
			xfree(buf);
			continue;
		}

		*foo++ = 0;
		
		if (!strcasecmp(buf, "last_sysmsg")) {
			if (atoi(foo))
				last_sysmsg = atoi(foo);
		}
		
		xfree(buf);
	}
	
	fclose(f);
	
	return 0;
}

/*
 * sysmsg_write()
 *
 * 0/-1
 */
int sysmsg_write()
{
	const char *filename;
	FILE *f;

	if (!(filename = prepare_path("sysmsg", 1)))
		return -1;
	
	if (!(f = fopen(filename, "w")))
		return -1;
	
	fchmod(fileno(f), 0600);
	
	fprintf(f, "last_sysmsg %i\n", last_sysmsg);
	
	fclose(f);
	
	return 0;
}

/*
 * do_connect()
 *
 * przygotowuje wszystko pod po��czenie gg_login i ��czy si�.
 */
void do_connect()
{
	list_t l;
	struct gg_login_params p;

	for (l = watches; l; l = l->next) {
		struct gg_dcc *d = l->data;
		
		if (d->type == GG_SESSION_DCC_SOCKET) {
			gg_dcc_port = d->port;
			
		}
	}

	if (config_random_reason)
		change_status(config_status, NULL, 2);

	memset(&p, 0, sizeof(p));

	p.uin = config_uin;
	p.password = config_password;
	p.status = config_status;
	p.status_descr = config_reason;
	p.async = 1;
#ifdef HAVE_VOIP
	p.has_audio = 1;
#endif
	p.protocol_version = config_protocol;
	p.last_sysmsg = last_sysmsg;

	if (config_server) {
		char *server, **servers = array_make(config_server, ",; ", 0, 1, 0);

		if (server_index >= array_count(servers))
			server_index = 0;

		if ((server = xstrdup(servers[server_index++]))) {
			char *tmp = strchr(server, ':');
			
			if (tmp) {
				p.server_port = atoi(tmp + 1);
				*tmp = 0;
				p.server_addr = inet_addr(server);
			} else {
				p.server_port = GG_DEFAULT_PORT;
				p.server_addr = inet_addr(server);
			}

			xfree(server);
		}

		array_free(servers);
	}

	if (config_proxy_forwarding) {
		char *fwd = xstrdup(config_proxy_forwarding), *tmp = strchr(fwd, ':');

		if (!tmp) {
			p.external_addr = inet_addr(fwd);
			p.external_port = gg_dcc_port;
		} else {
			*tmp = 0;
			p.external_addr = inet_addr(fwd);
			p.external_port = atoi(tmp + 1);
		}

		xfree(fwd);
	}

	if (!config_password_cp1250)
		iso_to_cp(p.password);
	if (p.status_descr)
		iso_to_cp(p.status_descr);

	if (!(sess = gg_login(&p))) {
		print("conn_failed", format_find((errno == ENOMEM) ? "conn_failed_memory" : "conn_failed_connecting"));
		do_reconnect();
	} else
		list_add(&watches, sess, 0);

	if (!config_password_cp1250)
		cp_to_iso(p.password);
	if (p.status_descr)
		cp_to_iso(p.status_descr);
}

/*
 * do_reconnect()
 *
 * je�li jest w��czony autoreconnect, wywo�uje timer, kt�ry za podan�
 * ilo�� czasu spr�buje si� po��czy� jeszcze raz.
 */
void do_reconnect()
{
	if (config_auto_reconnect && connecting)
		reconnect_timer = time(NULL);
}

/*
 * ekg_logoff()
 *
 * roz��cza si�, zmieniaj�c uprzednio stan na niedost�pny z opisem.
 *
 *  - sess - opis sesji,
 *  - reason - pow�d, mo�e by� NULL.
 */
void ekg_logoff(struct gg_session *sess, const char *reason)
{
	if (!sess)
		return;

	if (sess->state != GG_STATE_CONNECTED || GG_S_NA(sess->status))
		return;

	if (reason) {
		char *tmp = xstrdup(reason);
		iso_to_cp(tmp);
		gg_change_status_descr(sess, GG_STATUS_NOT_AVAIL_DESCR, tmp);
		xfree(tmp);
	} else
		gg_change_status(sess, GG_STATUS_NOT_AVAIL);

	gg_logoff(sess);

	update_status();

	last_conn_event = time(NULL);
}

/*
 * ekg_hash()
 *
 * liczy prosty hash z nazwy, wykorzystywany przy przeszukiwaniu list
 * zmiennych, format�w itp.
 *
 *  - name - nazwa.
 */
int ekg_hash(const char *name)
{
	int hash = 0;

	for (; *name; name++) {
		hash ^= *name;
		hash <<= 1;
	}

	return hash;
}

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

/*
 * event_add()
 *
 * dodaje zdarzenie do listy zdarze�.
 *
 *  - flags,
 *  - target,
 *  - action,
 *  - quiet.
 *
 * 0/-1
 */
int event_add(int flags, const char *target, const char *action, int quiet)
{
        int f;
        list_t l;
        struct event e;
	uin_t uin;
	struct userlist *u;
	char **arr = NULL;

	if (!target || !action)
		return -1;

	uin = str_to_uin(target);

	if ((u = userlist_find(uin, target)))
		uin = u->uin;

	for (l = events; l; l = l->next) {
		struct event *ev = l->data;

		if (ev->target[0] == '@')
			array_add(&arr, xstrdup(ev->target));
	}

        for (l = events; l; l = l->next) {
                struct event *ev = l->data;
		int match = 0, i;

		if (target[0] == '@') {
			struct userlist *uu = userlist_find(str_to_uin(ev->target), ev->target);

			if (uu && group_member(uu, target + 1))
				match = 1;
		}

		for (i = 0; arr && arr[i]; i++) {
			if (u && group_member(u, arr[i] + 1)) {
				match = 1;
				break;
			}
		}

		if (!strcasecmp(ev->target, target) || !strcmp(ev->target, itoa(uin)) || (u && u->display && !strcasecmp(ev->target, u->display)))
			match = 1;

		if (match && (f = ev->flags & flags)) {
			printq("events_exist", event_format(f), event_format_target(target));
			array_free(arr);
			return -1;
		}
        }

	array_free(arr);

	for (f = 1; ; f++) {
		int taken = 0;
		
		for (l = events; l; l = l->next) {
			struct event *e = l->data;

			if (!strcmp(e->name, itoa(f))) {
				taken = 1;
				break;
			}
		}

		if (!taken)
			break;
	}

	e.name = xstrdup(itoa(f));
        e.target = xstrdup(target);
        e.flags = flags;
        e.action = xstrdup(action);

        list_add(&events, &e, sizeof(e));

	printq("events_add", e.name);

        return 0;
}

/*
 * event_remove()
 *
 * usuwa zdarzenie z listy zdarze�.
 *
 *  - name.
 *
 * 0/-1
 */
int event_remove(const char *name, int quiet)
{
	list_t l;
	int removed = 0, remove_all;
	
	if (!name)
		return -1;

	remove_all = !strcmp(name, "*");

	for (l = events; l; ) {
		struct event *e = l->data;

		l = l->next;

		if (remove_all || (name && e->name && !strcasecmp(name, e->name))) {
			if (!remove_all)
				printq("events_del", name);
			xfree(e->action);
			list_remove(&events, e, 1);
			removed = 1;
        	}
	}

	if (!removed) {
		if (remove_all) {
			printq("events_list_empty");
		} else {
        		printq("events_del_noexist", name);
		}

        	return -1;
	} else {
		if (remove_all)
			printq("events_del_all");

		return 0;
	}
}

/*
 * event_format()
 *
 * zwraca �a�cuch zdarze� w oparciu o flagi. statyczny bufor.
 *
 *  - flags.
 */
const char *event_format(int flags)
{
        static char buf[200];
	int i, first = 1;

	buf[0] = 0;

	if (flags == EVENT_ALL)
		return "*";

	for (i = 0; event_labels[i].name; i++) {
		if ((flags & event_labels[i].event)) {
			if (!first)
				strncat(buf, ",", sizeof(buf) - 1 - strlen(buf));
			strncat(buf, event_labels[i].name, sizeof(buf) - 1 - strlen(buf));
			buf[sizeof(buf) - 1] = 0;
			first = 0;
		}
	}

	return buf;
}

/*
 * event_format_target()
 *
 * zajmuje si� sformatowaniem uina/aliasu w �ancuchu
 * znak�w ze zdarzenia. nie zwalnia�.
 */
const char *event_format_target(const char *target)
{
	struct userlist *u;
	uin_t uin;

	if (!target)
		return "";

	uin = str_to_uin(target);
	u = userlist_find(0, target);

	if (u)
		return format_user(u->uin);

	if (uin)
		return format_user(uin);
	else
		return target;
}

/*
 * event_flags()
 *
 * zwraca flagi na podstawie �a�cucha.
 *
 *  - events.
 */
int event_flags(const char *events)
{
	int i, j, flags = 0;
	char **a;

	if (!(a = array_make(events, "|,:", 0, 1, 0)))
		return 0;

	for (j = 0; a[j]; j++) {
		if (!strcmp(a[j], "*")) {
			flags = EVENT_ALL;
			break;
		}
		for (i = 0; event_labels[i].name; i++)
			if (!strcasecmp(a[j], event_labels[i].name))
				flags |= event_labels[i].event;
	}

	array_free(a);

	return flags;
}

/*
 * event_check()
 *
 * sprawdza i ewentualnie uruchamia akcj� na podane zdarzenie.
 *
 *  - event,
 *  - uin,
 *  - data.
 *
 * 0/-1
 */
int event_check(int event, uin_t uin, const char *data)
{
	const char *uin_number = NULL, *uin_display = NULL;
        char *action = NULL, **actions, *edata = NULL;
	struct userlist *u;
	int i;
        list_t l;

	uin_number = itoa(uin);

	if ((u = userlist_find(uin, NULL)))
		uin_display = u->display;
	else
		uin_display = uin_number;

        for (l = events; l; l = l->next) {
                struct event *e = l->data;

		if (e->flags & INACTIVE_EVENT)
			return 0;
		
                if (e->flags & event) {

			if (!strcmp(e->target, "*") || uin == 1)
				action = e->action;

			if (!strcmp(e->target, uin_number) || !strcasecmp(e->target, uin_display) || (u && group_member(u, e->target + 1))) {
				action = e->action;
				break;
			}
        	}
	}

        if (!action)
                return -1;

	if (data) {
		int size = 1;
		const char *p;
		char *q;

		for (p = data; *p; p++) {
			if (strchr("`!#$&*?|\\\'\"{}[]<>()\n\r", *p))
				size += 2;
			else
				size++;
		}
		
		edata = xmalloc(size);

		for (p = data, q = edata; *p; p++, q++) {
			if (strchr("`!#$&*?|\\\'\"{}[]<>()", *p))
				*q++ = '\\';
			if (*p == '\n') {
				*q++ = '\\';
				*q = 'n';
				continue;
			}
			if (*p == '\r') {
				*q++ = '\\';
				*q = 'r';
				continue;
			}
			*q = *p;
		}

		*q = 0;
	}

	actions = array_make(action, ";", 0, 0, 0);

	for (i = 0; actions && actions[i]; i++) {	
		char *tmp = format_string(actions[i], uin_number, uin_display, ((data) ? data : ""), ((edata) ? edata : ""));

		if (!i)
			gg_debug(GG_DEBUG_MISC, "// event_check();\n");

		gg_debug(GG_DEBUG_MISC, "//    event: %d doing: %s\n", event, tmp);
		command_exec(NULL, tmp);
		xfree(tmp);
	}

	array_free(actions);
	xfree(edata);

        return 0;
}

/*
 * event_free()
 *
 * zwalnia pami�� zwi�zan� ze zdarzeniami.
 */
void event_free()
{
	list_t l;

	if (!events)
		return;

	for (l = events; l; l = l->next) {
		struct event *e = l->data;

		xfree(e->action);
	}

	list_destroy(events, 1);
	events = NULL;
}

/*
 * last_add()
 *
 * dodaje wiadomo�� do listy ostatnio otrzymanych.
 * 
 *  - type - rodzaj wiadomo�ci,
 *  - uin - nadawca,
 *  - t - czas,
 *  - st - czas nadania,
 *  - msg - tre�� wiadomo�ci.
 */
void last_add(int type, uin_t uin, time_t t, time_t st, const char *msg)
{
	list_t l;
	struct last ll;
	int count = 0;

	/* nic nie zapisujemy, je�eli user sam nie wie czego chce. */
	if (config_last_size <= 0)
		return;
	
	if (config_last & 2) 
		count = last_count(uin);
	else
		count = list_count(lasts);
				
	/* usuwamy ostatni� wiadomo��, w razie potrzeby... */
	if (count >= config_last_size) {
		time_t tmp_time = 0;
		
		/* najpierw j� znajdziemy... */
		for (l = lasts; l; l = l->next) {
			struct last *lll = l->data;

			if (config_last & 2 && lll->uin != uin)
				continue;

			if (!tmp_time)
				tmp_time = lll->time;
			
			if (lll->time <= tmp_time)
				tmp_time = lll->time;
		}
		
		/* ...by teraz usun�� */
		for (l = lasts; l; l = l->next) {
			struct last *lll = l->data;

			if (lll->time == tmp_time && lll->uin == uin) {
				xfree(lll->message);
				list_remove(&lasts, lll, 1);
				break;
			}
		}

	}

	ll.type = type;
	ll.uin = uin;
	ll.time = t;
	ll.sent_time = st;
	ll.message = xstrdup(msg);
	
	list_add(&lasts, &ll, sizeof(ll));

	return;
}

/*
 * last_del()
 *
 * usuwa wiadomo�ci skojarzone z dan� osob�.
 *
 *  - uin - numerek osoby.
 */
void last_del(uin_t uin)
{
	list_t l;

	for (l = lasts; l; ) {
		struct last *ll = l->data;

		l = l->next;

		if (uin == ll->uin) {
			xfree(ll->message);
			list_remove(&lasts, ll, 1);
		}
	}
}

/*
 * last_count()
 *
 * zwraca ilo�� wiadomo�ci w last dla danej osoby.
 *
 *  - uin.
 */
int last_count(uin_t uin)
{
	int count = 0;
	list_t l;

	for (l = lasts; l; l = l->next) {
		struct last *ll = l->data;

		if (uin == ll->uin)
			count++;
	}

	return count;
}

/*
 * last_free()
 *
 * zwalnia miejsce po last.
 */
void last_free()
{
	list_t l;

	if (!lasts)
		return;

	for (l = lasts; l; l = l->next) {
		struct last *ll = l->data;
		
		xfree(ll->message);
	}

	list_destroy(lasts, 1);
	lasts = NULL;
}

/*
 * put_log()
 *
 * wrzuca do log�w informacj� od/do danego numerka. podaje si� go z tego
 * wzgl�du, �e gdy `log = 2', informacje lec� do $config_log_path/$uin.
 *
 *  - uin - numer delikwenta,
 *  - format... - akceptuje tylko %s, %d i %ld.
 */
void put_log(uin_t uin, const char *format, ...)
{
 	char *lp = config_log_path;
	char path[PATH_MAX], *buf;
	const char *p;
	int size = 0;
	va_list ap;
	FILE *f;

	if (!config_log)
		return;

	/* oblicz d�ugo�� tekstu */
	va_start(ap, format);
	for (p = format; *p; p++) {
		if (*p == '%') {
			p++;
			if (!*p)
				break;
			
			if (*p == 'l') {
				p++;
				if (!*p)
					break;
			}
			
			if (*p == 's') {
				char *tmp = va_arg(ap, char*);

				size += strlen(tmp);
			}
			
			if (*p == 'd') {
				int tmp = va_arg(ap, int);

				size += strlen(itoa(tmp));
			}
		} else
			size++;
	}
	va_end(ap);

	/* zaalokuj bufor */
	buf = xmalloc(size + 1);
	*buf = 0;

	/* utw�rz tekst z logiem */
	va_start(ap, format);
	for (p = format; *p; p++) {
		if (*p == '%') {
			p++;
			if (!*p)
				break;
			if (*p == 'l') {
				p++;
				if (!*p)
					break;
			}

			if (*p == 's') {
				char *tmp = va_arg(ap, char*);

				strcat(buf, tmp);
			}

			if (*p == 'd') {
				int tmp = va_arg(ap, int);

				strcat(buf, itoa(tmp));
			}
		} else {
			buf[strlen(buf) + 1] = 0;
			buf[strlen(buf)] = *p;
		}
	}

	/* teraz skonstruuj �cie�k� log�w */

	if (!lp)
		lp = (config_log & 2) ? "." : "gg.log";

	if (*lp == '~')
		snprintf(path, sizeof(path), "%s%s", home_dir, lp + 1);
	else {
		strncpy(path, lp, sizeof(path) - 1);
		path[sizeof(path) - 1] = 0;
	}

	if ((config_log & 2)) {
		if (mkdir(path, 0700) && errno != EEXIST)
			goto cleanup;
		snprintf(path + strlen(path), sizeof(path) - strlen(path), "/%u", uin);
	}

#ifdef HAVE_ZLIB
	/* nawet je�li chcemy gzipowane logi, a istnieje nieskompresowany log,
	 * olewamy kompresj�. je�li loga nieskompresowanego nie ma, dodajemy
	 * rozszerzenie .gz i balujemy. */
	if (config_log & 4) {
		struct stat st;
		
		if (stat(path, &st) == -1) {
			gzFile f;

			snprintf(path + strlen(path), sizeof(path) - strlen(path), ".gz");

			if (!(f = gzopen(path, "a")))
				goto cleanup;

			gzputs(f, buf);
			gzclose(f);
			chmod(path, 0600);

			goto cleanup;
		}
	}
#endif

	if (!(f = fopen(path, "a")))
		goto cleanup;
	fputs(buf, f);
	fclose(f);
	chmod(path, 0600);

cleanup:
	xfree(buf);
}

/*
 * log_escape()
 *
 * je�li trzeba, eskejpuje tekst do log�w.
 * 
 *  - str - tekst.
 *
 * zaalokowany bufor.
 */
char *log_escape(const char *str)
{
	const char *p;
	char *res, *q;
	int size, needto = 0;

	if (!str)
		return NULL;
	
	for (p = str; *p; p++) {
		if (*p == '"' || *p == '\'' || *p == '\r' || *p == '\n' || *p == ',')
			needto = 1;
	}

	if (!needto)
		return xstrdup(str);

	for (p = str, size = 0; *p; p++) {
		if (*p == '"' || *p == '\'' || *p == '\r' || *p == '\n' || *p == '\\')
			size += 2;
		else
			size++;
	}

	q = res = xmalloc(size + 3);
	
	*q++ = '"';
	
	for (p = str; *p; p++, q++) {
		if (*p == '\\' || *p == '"' || *p == '\'') {
			*q++ = '\\';
			*q = *p;
		} else if (*p == '\n') {
			*q++ = '\\';
			*q = 'n';
		} else if (*p == '\r') {
			*q++ = '\\';
			*q = 'r';
		} else
			*q = *p;
	}
	*q++ = '"';
	*q = 0;

	return res;
}

/* 
 * log_timestamp()
 *
 * zwraca timestamp log�w zgodnie z �yczeniem u�ytkownika. 
 *
 *  - t - czas, kt�ry mamy zamieni�.
 *
 * zwraca na przemian jeden z dw�ch statycznych bufor�w, wi�c w obr�bie
 * jednego wyra�enia mo�na wywo�a� t� funkcj� dwukrotnie.
 */
const char *log_timestamp(time_t t)
{
	static char buf[2][100];
	struct tm *tm = localtime(&t);
	static int i = 0;

	i = i % 2;

	if (config_log_timestamp) {
		strftime(buf[i], sizeof(buf[0]), config_log_timestamp, tm);
		return buf[i++];
	} else
		return itoa(t);
}

/*
 * mesg_set()
 *
 * w��cza/wy��cza mo�liwo�� pisania do naszego terminala za pomoc�
 * write/talk/wall.
 *
 * - what - 0 wy��cza, 1 w��cza, 2 zwraca aktualne ustawienie.
 * 
 * -1 je�li b�ad, lub aktualny stan (0/1)
*/
int mesg_set(int what)
{
	const char *tty;
	struct stat s;

	if (!(tty = ttyname(1)))
		return -1;

	if (!stat(tty, &s))
		return -1;

	switch (what) {
		case 0:
			chmod(tty, s.st_mode & ~S_IWGRP);
			break;
		case 1:
			chmod(tty, s.st_mode | S_IWGRP);
			break;
		case 2:
			return ((s.st_mode & S_IWGRP) ? 1 : 0);
	}

	return 0;
}

/*
 * mesg_changed()
 *
 * wywo�ywane przy zmianie ustawie�.
*/
void mesg_changed()
{
	if ((config_mesg_allow == 0 || config_mesg_allow == 1) && mesg_set(2) != config_mesg_allow)
		mesg_set(config_mesg_allow);
}

/*
 * msg_encrypt()
 * 
 * je�li mo�na, podmienia wiadomo�� na wiadomo��
 * zaszyfrowan�.
 */
int msg_encrypt(uin_t uin, unsigned char **msg)
{
#ifdef HAVE_OPENSSL
	int len;
		
	if (config_encryption == 1) {
		unsigned char *enc = xmalloc(4096);	/* XXX idiotyzm */
		memset(enc, 0, 4096);
		
		len = SIM_Message_Encrypt(*msg, enc, strlen(*msg), uin);
		
		gg_debug(GG_DEBUG_MISC, "// encrypted length: %d\n", len);

		if (len > 0) {
			xfree(*msg);
			*msg = enc;
			gg_debug(GG_DEBUG_MISC, "// encrypted message: %s\n", enc);
		} else
			xfree(enc);

		return len;
	} else {
		char *res;

		res = sim_message_encrypt(*msg, uin);
		if (res) {
			xfree(*msg);
			*msg = res;
			gg_debug(GG_DEBUG_MISC, "// simlite encrypted: %s\n", res);
			return 1;
		}
		return 0;
	}
#else
	return 0;
#endif
}

/*
 * cp_to_iso()
 *
 * zamienia krzaczki pisane w cp1250 na iso-8859-2, przy okazji maskuj�c
 * znaki, kt�rych nie da si� wy�wietli�, za wyj�tkiem \r i \n.
 *
 *  - buf.
 */
void cp_to_iso(unsigned char *buf)
{
	if (!buf)
		return;

	while (*buf) {
		if (*buf == (unsigned char)'�') *buf = '�';
		if (*buf == (unsigned char)'�') *buf = '�';
		if (*buf == 140) *buf = '�';
		if (*buf == 156) *buf = '�';
		if (*buf == 143) *buf = '�';
		if (*buf == 159) *buf = '�';

                if (*buf != 13 && *buf != 10 && (*buf < 32 || (*buf > 127 && *buf < 160)))
                        *buf = '?';

		buf++;
	}
}

/*
 * iso_to_cp()
 *
 * zamienia sensowny format kodowania polskich znaczk�w na bezsensowny.
 *
 *  - buf.
 */
void iso_to_cp(unsigned char *buf)
{
	if (!buf)
		return;

	while (*buf) {
		if (*buf == (unsigned char)'�') *buf = '�';
		if (*buf == (unsigned char)'�') *buf = '�';
		if (*buf == (unsigned char)'�') *buf = '�';
		if (*buf == (unsigned char)'�') *buf = '�';
		if (*buf == (unsigned char)'�') *buf = '�';
		if (*buf == (unsigned char)'�') *buf = '�';
		buf++;
	}
}

/*
 * hide_pl()
 *
 * maskuje polsk� literk� w iso.
 *
 *  - c.
 */
unsigned char hide_pl(const unsigned char *c)
{
	if (!c)
		return 0;

	if (*c == (unsigned char)'�') return 'a';
	if (*c == (unsigned char)'�') return 'e';
	if (*c == (unsigned char)'�') return 'c';
	if (*c == (unsigned char)'�') return 'l';
	if (*c == (unsigned char)'�') return 'n';
	if (*c == (unsigned char)'�') return 'o';
	if (*c == (unsigned char)'�') return 's';
	if (*c == (unsigned char)'�') return 'z';
	if (*c == (unsigned char)'�') return 'z';

	if (*c == (unsigned char)'�') return 'A';
	if (*c == (unsigned char)'�') return 'E';
	if (*c == (unsigned char)'�') return 'C';
	if (*c == (unsigned char)'�') return 'L';
	if (*c == (unsigned char)'�') return 'N';
	if (*c == (unsigned char)'�') return 'O';
	if (*c == (unsigned char)'�') return 'S';
	if (*c == (unsigned char)'�') return 'Z';
	if (*c == (unsigned char)'�') return 'Z';

	return *c;
}

/*
 * strip_spaces()
 *
 * pozbywa si� spacji na pocz�tku i ko�cu �a�cucha.
 */
char *strip_spaces(char *line)
{
	char *buf;
	
	for (buf = line; isspace(*buf); buf++);

	while (isspace(line[strlen(line) - 1]))
		line[strlen(line) - 1] = 0;
	
	return buf;
}

/*
 * find_in_uins()
 *
 * sprawdza, czy w ci�gu uin'�w znajduje si� dany uin.
 *
 * 1 je�li znaleziono, 0 je�li nie.
 */
int find_in_uins(int uin_count, uin_t *uins, uin_t uin)
{
	int i;

	for (i = 0; i < uin_count; i++)
		if (uins[i] == uin)
			return 1;

	return 0;
}

/*
 * str_to_uin()
 *
 * funkcja, kt�ra zajmuje si� zamian� stringa na 
 * liczb� i sprawdzeniem, czy to prawid�owy uin.
 *
 * zwraca uin lub 0 w przypadku b��du.
 */
uin_t str_to_uin(const char *text)
{
	char *tmp;
	long num;

	errno = 0;
	num = strtol(text, &tmp, 0);

	if (*text == '\0' || *tmp != '\0')
		return 0;

	if ((errno == ERANGE || (num == LONG_MAX || num == LONG_MIN)) || num > UINT_MAX || num < 0)
		return 0;

	return (uin_t) num;
}

/*
 * valid_nick()
 *
 * sprawdza, czy nick nie zawiera znak�w specjalnych,
 * co mog�oby powodowa� problemy.
 *
 * zwraca 1 je�li nick jest w porz�dku, w przeciwnym razie 0.
 */
int valid_nick(const char *nick)
{
	int i;
	const char *wrong[] = { "(null)", "__debug", "__status",
				 "__current", "*", NULL };

	if (!nick)
		return 0;

	for (i = 0; wrong[i]; i++) {
		if (!strcmp(nick, wrong[i]))
			return 0;
	}

	if (nick[0] == '@' || nick[0] == '#' || strchr(nick, ','))
		return 0;

	return 1;
}

/*
 * play_sound()
 *
 * odtwarza dzwi�k o podanej nazwie.
 *
 * 0/-1
 */
int play_sound(const char *sound_path)
{
	int pid;

	if (!config_sound_app || !sound_path) {
		errno = EINVAL;
		return -1;
	}

	if ((pid = fork()) == -1)
		return -1;

	if (!pid) {
		int i;

		for (i = 0; i < 255; i++)
			close(i);
			
		execlp(config_sound_app, config_sound_app, sound_path, (void *) NULL);
		exit(1);
	}

	process_add(pid, "\003");

	return 0;
}

/*
 * process_add()
 *
 * dopisuje do listy uruchomionych dzieci proces�w.
 *
 *  - pid.
 *  - name.
 *
 * 0/-1
 */
int process_add(int pid, const char *name)
{
	struct process p;

	p.pid = pid;
	p.name = xstrdup(name);
	
	return (list_add(&children, &p, sizeof(p)) ? 0 : -1);
}

/*
 * process_remove()
 *
 * usuwa proces z listy dzieciak�w.
 *
 *  - pid.
 * 
 * 0/-1
 */
int process_remove(int pid)
{
	list_t l;

	for (l = children; l; l = l->next) {
		struct process *p = l->data;

		if (p->pid == pid) {
			list_remove(&children, p, 1);
			return 0;
		}
	}

	return -1;
}

/*
 * prepare_path()
 *
 * zwraca pe�n� �cie�k� do podanego pliku katalogu ~/.gg/
 *
 *  - filename - nazwa pliku,
 *  - do_mkdir - czy tworzy� katalog ~/.gg ?
 */
const char *prepare_path(const char *filename, int do_mkdir)
{
	static char path[PATH_MAX];
	
	if (do_mkdir) {
		if (mkdir(config_dir, 0700) && errno != EEXIST)
			return NULL;
		if (config_profile) {
			snprintf(path, sizeof(path), "%s/%s", config_dir, config_profile);
			if (mkdir(path, 0700) && errno != EEXIST)
				return NULL;
		}
	}
	
	if (!filename || !*filename) {
		if (config_profile)
			snprintf(path, sizeof(path), "%s/%s", config_dir, config_profile);
		else
			snprintf(path, sizeof(path), "%s", config_dir);
	} else {
		if (config_profile)
			snprintf(path, sizeof(path), "%s/%s/%s", config_dir, config_profile, filename);
		else
			snprintf(path, sizeof(path), "%s/%s", config_dir, filename);
	}
	
	return path;
}

char *random_line(const char *path)
{
	int max = 0, item, tmp = 0;
	char *line;
	FILE *f;

	if (!path)
		return NULL;

	if ((f = fopen(path, "r")) == NULL)
		return NULL;
	
	 while ((line = read_file(f))) {
		xfree(line);
		max++;
	}

	rewind(f);

	item = rand() % max;

	while ((line = read_file(f))) {
		if (tmp == item) {
			fclose(f);
			return line;
		}
		xfree(line);
		tmp++;
	}

	fclose(f);
	return NULL;
}

/*
 * read_file()
 *
 * czyta i zwraca linijk� tekstu z pliku alokuj�c przy tym odpowiedni buforek.
 * usuwa znaki ko�ca linii.
 */
char *read_file(FILE *f)
{
	char buf[1024], *res = NULL;

	while (fgets(buf, sizeof(buf) - 1, f)) {
		int first = (res) ? 0 : 1;
		int new_size = ((res) ? strlen(res) : 0) + strlen(buf) + 1;

		res = xrealloc(res, new_size);
		if (first)
			*res = 0;
		strcpy(res + strlen(res), buf);
		
		if (strchr(buf, '\n'))
			break;
	}

	if (res && strlen(res) > 0 && res[strlen(res) - 1] == '\n')
		res[strlen(res) - 1] = 0;
	if (res && strlen(res) > 0 && res[strlen(res) - 1] == '\r')
		res[strlen(res) - 1] = 0;

	return res;
}

/*
 * send_sms()
 *
 * wysy�a sms o podanej tre�ci do podanej osoby.
 *
 * 0/-1
 */
int send_sms(const char *recipient, const char *message, int show_result)
{
	int pid, fd[2] = { 0, 0 };
	struct gg_exec s;
	char *tmp;

	if (!config_sms_app) {
		errno = EINVAL;
		return -1;
	}

	if (!recipient || !message) {
		errno = EINVAL;
		return -1;
	}

	if (pipe(fd))
		return -1;
		
	if (!(pid = fork())) {
		if (fd[1]) {
			close(fd[0]);
			dup2(fd[1], 2);
			dup2(fd[1], 1);
			close(fd[1]);
		}	
		execlp(config_sms_app, config_sms_app, recipient, message, (void *) NULL);
		exit(1);
	}

	if (pid < 0) {
		close(fd[0]);
		close(fd[1]);
		return -1;
	}

	memset(&s, 0, sizeof(s));
	
	s.fd = fd[0];
	s.check = GG_CHECK_READ;
	s.state = GG_STATE_READING_DATA;
	s.type = GG_SESSION_USER3;
	s.id = pid;
	s.timeout = 60;
	s.buf = string_init(NULL);

	fcntl(s.fd, F_SETFL, O_NONBLOCK);

	list_add(&watches, &s, sizeof(s));
	close(fd[1]);
	
	tmp = saprintf((show_result) ? "\001%s" : "\002%s", recipient);
	process_add(pid, tmp);
	xfree(tmp);

	return 0;
}

/*
 * sms_away_add()
 *
 * dodaje osob� do listy delikwent�w, od kt�rych wiadomo�� wys�ano sms'em
 * podczas naszej nieobecno�ci. je�li jest ju� na li�cie, to zwi�ksza 
 * przyporz�dkowany mu licznik.
 *
 *  - uin.
 */
void sms_away_add(uin_t uin)
{
	struct sms_away sa;
	list_t l;

	if (!config_sms_away_limit)
		return;

	sa.uin = uin;
	sa.count = 1;

	for (l = sms_away; l; l = l->next) {
		struct sms_away *s = l->data;

		if (s->uin == uin) {
			s->count += 1;
			return;
		}
	}

	list_add(&sms_away, &sa, sizeof(sa));
}

/*
 * sms_away_check()
 *
 * sprawdza czy wiadomo�� od danej osoby mo�e zosta� przekazana
 * na sms podczas naszej nieobecno�ci, czy te� mo�e przekroczono
 * ju� limit.
 *
 *  - uin
 *
 * 1 je�li tak, 0 je�li nie.
 */
int sms_away_check(uin_t uin)
{
	int x = 0;
	list_t l;

	if (!config_sms_away_limit || !sms_away)
		return 1;

	/* limit dotyczy ��cznej liczby sms'�w */
	if (config_sms_away == 1) {
		for (l = sms_away; l; l = l->next) {
			struct sms_away *s = l->data;

			x += s->count;
		}
		
		if (x > config_sms_away_limit)
			return 0;
		else
			return 1;
	}

	/* limit dotyczy liczby sms'�w od jednej osoby */
	for (l = sms_away; l; l = l->next) {
		struct sms_away *s = l->data;

		if (s->uin == uin) {
			if (s->count > config_sms_away_limit)
				return 0;
			else 
				return 1;
		}
	}

	return 1;
}

/*
 * sms_away_free()
 *
 * pozbywa si� listy sms_away.
 */
void sms_away_free()
{
	if (!sms_away)
		return;

	list_destroy(sms_away, 1);
	sms_away = NULL;
}

#ifdef WITH_IOCTLD

/*
 * ioctld_parse_seq()
 *
 * zamie� string na odpowiedni� struktur� dla ioctld.
 *
 *  - seq,
 *  - data.
 *
 * 0/-1.
 */
int ioctld_parse_seq(const char *seq, struct action_data *data)
{
	char **entries;
	int i;

        if (!data || !seq)
                return -1;

	memset(data, 0, sizeof(struct action_data));

	entries = array_make(seq, ",", 0, 0, 1);

	for (i = 0; entries[i] && i < IOCTLD_MAX_ITEMS; i++) {
		int delay;
		char *tmp;
		
		if ((tmp = strchr(entries[i], '/'))) {
			*tmp = 0;
			delay = atoi(tmp + 1);
		} else
			delay = IOCTLD_DEFAULT_DELAY;
			
		data->value[i] = atoi(entries[i]);
		data->delay[i] = delay;
	}

	array_free(entries);

	return 0;
}

/*
 * ioctld_socket()
 *
 * inicjuje gniazdo dla ioctld.
 *
 *  - path - �cie�ka do gniazda.
 *
 * 0/-1.
 */
int ioctld_socket(char *path)
{
	struct sockaddr_un sockun;
	int i, usecs = 50000;

	if (ioctld_sock != -1)
		close(ioctld_sock);

	if ((ioctld_sock = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
		return -1;

	sockun.sun_family = AF_UNIX;
	strcpy(sockun.sun_path, path);

	for (i = 5; i; i--) {
		if (connect(ioctld_sock, (struct sockaddr*) &sockun, sizeof(sockun)) != -1)
			return 0;

		usleep(usecs);
	}

        return -1;
}

/*
 * ioctld_send()
 *
 * wysy�a do ioctld polecenie uruchomienia danej akcji.
 *
 *  - seq - sekwencja danych,
 *  - act - rodzaj akcji.
 *
 * 0/-1.
 */
int ioctld_send(const char *seq, int act, int quiet)
{
	struct action_data data;

	if (*seq == '$')	/* dla kompatybilno�ci ze starym zachowaniem */
		seq++;

	if (!isdigit(*seq)) {
		const char *tmp = format_find(seq);

		if (!strcmp(tmp, "")) {
			printq("events_seq_not_found", seq);
			return -1;
		}

		seq = tmp;
	}

	if (ioctld_parse_seq(seq, &data)) {
		printq("events_seq_incorrect", seq);
		return -1;
	}

	data.act = act;

	return send(ioctld_sock, &data, sizeof(data), 0);
}

#endif /* WITH_IOCTLD */

/*
 * init_control_pipe()
 *
 * inicjuje potok nazwany do zewn�trznej kontroli ekg.
 *
 *  - pipe_file.
 *
 * zwraca deskryptor otwartego potoku lub -1.
 */
int init_control_pipe(const char *pipe_file)
{
	int fd;
	struct stat st;
	char *err_str = NULL;

	if (!pipe_file)
		return 0;

	if (!stat(pipe_file, &st) && !S_ISFIFO(st.st_mode))
		err_str = saprintf("Plik %s nie jest potokiem. Ignoruj�.\n", pipe_file);

	if (mkfifo(pipe_file, 0600) < 0 && errno != EEXIST)
		err_str = saprintf("Nie mog� stworzy� potoku %s: %s. Ignoruj�.\n", pipe_file, strerror(errno));

#ifdef O_NDELAY
	if ((fd = open(pipe_file, O_RDWR | O_NDELAY)) < 0)
#else
	if ((fd = open(pipe_file, O_RDWR)) < 0)
#endif
		err_str = saprintf("Nie mog� otworzy� potoku %s: %s. Ignoruj�.\n", pipe_file, strerror(errno));

	if (err_str) {
		print("generic_error", err_str);
		xfree(err_str);
		return -1;
	}

	return fd;
}

/*
 * timestamp()
 *
 * zwraca statyczny buforek z �adnie sformatowanym czasem.
 *
 *  - format.
 */
const char *timestamp(const char *format)
{
	static char buf[100];
	time_t t;
	struct tm *tm;

	time(&t);
	tm = localtime(&t);
	strftime(buf, sizeof(buf), format, tm);

	return buf;
}

/*
 * unidle()
 *
 * uaktualnia licznik czasu ostatniej akcji, �eby przypadkiem nie zrobi�o
 * autoawaya, kiedy piszemy.
 */
void unidle()
{
	time(&last_action);
}

/*
 * transfer_id()
 *
 * zwraca pierwszy wolny identyfikator transferu dcc.
 */
int transfer_id()
{
	list_t l;
	int id = 1;

	for (l = transfers; l; l = l->next) {
		struct transfer *t = l->data;

		if (t->id >= id)
			id = t->id + 1;
	}

	return id;
}

/*
 * on_off()
 *
 * zwraca 1 je�li tekst znaczy w��czy�, 0 je�li wy��czy�, -1 je�li co innego.
 *
 *  - value.
 */
int on_off(const char *value)
{
	if (!value)
		return -1;

	if (!strcasecmp(value, "on") || !strcasecmp(value, "true") || !strcasecmp(value, "yes") || !strcasecmp(value, "tak") || !strcmp(value, "1"))
		return 1;

	if (!strcasecmp(value, "off") || !strcasecmp(value, "false") || !strcasecmp(value, "no") || !strcasecmp(value, "nie") || !strcmp(value, "0"))
		return 0;

	return -1;
}

/*
 * timer_add()
 *
 * dodaje timera.
 *
 *  - period - za jaki czas w sekundach ma by� uruchomiony,
 *  - persistent - czy sta�y timer,
 *  - type - rodzaj timera,
 *  - at - zwyk�y timer czy at?
 *  - name - nazwa timera w celach identyfikacji. je�li jest r�wna NULL,
 *           zostanie przyznany pierwszy numerek z brzegu.
 *  - command - komenda wywo�ywana po up�yni�ciu czasu.
 *
 * zwraca zaalokowan� struct timer lub NULL.
 */
struct timer *timer_add(time_t period, int persistent, int type, int at, const char *name, const char *command)
{
	struct timer t;
	struct timeval tv;
	struct timezone tz;

	if (!name) {
		int i;

		for (i = 1; ; i++) {
			int gotit = 0;
			list_t l;

			for (l = timers; l; l = l->next) {
				struct timer *tt = l->data;

				if (!strcmp(tt->name, itoa(i))) {
					gotit = 1;
					break;
				}
			}

			if (!gotit)
				break;
		}

		name = itoa(i);
	}

	memset(&t, 0, sizeof(t));

	gettimeofday(&tv, &tz);
	tv.tv_sec += period;
	memcpy(&t.ends, &tv, sizeof(tv));
	t.period = period;
	t.name = xstrdup(name);
	t.command = xstrdup(command);
	t.type = type;
	t.at = at;
	t.persistent = persistent;

	return list_add(&timers, &t, sizeof(t));
}

/*
 * timer_remove()
 *
 * usuwa timer.
 *
 *  - name - nazwa timera, mo�e by� NULL,
 *  - at - zwyk�y timer czy at?
 *  - command - komenda timera, mo�e by� NULL.
 *
 * 0/-1
 */
int timer_remove(const char *name, int at, const char *command)
{
	list_t l;
	int removed = 0;

	for (l = timers; l; ) {
		struct timer *t = l->data;

		l = l->next;

		if ((at == t->at) && ((name && !strcasecmp(name, t->name)) || (command && !strcasecmp(command, t->command)))) { 
			xfree(t->name);
			xfree(t->command);
			xfree(t->id);
			list_remove(&timers, t, 1);
			removed = 1;
		}
	}

	return ((removed) ? 0 : -1);
}

/*
 * timer_remove_user()
 *
 * usuwa wszystkie timery u�ytkownika.
 *
 *  - at - czy to at? nie ma znaczenia, je�li r�wne -1
 *
 * 0/-1
 */
int timer_remove_user(int at)
{
	list_t l;
	int removed = 0;

	for (l = timers; l; ) {
		struct timer *t = l->data;

		l = l->next;

		if (t->type == TIMER_COMMAND && (at == -1 || at == t->at)) { 
			xfree(t->name);
			xfree(t->command);
			xfree(t->id);
			list_remove(&timers, t, 1);
			removed = 1;
		}
	}

	return ((removed) ? 0 : -1);
}

/*
 * timer_free()
 *
 * zwalnia pami�� po timerach.
 */
void timer_free()
{
	list_t l;

	for (l = timers; l; l = l->next) {
		struct timer *t = l->data;
		
		xfree(t->name);
		xfree(t->command);
		xfree(t->id);
	}

	list_destroy(timers, 1);
	timers = NULL;
}

/* 
 * xstrmid()
 *
 * wycina fragment tekstu alokuj�c dla niego pami��.
 *
 *  - str - tekst �r�d�owy,
 *  - start - pierwszy znak,
 *  - length - d�ugo�� wycinanego tekstu, je�li -1 do ko�ca.
 */
char *xstrmid(const char *str, int start, int length)
{
	char *res, *q;
	const char *p;

	if (!str)
		return xstrdup("");

	if (start > strlen(str))
		start = strlen(str);

	if (length == -1)
		length = strlen(str) - start;

	if (length < 1)
		return xstrdup("");

	if (length > strlen(str) - start)
		length = strlen(str) - start;
	
	res = xmalloc(length + 1);
	
	for (p = str + start, q = res; length; p++, q++, length--)
		*q = *p;

	*q = 0;

	return res;
}

/*
 * http_error_string()
 *
 * zwraca tekst opisuj�cy pow�d b��du us�ug po http.
 *
 *  - h - warto�� gg_http->error lub 0, gdy funkcja zwr�ci�a NULL.
 */
const char *http_error_string(int h)
{
	switch (h) {
		case 0:
			return format_find((errno == ENOMEM) ? "http_failed_memory" : "http_failed_connecting");
		case GG_ERROR_RESOLVING:
			return format_find("http_failed_resolving");
		case GG_ERROR_CONNECTING:
			return format_find("http_failed_connecting");
		case GG_ERROR_READING:
			return format_find("http_failed_reading");
		case GG_ERROR_WRITING:
			return format_find("http_failed_writing");
	}

	return "?";
}

/*
 * update_status()
 *
 * uaktualnia w�asny stan w li�cie kontakt�w, je�li dopisali�my siebie.
 */
void update_status()
{
	struct userlist *u = userlist_find(config_uin, NULL);

	if (!u)
		return;

	if ((u->descr && config_reason && strcmp(u->descr, config_reason)) || (!u->descr && config_reason) || (u->descr && !config_reason))
		event_check(EVENT_DESCR, config_uin, u->descr);

	xfree(u->descr);
	u->descr = xstrdup(config_reason);

	if (!sess || sess->state != GG_STATE_CONNECTED)
		u->status = (u->descr) ? GG_STATUS_NOT_AVAIL_DESCR : GG_STATUS_NOT_AVAIL;
	else
		u->status = config_status;

	if (ignored_check(u->uin))
		u->status = GG_STATUS_NOT_AVAIL;

	update_status_myip();
}

/*
 * update_status_myip()
 *
 * zajmuje si� wpisaniem w�asnego adresu IP i portu na li�cie.
 */
void update_status_myip()
{
	struct userlist *u = userlist_find(config_uin, NULL);

	if (!u)
		return;

	if (!sess || sess->state != GG_STATE_CONNECTED)
		goto fail;
	
	if (config_dcc && config_dcc_ip) {
		list_t l;

		if (strcmp(config_dcc_ip, "auto"))
			u->ip.s_addr = inet_addr(config_dcc_ip);
		else {
			struct sockaddr_in foo;
			int bar = sizeof(foo);

			if (getsockname(sess->fd, (struct sockaddr *) &foo, &bar))
				goto fail;

			u->ip.s_addr = foo.sin_addr.s_addr; 
		}

		u->port = 0;

		for (l = watches; l; l = l->next) {
			struct gg_dcc *d = l->data;

			if (d->type == GG_SESSION_DCC_SOCKET) {
				u->port = d->port;
				break;
			}
		}

		return;
	}

fail:
	memset(&u->ip, 0, sizeof(struct in_addr));
	u->port = 0;
}

/*
 * change_status()
 *
 * zmienia stan sesji.
 *
 *  - status - nowy stan. warto�� stanu libgadu, bez _DESCR.
 *  - reason - opis stanu, mo�e by� NULL.
 *  - autom - czy zmiana jest automatyczna?
 */
void change_status(int status, const char *arg, int autom)
{
	const char *filename, *config_x_reason, *format, *format_descr, *auto_format = NULL, *auto_format_descr = NULL;
	int random_mask, status_descr;
	char *reason = NULL, *tmp = NULL;
	int private_mask = (GG_S_F(config_status) ? GG_STATUS_FRIENDS_MASK : 0);

	status = GG_S(status);

	switch (status) {
		case GG_STATUS_BUSY:
			status_descr = GG_STATUS_BUSY_DESCR;
			random_mask = 1;
			filename = "away.reasons";
			config_x_reason = config_away_reason;
			format = "away";
			format_descr = "away_descr";
			auto_format = "auto_away";
			auto_format_descr = "auto_away_descr";
			break;
		case GG_STATUS_AVAIL:
			status_descr = GG_STATUS_AVAIL_DESCR;
			random_mask = 4;
			filename = "back.reasons";
			config_x_reason = config_back_reason;
			format = "back";
			format_descr = "back_descr";
			auto_format = "auto_back";
			auto_format_descr = "auto_back_descr";
			break;
		case GG_STATUS_INVISIBLE:
			status_descr = GG_STATUS_INVISIBLE_DESCR;
			random_mask = 8;
			filename = "quit.reasons";
			config_x_reason = config_quit_reason;
			format = "invisible";
			format_descr = "invisible_descr";
			break;
		case GG_STATUS_NOT_AVAIL:
			status_descr = GG_STATUS_NOT_AVAIL_DESCR;
			random_mask = 8;
			filename = "quit.reasons";
			config_x_reason = config_quit_reason;
			format = "invisible";
			format_descr = "invisible_descr";
			break;
		default:
			return;
	}

	if (!(autom % 60))
		tmp = saprintf("%dm", autom / 60);
	else
		tmp = saprintf("%ds", autom);
								
	if (!arg) {
		if (config_random_reason & random_mask) {
			reason = random_line(prepare_path(filename, 0));
			if (!reason && config_x_reason)
			    	reason = xstrdup(config_x_reason);
		} else if (config_x_reason)
		    	reason = xstrdup(config_x_reason);
	} else
		reason = xstrdup(arg);

	if (!reason && config_keep_reason && config_reason)
		reason = xstrdup(config_reason);
	
	if (arg && !strcmp(arg, "-")) {
		xfree(reason);
		reason = NULL;
	}

	if (reason)
		status = status_descr;

	if (reason) {
		char *r1, *r2;

		r1 = xstrmid(reason, 0, GG_STATUS_DESCR_MAXSIZE);
		r2 = xstrmid(reason, GG_STATUS_DESCR_MAXSIZE, -1);

		switch (autom) {
			case 0:
				print(format_descr, r1, r2);
				break;
			case 1:
				print(auto_format_descr, tmp, r1, r2);
				break;
			case 2:
				break;
		}
	
		xfree(reason);
		reason = r1;

		xfree(r2);
	} else {
		switch (autom) {
			case 0:
				print(format);
				break;
			case 1:
				print(auto_format, tmp);
				break;
			case 2:
				break;
		}
	}

	if (!ignored_check(config_uin)) {
		ui_event("my_status", (reason) ? format_descr : format, reason, NULL);
		ui_event("my_status_raw", status, reason, NULL);
	}

	xfree(config_reason);
	config_reason = reason;
	config_status = status | private_mask;

	if (sess && sess->state == GG_STATE_CONNECTED) {
		if (config_reason) {
		    	iso_to_cp(config_reason);
			gg_change_status_descr(sess, config_status, config_reason);
			cp_to_iso(config_reason);
		} else
		    	gg_change_status(sess, config_status);
	}
	
	xfree(tmp);

	update_status();
}

/*
 * ekg_status_label()
 *
 * tworzy etykiet� opisuj�c� stan, z podanym prefiksem.
 *
 *  - status,
 *  - prefix.
 *
 * zwraca statyczny tekst.
 */
const char *ekg_status_label(int status, const char *prefix)
{
	static char buf[100];
	const char *label = "unknown";
	
	switch (GG_S(status)) {
		case GG_STATUS_AVAIL:
			label = "avail";
			break;
		case GG_STATUS_AVAIL_DESCR:
			label = "avail_descr";
			break;
		case GG_STATUS_NOT_AVAIL:
			label = "not_avail";
			break;
		case GG_STATUS_NOT_AVAIL_DESCR:
			label = "not_avail_descr";
			break;
		case GG_STATUS_BUSY:
			label = "busy";
			break;
		case GG_STATUS_BUSY_DESCR:
			label = "busy_descr";
			break;
		case GG_STATUS_INVISIBLE:
			label = "invisible";
			break;
		case GG_STATUS_INVISIBLE_DESCR:
			label = "invisible_descr";
			break;
		case GG_STATUS_BLOCKED:
			label = "blocked";
			break;
	}

	snprintf(buf, sizeof(buf), "%s%s", (prefix) ? prefix : "", label);

	return buf;
}

struct color_map default_color_map[26] = {
	{ 'k', 0, 0, 0 },
	{ 'r', 168, 0, 0, },
	{ 'g', 0, 168, 0, },
	{ 'y', 168, 168, 0, },
	{ 'b', 0, 0, 168, },
	{ 'm', 168, 0, 168, },
	{ 'c', 0, 168, 168, },
	{ 'w', 168, 168, 168, },
	{ 'K', 96, 96, 96 },
	{ 'R', 255, 0, 0, },
	{ 'G', 0, 255, 0, },
	{ 'Y', 255, 255, 0, },
	{ 'B', 0, 0, 255, },
	{ 'M', 255, 0, 255, },
	{ 'C', 0, 255, 255, },
	{ 'W', 255, 255, 255, },

	/* dodatkowe mapowanie r�nych kolor�w istniej�cych w GG */
	{ 'C', 128, 255, 255, },
	{ 'G', 128, 255, 128, },
	{ 'M', 255, 128, 255, },
	{ 'B', 128, 128, 255, },
	{ 'R', 255, 128, 128, },
	{ 'Y', 255, 255, 128, }, 
	{ 'm', 168, 128, 168, },
	{ 'c', 128, 168, 168, },
	{ 'g', 64, 168, 64, },
	{ 'm', 128, 64, 128, }
};

/*
 * color_map()
 *
 * funkcja zwracaj�ca kod koloru z domy�lnej 16-kolorowej palety terminali
 * ansi odpadaj�cemu podanym warto�ciom RGB.
 */
char color_map(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned long mindist = 255 * 255 * 255;
	struct color_map *map = default_color_map;
	char ch = 0;
	int i;

	fprintf(stderr, "color=%.2x%.2x%.2x\n", r, g, b);

#define __sq(x) ((x)*(x))
	for (i = 0; i < 26; i++) {
		unsigned long dist = __sq(r - map[i].r) + __sq(g - map[i].g) + __sq(b - map[i].b);

		fprintf(stderr, "%d(%c)=%.2x%.2x%.2x, dist=%ld\n", i, map[i].color, map[i].r, map[i].g, map[i].b, dist);

		if (dist < mindist) {
			ch = map[i].color;
			mindist = dist;
		}
	}
#undef __sq

	fprintf(stderr, "mindist=%ld, color=%c\n", mindist, ch);

	return ch;	
}

/*
 * strcasestr()
 *
 * robi to samo co strstr() tyle �e bez zwracania uwagi na wielko��
 * znak�w.
 */
char *strcasestr(const char *haystack, const char *needle)
{
	int i, hlen = strlen(haystack), nlen = strlen(needle);

	for (i = 0; i <= hlen - nlen; i++) {
		if (!strncasecmp(haystack + i, needle, nlen))
			return (char*) (haystack + i);
	}

	return NULL;
}
