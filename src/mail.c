/* $Id$ */

/*
 *  (C) Copyright 2001-2002 Piotr Domagalski <szalik@szalik.net>
 *                          Pawe� Maziarz <drg@infomex.pl>
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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pwd.h>
#include <dirent.h>
#include <fcntl.h>
#ifndef _AIX
#  include <string.h>
#endif
#include <errno.h>
#include "compat.h"
#include "dynstuff.h"
#include "mail.h"
#include "stuff.h"
#include "themes.h"
#include "ui.h"
#include "xmalloc.h"

int config_check_mail = 0;
int config_check_mail_frequency = 15;
char *config_check_mail_folders = NULL;

int mail_count = 0;
int last_mail_count = 0;

list_t mail_folders = NULL;

/*
 * check_mail()
 *
 * wywo�uje odpowiednie sprawdzanie poczty.
 */
int check_mail()
{
	if (!config_check_mail)
		return -1;

	if (config_check_mail & 1)
		check_mail_mbox();
	else
		if (config_check_mail & 2)
			check_mail_maildir();

	return 0;
}

/*
 * check_mail_update()
 *
 * modyfikuje liczb� nowych emaili i daje o tym zna�.
 */
int check_mail_update(int update)
{
	if (update == mail_count)
		return -1;

	last_mail_count = mail_count;
	mail_count = update;

	if (mail_count && mail_count > last_mail_count) {
		if (config_check_mail & 4) {
			if (mail_count == 1)
				print("new_mail_one", itoa(mail_count));
			else {
				if (mail_count >= 2 && mail_count <= 4)
					print("new_mail_two_four", itoa(mail_count));
				else
					print("new_mail_more", itoa(mail_count));
			}
		}

		if (config_beep && config_beep_mail)
			ui_beep();
	}

	return 0;
}

/*
 * check_mail_mbox()
 *
 * tworzy dzieciaka, kt�ry sprawdza wszystkie pliki typu
 * mbox i liczy ile jest nowych wiadomo�ci, potem zwraca
 * wynik rurk�. sprawdza tylko te pliki, kt�re by�y
 * modyfikowane od czasu ostatniego sprawdzania.
 */
int check_mail_mbox()
{
	int fd[2], pid, cont = 0;
	struct gg_exec x;
	list_t l;

	for (l = mail_folders; l; l = l->next) {
		struct mail_folder *m = l->data;
		struct stat st;

		if (stat(m->fname, &st) == -1) {
			cont = 1;
			continue;
		}

		if ((st.st_mtime != m->mtime) || (st.st_size != m->size)) {
			m->mtime = st.st_mtime;
			m->size = st.st_size;
			cont = 1;
		}
	}

	if (!cont || pipe(fd))
		return 1;

 	if ((pid = fork()) < 0) {
		close(fd[0]);
		close(fd[1]);
		return 1;
	}

	if (!pid) {	/* born to be wild */
		char *str_new = NULL, *line = NULL;
		int f_new = 0, new = 0, in_header = 0;
		FILE *f;
		struct stat st;
		struct timeval foo[1];

		close(fd[0]);

		for (l = mail_folders; l; l = l->next) {
			struct mail_folder *m = l->data;

			if ((stat(m->fname, &st) == -1) || !(f = fopen(m->fname, "r")))
				continue;

			while ((line = read_file(f))) {
				if (!strncmp(line, "From ", 5)) {
					in_header = 1;
					f_new++;
				}

				if (in_header && (!strncmp(line, "Status: RO", 10) || !strncmp(line, "Status: O", 9))) 
					f_new--;	

				strip_spaces(line);

				if (strlen(line) == 0)
					in_header = 0;

				xfree(line);
			}

			fclose(f);

#ifdef HAVE_UTIMES
			/* przecie� my nic nie ruszali�my ;> */			
			foo[0].tv_sec = st.st_atime;
			foo[1].tv_sec = st.st_mtime;
			utimes(m->fname, (const struct timeval *) &foo);
#endif

			new += f_new;
			f_new = 0;
		}

		str_new = saprintf("%d", new);

		write(fd[1], str_new, sizeof(str_new));
		close(fd[1]);

		xfree(str_new);

		exit(0);
	}

	x.fd = fd[0];
	x.check = GG_CHECK_READ;
	x.state = GG_STATE_READING_DATA;
	x.type = GG_SESSION_USER4;
	x.id = pid;
	x.timeout = 60;
	x.buf = string_init(NULL);

	fcntl(x.fd, F_SETFL, O_NONBLOCK);

	list_add(&watches, &x, sizeof(x));
	process_add(pid, "\002");

	close(fd[1]);

	return 0;
}

