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
#include "stuff.h"
#include "dynstuff.h"
#include "themes.h"
#include "config.h"

char *prompt_cache = NULL, *prompt2_cache = NULL, *error_cache = NULL, *timestamp_cache = NULL;
char *readline_prompt = NULL, *readline_prompt_away = NULL;

struct list *formats = NULL;

/*
 * find_format()
 *
 * odnajduje warto�� danego formatu. je�li nie znajdzie, zwraca pusty ci�g,
 * �eby nie musie� uwa�a� na �adne null-references.
 *
 *  - name.
 */
char *find_format(char *name)
{
	struct list *l;
	
	for (l = formats; l; l = l->next) {
		struct format *f = l->data;

		if (!strcasecmp(f->name, name))
			return f->value;
	}
	
	return "";
}

/*
 * va_format_string()
 *
 * formatuje zgodnie z podanymi parametrami ci�g znak�w.
 *
 *  - format - warto��, nie nazwa formatu,
 *  - ap - argumenty.
 */
char *va_format_string(char *format, va_list ap)
{
	struct string *buf;
	char *p, *args[9];
	int i;
	// void **args = (void**) ap;

	for (i = 0; i < 9; i++)
		args[i] = NULL;

	for (i = 0; i < 9; i++) {
		if (!(args[i] = va_arg(ap, char*)))
			break;
	}

	if (!prompt_cache) {
		prompt_cache = "dummy";
		prompt_cache = format_string(find_format("prompt"));
	}
	if (!prompt2_cache) {
		prompt2_cache = "dummy";
		prompt2_cache = format_string(find_format("prompt2"));
	}
	if (!error_cache) {
		error_cache = "dummy";
		error_cache = format_string(find_format("error"));
	}
	if (!timestamp_cache)
		timestamp_cache = find_format("timestamp");
	
	if (!(buf = string_init("")))
		return NULL;

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
			if (config_display_color) {
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
			}

			fill_before = 0;
			fill_after = 0;
			fill_length = 0;
			fill_char = ' ';
			fill_soft = 1;

			if (*p == '[' || *p == '(') {
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

				fill_length = strtol(p, &p, 0);
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

	return string_free(buf, 0);
}

/*
 * format_string()
 *
 * j.w. tyle �e nie potrzeba dawa� mu va_list, a wystarcz� zwyk�e parametry.
 *
 *  - format... - j.w.,
 */
char *format_string(char *format, ...)
{
	va_list ap;
	char *tmp;
	
	va_start(ap, format);
	tmp = va_format_string(format, ap);
	va_end(ap);

	return tmp;
}

/*
 * my_printf()
 *
 * drukuje na stdout tekst, bior�c pod uwag� nazw�, nie warto�� formatu.
 * parametry takie jak zdefiniowano. pierwszy to %1, drugi to %2.
 */
void my_printf(char *theme, ...)
{
	va_list ap;
	char *tmp;
	
	va_start(ap, theme);
	tmp = va_format_string(find_format(theme), ap);
	my_puts("%s", (tmp) ? tmp : "");
	free(tmp);
	va_end(ap);
}

/*
 * reset_theme_cache()
 *
 * usuwa cache'owane prompty. przydaje si� przy zmianie theme'u.
 */
inline void reset_theme_cache()
{
	free(prompt_cache);
	free(prompt2_cache);
	free(error_cache);
	
	prompt_cache = prompt2_cache = error_cache = NULL;
	readline_prompt = readline_prompt_away = NULL;
	timestamp_cache = NULL;
}

/*
 * add_format()
 *
 * dodaje dan� formatk� do listy.
 *
 *  - name - nazwa,
 *  - value - warto��,
 *  - replace - je�li znajdzie, to zostawia (=0) lub zamienia (=1).
 */
int add_format(char *name, char *value, int replace)
{
	struct format f;
	struct list *l;

	for (l = formats; l; l = l->next) {
		struct format *g = l->data;

		if (!strcasecmp(name, g->name)) {
			if (replace) {
				free(g->value);
				g->value = strdup(value);
			}

			return 0;
		}
	}

	f.name = strdup(name);
	f.value = strdup(value);
	list_add(&formats, &f, sizeof(f));
	
	return 0;
}

/*
 * del_format()
 *
 * usuwa formatk� o danej nazwie.
 *
 *  - name.
 */
int del_format(char *name)
{
	struct list *l;

	for (l = formats; l; l = l->next) {
		struct format *f = l->data;

		if (!strcasecmp(f->name, name)) {
			free(f->value);
			free(f->name);
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
static FILE *try_open(FILE *prevfd, char *prefix, char *filename)
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
 * read_theme()
 *
 * wczytuje opis wygl�du z podanego pliku. 
 *
 *  - filename.
 */
int read_theme(char *filename, int replace)
{
        char *buf, *tmp;
        FILE *f = NULL;

        if (!filename) {
                filename = prepare_path("default.theme");
		if (!filename || !(f = fopen(filename, "r")))
			return -1;
        } else {
		if (strchr(filename, '/'))
			f = try_open(NULL, NULL, filename);
		else {
			f = try_open(NULL, THEMES_DIR, filename);
			tmp = prepare_path("");
			f = try_open(f, tmp, filename);
			tmp = prepare_path("themes");
			f = try_open(f, tmp, filename);
		}
		if (!f)
			return -1;
	}

        while ((buf = read_file(f))) {
                char *value, *p;

                if (buf[0] == '#') {
			free(buf);
                        continue;
		}

                if (!(value = strchr(buf, ' '))) {
			free(buf);
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
			del_format(buf + 1);
		else
			add_format(buf, value, replace);

		free(buf);
        }

        fclose(f);

        return 0;
}

/*
 * init_theme()
 *
 * ustawia domy�lne warto�ci formatek.
 */
void init_theme()
{
	add_format("prompt", "%g-%G>%g-%n", 1);
	add_format("prompt2", "%c-%C>%c-%n", 1);
	add_format("error", "%r-%R>%r-%n", 1);
	add_format("timestamp", "%H:%M", 1);
	add_format("readline_prompt", "% ", 1);
	add_format("readline_prompt_away", "/ ", 1);
	add_format("readline_prompt_query", "%1> ", 1);

	add_format("known_user", "%W%1%n/%2", 1);
	add_format("unknown_user", "%W%1%n", 1);
	add_format("user_not_given", "Nie podany u�ytkownik!\n", 1);
	
	add_format("none", "%1\n", 1);
	add_format("not_enough_params", "%! Za ma�o parametr�w\n", 1);
	add_format("invalid_uin", "%! Nieprawid�owy numer u�ytkownika\n", 1);
	add_format("user_added", "%> U�ytkownik %1 zosta� dopisany do listy kontakt�w\n", 1);
	add_format("error_adding", "%! Wyst�pi� b��d podczas dopisywania u�ytkownika\n", 1);
	add_format("away", "%> Zmieniono stan na zaj�ty %c(%C%#%c)%n\n", 1);
	add_format("away_descr", "%> Zmieniono stan na zaj�ty: %1 %c(%C%#%c)%n\n", 1);
	add_format("back", "%> Zmieniono stan na dost�pny %c(%C%#%c)%n\n", 1);
	add_format("invisible", "%> Zmieniono stan na niewidoczny %c(%C%#%c)%n\n", 1);
	add_format("user_not_found", "%! Nie znaleziono u�ytkownika %W%1%n\n", 1);
	add_format("user_deleted", "%> U�ytkownik %1 zosta� usuni�ty z listy kontakt�w\n", 1);
	add_format("error_deleting", "%! Wyst�pi� b��d podczas usuwania u�ytkownika\n", 1);
	add_format("user_exists", "%! U�ytkownik %W%1%n ju� istnieje w li�cie kontakt�w\n", 1);
	add_format("help", "%> %1%2 - %3\n", 1);
	add_format("help_more", "%) %1\n", 1);
	add_format("generic", "%> %1\n", 1);
	add_format("ignored_list", "%> %1\n", 1);
	add_format("ignored_list_empty", "%! Lista ignorowanych u�ytkownik�w jest pusta\n", 1);
	add_format("ignored_added", "%> U�ytkownika %W%1%n dodano do listy ignorowanych\n", 1);
	add_format("error_adding_ignored", "%! Dodanie do listy ignorowanych nie powiod�o si�\n", 1);
	add_format("ignored_deleted", "%> U�ytkownika %W%1%n usuni�to z listy ignorowanych\n", 1);
	add_format("error_not_ignored", "%! U�ytkownik %W%1%n nie jest ignorowany\n", 1);
	add_format("list_empty", "%! Lista kontakt�w jest pusta\n", 1);
	add_format("list_avail", "%> %1 %Y(dost�pny)%n %b%2:%3%n\n", 1);
	add_format("list_busy", "%> %1 %G(zaj�ty)%n %b%2:%3%n\n", 1);
	add_format("list_busy_descr", "%> %1 %G(zaj�ty: %n%4%G)%n %b%2:%3%n\n", 1);
	add_format("list_not_avail", "%> %1 %r(niedost�pny)%n\n", 1);
	add_format("list_not_avail_descr", "%> %1 %r(niedost�pny: %n%4%r)%n\n", 1);
	add_format("list_invisible", "%> %1 %c(niewidoczny)%n %b%2:%3%n\n", 1);
	add_format("list_unknown", "%> %1\n", 1);
	add_format("saved", "%> Zapisano ustawienia\n", 1);
	add_format("error_saving", "%! Podczas zapisu ustawie� wyst�pi� b��d\n", 1);
	add_format("quit", "%> Papa\n", 1);
	add_format("quit_descr", "%> Papa: %1\n", 1);
	add_format("message_header", "%g.-- %n%1 %c(%C%#%c/%2)%n %g--- -- -\n", 1);
	add_format("message_footer", "%g`----- ---- --- -- -%n\n", 1);
	add_format("message_line", "%g|%n %1\n", 1);
	add_format("message_line_width", "75", 1);
	add_format("chat_header", "%c.-- %n%1 %c(%C%#%c/%2)%n %c--- -- -\n", 1);
	add_format("chat_footer", "%c`----- ---- --- -- -%n\n", 1);
	add_format("chat_line", "%c|%n %1\n", 1);
	add_format("chat_line_width", "75", 1);
	add_format("sysmsg_header", "%m.-- %WWiadomo�� systemowa %m--- -- -\n", 1);
	add_format("sysmsg_line", "%m|%n %1\n", 1);
	add_format("sysmsg_line_width", "75", 1);
	add_format("sysmsg_footer", "%m`----- ---- --- -- -%n\n", 1);	
	add_format("ack_queued", "%> Wiadomo�� do %1 zostanie dostarczona p�niej %c(%C%#%c)%n\n", 1);
	add_format("ack_delivered", "%> Wiadomo�� do %1 zosta�a dostarczona %c(%C%#%c)%n\n", 1);
	add_format("status_avail", "%> %1 jest dost�pny %c(%C%#%c)%n\n", 1);
	add_format("status_busy", "%> %1 jest zaj�ty %c(%C%#%c)%n\n", 1);
	add_format("status_busy_descr", "%> %1 jest zaj�ty: %2 %c(%C%#%c)%n\n", 1);
	add_format("status_not_avail", "%> %1 jest niedost�pny %c(%C%#%c)%n\n", 1);
	add_format("status_not_avail_descr", "%> %1 jest niedost�pny: %2 %c(%C%#%c)%n\n", 1);
	add_format("status_invisible", "%> %1 jest niewidoczny %c(%C%#%c)%n\n", 1);
	add_format("conn_broken", "%! Serwer zerwa� po��czenie: %1 %c(%C%#%c)%n\n", 1);
	add_format("auto_away", "%> Automagicznie zmieniono stan na zaj�ty po %1 nieaktywno�ci %c(%C%#%c)%n\n", 1);
	add_format("welcome", "%> EKG-%1 (Eksperymentalny Klient Gadu-gadu)\n%> (C) Copyright 2001, 2002 Wojtek Kaniewski <wojtekka@irc.pl> i inni\n%> Program jest rozprowadzany na zasadach licencji GPL\n\n", 1);
	add_format("error_reading_config", "%! Nie mo�na odczyta� pliku konfiguracyjnego: %1\n", 1);
	add_format("offline_mode", "%! Tryb off-line\n", 1);
	add_format("connecting", "%> ��cz� si� z serwerem...\n", 1);
	add_format("conn_failed", "%! Po��czenie nie uda�o si�: %1\n", 1);
	add_format("conn_stopped", "%! Przerwano ��czenie\n", 1);
	add_format("conn_timeout", "%! Przekroczono limit czasu operacji ��czenia z serwerem\n", 1);
	add_format("disconn_warning", "%! Serwer roz��czy� nas.\n", 1); 
	add_format("connected", "%> Po��czono %c(%C%#%c)%n\n", 1);
	add_format("disconnected", "%> Roz��czono %c(%C%#%c)%n\n", 1);
	add_format("disconnected_descr", "%> Roz��czono %c(%C%#%c): %1%n\n", 1);
	add_format("theme_loaded", "%> Wczytano opis wygl�du o nazwie %W%1%n\n", 1);
	add_format("theme_default", "%> Ustawiono domy�lny opis wygl�du\n", 1);
	add_format("error_loading_theme", "%! Wyst�pi� b��d podczas �adowania opisu wygl�du: %1\n", 1);
	add_format("not_connected", "%! Brak po��czenia z serwerem\n", 1);
	add_format("variable", "%> %1 = %2\n", 1);
	add_format("variable_not_found", "%! Nieznana zmienna: %1\n", 1);
	add_format("variable_invalid", "%! Nieprawid�owa warto�� zmiennej\n", 1);
	add_format("not_implemented", "%! Tej funkcji jeszcze nie ma\n", 1);
	add_format("no_config", "%! Niekompletna konfiguracja. Wpisz:\n%!   %Wset uin <numerek-gg>%n\n%!   %Wset password <has�o>%n\n%!   %Wsave%n\n%! Nast�pnie wydaj polecenie:\n%!   %Wconnect%n\n%! Je�li nie masz swojego numerka, wpisz:\n%!   %Wregister <e-mail> <has�o>%n\n\n", 1);
	
	add_format("register", "%> Rejestracja poprawna: Wygrany numerek: %W%1%n\n", 1);
	add_format("register_failed", "%! B��d podczas rejestracji\n", 1);
	add_format("register_pending", "%! Rejestracja w toku\n", 1);
	add_format("register_timeout", "%! Przekroczono limit czasu operacji szukania\n", 1);
	
	add_format("remind", "%> Has�o zosta�o wys�ane\n", 1);
	add_format("remind_failed", "%! B��d podczas wysy�ania has�a\n", 1);
	add_format("remind_timeout", "%! Przekroczono limit czasu operacji wys�ania has�a\n", 1);
	
	add_format("passwd", "%> Has�o zosta�o zmienione\n", 1);
	add_format("passwd_failed", "%! B��d podczas zmiany has�a\n", 1);
	add_format("passwd_timeout", "%! Przekroczono limit czasu operacji zmiany has�a\n", 1);
	
	add_format("change", "%> Informacje w katalogu publicznym zosta�y zmienione\n", 1);
	add_format("change_failed", "%! B��d podczas zmiany informacji w katalogu publicznym\n", 1);
	add_format("change_timeout", "%! Przekroczono limit czasu operacji zmiany katalogu publicznego\n", 1);
	
	add_format("sms_msg", "EKG: msg %1 %# >> %2", 1);
	add_format("sms_chat", "EKG: chat %1 %# >> %2", 1);
	add_format("sms_error", "%! B��d wysy�ania SMS\n", 1);
	add_format("sms_unknown", "%! U�ytkownik %1 nie ma podanego numeru kom�rki\n", 1);
	add_format("sms_sent", "%> SMS do %W%1%n zosta� wys�any\n", 1);
	add_format("sms_failed", "%! SMS do %W%1%n nie zosta� wys�any\n", 1);

	add_format("already_connected", "%! Klient jest ju� po��czony\n", 1);
	add_format("during_connect", "%! ��czenie trwa\n", 1);
	add_format("search_failed", "%! Wyst�pi� b��d podczas szukania: %1\n", 1);
	add_format("search_timeout", "%! Przekroczono limit czasu operacji szukania\n", 1);
	add_format("search_not_found", "%! Nie znaleziono\n", 1);
	add_format("unknown_command", "%! Nieznane polecenie: %W%1%n\n", 1);
	add_format("already_searching", "%! Szukanie trwa. Poczekaj, albo u�yj %Wfind -stop%n\n", 1);

	/* 1 uin, 2 name, 3 nick, 4 city, 5 born, 6 gender, 7 active */

	add_format("search_results_multi_active", "%G!%n", 1);
	add_format("search_results_multi_inactive", " ", 1);
	add_format("search_results_multi_unknown", "-", 1);
	add_format("search_results_multi_female", "k", 1);
	add_format("search_results_multi_male", "m", 1);
	add_format("search_results_multi", "%7 %[-10]1 %K/%n %[12]3 %K/%n %6 %K/%n %[20]2 %K/%n %[4]5 %K/%n %[16]4\n", 1);

	add_format("search_results_single_active", "%G(aktywny)%n", 1);
	add_format("search_results_single_inactive", "%r(nieaktywny)%n", 1);
	add_format("search_results_single_unknown", "%W-%n", 1);
	add_format("search_results_single_female", "%Mkobieta%n", 1);
	add_format("search_results_single_male", "%Cm�czyzna%n", 1);
	add_format("search_results_single", "%) Nick: %W%3%n\n%) Numerek: %W%1%n\n%) Imi� i nazwisko: %W%2%n\n%) Miejscowo��: %W%4%n\n%) Rok urodzenia: %W%5%n\n%) P�e�: %6\n", 1);

	add_format("search_stopped", "%) Zatrzymano szukanie\n", 1);

	add_format("process", "%> %(-5)1 %2\n", 1);
	add_format("no_processes", "%! Nie ma dzia�aj�cych proces�w\n", 1);
	add_format("process_exit", "%> Proces %1 (%2) zako�czy� dzia�anie z wynikiem %3\n", 1);

	add_format("modify_done", "%> Zmieniono wpis w li�cie kontakt�w\n", 1);
	add_format("user_info", "%) Pseudonim: %W%3%n\n%) Numer: %W%7%n\n%) Stan: %8\n%) Imi� i nazwisko: %W%1 %2%n\n%) Alias: %W%4%n\n%) Numer telefonu: %W%5%n\n%) Grupy: %W%6%n\n", 1);
	add_format("user_info_avail", "%Ydost�pny%n", 1);
	add_format("user_info_busy", "%Gzaj�ty%n", 1);
	add_format("user_info_busy_descr", "%Gzaj�ty%n (%1)", 1);
	add_format("user_info_not_avail", "%rniedost�pny%n", 1);
	add_format("user_info_not_avail_descr", "%rniedost�pny%n (%1)", 1);
	add_format("user_info_invisible", "%cniewidoczny%n", 1);

	add_format("config_changed", "Zapisa� now� konfiguracj�? (tak/nie) ", 1);
	add_format("config_unknown", "%! Zmiana tego ustawienia mo�e nie odnie�� zamierzonego efektu\n", 1);

	add_format("private_mode_is_on", "%> Tryb ,,tylko dla przyjaci�'' jest w��czony\n", 1);
	add_format("private_mode_is_off", "%> Tryb ,,tylko dla przyjaci�'' jest wy��czony\n", 1);
	add_format("private_mode_on", "%> W��czono tryb ,,tylko dla przyjaci�''\n", 1);
	add_format("private_mode_off", "%> Wy��czono tryb ,,tylko dla przyjaci�''\n", 1);
	add_format("private_mode_invalid", "%! Nieprawid�owa warto��\n", 1);
	
	add_format("show_status", "%) Aktualny stan: %1%2\n", 1);
	add_format("show_status_avail", "%Ydost�pny%n", 1);
	add_format("show_status_busy", "%Gzaj�ty%n", 1);
	add_format("show_status_busy_descr", "%Gzaj�ty%n (%1)", 1);
	add_format("show_status_invisible", "%bniewidoczny%n", 1);
	add_format("show_status_not_avail", "%rniedost�pny%n", 1);
	add_format("show_status_private_on", ", tylko dla znajomych", 1);
	add_format("show_status_private_off", "", 1);

	add_format("aliases_invalid", "%! Nieprawid�owy parametr\n", 1);
	add_format("aliases_list_empty", "%! Brak alias�w\n", 1);
	add_format("aliases_list", "%> %W%1 %G-%Y %2%n\n", 1);
	add_format("aliases_add", "%> Dodano alias %W%1 %n-> %2\n", 1);
	add_format("aliases_del", "%) Usuni�to alias %W%1%n\n", 1);
	add_format("aliases_exist", "%! Alias %W%1%n ju� istnieje\n", 1);
	add_format("aliases_noexist", "%! Alias %W%1%n nie istnieje\n", 1);

	add_format("dcc_create_error", "%! Nie mo�na w��czy� DCC: %1\n", 1);
	add_format("dcc_error_unknown", "%! B��d krytyczny obs�ugi DCC: %1\n", 1);
	add_format("dcc_error_handshake", "%! Nie mo�na nawi�za� po��cznienia z %1\n", 1);
	add_format("dcc_timeout", "%! Timeout DCC\n", 1);
	add_format("dcc_unknown_command", "%! Nieznana opcja: %W%1%n\n", 1);
	add_format("dcc_not_supported", "%! Opcja %W%1%n nie jest jeszcze obs�ugiwana\n", 1);
	add_format("dcc_open_error", "%! Nie mo�na otworzy� %W%1%n: %2\n", 1);
	add_format("dcc_open_directory", "%! Nie mo�na otworzy� %W%1%n: Jest katalogiem\n", 1);
	add_format("dcc_show_pending_header", "%> Po��czenia oczekuj�ce:\n", 1);
	add_format("dcc_show_pending_send", "%) #%1, %2, wysy�anie %W%3%n\n", 1);
	add_format("dcc_show_pending_get", "%) #%1, %2, odbi�r %W%3%n\n", 1);
	add_format("dcc_show_active_header", "%> Po��czenia aktywne:\n", 1);
	add_format("dcc_show_active_send", "%) #%1, %2, wysy�anie %W%3%n\n", 1);
	add_format("dcc_show_active_get", "%) #%1, %2, odbi�r %W%3%n\n", 1);
	add_format("dcc_show_empty", "%> Brak bezpo�rednich po��cze�\n", 1);
	add_format("dcc_show_debug", "%> id=%1, type=%2, filename=%3, uin=%4, dcc=%5\n", 1);
	
	add_format("dcc_done_get", "%> Zako�czono pobieranie pliku %W%2%n od %1\n", 1);
	add_format("dcc_done_send", "%> Zako�czono wysy�anie pliku %W%2%n do %1\n", 1);
	
	add_format("dcc_get_offer", "%) %1 przesy�a plik %W%2%n o rozmiarze %W%3%n\n%) Wpisz %Wdcc get #%4%n by go odebra�, lub %Wdcc close #%4%n by anulowa�\n", 1);
	add_format("dcc_get_not_found", "%! Nie znaleziono po��czenia %W%1%n\n", 1);
	add_format("dcc_get_getting", "%) Rozpocz�to pobieranie pliku %W%2%n od %1\n", 1);
	add_format("dcc_get_cant_create", "%! Nie mo�na utworzy� pliku %W%1%n\n", 1);
	

	add_format("query_started", "%) Rozpocz�to rozmow� z %1. Aby zako�czy�, wci�nij Ctrl-D\n", 1);
	add_format("query_finished", "\n%) Zako�czono rozmow� z %1\n", 1);

        add_format("events_list_empty", "%! Brak zdarze�\n", 1);
        add_format("events_list", "%> on %G%1 %W%2 %B%3%n\n", 1);
        add_format("events_incorrect", "%! Nieprawid�owo zdefiniowane zdarzenie.\n", 1);
        add_format("events_add", "%> 'on %G%1 %W%2 %Y%3'%n - zdarzenie dodane.\n", 1);
        add_format("events_exist", "%! Zdarzenie %1 istnieje dla %2.\n", 1);
        add_format("events_del", "%> on %G%1 %W%2 %Y%3'%n - zdarzenie usuni�te.\n", 1);
        add_format("events_del_flags", "%> Flagi %G%1%n usuni�te.\n", 1);
        add_format("events_add_flags", "%> Flagi %G%1%n dodane.\n", 1);
        add_format("events_noexist", "%! Niezidentyfikowane zdarzenie.\n", 1);
        add_format("events_del_noexist", "%! Zdarzenie %W%1%n nie istnieje dla u�ytkownika %G%2%n\n", 1);
        add_format("events_seq_not_found", "%! Sekwencja %W%1%n nie znaleziona.\n", 1);
        add_format("events_act_no_params", "%! %W%1%n - brak parametr�w.\n", 1);        
	add_format("events_act_toomany_params", "%! %W%1%n - za du�o parametr�w.\n", 1);
	add_format("events_seq_incorrect", "%W%1%n - nieprawid�owa sekwencja\n", 1);
        add_format("config_line_incorrect", "%! Nieprawid�owa linia '%W%1%n', pomijam\n", 1);
        add_format("temporary_run_event", "%) Startujemy z akcj� '%B%1%n'\n", 1);
	add_format("autosaved", "%> Automatycznie zapisano ustawienia\n", 1);
	add_format("userlist_put_ok", "%) List� kontakt�w zachowano na serwerze\n", 1);
	add_format("userlist_put_error", "%! B��d podczas wysy�ania listy kontakt�w: %1\n", 1);
	add_format("userlist_get_ok", "%) List� kontakt�w wczytano z serwera\n", 1);
	add_format("userlist_get_error", "%! B��d podczas pobierania listy kontakt�w: %1\n", 1);
	add_format("change_not_enough_params", "%! Polecenie wymaga podania %wwszystkich%n parametr�w\n", 1);

	add_format("more", "-- Wci�nij Enter by kontynuowa� --", 1);
};

