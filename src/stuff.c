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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <limits.h>
#ifndef _AIX
#  include <string.h>
#endif
#include <readline/readline.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include "config.h"
#include "libgadu.h"
#include "stuff.h"
#include "dynstuff.h"
#include "themes.h"
#include "commands.h"
#include "vars.h"

struct gg_session *sess = NULL;
struct list *userlist = NULL;
struct list *ignored = NULL;
struct list *children = NULL;
struct list *aliases = NULL;
struct list *watches = NULL;
struct list *transfers = NULL;
struct list *events = NULL;
int in_readline = 0;
int no_prompt = 0;
int away = 0;
int in_autoexec = 0;
int auto_reconnect = 10;
int reconnect_timer = 0;
int auto_away = 600;
int log = 0;
int log_ignored = 0;
char *log_path = NULL;
int display_color = 1;
int enable_beep = 1, enable_beep_msg = 1, enable_beep_chat = 1, enable_beep_notify = 1;
char *sound_msg_file = NULL;
char *sound_chat_file = NULL;
char *sound_sysmsg_file = NULL;
char *sound_app = NULL;
int config_uin = 0;
int last_sysmsg = 0;
char *config_password = NULL;
int sms_away = 0;
char *sms_number = NULL;
char *sms_send_app = NULL;
int sms_max_length = 100;
int search_type = 0;
int config_changed = 0;
int display_ack = 1;
int completion_notify = 1;
char *bold_font = NULL;		/* dla kompatybilno�ci z gnu gadu */
int private_mode = 0;
int connecting = 0;
int display_notify = 1;
char *default_theme = NULL;
int default_status = GG_STATUS_AVAIL;
char *reg_password = NULL;
int use_dcc = 0;
char *query_nick = NULL;
uin_t query_uin = 0;
int sock = 0;
int length = 0;
struct sockaddr_un addr;

/*
 * my_puts()
 *
 * wy�wietla dany tekst na ekranie, uwa�aj�c na trwaj�ce w danych chwili
 * readline().
 */
void my_puts(char *format, ...)
{
        int old_end = rl_end, i;
	char *old_prompt = rl_prompt;
        va_list ap;

        if (in_readline) {
                rl_end = 0;
                rl_prompt = "";
                rl_redisplay();
                printf("\r");
                for (i = 0; i < strlen(old_prompt); i++)
                        printf(" ");
                printf("\r");
        }

        va_start(ap, format);
        vprintf(format, ap);
        va_end(ap);

        if (in_readline) {
                rl_end = old_end;
                rl_prompt = old_prompt;
		rl_forced_update_display();
        }
}

/*
 * current_prompt() // funkcja wewn�trzna
 *
 * zwraca wska�nik aktualnego prompta. statyczny bufor, nie zwalnia�.
 */
static char *current_prompt()
{
	static char buf[80];	/* g�upio strdup()owa� wszystko */
	char *prompt;
	
	if (query_nick) {
		if ((prompt = format_string(find_format("readline_prompt_query"), query_nick, NULL))) {
			strncpy(buf, prompt, sizeof(buf)-1);
			prompt = buf;
		}
	} else {
		if (!readline_prompt)
			readline_prompt = find_format("readline_prompt");
		if (!readline_prompt_away)
			readline_prompt_away = find_format("readline_prompt_away");

		prompt = (!away) ? readline_prompt : readline_prompt_away;
	}

	if (no_prompt || !prompt)
		prompt = "";

	return prompt;
}

/*
 * my_readline()
 *
 * malutki wrapper na readline(), kt�ry przygotowuje odpowiedniego prompta
 * w zale�no�ci od tego, czy jeste�my zaj�ci czy nie i informuje reszt�
 * programu, �e jeste�my w trakcie czytania linii i je�li chc� wy�wietla�,
 * powinny najpierw sprz�tn��.
 */
char *my_readline()
{
        char *res, *prompt = current_prompt();

        in_readline = 1;
#ifdef HAS_RL_SET_PROMPT
	rl_set_prompt(prompt);
#endif
        res = readline(prompt);
        in_readline = 0;

        return res;
}

/*
 * reset_prompt()
 *
 * dostosowuje prompt aktualnego my_readline() do awaya.
 */
void reset_prompt()
{
#ifdef HAS_RL_SET_PROMPT
	rl_set_prompt(current_prompt());
	rl_redisplay();
#endif
}

/*
 * prepare_path()
 *
 * zwraca pe�n� �cie�k� do podanego pliku katalogu ~/.gg/
 *
 *  - filename - nazwa pliku.
 */
char *prepare_path(char *filename)
{
	static char path[PATH_MAX];
	char *home = getenv("HOME");
	struct passwd *pw;
	
	if (!home) {
		if (!(pw = getpwuid(getuid())))
			return NULL;
		home = pw->pw_dir;
	}
	
	if (config_user != "") {
	  snprintf(path, sizeof(path), "%s/.gg/%s/%s", home, config_user, filename);
	} else {
	  snprintf(path, sizeof(path), "%s/.gg/%s", home, filename);
	}
	
	return path;
}

/*
 * put_log()
 *
 * wrzuca do log�w informacj� od/do danego numerka. podaje si� go z tego
 * wzgl�du, �e gdy `log = 2', informacje lec� do $log_path/$uin.
 *
 * - uin,
 * - format...
 */
