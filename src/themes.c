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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifndef _AIX
#  include <string.h>
#endif
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>
#include "stuff.h"
#include "dynstuff.h"
#include "themes.h"
#include "config.h"
#include "xmalloc.h"
#include "ui.h"

#ifndef PATH_MAX
#  define PATH_MAX _POSIX_PATH_MAX
#endif

int automaton_color_escapes;

char *prompt_cache = NULL, *prompt2_cache = NULL, *error_cache = NULL;
const char *timestamp_cache = NULL;

int no_prompt_cache = 0;

list_t formats = NULL;

/*
 * format_find()
 *
 * odnajduje warto�� danego formatu. je�li nie znajdzie, zwraca pusty ci�g,
 * �eby nie musie� uwa�a� na �adne null-references.
 *
 *  - name.
 */
const char *format_find(const char *name)
{
	char *tmp;
	int hash;
	list_t l;

	if (!name)
		return "";

	hash = ekg_hash(name);

	if (config_speech_app && !strchr(name, ',')) {
		char *name2 = saprintf("%s,speech", name);
		const char *tmp;
		
		if (strcmp((tmp = format_find(name2)), "")) {
			xfree(name2);
			return tmp;
		}
		
		xfree(name2);
	}

	if (config_theme && (tmp = strchr(config_theme, ',')) && !strchr(name, ',')) {
		char *name2 = saprintf("%s,%s", name, tmp + 1);
		const char *tmp;
		
		if (strcmp((tmp = format_find(name2)), "")) {
			xfree(name2);
			return tmp;
		}
		
		xfree(name2);
	}
	
	for (l = formats; l; l = l->next) {
		struct format *f = l->data;

		if (hash == f->name_hash && !strcasecmp(f->name, name))
			return f->value;
	}
	
	return "";
}

/*
 * isalpha_pl_PL()
 * 
 * makro udaj�ce isalpha() z LC_CTYPE="pl_PL". niestety ncurses co� psuje
 * i �le wykrywa p�e�.
 */
#define isalpha_pl_PL(x) ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z') || x == '�' || x == '�' || x == '�' || x == '�' || x == '�' || x == '�' || x == '�' || x == '�' || x == '�' || x == '�' || x == '�' || x == '�' || x == '�' || x == '�' || x == '�' || x == '�' || x == '�' || x == '�')

/*
 * va_format_string()
 *
 * formatuje zgodnie z podanymi parametrami ci�g znak�w.
 *
 *  - format - warto��, nie nazwa formatu,
 *  - ap - argumenty.
 */
char *va_format_string(const char *format, va_list ap)
{
	static int dont_resolve = 0;
	string_t buf = string_init(NULL);
	const char *p, *args[9];
	int i, argc = 0;

	/* liczymy ilo�� argument�w */
	for (p = format; *p; p++) {
		if (*p != '%')
			continue;

		p++;

		if (!*p)
			break;

		if (*p == '@') {
			p++;

			if (!*p)
				break;

			if ((*p - '0') > argc)
				argc = *p - '0';
			
		} else if (*p == '(' || *p == '[') {
			if (*p == '(') {
				while (*p && *p != ')')
					p++;
			} else {
				while (*p && *p != ']')
					p++;
			}

			if (*p)
				p++;
			
			if (!*p)
				break;
			
			if ((*p - '0') > argc)
				argc = *p - '0';
		} else {
			if (*p >= '1' && *p <= '9' && (*p - '0') > argc)
				argc = *p - '0';
		}
	}

	for (i = 0; i < 9; i++)
		args[i] = NULL;

	for (i = 0; i < argc; i++)
		args[i] = va_arg(ap, char*);

	if (!dont_resolve) {
		dont_resolve = 1;
		if (no_prompt_cache) {
			/* zawsze czytaj */
			timestamp_cache = format_find("timestamp");
			prompt_cache = format_string(format_find("prompt"));
			prompt2_cache = format_string(format_find("prompt2"));
			error_cache = format_string(format_find("error"));
		} else {
			/* tylko je�li nie s� keszowanie */
			if (!timestamp_cache)
				timestamp_cache = format_find("timestamp");
			if (!prompt_cache)
				prompt_cache = format_string(format_find("prompt"));
			if (!prompt2_cache)
				prompt2_cache = format_string(format_find("prompt2"));
			if (!error_cache)
				error_cache = format_string(format_find("error"));
		}
		dont_resolve = 0;
	}
	
	p = format;
	
	while (*p) {
		if (*p == '%') {
			int fill_before, fill_after, fill_soft, fill_length;
			char fill_char;

			p++;
			if (!*p)
				break;
			if (*p == '%')
				string_append_c(buf, '%');
			if (*p == '>')
				string_append(buf, prompt_cache);
			if (*p == ')')
				string_append(buf, prompt2_cache);
			if (*p == '!')
				string_append(buf, error_cache);
			if (*p == '#')
				string_append(buf, timestamp(timestamp_cache));
			if (config_display_color && isalpha(*p) && 
			    automaton_color_escapes) {
				string_append_c(buf, '\033');
				string_append_c(buf, *p);
			} else if (config_display_color) {
				if (*p == 'k')
					string_append(buf, "\033[0;30m");
				if (*p == 'K')
					string_append(buf, "\033[1;30m");
				if (*p == 'l')
					string_append(buf, "\033[40m");
				if (*p == 'r')
					string_append(buf, "\033[0;31m");
				if (*p == 'R')
					string_append(buf, "\033[1;31m");
				if (*p == 's')
					string_append(buf, "\033[41m");
				if (*p == 'g')
					string_append(buf, "\033[0;32m");
				if (*p == 'G')
					string_append(buf, "\033[1;32m");
				if (*p == 'h')
					string_append(buf, "\033[42m");
				if (*p == 'y')
					string_append(buf, "\033[0;33m");
				if (*p == 'Y')
					string_append(buf, "\033[1;33m");
				if (*p == 'z')
					string_append(buf, "\033[43m");
				if (*p == 'b')
					string_append(buf, "\033[0;34m");
				if (*p == 'B')
					string_append(buf, "\033[1;34m");
				if (*p == 'e')
					string_append(buf, "\033[44m");
				if (*p == 'm' || *p == 'p')
					string_append(buf, "\033[0;35m");
				if (*p == 'M' || *p == 'P')
					string_append(buf, "\033[1;35m");
				if (*p == 'q')
					string_append(buf, "\033[45m");
				if (*p == 'c')
					string_append(buf, "\033[0;36m");
				if (*p == 'C')
					string_append(buf, "\033[1;36m");
				if (*p == 'd')
					string_append(buf, "\033[46m");
				if (*p == 'w')
					string_append(buf, "\033[0;37m");
				if (*p == 'W')
					string_append(buf, "\033[1;37m");
				if (*p == 'x')
					string_append(buf, "\033[47m");
				if (*p == 'i')
					string_append(buf, "\033[5m");
				if (*p == 'n')
					string_append(buf, "\033[0m");
		                if (*p == 'T')
					string_append(buf, "\033[1m");
			}

			if (*p == '@') {
				char *str = (char*) args[*(p + 1) - '1'];

				if (str) {
					char *q = str + strlen(str) - 1;

					while (q >= str && !isalpha_pl_PL(*q))
						q--;

					if (*q == 'a')
						string_append(buf, "a");
					else
						string_append(buf, "y");
				}
				p += 2;
				continue;
			}

			fill_before = 0;
			fill_after = 0;
			fill_length = 0;
			fill_char = ' ';
			fill_soft = 1;

			if (*p == '[' || *p == '(') {
				char *q;

				fill_soft = (*p == '(');

				p++;
				fill_char = ' ';

				if (*p == '.') {
					fill_char = '0';
					p++;
				} else if (*p == ',') {
					fill_char = '.';
					p++;
				} else if (*p == '_') {
					fill_char = '_';
					p++;
				}

				fill_length = strtol(p, &q, 0);
				p = q;
				if (fill_length > 0)
					fill_after = 1;
				else {
					fill_length = -fill_length;
					fill_before = 1;
				}
				p++;
			}

			if (*p >= '1' && *p <= '9') {
				char *str = (char*) args[*p - '1'];
				int i, len;

				if (!str)
					str = "";
				len = strlen(str);

				if (fill_length) {
					if (len >= fill_length) {
						if (!fill_soft)
							len = fill_length;
						fill_length = 0;
					} else
						fill_length -= len;
				}

				if (fill_before)
					for (i = 0; i < fill_length; i++)
						string_append_c(buf, fill_char);

				string_append_n(buf, str, len);

				if (fill_after) 
					for (i = 0; i < fill_length; i++)
						string_append_c(buf, fill_char);

			}
		} else
			string_append_c(buf, *p);

		p++;
	}

	if (!dont_resolve && no_prompt_cache)
		theme_cache_reset();

	return string_free(buf, 0);
}