/*
 * check_mail_maildir()
 *
 * tworzy dzieciaka, kt�ry sprawdza wszystkie 
 * katalogi typu Maildir i liczy, ile jest w nich
 * nowych wiadomo�ci. zwraca wynik rurk�.
 */
int check_mail_maildir()
{
	int fd[2], pid;
	struct gg_exec x;

	if (pipe(fd))
		return 1;

	if ((pid = fork()) < 0) {
		close(fd[0]);
		close(fd[1]);
		return 1;
	}

	if (!pid) {	/* born to be wild */
		int d_new = 0, new = 0;
		char *str_new = NULL;
		struct dirent *d;
		DIR *dir;
		list_t l;

		close(fd[0]);

		for (l = mail_folders; l; l = l->next) {
			struct mail_folder *m = l->data;
			char *tmp = saprintf("%s/%s", m->fname, "new");

			if (!(dir = opendir(tmp))) {
				xfree(tmp);
				continue;
			}

			while ((d = readdir(dir))) {
				char *fname = saprintf("%s/%s", tmp, d->d_name);
				struct stat st;

				if (!stat(fname, &st) && S_ISREG(st.st_mode))
					d_new++;

				xfree(fname);
			}
	
			xfree(tmp);
			closedir(dir);

			new += d_new;
			d_new = 0;
		}

		str_new = saprintf("%d", new);

		write(fd[1], str_new, sizeof(str_new));
		close(fd[1]);

		xfree(str_new);

		exit(0);
	}

	x.fd = fd[0];
	x.check = GG_CHECK_READ;
	x.state = GG_STATE_READING_DATA;
	x.type = GG_SESSION_USER4;
	x.id = pid;
	x.timeout = 60;
	x.buf = string_init(NULL);

	fcntl(x.fd, F_SETFL, O_NONBLOCK);

	list_add(&watches, &x, sizeof(x));
	process_add(pid, "\002");

	close(fd[1]);

	return 0;
}

/*
 * changed_check_mail()
 *
 * wywo�ywane przy zmianie zmiennej check_mail.
 */
void changed_check_mail(const char *var)
{
	if (config_check_mail) {
		list_t l;
		struct timer *t;

		/* konieczne, je�li by�a zmiana typu skrzynek */
		changed_check_mail_folders("check_mail_folders");

		for (l = timers; l; l=l->next ) {
			t = l->data;

			if (!strcmp(t->name, "check-mail-time")) {
				t->period = config_check_mail_frequency;
				return;
			}
		}
		 
		t = timer_add(config_check_mail_frequency, "check-mail-time", "check_mail");
		t->ui = 1;
	} else
		timer_remove("check-mail-time", NULL);
}

/*
 * changed_check_mail_folders()
 *
 * wywo�ywane przy zmianie check_mail_folders.
 */
void changed_check_mail_folders(const char *var)
{
	struct mail_folder foo;

	check_mail_free();

	if (config_check_mail_folders) {
		char **f = NULL;
		int i;
		
		f = array_make(config_check_mail_folders, ", ", 0, 1, 0);

		for (i = 0; f[i]; i++) {
			if (f[i][0] != '/') {
				char *buf = saprintf("%s/%s", home_dir, f[i]);
				
				xfree(f[i]);
				f[i] = buf;
			}

			foo.fname = f[i];
			foo.mtime = 0;
			foo.size = 0;
			foo.check = 1;

			list_add(&mail_folders, &foo, sizeof(foo));
		}

		xfree(f);
	}

	if (config_check_mail & 1) {
		char *inbox = xstrdup(getenv("MAIL"));

		if (!inbox) {
			struct passwd *pw = getpwuid(getuid());

			if (!pw)
				return;

			inbox = saprintf("%s/%s", "/var/mail", pw->pw_name);
		}

		foo.fname = inbox;
		foo.mtime = 0;
		foo.size = 0;
		foo.check = 1;

		list_add(&mail_folders, &foo, sizeof(foo));
	} else
		if (config_check_mail & 2) {
			char *inbox = saprintf("%s/Maildir", home_dir);
			
			foo.fname = inbox;
			foo.mtime = 0;
			foo.size = 0;
			foo.check = 1;

			list_add(&mail_folders, &foo, sizeof(foo));
		}
}

/*
 * check_mail_free()
 * 
 * czy�ci pami�� po li�cie folder�w z poczt�.
 */
void check_mail_free()
{
	if (mail_folders) {
		list_t l;

		for (l = mail_folders; l; l = l->next) {
			struct mail_folder *m = l->data;

			xfree(m->fname);
		}

		list_destroy(mail_folders, 1);
		mail_folders = NULL;
	}
}