void put_log(uin_t uin, char *format, ...)
{
 	char *home = getenv("HOME"), *lp = log_path;
	char path[PATH_MAX];
	struct passwd *pw;
	va_list ap;
	FILE *f;

	if (!log)
		return;
	
	if (!lp) {
		if (log == 2)
			lp = ".";
		else
			lp = "gg.log";
	}

	if (*lp == '~') {
		if (!home) {
			if (!(pw = getpwuid(getuid())))
				return;
			home = pw->pw_dir;
		}
		snprintf(path, sizeof(path), "%s%s", home, lp + 1);
	} else
		strncpy(path, lp, sizeof(path));

	if (log == 2) {
		mkdir(path, 0700);
		snprintf(path + strlen(path), sizeof(path) - strlen(path), "/%lu", uin);
	}

	if (!(f = fopen(path, "a")))
		return;

	fchmod(fileno(f), 0600);

	va_start(ap, format);
	vfprintf(f, format, ap);
	va_end(ap);

	fclose(f);
}

/*
 * full_timestamp()
 *
 * zwraca statyczny bufor z pe�n� reprezentacj� czasu. pewnie b�dzie ona
 * zgodna z aktualnym ustawieniem locali. przydaje si� do log�w.
 */
char *full_timestamp()
{
	time_t t = time(NULL);
	char *foo = ctime(&t);

	foo[strlen(foo) - 1] = 0;

	return foo;
}

/*
 * read_config()
 *
 * czyta z pliku ~/.gg/config lub podanego konfiguracj� i list� ignorowanych
 * u�yszkodnik�w.
 *
 *  - filename.
 */
int read_config(char *filename)
{
	char *buf, *foo;
	FILE *f;

	if (!filename) {
		if (!(filename = prepare_path("config")))
			return -1;
	}
	
	if (!(f = fopen(filename, "r")))
		return -1;

	while ((buf = read_file(f))) {
		if (buf[0] == '#') {
			free(buf);
			continue;
		}

		if (!(foo = strchr(buf, ' '))) {
			free(buf);
			continue;
		}

		*foo++ = 0;

		if (!strcasecmp(buf, "ignore")) {
			if (atoi(foo))
				add_ignored(atoi(foo));
		} else if (!strcasecmp(buf, "alias")) {
			add_alias(foo, 1);
		} else if (!strcasecmp(buf, "on")) {
                        int flags;
                        uin_t uin;
                        char **pms = split_params(foo, 3);
                        if ((flags = get_flags(pms[0])) && (uin = atoi(pms[1]))
&& !correct_event(pms[2]))
                                add_event(get_flags(pms[0]), atoi(pms[1]), strdup(pms[2]));
                        else
                            my_printf("config_line_incorrect");
                } else
			set_variable(buf, foo);

		free(buf);
	}
	
	fclose(f);
	
	return 0;
}

/*
 * read_sysmsg()
 *
 *  - filename.
 */
int read_sysmsg(char *filename)
{
	char *buf, *foo;
	FILE *f;

	if (!filename) {
		if (!(filename = prepare_path("sysmsg")))
			return -1;
	}
	
	if (!(f = fopen(filename, "r")))
		return -1;

	while ((buf = read_file(f))) {
		if (buf[0] == '#') {
			free(buf);
			continue;
		}

		if (!(foo = strchr(buf, ' '))) {
			free(buf);
			continue;
		}

		*foo++ = 0;
		
		if (!strcasecmp(buf, "last_sysmsg")) {
			if (atoi(foo))
				last_sysmsg = atoi(foo);
		}
		free(buf);
	}
	
	fclose(f);
	
	return 0;
}


/*
 * write_config()
 *
 * zapisuje aktualn� konfiguracj� -- zmienne i list� ignorowanych do pliku
 * ~/.gg/config lub podanego.
 *
 *  - filename.
 */
int write_config(char *filename)
{
	struct variable *v = variables;
	struct list *l;
	char *tmp;
	FILE *f;

	if (!(tmp = prepare_path("")))
		return -1;
	mkdir(tmp, 0700);

	if (!filename) {
		if (!(filename = prepare_path("config")))
			return -1;
	}
	
	if (!(f = fopen(filename, "w")))
		return -1;
	
	fchmod(fileno(f), 0600);

	while (v->name) {
		if (v->type == VAR_STR) {
			if (*(char**)(v->ptr)) {
				if (!v->display) {
					char *tmp = encode_base64(*(char**)(v->ptr));
					fprintf(f, "%s \001%s\n", v->name, tmp);
					free(tmp);
				} else 	
					fprintf(f, "%s %s\n", v->name, *(char**)(v->ptr));
			}
		} else
			fprintf(f, "%s %d\n", v->name, *(int*)(v->ptr));
		v++;
	}	

	for (l = ignored; l; l = l->next) {
		struct ignored *i = l->data;

		fprintf(f, "ignore %lu\n", i->uin);
	}

	for (l = aliases; l; l = l->next) {
		struct alias *a = l->data;

		fprintf(f, "alias %s %s\n", a->alias, a->cmd);
	}

        for (l = events; l; l = l->next) {
                struct event *e = l->data;

                fprintf(f, "on %s %lu %s\n", format_events(e->flags), e->uin, e->action);
        }

	fclose(f);
	
	return 0;
}

/*
 * write_sysmsg()
 *
 *  - filename.
 */