/*
 * format_string()
 *
 * j.w. tyle �e nie potrzeba dawa� mu va_list, a wystarcz� zwyk�e parametry.
 *
 *  - format... - j.w.,
 */
char *format_string(const char *format, ...)
{
	va_list ap;
	char *tmp;
	
	va_start(ap, format);
	tmp = va_format_string(format, ap);
	va_end(ap);

	return tmp;
}

/*
 * print()
 *
 * drukuje na stdout tekst, bior�c pod uwag� nazw�, nie warto�� formatu.
 * parametry takie jak zdefiniowano. pierwszy to %1, drugi to %2.
 */
void print(const char *theme, ...)
{
	va_list ap;
	char *tmp;
	
	va_start(ap, theme);
	tmp = va_format_string(format_find(theme), ap);
	
	ui_print("__current", 0, (tmp) ? tmp : "");
	
	xfree(tmp);
	va_end(ap);
}

/*
 * print_status()
 *
 * wy�wietla tekst w oknie statusu.
 */
void print_status(const char *theme, ...)
{
	va_list ap;
	char *tmp;
	
	va_start(ap, theme);
	tmp = va_format_string(format_find(theme), ap);
	
	ui_print("__status", 0, (tmp) ? tmp : "");
	
	xfree(tmp);
	va_end(ap);
}

/*
 * print_window()
 *
 * wy�wietla tekst w podanym oknie.
 *  
 *  - target - nazwa okna,
 *  - separate - czy niezb�dne jest otwieranie nowego okna?
 *  - theme, ... - tre��.
 */
void print_window(const char *target, int separate, const char *theme, ...)
{
	va_list ap;
	char *tmp;
	
	va_start(ap, theme);
	tmp = va_format_string(format_find(theme), ap);
	
	ui_print(target, separate, (tmp) ? tmp : "");
	
	xfree(tmp);
	va_end(ap);
}

/*
 * theme_cache_reset()
 *
 * usuwa cache'owane prompty. przydaje si� przy zmianie theme'u.
 */
void theme_cache_reset()
{
	xfree(prompt_cache);
	xfree(prompt2_cache);
	xfree(error_cache);
	
	prompt_cache = prompt2_cache = error_cache = NULL;
	timestamp_cache = NULL;
}

/*
 * format_add()
 *
 * dodaje dan� formatk� do listy.
 *
 *  - name - nazwa,
 *  - value - warto��,
 *  - replace - je�li znajdzie, to zostawia (=0) lub zamienia (=1).
 */
int format_add(const char *name, const char *value, int replace)
{
	struct format f;
	list_t l;
	int hash;

	if (!name || !value)
		return -1;

	hash = ekg_hash(name);

	if (hash == ekg_hash("no_prompt_cache") && !strcasecmp(name, "no_prompt_cache")) {
		no_prompt_cache = 1;
		return 0;
	}
	
	for (l = formats; l; l = l->next) {
		struct format *g = l->data;

		if (hash == g->name_hash && !strcasecmp(name, g->name)) {
			if (replace) {
				xfree(g->value);
				g->value = xstrdup(value);
			}

			return 0;
		}
	}

	f.name = xstrdup(name);
	f.name_hash = ekg_hash(name);
	f.value = xstrdup(value);
	list_add(&formats, &f, sizeof(f));
	
	return 0;
}

/*
 * format_remove()
 *
 * usuwa formatk� o danej nazwie.
 *
 *  - name.
 */
int format_remove(const char *name)
{
	list_t l;

	for (l = formats; l; l = l->next) {
		struct format *f = l->data;

		if (!strcasecmp(f->name, name)) {
			xfree(f->value);
			xfree(f->name);
			list_remove(&formats, f, 1);
		
			return 0;
		}
	}

        return -1;
}

/*
 * try_open() // funkcja wewn�trzna
 *
 * pr�buje otworzy� plik, je�li jeszcze nie jest otwarty.
 *
 *  - prevfd - deskryptor z poprzedniego wywo�ania,
 *  - prefix - �cie�ka,
 *  - filename - nazwa pliku.
 */
static FILE *try_open(FILE *prevfd, const char *prefix, const char *filename)
{
	char buf[PATH_MAX];
	FILE *f;

	if (prevfd)
		return prevfd;

	if (prefix)
		snprintf(buf, sizeof(buf) - 1, "%s/%s", prefix, filename);
	else
		snprintf(buf, sizeof(buf) - 1, "%s", filename);

	if ((f = fopen(buf, "r")))
		return f;

	if (prefix)
		snprintf(buf, sizeof(buf) - 1, "%s/%s.theme", prefix, filename);
	else
		snprintf(buf, sizeof(buf) - 1, "%s.theme", filename);

	if ((f = fopen(buf, "r")))
		return f;

	return NULL;
}