int write_sysmsg(char *filename)
{
	char *tmp;
	FILE *f;

	if (!(tmp = prepare_path("")))
		return -1;
	mkdir(tmp, 0700);

	if (!filename) {
		if (!(filename = prepare_path("sysmsg")))
			return -1;
	}
	
	if (!(f = fopen(filename, "w")))
		return -1;
	
	fchmod(fileno(f), 0600);
	
	fprintf(f, "last_sysmsg %i\n", last_sysmsg);
	
	fclose(f);
	
	return 0;
}

/*
 * get_token()
 *
 * zwraca kolejny token oddzielany podanym znakiem. niszczy wej�ciowy
 * ci�g znak�w. bo po co on komu?
 *
 *  - ptr - gdzie ma zapisywa� aktualn� pozycj� w ci�gu,
 *  - sep - znak oddzielaj�cy tokeny.
 */
char *get_token(char **ptr, char sep)
{
	char *foo, *res;

	if (!ptr || !sep || !*ptr)
		return NULL;

	res = *ptr;

	if (!(foo = strchr(*ptr, sep)))
		*ptr += strlen(*ptr);
	else {
		*ptr = foo + 1;
		*foo = 0;
	}

	return res;
}

/*
 * userlist_compare()
 *
 * funkcja pomocna przy list_add_sorted().
 *
 *  - data1, data2 - dwa wpisy userlisty do por�wnania.
 *
 * zwraca wynik strcasecmp() na nazwach user�w.
 */
int userlist_compare(void *data1, void *data2)
{
	struct userlist *a = data1, *b = data2;
	
	if (!a || !a->comment || !b || !b->comment)
		return 0;

	return strcasecmp(a->comment, b->comment);
}

/*
 * read_userlist()
 *
 * wczytuje list� kontakt�w z pliku ~/.gg/userlist. mo�e ona by� w postaci
 * linii ,,<numerek> <opis>'' lub w postaci eksportu tekstowego listy
 * kontakt�w windzianego klienta.
 *
 *  - filename.
 */
int read_userlist(char *filename)
{
	FILE *f;
	char *buf;

	if (!filename) {
		if (!(filename = prepare_path("userlist")))
			return -1;
	}
	
	if (!(f = fopen(filename, "r")))
		return -1;

	while ((buf = read_file(f))) {
		struct userlist u;
		char *comment;
		
		if (buf[0] == '#') {
			free(buf);
			continue;
		}

		if (!strchr(buf, ';')) {
			if (!(comment = strchr(buf, ' '))) {
				free(buf);
				continue;
			}

			u.uin = strtol(buf, NULL, 0);
		
			if (!u.uin) {
				free(buf);
				continue;
			}

			u.first_name = NULL;
			u.last_name = NULL;
			u.nickname = NULL;
			u.comment = strdup(++comment);
			u.mobile = NULL;
			u.group = NULL;

		} else {
			char *first_name, *last_name, *nickname, *comment, *mobile, *group, *uin, *foo = buf;

			first_name = get_token(&foo, ';');
			last_name = get_token(&foo, ';');
			nickname = get_token(&foo, ';');
			comment = get_token(&foo, ';');
			mobile = get_token(&foo, ';');
			group = get_token(&foo, ';');
			uin = get_token(&foo, ';');

			if (!uin || !(u.uin = strtol(uin, NULL, 0))) {
				free(buf);
				continue;
			}

			u.first_name = strdup(first_name);
			u.last_name = strdup(last_name);
			u.nickname = strdup(nickname);
			u.comment = strdup(comment);
			u.mobile = strdup(mobile);
			u.group = strdup(group);
		}

		free(buf);

		u.status = GG_STATUS_NOT_AVAIL;

		list_add_sorted(&userlist, &u, sizeof(u), userlist_compare);
	}
	
	fclose(f);

	return 0;
}

/*
 * write_userlist()
 *
 * zapisuje list� kontakt�w w pliku ~/.gg/userlist
 *
 *  - filename.
 */
int write_userlist(char *filename)
{
	struct list *l;
	char *tmp;
	FILE *f;

	if (!(tmp = prepare_path("")))
		return -1;
	mkdir(tmp, 0700);

	if (!filename) {
		if (!(filename = prepare_path("userlist")))
			return -1;
	}
	
	if (!(f = fopen(filename, "w")))
		return -2;

	fchmod(fileno(f), 0600);
	
	for (l = userlist; l; l = l->next) {
		struct userlist *u = l->data;

		fprintf(f, "%s;%s;%s;%s;%s;%s;%lu\r\n", (u->first_name) ?
			u->first_name : u->comment, (u->last_name) ?
			u->last_name : "", (u->nickname) ? u->nickname :
			u->comment, u->comment, (u->mobile) ? u->mobile :
			"", (u->group) ? u->group : "", u->uin);
	}	

	fclose(f);
	
	return 0;
}

/*
 *
 * clear_userlist()
 *
 * czysci tablice userlist
 *
 */

void clear_userlist(void) {
        struct list *l;

        for (l = userlist; l; l = l->next) {
                struct userlist *u = l->data;
                u->status = GG_STATUS_NOT_AVAIL;
        };
};

/*
 * add_user()
 *
 * dodaje u�ytkownika do listy.
 *
 *  - uin,
 *  - comment.
 */
int add_user(uin_t uin, char *comment)
{
	struct userlist u;

	u.uin = uin;
	u.status = GG_STATUS_NOT_AVAIL;
	u.first_name = NULL;
	u.last_name = NULL;
	u.nickname = NULL;
	u.mobile = NULL;
	u.group = NULL;
	u.comment = strdup(comment);

	list_add_sorted(&userlist, &u, sizeof(u), userlist_compare);
	
	return 0;
}

/*
 * del_user()
 *
 * usuwa danego u�ytkownika z listy kontakt�w.
 *
 *  - uin.
 */
int del_user(uin_t uin)
{
	struct list *l;
	
	for (l = userlist; l; l = l->next) {
		struct userlist *u = l->data;

		if (u->uin == uin) {
			free(u->first_name);
			free(u->last_name);
			free(u->nickname);
			free(u->comment);
			free(u->mobile);
			free(u->group);

			list_remove(&userlist, u, 1);

			return 0;
		}
	}

	return -1;
}

/*
 * replace_user()
 *
 * usuwa i dodaje na nowo u�ytkownika, �eby zosta� umieszczony na odpowiednim
 * (pod wzgl�dem kolejno�ci alfabetycznej) miejscu. g�upie to troch�, ale
 * przy listach jednokierunkowych nie za bardzo chce mi si� miesza� z
 * przesuwaniem element�w listy.
 * 
 *  - u.
 *
 * zwraca zero je�li jest ok, -1 je�li b��d.
 */
int replace_user(struct userlist *u)
{
	if (list_remove(&userlist, u, 0))
		return -1;
	if (list_add_sorted(&userlist, u, 0, userlist_compare))
		return -1;

	return 0;
}

/*
 * find_user()
 *
 * znajduje odpowiedni� struktur� `userlist' odpowiadaj�c� danemu numerkowi
 * lub jego opisowi.
 *
 *  - uin,
 *  - comment.
 */
struct userlist *find_user(uin_t uin, char *comment)
{
	struct list *l;

	for (l = userlist; l; l = l->next) {
		struct userlist *u = l->data;

                if (uin && u->uin == uin)
			return u;
                if (comment && !strcasecmp(u->comment, comment))
                        return u;
        }

        return NULL;
}

/*
 * get_uin()
 *
 * je�li podany tekst jest liczb�, zwraca jej warto��. je�li jest nazw�
 * u�ytkownika w naszej li�cie kontakt�w, zwraca jego numerek. inaczej
 * zwraca zero.
 *
 *  - text.
 */
uin_t get_uin(char *text)
{
	uin_t uin = atoi(text);
	struct userlist *u;

	if (!uin) {
		if (!(u = find_user(0, text)))
			return 0;
		uin = u->uin;
	}

	return uin;
}

/*
 * format_user()
 *
 * zwraca �adny (ew. kolorowy) tekst opisuj�cy dany numerek. je�li jest
 * w naszej li�cie kontakt�w, formatuje u�ywaj�c `known_user', w przeciwnym
 * wypadku u�ywa `unknown_user'. wynik jest w statycznym buforze.
 *
 *  - uin - numerek danej osoby.
 */
char *format_user(uin_t uin)
{
	struct userlist *u = find_user(uin, NULL);
	static char buf[100], *tmp;
	
	if (!u)
		tmp = format_string(find_format("unknown_user"), itoa(uin));
	else
		tmp = format_string(find_format("known_user"), u->comment, itoa(uin));
	
	strncpy(buf, tmp, sizeof(buf) - 1);
	
	free(tmp);

	return buf;
}

/*
 * del_ignored()
 *
 * usuwa z listy ignorowanych numerk�w.
 *
 *  - uin.
 */
int del_ignored(uin_t uin)
{
	struct list *l;

	for (l = ignored; l; l = l->next) {
		struct ignored *i = l->data;

		if (i->uin == uin) {
			list_remove(&ignored, i, 1);
			return 0;
		}
	}

	return -1;
}

/*
 * add_ignored()
 *
 * dopisuje do listy ignorowanych numerk�w.
 *
 *  - uin.
 */
int add_ignored(uin_t uin)
{
	struct list *l;
	struct ignored i;

	for (l = ignored; l; l = l->next) {
		struct ignored *j = l->data;

		if (j->uin == uin)
			return -1;
	}

	i.uin = uin;
	list_add(&ignored, &i, sizeof(i));
	
	return 0;
}

/*
 * is_ignored()
 *
 * czy dany numerek znajduje si� na li�cie ignorowanych.
 *
 *  - uin.
 */