/*
 * theme_read()
 *
 * wczytuje opis wygl�du z podanego pliku. 
 *
 *  - filename - nazwa pliku z opisem.
 *  - replace - czy zast�powa� istniej�ce wpisy,
 *
 * zwraca 0 je�li wszystko w porz�dku, -1 w przypadku b��du.
 */
int theme_read(const char *filename, int replace)
{
        char *buf;
        FILE *f = NULL;

        if (!filename) {
                filename = prepare_path("default.theme", 0);
		if (!filename || !(f = fopen(filename, "r")))
			return -1;
        } else {
		char *fn = xstrdup(filename), *tmp;

		if ((tmp = strchr(fn, ',')))
			*tmp = 0;
		
		f = try_open(NULL, NULL, fn);

		if (!strchr(filename, '/')) {
			f = try_open(f, prepare_path("", 0), fn);
			f = try_open(f, prepare_path("themes", 0), fn);
			f = try_open(f, THEMES_DIR, fn);
		}

		xfree(fn);

		if (!f)
			return -1;
	}

	theme_init();
	ui_event("theme_init");

        while ((buf = read_file(f))) {
                char *value, *p;

                if (buf[0] == '#') {
			xfree(buf);
                        continue;
		}

                if (!(value = strchr(buf, ' '))) {
			xfree(buf);
			continue;
		}

		*value++ = 0;

		for (p = value; *p; p++) {
			if (*p == '\\') {
				if (!*(p + 1))
					break;
				if (*(p + 1) == 'n')
					*p = '\n';
				memmove(p + 1, p + 2, strlen(p + 1));
			}
		}

		if (buf[0] == '-')
			format_remove(buf + 1);
		else
			format_add(buf, value, replace);

		xfree(buf);
        }

        fclose(f);

        return 0;
}

/*
 * theme_free()
 *
 * usuwa formatki z pami�ci.
 */
void theme_free()
{
	list_t l;

	for (l = formats; l; l = l->next) {
		struct format *f = l->data;

		xfree(f->name);
		xfree(f->value);
	}	

	list_destroy(formats, 1);
	formats = NULL;

	theme_cache_reset();
}

/*
 * theme_init()
 *
 * ustawia domy�lne warto�ci formatek.
 */