int is_ignored(uin_t uin)
{
	struct list *l;

	for (l = ignored; l; l = l->next) {
		struct ignored *i = l->data;

		if (i->uin == uin)
			return 1;
	}

	return 0;
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
 * timestamp()
 *
 * zwraca statyczny buforek z �adnie sformatowanym czasem.
 *
 *  - format.
 */
char *timestamp(char *format)
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
 * parse_autoexec()
 *
 * wykonuje po kolei wszystkie komendy z pliku ~/.gg/autoexec.
 *
 *  - filename.
 */
void parse_autoexec(char *filename)
{
	char *buf;
	FILE *f;

	if (!filename) {
		if (!(filename = prepare_path("autoexec")))
			return;
	}
	
	if (!(f = fopen(filename, "r")))
		return;
	
	in_autoexec = 1;
	
	while ((buf = read_file(f))) {
		if (buf[0] == '#') {
			free(buf);
			continue;
		}

		if (execute_line(buf)) {
			fclose(f);
			free(buf);
			exit(0);
		}

		free(buf);
	}

	in_autoexec = 0;
	
	fclose(f);
}

/*
 * send_userlist()
 *
 * wysy�a do serwera userlist�, wywo�uj�c gg_notify()
 */
void send_userlist()
{
        struct list *l;
        uin_t *uins;
        int i, count;

	count = list_count(userlist);

        uins = (void*) malloc(count * sizeof(uin_t));

	for (i = 0, l = userlist; l; i++, l = l->next) {
		struct userlist *u = l->data;

                uins[i] = u->uin;
	}

        gg_notify(sess, uins, count);

        free(uins);
}

/*
 * do_reconnect()
 *
 * je�li jest w��czony autoreconnect, wywo�uje timer, kt�ry za podan�
 * ilo�� czasu spr�buje si� po��czy� jeszcze raz.
 */
void do_reconnect()
{
	if (auto_reconnect && connecting)
		reconnect_timer = time(NULL);
}

/*
 * send_sms()
 *
 * wysy�a sms o podanej tre�ci do podanej osoby.
 */
int send_sms(char *recipient, char *message, int show_result)
{
	int pid;
	char buf[50];

	if (!sms_send_app) {
		errno = EINVAL;
		return -1;
	}

	if (!recipient || !message) {
		errno = EINVAL;
		return -1;
	}

	if ((pid = fork()) == -1)
		return -1;

	if (!pid) {
		int i;

		for (i = 0; i < 255; i++)
			close(i);
			
		execlp(sms_send_app, sms_send_app, recipient, message, NULL);
		exit(1);
	}

	if (show_result)
		snprintf(buf, sizeof(buf), "\001%s", recipient);
	else
		strcpy(buf, "\002");

	add_process(pid, buf);

	return 0;
}

/*
 * play_sound()
 *
 * odtwarza dzwi�k o podanej nazwie.
 */
int play_sound(char *sound_path)
{
	int pid;

	if (!sound_app || !sound_path) {
		errno = EINVAL;
		return -1;
	}

	if ((pid = fork()) == -1)
		return -1;

	if (!pid) {
		int i;

		for (i = 0; i < 255; i++)
			close(i);
			
		execlp(sound_app, sound_app, sound_path, NULL);
		exit(1);
	}

	add_process(pid, "\002");

	return 0;
}

/*
 * read_file()
 *
 * czyta linijk� tekstu z pliku alokuj�c przy tym odpowiedni buforek.
 */
char *read_file(FILE *f)
{
	char buf[1024], *new, *res = NULL;

	while (fgets(buf, sizeof(buf) - 1, f)) {
		int new_size = ((res) ? strlen(res) : 0) + strlen(buf) + 1;

		if (!(new = realloc(res, new_size))) {
			/* je�li braknie pami�ci, pomijamy reszt� linii */
			if (strchr(buf, '\n'))
				break;
			else
				continue;
		}
		if (!res)
			*new = 0;
		res = new;
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
 * add_ignored()
 *
 * dopisuje do listy uruchomionych dzieci proces�w.
 *
 *  - pid.
 */
int add_process(int pid, char *name)
{
	struct process p;

	p.pid = pid;
	p.name = strdup(name);
	list_add(&children, &p, sizeof(p));
	
	return 0;
}

/*
 * del_process()
 *
 * usuwa proces z listy dzieciak�w.
 *
 *  - pid.
 */
int del_process(int pid)
{
	struct list *l;

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
 * on_off()
 *
 * zwraca 1 je�li tekst znaczy w��czy�, 0 je�li wy��czy�, -1 je�li co innego.
 *
 *  - value.
 */
int on_off(char *value)
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
 * add_alias()
 *
 * dopisuje alias do listy alias�w.
 *
 * - string - linia w formacie 'alias cmd'
 * - quiet - czy wypluwa� mesgi na stdout
 */
int add_alias(char *string, int quiet)
{
	char *cmd;
	struct list *l;
	struct alias a;

	if (!string || !(cmd = strchr(string, ' '))) {
		if (!quiet)
			my_printf("not_enough_params");
		return -1;
	}

	*cmd++ = 0;

	for (l = aliases; l; l = l->next) {
		struct alias *j = l->data;

		if (!strcmp(string, j->alias)) {
			if (!quiet)
				my_printf("aliases_exist", string);
			return -1;
		}
	}

	a.alias = strdup(string);
	a.cmd = strdup(cmd);
	list_add(&aliases, &a, sizeof(a));

	if (!quiet)
		my_printf("aliases_add", a.alias, a.cmd);

	return 0;
}

/*
 * del_alias()
 *
 * usuwa alias z listy alias�w.
 *
 * - name - alias.
 */
int del_alias(char *name)
{
	struct list *l;

	if (!name) {
		my_printf("not_enough_params");
		return -1;
	}

	for (l = aliases; l; l = l->next) {
		struct alias *a = l->data;

		if (!strcmp(a->alias, name)) {
			my_printf("aliases_del", name);
			list_remove(&aliases, a, 1);
			return 0;
		}
	}

	my_printf("aliases_noexist", name);

	return -1;
}

/*
 * is_alias()
 *
 * sprawdza czy komenda w foo jest aliasem, je�li tak - zwraca cmd,
 * w przeciwnym razie NULL.
 *
 * - foo
 */
char *is_alias(char *foo)
{
	struct list *l;
	char *param = NULL, *buf, *line = strdup(foo);

	if ((param = strchr(line, ' ')))
		*param++ = 0;

	for (l = aliases; l; l = l->next) {
		struct alias *j = l->data;

		if (!strcmp(line, j->alias)) {
			buf = malloc(strlen(j->cmd) + ((param) ? strlen(param) : 0) + 4);
			strcpy(buf, j->cmd);
			if (param) {
				strcat(buf, " ");
				strcat(buf, param);
			}
			free(line);
			return buf;
		}
	}

	free(line);

	return NULL;
}

/*
 * format_events()
 *
 * zwraca �a�cuch zdarze� w oparciu o flagi.
 *
 * - flags
 */
char *format_events(int flags)
{
        char buff[64] = "";

        if (flags & EVENT_MSG) strcat(buff, *buff ? "|msg" : "msg");
        if (flags & EVENT_CHAT) strcat(buff, *buff ? "|chat" : "chat");
        if (flags & EVENT_AVAIL) strcat(buff, *buff ? "|avail" : "avail");
        if (flags & EVENT_NOT_AVAIL) strcat(buff, *buff ? "|disconnect" : "disconnect");
        if (flags & EVENT_AWAY) strcat(buff, *buff ? "|away" : "away");
        if (flags & EVENT_DCC) strcat(buff, *buff ? "|dcc" : "dcc");
        if (strlen(buff) > 33) strcpy(buff, "*");

        return strdup(buff);
}

/*
 * get_flags()
 *
 * zwraca flagi na podstawie �a�cucha.
 *
 * - events
 */
int get_flags(char *events)
{
        int flags = 0;

        if(!strncasecmp(events, "*", 1)) return EVENT_MSG|EVENT_CHAT|EVENT_AVAIL|EVENT_NOT_AVAIL|EVENT_AWAY|EVENT_DCC;
        if (strstr(events, "msg") || strstr(events, "MSG")) flags |= EVENT_MSG;
        if (strstr(events, "chat") || strstr(events, "CHAT")) flags |= EVENT_CHAT;
        if (strstr(events, "avail") || strstr(events, "AVAIL")) flags |= EVENT_AVAIL;
        if (strstr(events, "disconnect") || strstr(events, "disconnect")) flags
|= EVENT_NOT_AVAIL;
        if (strstr(events, "away") || strstr(events, "AWAY")) flags |= EVENT_AWAY;
        if (strstr(events, "dcc") || strstr(events, "DCC")) flags |= EVENT_DCC;

        return flags;
}

/*
 * add_event()
 *
 * Dodaje zdarzenie do listy zdarze�.
 *
 * - flags
 * - uin
 * - action
 */
int add_event(int flags, uin_t uin, char *action)
{
        int f;
        struct list *l;
        struct event e;

        for (l = events; l; l = l->next) {
                struct event *ev = l->data;

                if (ev->uin == uin && (f = ev->flags & flags) != 0) {
                        my_printf("events_exist", format_events(f), (uin ==1) ?
"*"  : format_user(uin));
                        return -1;
                }
        }

        e.uin = uin;
        e.flags = flags;
        e.action = action;

        list_add(&events, &e, sizeof(e));

        my_printf("events_add", format_events(flags), (uin ==1) ? "*"  : format_user(uin), action);

        return 0;
}

/*
 * del_event()
 *
 * usuwa zdarzenie z listy zdarze�.
 *
 * - flags
 * - uin
 */
int del_event(int flags, uin_t uin)
{
        struct list *l;

        for (l = events; l; l = l->next) {
                struct event *e = l->data;

                if (e->uin == uin && e->flags & flags) {
                        if ((e->flags &=~ flags) == 0) {
                                my_printf("events_del", format_events(flags), (uin ==1) ? "*"  : format_user(uin), e->action);
                                list_remove(&events, e, 1);
                                return 0;
                        }
                        else {
                                my_printf("events_del_flags", format_events(flags));
                                list_remove(&events, e, 0);
                                list_add_sorted(&events, e, 0, NULL);
                                return 0;
                        }
                }
        }

        my_printf("events_del_noexist", format_events(flags), (uin ==1) ? "3"  : format_user(uin));

        return 1;
}

/*
 * check_event()
 *
 * sprawdza i ewentualnie uruchamia akcj� na podane zdarzenie.
 *
 * - event
 * - uin
 */
int check_event(int event, uin_t uin)
{
        char *evt_ptr = NULL, *evs[10];
        struct list *l;
        int y = 0, i;

        for (l = events; l; l = l->next) {
                struct event *e = l->data;

                if (e->flags & event) {
                        if (e->uin == uin) {
                                evt_ptr = strdup(e->action);
                                break;
                        }
                        else if (e->uin == 1)
                                evt_ptr = strdup(e->action);
                }
        }

        if (evt_ptr == NULL)
                return 1;

        if (strchr(evt_ptr, ';')) {
                evs[y] = strtok(evt_ptr, ";");
                while ((evs[++y] = strtok(NULL, ";"))) {
                        if (y >= 10) break;
                }
        }
        else
                return run_event(evt_ptr);

        for (i = 0; i < y; i++) {
                if (!evs[i]) continue;
                while (*evs[i] == ' ') evs[i]++;
                run_event(evs[i]);
        }

        return 0;
}

/*
 * run_event()
 *
 * wykonuje dan� akcj�.
 *
 * - action
 */
int run_event(char *act)
{
        uin_t uin;
        char *cmd = NULL, *arg = NULL, *data = NULL, *action = strdup(act);

        if(strstr(action, " ")) {
                cmd = strtok(action, " ");
                arg = strtok(NULL, "");
        }
        else
                cmd = action;

        if (!strncasecmp(cmd, "blink_leds", 4))
                return send_event(strdup(arg), ACT_BLINK_LEDS);

        else if(!strncasecmp(cmd, "beeps_spk", 4))
                return send_event(strdup(arg), ACT_BEEPS_SPK);

        else if(!strncasecmp(cmd, "chat", 4) || !strncasecmp(cmd, "msg", 3)) {
                struct userlist *u;
                char sender[50];

                if (!strstr(arg, " "))
                        return 1;

                strtok(arg, " ");
                data = strtok(NULL, "");

                if (!(uin = get_uin(arg)))
                        return 1;

                if ((u = find_user(uin, NULL)))
                        snprintf(sender, sizeof(sender), "%s/%lu", u->comment, u->uin);
                else
                        snprintf(sender, strlen(sender), "%s", arg);

                put_log(uin, "<<* %s %s (%s)\n%s\n", (!strcasecmp(cmd, "chat"))
? "Rozmowa do" : "Wiadomo�� do", sender, full_timestamp(), data);

                iso_to_cp(data);
                gg_send_message(sess, (!strcasecmp(cmd, "msg")) ? GG_CLASS_MSG : GG_CLASS_CHAT, uin, data);

                return 0;
        }

        else
                my_printf("temporary_run_event", action);

        return 0;
}

/*
 * send_event()
 *
 * wysy�a do ioctl_daemon'a polecenie uruchomienia akcji z ioctl.
 *
 * - seq
 * - act
*/
int send_event(char *seq, int act)
{
        char *s;
        struct action_data data;

        if (*seq == '$') {
                seq++;
                s = find_format(seq);
                if (!strcmp(s, "")) {
                        my_printf("events_seq_not_found", seq);
                        return 1;
                }
        } else
                s = seq;

        data.act = act;

        if (events_parse_seq(s, &data))
                return 1;

        sendto(sock, &data, sizeof(data), 0,(struct sockaddr *)&addr, length);

        return 0;
}

/* correct_event()
 *
 * sprawdza czy akcja na zdarzenie jest poprawna.
 *
 * - act
*/
int correct_event(char *act)
{
        int y = 0, i;
        char *cmd = NULL, *arg = NULL, *action = strdup(act), *evs[10];
        struct action_data test;

        if (!strncasecmp(action, "clear",  5))
                return 0;

        evs[y] = strtok(action, ";");
        while ((evs[++y] = strtok(NULL, ";")))
                if (y >= 10) break;

        for (i = 0; i < y; i++) {

                if (!evs[i]) {
                        y--;
                        continue;
                }

                while (*evs[i] == ' ') evs[i]++;

                if (strstr(evs[i], " ")) {
                        cmd = strtok(evs[i], " ");
                        arg = strtok(NULL, "");
                } else
                        cmd = evs[i];

                if (!strncasecmp(cmd, "blink_leds", 10) || !strncasecmp(cmd, "beeps_spk", 9)) {
                        if (arg == NULL) {
                                my_printf("events_act_no_params", cmd);
                                return 1;
                        }

                        if (*arg == '$') {
                                arg++;
                                if (!strcmp(find_format(arg), "")) {
                                        my_printf("events_seq_not_found", arg);
                                        return 1;
                                }
                                continue;
                        }

                        if (events_parse_seq(arg, &test)) {
                                my_printf("events_seq_incorrect", arg);
                                return 1;
                        }

                        continue;
                }

                else if (!strncasecmp(cmd, "chat", 4) || !strncasecmp(cmd, "msg", 3)) {
                        if (arg == NULL) {
                                my_printf("events_act_no_params", cmd);
                                return 1;
                        }

                        if (!strstr(arg, " ")) {
                                my_printf("events_act_no_params", cmd);
                                return 1;
                        }

                        strtok(arg, " ");

                        if (!get_uin(arg)) {
                                my_printf("user_not_found", arg);
                                return 1;
                        }

                        continue;
                }

                else {
                        my_printf("events_noexist");
                        return 1;
                }
        }

        if (y == 0) {
                my_printf("events_noexist");
                return 1;
        }

        return 0;
}

/*
 * events_parse_seq()
 *
 * zamie� string na odpowiedni� struktur�, zwraca >0 w przypadku b��du.
 *
 * seq
 * data
 */
int events_parse_seq(char *seq, struct action_data *data)
{
        char tmp_buff[16] = "";
        int i = 0, a, l = 0, default_delay = 10000;

        if (!seq || !isdigit(seq[0]))
                return 1;

        for (a = 0; a <= strlen(seq) && a < MAX_ITEMS; a++) {
                if (i > 15 ) return 2;
                if (isdigit(seq[a]))
                        tmp_buff[i++] = seq[a];
                else if (seq[a] == '/') {
                        data->value[l] = atoi(tmp_buff);
                        bzero(tmp_buff, 16);
                        for (i = 0; isdigit(seq[++a]); i++)
                                tmp_buff[i] = seq[a];
                        data->delay[l] = default_delay = atoi(tmp_buff);
                        bzero(tmp_buff, 16);
                        i = 0;
                        l++;
                }
                else if (seq[a] == ',') {
                        data->value[l] = atoi(tmp_buff);
                        data->delay[l] = default_delay;
                        bzero(tmp_buff, 16);
                        i = 0;
                        l++;
                }
                else if (seq[a] == ' ')
                        continue;
                else if (seq[a] == '\0') {
                        data->value[l] = atoi(tmp_buff);
                        data->delay[l] = default_delay;
                        data->value[++l] = data->delay[l] = -1;
                }
                else return 3;
        }
        return 0;
}

/*
 * init_socket()
 *
 * inicjuje gniazdo oraz struktur� addr dla ioctl_daemon'a.
 *
 * - sock_path
*/
int init_socket(char *sock_path)
{
        sock = socket(AF_UNIX, SOCK_DGRAM, 0);

        if (sock < 0) perror("socket");

        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, sock_path);
        length = sizeof(addr);

        return 0;
}

static char base64_set[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * encode_base64()
 *
 * zapisuje ci�g znak�w w base64. alokuje pami��. 
 */
char *encode_base64(char *buf)
{
	char *out, *res;
	int i = 0, j = 0, k = 0, len = strlen(buf);
	
	if (!(res = out = malloc((len / 3 + 1) * 4 + 2))) {
		gg_debug(GG_DEBUG_MISC, "// encode_base64() not enough memory\n");
		return NULL;
	}
	
	while (j <= len) {
		switch (i % 4) {
			case 0:
				k = (buf[j] & 252) >> 2;
				break;
			case 1:
				k = ((buf[j] & 3) << 4) | ((buf[++j] & 240) >> 4);
				break;
			case 2:
				k = ((buf[j] & 15) << 2) | ((buf[++j] & 192) >> 6);
				break;
			case 3:
				k = buf[j++] & 63;
				break;
		}
		*out++ = base64_set[k];
		i++;
	}

	if (i % 4)
		for (j = 0; j < 4 - (i % 4); j++, out++)
			*out = '=';
	
	*out = 0;
	
	return res;
}

/*
 * decode_base64()
 *
 * wczytuje ci�g znak�w base64, zwraca zaalokowany buforek.
 */
char *decode_base64(char *buf)
{
	char *res, *save, *end, *foo, val;
	int index = 0;
	
	if (!(save = res = calloc(1, (strlen(buf) / 4 + 1) * 3 + 2))) {
		gg_debug(GG_DEBUG_MISC, "// decode_base64() not enough memory\n");
		return NULL;
	}

	end = buf + strlen(buf);

	while (*buf && buf < end) {
		if (*buf == '\r' || *buf == '\n') {
			buf++;
			continue;
		}
		if (!(foo = strchr(base64_set, *buf)))
			foo = base64_set;
		val = (int)foo - (int)base64_set;
		*buf = 0;
		buf++;
		switch (index) {
			case 0:
				*res |= val << 2;
				break;
			case 1:
				*res++ |= val >> 4;
				*res |= val << 4;
				break;
			case 2:
				*res++ |= val >> 2;
				*res |= val << 6;
				break;
			case 3:
				*res++ |= val;
				break;
		}
		index++;
		index %= 4;
	}
	*res = 0;
	
	return save;
}

	
/*
 * changed_debug()
 *
 * funkcja wywo�ywana przy zmianie warto�ci zmiennej ,,debug''.
 */
void changed_debug(char *var)
{
	if (display_debug)
		gg_debug_level = 255;
	else
		gg_debug_level = 0;
}

/*
 * changed_dcc()
 *
 * funkcja wywo�ywana przy zmianie warto�ci zmiennej ,,dcc''.
 */
void changed_dcc(char *var)
{
	struct gg_dcc *dcc = NULL;
	struct list *l;
	
	for (l = watches; l; l = l->next) {
		struct gg_common *c = l->data;

		if (c->type == GG_SESSION_DCC_SOCKET)
			dcc = l->data;
	}

	if (!use_dcc && dcc) {
		list_remove(&watches, dcc, 0);
		gg_free_dcc(dcc);
	}

	if (use_dcc && !dcc) {
		if (!(dcc = gg_dcc_create_socket(config_uin, 0))) {
			my_printf("dcc_create_error", strerror(errno));
		} else
			list_add(&watches, dcc, 0);
	}
}

/*
 * changed_theme()
 *
 * funkcja wywo�ywana przy zmianie warto�ci zmiennej ,,theme''.
 */
void changed_theme(char *var)
{
    init_theme();
    reset_theme_cache();
    if(default_theme) {
	if(!read_theme(default_theme, 1)) {
	    reset_theme_cache();
    	    my_printf("theme_loaded", default_theme);
	} else
	    my_printf("error_loading_theme", strerror(errno));
    }
}

/*
 * prepare_connect()
 *
 * przygotowuje wszystko pod po��czenie gg_login.
 */
void prepare_connect()
{
	struct list *l;

	for (l = watches; l; l = l->next) {
		struct gg_dcc *d = l->data;
		
		if (d->type == GG_SESSION_DCC_SOCKET) {
			gg_dcc_port = d->port;
			
		}
	}
}

/*
 * transfer_id()
 *
 * zwraca pierwszy wolny identyfikator transferu dcc.
 */
int transfer_id()
{
	struct list *l;
	int id = 1;

	for (l = transfers; l; l = l->next) {
		struct transfer *t = l->data;

		if (t->id >= id)
			id = t->id + 1;
	}

	return id;
}