void theme_init()
{
	theme_cache_reset();

	/* wykorzystywane w innych formatach */
	format_add("prompt", "%K:%g:%G:%n", 1);
	format_add("prompt,speech", " ", 1);
	format_add("prompt2", "%K:%c:%C:%n", 1);
	format_add("prompt2,speech", " ", 1);
	format_add("error", "%K:%r:%R:%n", 1);
	format_add("error,speech", "b��d!", 1);
	format_add("timestamp", "%H:%M", 1);
	format_add("timestamp,speech", " ", 1);
	
	/* prompty dla ui-readline */
	format_add("readline_prompt", "% ", 1);
	format_add("readline_prompt_away", "/ ", 1);
	format_add("readline_prompt_invisible", ". ", 1);
	format_add("readline_prompt_query", "%1> ", 1);
	format_add("readline_prompt_win", "%1%% ", 1);
	format_add("readline_prompt_away_win", "%1/ ", 1);
	format_add("readline_prompt_invisible_win", "%1. ", 1);
	format_add("readline_prompt_query_win", "%2:%1> ", 1);
	format_add("readline_prompt_win_act", "%1 (act/%2)%% ", 1);
	format_add("readline_prompt_away_win_act", "%1 (act/%2)/ ", 1);
	format_add("readline_prompt_invisible_win_act", "%1 (act/%2). ", 1);
	format_add("readline_prompt_query_win_act", "%2:%1 (act/%3)> ", 1);
					
	format_add("readline_more", "-- Wci�nij Enter by kontynuowa� lub Ctrl-D by przerwa� --", 1);

	/* prompty i statusy dla ui-ncurses */
	format_add("ncurses_prompt_none", "", 1);
	format_add("ncurses_prompt_query", "[%1] ", 1);
	format_add("statusbar", " %c(%w%{time}%c)%w %c(%w%{?!nick uin}%{nick}%c/%{?away %w}%{?avail %W}%{?invisible %K}%{?notavail %k}%{uin}%c) (%wwin%c/%w%{window}%{?query %c:%W}%{query}%c)%w%{?activity  %c(%wact%c/%w}%{activity}%{?activity %c)%w}%{?more  %c(%Gmore%c)}%{?debug  %c(%Cdebug%c)}", 1);

	/* dla funkcji format_user() */
	format_add("known_user", "%T%1%n/%2", 1);
	format_add("known_user,speech", "%1", 1);
	format_add("unknown_user", "%T%1%n", 1);
	
	/* cz�sto wykorzystywane, r�ne, przydatne itd. */
	format_add("none", "%1\n", 1);
	format_add("generic", "%> %1\n", 1);
	format_add("debug", "%n%1\n", 1);
	format_add("not_enough_params", "%! Za ma�o parametr�w. Spr�buj %Thelp %1%n\n", 1);
	format_add("invalid_uin", "%! Nieprawid�owy numer u�ytkownika\n", 1);
	format_add("user_not_found", "%! Nie znaleziono u�ytkownika %T%1%n\n", 1);
	format_add("not_implemented", "%! Tej funkcji jeszcze nie ma\n", 1);
	format_add("unknown_command", "%! Nieznane polecenie: %T%1%n\n", 1);
	format_add("welcome", "%> %TEKG-%1%n (Eksperymentalny Klient Gadu-Gadu)\n%> Program jest rozprowadzany na zasadach licencji GPL v2\n%> %RPrzed u�yciem wci�nij F1 lub wpisz ,,help''%n\n\n", 1);
	format_add("welcome,speech", "witamy w e k g", 1);
	format_add("ekg_version", "%) EKG - Eksperymentalny Klient Gadu-Gadu (%T%1%n)\n%) libgadu-%1 (protok� %2, klient %3)\n", 1);
	format_add("group_empty", "%! Grupa %T%1%n jest pusta\n", 1);
	format_add("secure", "%Y(szyfrowane)%n ", 1);

	/* add, del */
	format_add("user_added", "%> Dopisano %T%1%n do listy kontakt�w\n", 1);
	format_add("user_deleted", "%> Usuni�to %T%1%n z listy kontakt�w\n", 1);
	format_add("user_exists", "%! %T%1%n ju� istnieje w li�cie kontakt�w\n", 1);
	format_add("error_adding", "%! B��d podczas dopisywania do listy kontakt�w\n", 1);
	format_add("error_deleting", "%! B��d podczas usuwania z listy kontakt�w\n", 1);

	/* zmiany stanu */
	format_add("away", "%> Zmieniono stan na zaj�ty\n", 1);
	format_add("away_descr", "%> Zmieniono stan na zaj�ty: %T%1%n%2\n", 1);
	format_add("back", "%> Zmieniono stan na dost�pny\n", 1);
	format_add("back_descr", "%> Zmieniono stan na dost�pny: %T%1%n%2%n\n", 1);
	format_add("invisible", "%> Zmieniono stan na niewidoczny\n", 1);
	format_add("invisible_descr", "%> Zmieniono stan na niewidoczny: %T%1%n%2\n", 1);
	format_add("private_mode_is_on", "%> Tryb ,,tylko dla przyjaci�'' jest w��czony\n", 1);
	format_add("private_mode_is_off", "%> Tryb ,,tylko dla przyjaci�'' jest wy��czony\n", 1);
	format_add("private_mode_on", "%> W��czono tryb ,,tylko dla przyjaci�''\n", 1);
	format_add("private_mode_off", "%> Wy��czono tryb ,,tylko dla przyjaci�''\n", 1);
	format_add("private_mode_invalid", "%! Nieprawid�owa warto��\n", 1);
	format_add("descr_too_long", "%! D�ugo�� opisu przekracza limit. Ilo�� uci�tych znak�w: %T%1%n\n", 1);
	
	/* pomoc */
	format_add("help", "%> %1%2 - %3%4\n", 1);
	format_add("help_more", "%) %1\n", 1);
	format_add("help_alias", "%) %T%1%n jest aliasem i nie posiada opisu\n", 1);
	format_add("help_footer", " \n%> Gwiazdka (%T*%n) oznacza, �e mo�na uzyska� wi�cej szczeg��w\n\n", 1);
	format_add("help_quick", "%> Przed u�yciem przeczytaj ulotk�. Plik %Tdocs/ULOTKA%n zawiera kr�tki\n%> przewodnik po za��czonej dokumentacji. Je�li go nie masz, mo�esz\n%> �ci�gn�� pakiet ze strony %Thttp://dev.null.pl/ekg/%n\n", 1);

	/* ignore, unignore */
	format_add("ignored_added", "%> Dodano %T%1%n do listy ignorowanych\n", 1);
	format_add("ignored_deleted", "%> Usuni�to %1 z listy ignorowanych\n", 1);
	format_add("ignored_list", "%> %1\n", 1);
	format_add("ignored_list_empty", "%! Lista ignorowanych u�ytkownik�w jest pusta\n", 1);
	format_add("error_adding_ignored", "%! B��d podczas dodawania do listy ignorowanych\n", 1);
	format_add("error_not_ignored", "%! %1 nie jest na li�cie ignorowanych\n", 1);

	/* lista kontakt�w */
	format_add("list_empty", "%! Lista kontakt�w jest pusta\n", 1);
	format_add("list_avail", "%> %1 %Y(dost�pn%@2)%n %b%3:%4%n\n", 1);
	format_add("list_avail_descr", "%> %1 %Y(dost�pn%@2: %n%5%Y)%n %b%3:%4%n\n", 1);
	format_add("list_busy", "%> %1 %G(zaj�t%@2)%n %b%3:%4%n\n", 1);
	format_add("list_busy_descr", "%> %1 %G(zaj�t%@2: %n%5%G)%n %b%3:%4%n\n", 1);
	format_add("list_not_avail", "%> %1 %r(niedost�pn%@2)%n\n", 1);
	format_add("list_not_avail_descr", "%> %1 %r(niedost�pn%@2: %n%5%r)%n\n", 1);
	format_add("list_invisible", "%> %1 %c(niewidoczn%@2)%n %b%3:%4%n\n", 1);
	format_add("list_invisible_descr", "%> %1 %c(niewidoczn%@2: %n%5%c)%n %b%3:%4%n\n", 1);
	format_add("list_unknown", "%> %1\n", 1);
	format_add("modify_done", "%> Zmieniono wpis w li�cie kontakt�w\n", 1);

	/* �egnamy si�, zapisujemy konfiguracj� */
	format_add("quit", "%> Papa\n", 1);
	format_add("quit_descr", "%> Papa: %T%1%n%2\n", 1);
	format_add("config_changed", "Zapisa� now� konfiguracj�? (tak/nie) ", 1);
	format_add("saved", "%> Zapisano ustawienia\n", 1);
	format_add("error_saving", "%! Podczas zapisu ustawie� wyst�pi� b��d\n", 1);

	/* przychodz�ce wiadomo�ci */
	format_add("message_header", "%g.-- %n%1 %c%2%4%g--- -- -%n\n", 1);
	format_add("message_conference_header", "%g.-- %g[%T%3%g] -- %n%1 %c%2%4%g--- -- -%n\n", 1);
	format_add("message_footer", "%g`----- ---- --- -- -%n\n", 1);
	format_add("message_line", "%g|%n %1\n", 1);
	format_add("message_line_width", "-8", 1);
	format_add("message_timestamp", "(%Y-%m-%d %H:%M) ", 1);
	format_add("message_timestamp_today", "(%H:%M) ", 1);
	format_add("message_timestamp_now", "", 1);

	format_add("message_header,speech", "wiadomo�� od %1: ", 1);
	format_add("message_conference_header,speech", "wiadomo�� od %1 w konferencji %3: ", 1);
	format_add("message_line,speech", "%1\n", 1);
	format_add("message_footer,speech", ".", 1);

	format_add("chat_header", "%c.-- %n%1 %c%2%4%c--- -- -%n\n", 1);
	format_add("chat_conference_header", "%c.-- %c[%T%3%c] -- %n%1 %c%2%4%c--- -- -%n\n", 1);
	format_add("chat_footer", "%c`----- ---- --- -- -%n\n", 1);
	format_add("chat_line", "%c|%n %1\n", 1);
	format_add("chat_line_width", "-8", 1);
	format_add("chat_timestamp", "(%Y-%m-%d %H:%M) ", 1);
	format_add("chat_timestamp_today", "(%H:%M) ", 1);
	format_add("chat_timestamp_now", "", 1);

	format_add("chat_header,speech", "wiadomo�� od %1: ", 1);
	format_add("chat_conference_header,speech", "wiadomo�� od %1 w konferencji %3: ", 1);
	format_add("chat_line,speech", "%1\n", 1);
	format_add("chat_footer,speech", ".", 1);

	format_add("sent_header", "%b.-- %n%1 %4%b--- -- -%n\n", 1);
	format_add("sent_conference_header", "%b.-- %b[%T%3%b] -- %4%n%1 %b--- -- -%n\n", 1);
	format_add("sent_footer", "%b`----- ---- --- -- -%n\n", 1);
	format_add("sent_line", "%b|%n %1\n", 1);
	format_add("sent_line_width", "-8", 1);
	format_add("sent_timestamp", "%H:%M", 1);

	format_add("sysmsg_header", "%m.-- %TWiadomo�� systemowa%m --- -- -%n\n", 1);
	format_add("sysmsg_line", "%m|%n %1\n", 1);
	format_add("sysmsg_line_width", "-8", 1);
	format_add("sysmsg_footer", "%m`----- ---- --- -- -%n\n", 1);	

	format_add("sysmsg_header,speech", "wiadomo�� systemowa:", 1);
	format_add("sysmsg_line,speech", "%1\n", 1);
	format_add("sysmsg_footer,speech", ".", 1);

	/* potwierdzenia wiadomo�ci */
	format_add("ack_queued", "%> Wiadomo�� do %1 zostanie dostarczona p�niej\n", 1);
	format_add("ack_delivered", "%> Wiadomo�� do %1 zosta�a dostarczona\n", 1);

	/* ludzie zmieniaj� stan */
	format_add("status_avail", "%> %1 jest dost�pn%@2\n", 1);
	format_add("status_avail_descr", "%> %1 jest dost�pn%@2: %T%3%n\n", 1);
	format_add("status_busy", "%> %1 jest zaj�t%@2\n", 1);
	format_add("status_busy_descr", "%> %1 jest zaj�t%@2: %T%3%n\n", 1);
	format_add("status_not_avail", "%> %1 jest niedost�pn%@2\n", 1);
	format_add("status_not_avail_descr", "%> %1 jest niedost�pn%@2: %T%3%n\n", 1);
	format_add("status_invisible", "%> %1 jest niewidoczn%@2\n", 1);
	format_add("status_invisible_descr", "%> %1 jest niewidoczn%@2: %T%3%n\n", 1);

	format_add("auto_away", "%> Automagicznie zmieniono stan na zaj�ty po %1 nieaktywno�ci\n", 1);
	format_add("auto_away_descr", "%> Automagicznie zmieniono stan na zaj�ty po %1 nieaktywno�ci: %T%2%n%3\n", 1);

	/* po��czenie z serwerem */
	format_add("connecting", "%> ��cz� si� z serwerem...\n", 1);
	format_add("conn_failed", "%! Po��czenie nie uda�o si�: %1\n", 1);
	format_add("conn_failed_resolving", "Nie znaleziono serwera", 1);
	format_add("conn_failed_connecting", "Nie mo�na po��czy� si� z serwerem", 1);
	format_add("conn_failed_invalid", "Nieprawid�owa odpowied� serwera", 1);
	format_add("conn_failed_disconnected", "Serwer zerwa� po��czenie", 1);
	format_add("conn_failed_password", "Nieprawid�owe has�o", 1);
	format_add("conn_failed_404", "B��d serwera HTTP", 1);
	format_add("conn_failed_memory", "Brak pami�ci", 1);
	format_add("conn_stopped", "%! Przerwano ��czenie\n", 1);
	format_add("conn_timeout", "%! Przekroczono limit czasu operacji ��czenia z serwerem\n", 1);
	format_add("connected", "%> Po��czono\n", 1);
	format_add("connected_descr", "%> Po��czono: %T%1%n%2\n", 1);
	format_add("disconnected", "%> Roz��czono\n", 1);
	format_add("disconnected_descr", "%> Roz��czono: %T%1%n%2\n", 1);
	format_add("already_connected", "%! Klient jest ju� po��czony. Wpisz %Treconnect%n aby po��czy� ponownie\n", 1);
	format_add("during_connect", "%! ��czenie trwa. Wpisz %Tdisconnect%n aby przerwa�\n", 1);
	format_add("conn_broken", "%! Serwer zerwa� po��czenie\n", 1);
	format_add("not_connected", "%! Brak po��czenia z serwerem. Wpisz %Tconnect%n\n", 1);
	format_add("not_connected_msg_queued", "%! Brak po��czenia z serwerem. Wiadomo�� b�dzie wys�ana po po��czeniu.%n\n", 1);

	/* obs�uga theme�w */
	format_add("theme_loaded", "%> Wczytano opis wygl�du o nazwie %T%1%n\n", 1);
	format_add("theme_default", "%> Ustawiono domy�lny opis wygl�du\n", 1);
	format_add("error_loading_theme", "%! B��d podczas �adowania opisu wygl�du: %1\n", 1);

	/* zmienne, konfiguracja */
	format_add("variable", "%> %1 = %2\n", 1);
	format_add("variable_not_found", "%! Nieznana zmienna: %1\n", 1);
	format_add("variable_invalid", "%! Nieprawid�owa warto�� zmiennej\n", 1);
	format_add("no_config", "%! Niekompletna konfiguracja. Wpisz:\n%!   %Tset uin <numerek-gg>%n\n%!   %Tset password <has�o>%n\n%!   %Tsave%n\n%! Nast�pnie wydaj polecenie:\n%!   %Tconnect%n\n%! Je�li nie masz swojego numerka, wpisz:\n%!   %Tregister <e-mail> <has�o>%n\n\n", 1);
	format_add("no_config,speech", "niekompletna konfiguracja. wpisz set uin, a potem numer gadu-gadu, potem set pas�ord, a za tym swoje has�o. wpisz sejf, �eby zapisa� ustawienia. wpisz konekt by si� po��czy�. je�li nie masz swojego numeru gadu-gadu, wpisz red�ister, a potem imejl i has�o.", 1);
	format_add("error_reading_config", "%! Nie mo�na odczyta� pliku konfiguracyjnego: %1\n", 1);
        format_add("config_line_incorrect", "%! Nieprawid�owa linia '%T%1%n', pomijam\n", 1);
	format_add("autosaved", "%> Automatycznie zapisano ustawienia\n", 1);
	
	/* rejestracja nowego numera */
	format_add("register", "%> Rejestracja poprawna. Wygrany numerek: %T%1%n\n", 1);
	format_add("register_failed", "%! B��d podczas rejestracji\n", 1);
	format_add("register_pending", "%! Rejestracja w toku\n", 1);
	format_add("register_timeout", "%! Przekroczono limit czasu operacji rejestrowania\n", 1);
	format_add("registered_today", "%! Ju� zarejestrowano jeden numer. Nie nadu�ywaj\n", 1);

	/* kasowanie konta uzytkownika z katalogu publiczengo */
	format_add("unregister", "%> Konto %T%1%n wykasowane.\n", 1);
	format_add("unregister_timeout", "%! Przekroczono limit czasu operacji usuwania konta\n", 1);
	format_add("unregister_bad_uin", "%! Nie poprawny numer: %T%1%n\n", 1);
	format_add("unregister_failed", "%! B��d podczas usuwania konta\n", 1);
	
	/* przypomnienie has�a */
	format_add("remind", "%> Has�o zosta�o wys�ane\n", 1);
	format_add("remind_failed", "%! B��d podczas wysy�ania has�a\n", 1);
	format_add("remind_timeout", "%! Przekroczono limit czasu operacji wys�ania has�a\n", 1);
	
	/* zmiana has�a */
	format_add("passwd", "%> Has�o zosta�o zmienione\n", 1);
	format_add("passwd_failed", "%! B��d podczas zmiany has�a\n", 1);
	format_add("passwd_timeout", "%! Przekroczono limit czasu operacji zmiany has�a\n", 1);
	
	/* zmiana informacji w katalogu publicznym */
	format_add("change_not_enough_params", "%! Polecenie wymaga podania %Twszystkich%n parametr�w\n", 1);
	format_add("change", "%> Informacje w katalogu publicznym zosta�y zmienione\n", 1);
	format_add("change_failed", "%! B��d podczas zmiany informacji w katalogu publicznym\n", 1);
	format_add("change_timeout", "%! Przekroczono limit czasu operacji zmiany katalogu publicznego\n", 1);
	
	/* sesemesy */
	format_add("sms_error", "%! B��d wysy�ania SMS\n", 1);
	format_add("sms_unknown", "%! %1 nie ma podanego numeru kom�rki\n", 1);
	format_add("sms_sent", "%> SMS do %T%1%n zosta� wys�any\n", 1);
	format_add("sms_failed", "%! SMS do %T%1%n nie zosta� wys�any\n", 1);
	format_add("sms_msg", "EKG: msg %1 %# >> %2", 1);
	format_add("sms_chat", "EKG: chat %1 %# >> %2", 1);

	/* wyszukiwanie u�ytkownik�w */
	format_add("search_failed", "%! Wyst�pi� b��d podczas szukania: %1\n", 1);
	format_add("search_timeout", "%! Przekroczono limit czasu operacji szukania\n", 1);
	format_add("search_not_found", "%! Nie znaleziono\n", 1);

	/* 1 uin, 2 name, 3 nick, 4 city, 5 born, 6 gender, 7 active */
	format_add("search_results_multi_active", "%G!%n", 1);
	format_add("search_results_multi_busy", "%g!%n", 1);
	format_add("search_results_multi_inactive", " ", 1);
	format_add("search_results_multi_unknown", "-", 1);
	format_add("search_results_multi_female", "k", 1);
	format_add("search_results_multi_male", "m", 1);
	format_add("search_results_multi", "%7 %[-10]1%K|%n%[12]3%K|%n%6%K|%n%[20]2%K|%n%[4]5%K|%n%[16]4\n", 1);

	format_add("search_results_single_active", "%G(aktywn%@1)%n", 1);
	format_add("search_results_single_busy", "%g(zaj�t%@1)%n", 1);
	format_add("search_results_single_inactive", "%r(nieaktywn%@1)%n", 1);
	format_add("search_results_single_unknown", "%T-%n", 1);
	format_add("search_results_single_female", "%Mkobieta%n", 1);
	format_add("search_results_single_male", "%Cm�czyzna%n", 1);
	format_add("search_results_single", "%) Nick: %T%3%n\n%) Numerek: %T%1%n\n%) Imi� i nazwisko: %T%2%n\n%) Miejscowo��: %T%4%n\n%) Rok urodzenia: %T%5%n\n%) P�e�: %6\n", 1);

	/* exec */
	format_add("process", "%> %(-5)1 %2\n", 1);
	format_add("no_processes", "%! Nie ma dzia�aj�cych proces�w\n", 1);
	format_add("process_exit", "%> Proces %1 (%2) zako�czy� dzia�anie z wynikiem %3\n", 1);
	format_add("exec", "%1\n",1);	/* %1 tre��, %2 pid */
	format_add("exec_error", "%! B��d uruchamiania procesu: %1\n", 1);

	/* szczeg�owe informacje o u�ytkowniku */
	format_add("user_info", "%) Pseudonim: %T%3%n\n%) Numer: %T%7%n\n%) Stan: %8\n%) Imi� i nazwisko: %T%1 %2%n\n%) Alias: %T%4%n\n%) Numer telefonu: %T%5%n\n%) Grupy: %T%6%n\n", 1);
	format_add("user_info_avail", "%Ydost�pn%@1%n", 1);
	format_add("user_info_avail_descr", "%Ydost�pn%@1%n (%2)", 1);
	format_add("user_info_busy", "%Gzaj�t%@1%n", 1);
	format_add("user_info_busy_descr", "%Gzaj�t%@1%n (%2)", 1);
	format_add("user_info_not_avail", "%rniedost�pn%@1%n", 1);
	format_add("user_info_not_avail_descr", "%rniedost�pn%@1%n (%2)", 1);
	format_add("user_info_invisible", "%cniewidoczn%@1%n", 1);
	format_add("user_info_invisible_descr", "%cniewidoczn%@1%n (%2)", 1);

	/* status */
	format_add("show_status_profile", "%) Profil: %T%1%n\n", 1);
	format_add("show_status_uin", "%) Numer: %T%1%n\n", 1);
	format_add("show_status_uin_nick", "%) Numer: %T%1%n (%T%2%n)\n", 1);
	format_add("show_status_status", "%) Aktualny stan: %T%1%2%n\n", 1);
	format_add("show_status_server", "%) Aktualny serwer: %T%1%n:%T%2%n\n", 1);
	format_add("show_status_avail", "%Ydost�pny%n", 1);
	format_add("show_status_avail_descr", "%Ydost�pny%n (%T%1%n%2)", 1);
	format_add("show_status_busy", "%Gzaj�ty%n", 1);
	format_add("show_status_busy_descr", "%Gzaj�ty%n (%T%1%n%2)", 1);
	format_add("show_status_invisible", "%bniewidoczny%n", 1);
	format_add("show_status_invisible_descr", "%bniewidoczny%n (%T%1%n%2)", 1);
	format_add("show_status_not_avail", "%rniedost�pny%n", 1);
	format_add("show_status_private_on", ", tylko dla znajomych", 1);
	format_add("show_status_private_off", "", 1);
	format_add("show_status_connected_since", "%) Po��czony od: %T%1%n\n", 1);
	format_add("show_status_disconnected_since", "%) Roz��czony od: %T%1%n\n", 1);
	format_add("show_status_last_conn_event", "%Y-%m-%d %H:%M", 1);
	format_add("show_status_last_conn_event_today", "%H:%M", 1);
	format_add("show_status_msg_queue", "%) Ilo�� wiadomo�ci w kolejce do wys�ania: %T%1%n\n", 1);

	/* aliasy */
	format_add("aliases_invalid", "%! Nieprawid�owy parametr\n", 1);
	format_add("aliases_list_empty", "%! Brak alias�w\n", 1);
	format_add("aliases_list", "%> %T%1%n: %2\n", 1);
	format_add("aliases_list_next", "%> %3  %2\n", 1);
	format_add("aliases_add", "%> Utworzono alias %T%1%n\n", 1);
	format_add("aliases_append", "%> Dodano do aliasu %T%1%n\n", 1);
	format_add("aliases_del", "%) Usuni�to alias %T%1%n\n", 1);
	format_add("aliases_exist", "%! Alias %T%1%n ju� istnieje\n", 1);
	format_add("aliases_noexist", "%! Alias %T%1%n nie istnieje\n", 1);
	format_add("aliases_command", "%! %T%1%n jest wbudowan� komend�\n", 1);

	/* po��czenia bezpo�rednie */
	format_add("dcc_create_error", "%! Nie mo�na w��czy� po��cze� bezpo�rednich: %1\n", 1);
	format_add("dcc_error_network", "%! B��d transmisji z %1\n", 1);
	format_add("dcc_error_refused", "%! Po��czenie z %1 zosta�o odrzucone\n", 1);
	format_add("dcc_error_unknown", "%! Nieznany b��d po��czenia bezpo�redniego\n", 1);
	format_add("dcc_error_handshake", "%! Nie mo�na nawi�za� po��czenia z %1\n", 1);
	format_add("dcc_timeout", "%! Przekroczono limit czasu operacji bezpo�redniego po��czenia\n", 1);
	format_add("dcc_unknown_command", "%! Nieznana opcja: %T%1%n\n", 1);
	format_add("dcc_not_supported", "%! Opcja %T%1%n nie jest jeszcze obs�ugiwana\n", 1);
	format_add("dcc_open_error", "%! Nie mo�na otworzy� %T%1%n: %2\n", 1);
	format_add("dcc_open_directory", "%! Nie mo�na otworzy� %T%1%n: Jest katalogiem\n", 1);
	format_add("dcc_show_pending_header", "%> Po��czenia oczekuj�ce:\n", 1);
	format_add("dcc_show_pending_send", "%) #%1, %2, wysy�anie %T%3%n\n", 1);
	format_add("dcc_show_pending_get", "%) #%1, %2, odbi�r %T%3%n\n", 1);
	format_add("dcc_show_pending_voice", "%) #%1, %2, rozmowa\n", 1);
	format_add("dcc_show_active_header", "%> Po��czenia aktywne:\n", 1);
	format_add("dcc_show_active_send", "%) #%1, %2, wysy�anie %T%3%n\n", 1);
	format_add("dcc_show_active_get", "%) #%1, %2, odbi�r %T%3%n\n", 1);
	format_add("dcc_show_active_voice", "%) #%1, %2, rozmowa\n", 1);
	format_add("dcc_show_empty", "%> Brak bezpo�rednich po��cze�\n", 1);
	format_add("dcc_show_debug", "%> id=%1, type=%2, filename=%3, uin=%4, dcc=%5\n", 1);
	
	format_add("dcc_done_get", "%> Zako�czono pobieranie pliku %T%2%n od %1\n", 1);
	format_add("dcc_done_send", "%> Zako�czono wysy�anie pliku %T%2%n do %1\n", 1);
	
	format_add("dcc_get_offer", "%) %1 przesy�a plik %T%2%n o rozmiarze %T%3%n\n%) Wpisz %Tdcc get #%4%n by go odebra�, lub %Tdcc close #%4%n by anulowa�\n", 1);
	format_add("dcc_voice_offer", "%) %1 chce rozmawia�\n%) Wpisz %Tdcc voice #%2%n by rozpocz�� rozmow�, lub %Tdcc close #%2%n by anulowa�\n", 1);
	format_add("dcc_voice_unsupported", "%) Nie wkompilowano obs�ugi rozm�w g�osowych. Przeczytaj %Tdocs/voip.txt%n\n", 1);
	format_add("dcc_voice_running", "%) Mo�na prowadzi� tylko jedn� rozmow� g�osow� na raz\n", 1);
	format_add("dcc_get_not_found", "%! Nie znaleziono po��czenia %T%1%n\n", 1);
	format_add("dcc_get_getting", "%) Rozpocz�to pobieranie pliku %T%2%n od %1\n", 1);
	format_add("dcc_get_cant_create", "%! Nie mo�na utworzy� pliku %T%1%n\n", 1);
	format_add("dcc_invalid_ip", "%! Nieprawid�owy adres IP\n", 1);

	/* query */
	format_add("query_started", "%) Rozpocz�to rozmow� z %T%1%n\n", 1);
	format_add("query_started_window", "%) Wci�nij %TAlt-I%n by ignorowa�, %TAlt-K%n by zamkn�� okno\n", 1);
	format_add("query_finished", "%) Zako�czono rozmow� z %T%1%n\n", 1);
	format_add("query_exist", "%! Rozmowa z %T%1%n jest ju� prowadzona w okienku nr %T%2%n\n", 1);

	/* zdarzenia */
        format_add("events_list_empty", "%! Brak zdarze�\n", 1);
        format_add("events_list", "%> on %G%1 %T%2 %B%3%n\n", 1);
	format_add("events_list_inactive", "%> on %G%1 %T%2 %B%3 %g(nieaktywne)%n\n", 1);
        format_add("events_incorrect", "%! Nieprawid�owo zdefiniowane zdarzenie\n", 1);
        format_add("events_add", "%> Dodano zdarzenie\n", 1);
        format_add("events_exist", "%! Zdarzenie %1 istnieje dla %2\n", 1);
        format_add("events_del", "%> Usuni�to zdarzenie\n", 1);
        format_add("events_del_flags", "%> Flagi %G%1%n usuni�te\n", 1);
        format_add("events_add_flags", "%> Flagi %G%1%n dodane\n", 1);
        format_add("events_noexist", "%! Nieznane zdarzenie\n", 1);
        format_add("events_del_noexist", "%! Zdarzenie %T%1%n nie istnieje dla u�ytkownika %G%2%n\n", 1);
        format_add("events_seq_not_found", "%! Sekwencja %T%1%n nie znaleziona\n", 1);
        format_add("events_act_no_params", "%! Brak parametr�w\n", 1);
	format_add("events_act_toomany_params", "%! Za du�o parametr�w\n", 1);
	format_add("events_seq_incorrect", "%! Nieprawid�owa sekwencja\n", 1);
        format_add("temporary_run_event", "%) Startujemy z akcj� '%B%1%n'\n", 1);

	/* lista kontakt�w z serwera */
	format_add("userlist_put_ok", "%) List� kontakt�w zachowano na serwerze\n", 1);
	format_add("userlist_put_error", "%! B��d podczas wysy�ania listy kontakt�w\n", 1);
	format_add("userlist_get_ok", "%) List� kontakt�w wczytano z serwera\n", 1);
	format_add("userlist_get_error", "%! B��d podczas pobierania listy kontakt�w\n", 1);

	/* szybka lista kontakt�w pod F2 */
	format_add("quick_list", "%)%1\n", 1);
	format_add("quick_list,speech", "lista kontakt�w: ", 1);
	format_add("quick_list_avail", " %T%1%n", 1);
	format_add("quick_list_avail,speech", "%1 jest dost�pny, ", 1);
	format_add("quick_list_busy", " %w%1%n", 1);
	format_add("quick_list_busy,speech", "%1 jest zaj�ty, ", 1);
	format_add("quick_list_invisible", " %K%1%n", 1);

	/* window */
	format_add("window_add", "%) Utworzono nowe okno\n", 1);
	format_add("window_noexist", "%! Wybrane okno nie istnieje\n", 1);
	format_add("window_no_windows", "%! Nie mo�na zamkn�� ostatniego okna\n", 1);
	format_add("window_del", "%) Zamkni�to okno\n", 1);
	format_add("windows_max", "%! Wyczerpano limit ilo�ci okien\n", 1);
	format_add("window_list_query", "%) %1: rozmowa z %T%2%n\n", 1);
	format_add("window_list_nothing", "%) %1: brak rozmowy\n", 1);
	format_add("window_invalid", "%! Nieprawid�owy parametr\n", 1);
	format_add("window_id_query_started", "%) Rozmowa z %T%2%n rozpocz�ta w oknie %W%1%n\n", 1);
	format_add("window_kill_status", "%! Nie mo�na zamkn�� okna statusowego\n", 1);

	/* bind */
	format_add("bind_seq_incorrect", "%! Sekwencja %T%1%n jest nieprawid�owa\n", 1); 
	format_add("bind_seq_add", "%) Dodano sekwencj� %T%1%n\n", 1);
	format_add("bind_seq_remove", "%) Usuni�to sekwencj� %T%1%n\n", 1);	
	format_add("bind_invalid", "%! Nieprawid�owy parametr\n", 1);
	format_add("bind_seq_list", "%) %1: %T%2%n\n", 1);
	format_add("bind_seq_exist", "%! Sekwencja %T%1%n ma ju� przypisan� akcj�\n", 1);
	format_add("bind_seq_list_empty", "%! Brak przypisanych akcji\n", 1);

	/* timery */
	format_add("timer_list", "%> %1, %2s, %3\n", 1);
	format_add("last_list_in", "%) %Y <<%n [%1] %2 %3\n", 1);
	format_add("last_list_out", "%) %G >>%n [%1] %2 %3\n", 1);
	format_add("last_list_empty", "%! Nie zalogowano �adnych wiadomo�ci\n", 1);
	format_add("last_list_empty_nick", "%! Nie zalogowano �adnych wiadomo�ci od/do %W%1%n\n", 1);
	format_add("last_list_timestamp", "%m-%d-%Y %H:%M", 1);
	format_add("last_list_timestamp_today", "%H:%M", 1);
	format_add("last_clear_uin", "%) Wiadomo�ci od/do %W%1%n wyczyszczone\n", 1);
	format_add("last_clear", "%) Wszystkie wiadomo�ci wyczyszczone\n", 1);

	/* queue */
	format_add("queue_list_timestamp", "%m-%d-%Y %H:%M", 1);
	format_add("queue_list_message", "%) %G >>%n [%1] %2 %3\n", 1);
	format_add("queue_clear","%) Kolejka wiadomo�ci wyczyszczona\n", 1);
	format_add("queue_clear_uin","%) Kolejka wiadomo�ci wyczyszczona dla %W%1%n\n", 1);
	format_add("queue_wrong_use", "%! Komenda dzia�a tylko przy braku po��czenia z serwerem\n", 1);
	format_add("queue_empty", "%! Kolejka wiadomo�ci jest pusta\n", 1);
	format_add("queue_empty_uin", "%! Brak wiadomo�ci w kolejce dla %W%1%n\n", 1);
	format_add("queue_flush", "%) Wys�ano zaleg�e wiadomo�ci z kolejki\n", 1);

	/* conference */
	format_add("conferences_invalid", "%! Nieprawid�owy parametr\n", 1);
	format_add("conferences_list_empty", "%! Brak konferencji\n", 1);
	format_add("conferences_list", "%> %T%1%n: %2\n", 1);
	format_add("conferences_list_ignored", "%> %T%1%n: %2 (%yingorowana%n)\n", 1);
	format_add("conferences_add", "%> Utworzono konferencj� %T%1%n\n", 1);
	format_add("conferences_not_added", "%) Nie utworzono konferencji %T%1%n\n", 1);
	format_add("conferences_del", "%> Usuni�to konferencj� %T%1%n\n", 1);
	format_add("conferences_exist", "%! Konferencja %T%1%n ju� istnieje\n", 1);
	format_add("conferences_noexist", "%! Konferencja %T%1%n nie istnieje\n", 1);
	format_add("conferences_name_error", "%! Nazwa konferencji powinna zaczyna� si� od %T#%n\n", 1);
	format_add("conferences_rename", "%> Nazwa konferencji zmieniona: %T%1%n --> %T%2%n\n", 1);
	format_add("conferences_ignore", "%> Konferencja %T%1%n b�dzie ignorowana\n", 1);
	format_add("conferences_unignore", "%> Konferencja %T%1%n nie b�dzie ignorowana\n", 1);
	
	/* wsp�lne dla us�ug http */
	format_add("http_failed_resolving", "Nie znaleziono serwera", 1);
	format_add("http_failed_connecting", "Nie mo�na po��czy� si� z serwerem", 1);
	format_add("http_failed_reading", "Serwer zerwa� po��czenie", 1);
	format_add("http_failed_writing", "Serwer zerwa� po��czenie", 1);
	format_add("http_failed_memory", "Brak pami�ci", 1);

	/* szyfrowanie */
	format_add("public_key_received", "%> %1 wys�a� sw�j klucz publiczny\n", 1);
	format_add("public_key_write_failed", "%! Nie uda�o si� zapisa� klucza publicznego: %1\n", 1);
	format_add("public_key_not_found", "%! Nie znaleziono klucza publicznego %1\n", 1);
	format_add("public_key_deleted", "%! Klucz publiczny %1 zosta� usuni�ty\n", 1);
};
