/* $Id$ */

/*
 *  (C) Copyright 2001-2002 Wojtek Kaniewski <wojtekka@irc.pl>,
 *                          Piotr Wysocki <wysek@linux.bydg.org>
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
#include <unistd.h>
#ifndef _AIX
#  include <string.h>
#endif
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <errno.h>
#include "config.h"
#include "libgadu.h"
#include "stuff.h"
#include "events.h"
#include "commands.h"
#include "themes.h"
#include "userlist.h"
#include "voice.h"
#include "xmalloc.h"
#include "ui.h"

void handle_msg(), handle_ack(), handle_status(), handle_notify(),
	handle_success(), handle_failure();

static int hide_notavail = 0;	/* czy ma ukrywa� niedost�pnych -- tylko zaraz po po��czeniu */

static struct handler handlers[] = {
	{ GG_EVENT_MSG, handle_msg },
	{ GG_EVENT_ACK, handle_ack },
	{ GG_EVENT_STATUS, handle_status },
	{ GG_EVENT_NOTIFY, handle_notify },
	{ GG_EVENT_NOTIFY_DESCR, handle_notify },
	{ GG_EVENT_CONN_SUCCESS, handle_success },
	{ GG_EVENT_CONN_FAILED, handle_failure },
	{ GG_EVENT_DISCONNECT, handle_disconnect },
	{ 0, NULL }
};

/*
 * print_message()
 *
 * funkcja �adnie formatuje tre�� wiadomo�ci, zawija linijki, wy�wietla
 * kolorowe ramki i takie tam.
 *
 *  - e - zdarzenie wiadomo�ci,
 *  - u - wpis u�ytkownika w userli�cie,
 *  - chat - rodzaj wiadomo�ci (0 - msg, 1 - chat, 2 - sysmsg)
 *
 * nie zwraca niczego. efekt wida� na ekranie.
 */
void print_message(struct gg_event *e, struct userlist *u, int chat)
{
	int width, next_width, i, j, mem_width = 0;
	char *mesg, *buf, *line, *next, *format = NULL, *format_first = "", *next_format = NULL, *head = NULL, *foot = NULL, *save;
	char *line_width = NULL, timestr[100], *target;
	struct tm *tm;

	if (e->event.msg.recipients) {
		struct conference *c = conference_find_by_uins(e->event.msg.sender, 
			e->event.msg.recipients, e->event.msg.recipients_count);

		if (!c) {
			string_t tmp = string_init(NULL);
			int first = 0, i;

			for (i = 0; i < e->event.msg.recipients_count; i++) {
				if (first++) 
					string_append_c(tmp, ',');

			        string_append(tmp, itoa(e->event.msg.recipients[i]));
			}

			string_append_c(tmp, ' ');
			string_append(tmp, itoa(e->event.msg.sender));

			c = conference_create(tmp->str);

			string_free(tmp, 1);
		} 
		
		if (c)
			target = xstrdup(c->name);
		else
			target = xstrdup((chat == 2) ? "__status" : ((u) ? u->display : itoa(e->event.msg.sender)));
	} else
	        target = xstrdup((chat == 2) ? "__status" : ((u) ? u->display : itoa(e->event.msg.sender)));
	
	switch (chat) {
		case 0:
			format = "message_line";
			format_first = "message_line_first";
			line_width = "message_line_width";
			head = "message_header";
			foot = "message_footer";
			break;		
		case 1:
			format = "chat_line"; 
			format_first = "chat_line_first";
			line_width = "chat_line_width";
			head = "chat_header";
			foot = "chat_footer";
			break;
		case 2:
			format = "sysmsg_line"; 
			line_width = "sysmsg_line_width";
			head = "sysmsg_header";
			foot = "sysmsg_footer";
			break;
		case 3:
			format = "sent_line"; 
			format_first = "sent_line_first";
			line_width = "sent_line_width";
			head = "sent_header";
			foot = "sent_footer";
			break;
	}	

	/* je�eli chcemy, dodajemy do bufora ,,last'' wiadomo��... */
	if (config_last & 1 && (chat >= 0 && chat <= 2))
		       last_add(0, e->event.msg.sender, e->event.msg.time, e->event.msg.message);
	
	tm = localtime(&e->event.msg.time);
	strftime(timestr, sizeof(timestr), format_find((chat) ? "chat_timestamp" : "message_timestamp"), tm);

	if (!(width = atoi(format_find(line_width))))
		width = ui_screen_width - 2;

	if (width < 0)
		width = ui_screen_width + width;

	next_width = width;
	
	if (!strcmp(format_find(format_first), "")) {
		print_window(target, 1, head, format_user(e->event.msg.sender), timestr);
		next_format = format;
		mem_width = width + 1;
	} else {
		char *tmp, *p;

		next_format = format;
		format = format_first;

		/* zmniejsz d�ugo�� pierwszej linii o d�ugo�� prefiksu z rozm�wc�, timestampem itd. */
		tmp = format_string(format_find(format), "", format_user(e->event.msg.sender), timestr);
		mem_width = width + strlen(tmp);
		for (p = tmp; *p && *p != '\n'; p++) {
			if (*p == 27) {
				/* pomi� kolorki */
				while (*p && *p != 'm')
					p++;
			} else
				width--;
		}
		
		xfree(tmp);

		tmp = format_string(format_find(next_format), "", "", "");
		next_width -= strlen(tmp);
		xfree(tmp);
	}

	buf = xmalloc(mem_width);
	mesg = save = xstrdup(e->event.msg.message);

	for (i = 0; i < strlen(mesg); i++)	/* XXX �adniejsze taby */
		if (mesg[i] == '\t')
			mesg[i] = ' ';
	
	while ((line = gg_get_line(&mesg))) {
		char *new_line = NULL;

		if (config_emoticons && (new_line = emoticon_expand(line)))
			line = new_line;

#ifdef WITH_WAP
		{
			FILE *wap;
			char waptime[10], waptime2[10];
			const char *waplog;

			strftime(waptime2, sizeof(waptime2), "%H:%M", tm);

			sprintf(waptime, "wap%5s", waptime2);
			if ((waplog = prepare_path(waptime, 1))) {
				if ((wap = fopen(waplog, "a"))) {
					fprintf(wap,"%s(%s):%s\n", target, waptime2, line);
					fclose(wap);
				}
			}
		}
#endif

		for (; strlen(line); line = next) {
			if (strlen(line) <= width) {
				strcpy(buf, line);
				next = line + strlen(line);
			} else {
				int len = width;
				
				for (j = width; j; j--)
					if (line[j] == ' ') {
						len = j;
						break;
					}

				strncpy(buf, line, len);
				buf[len] = 0;
				next = line + len;

				while (*next == ' ')
					next++;
			}

			print_window(target, 1, format, buf, format_user(e->event.msg.sender), timestr);

			width = next_width;
			format = next_format;
		}

		if (new_line)
			free(new_line);
	}

	xfree(buf);
	xfree(save);

	if (!strcmp(format_find(format_first), ""))
		print_window(target, 1, foot);

	xfree(target);
}

/*
 * handle_msg()
 *
 * funkcja obs�uguje przychodz�ce wiadomo�ci.
 *
 *  - e - opis zdarzenia.
 *
 * nie zwraca niczego.
 */
void handle_msg(struct gg_event *e)
{
	struct userlist *u = userlist_find(e->event.msg.sender, NULL);
	int chat = ((e->event.msg.msgclass & 0x0f) == GG_CLASS_CHAT);
	char *tmp;
	
	if (!e->event.msg.message)
		return;
	
	if (ignored_check(e->event.msg.sender)) {
		if (config_log_ignored) {
			char *tmp;
			cp_to_iso(e->event.msg.message);
			tmp = log_escape(e->event.msg.message);
			/* XXX eskejpowanie */
			put_log(e->event.msg.sender, "%sign,%ld,%s,%ld,%ld,%s\n", (chat) ? "chatrecv" : "msgsend", e->event.msg.sender, (u) ? u->display : "", time(NULL), e->event.msg.time, tmp);
			xfree(tmp);
		}
		return;
	};

	if ((e->event.msg.msgclass & GG_CLASS_CTCP)) {
		gg_debug(GG_DEBUG_MISC, "// ekg: received ctcp\n");

		if (config_dcc && u) {
			struct gg_dcc *d;

                        if (!(d = gg_dcc_get_file(u->ip.s_addr, u->port, config_uin, e->event.msg.sender))) {
				print_status("dcc_error", strerror(errno));
				return;
			}

			list_add(&watches, d, 0);
		}

		return;
	}
	
	cp_to_iso(e->event.msg.message);

	event_check((chat) ? EVENT_CHAT : EVENT_MSG, e->event.msg.sender, e->event.msg.message);
	
	if (e->event.msg.sender == 0) {
		if (e->event.msg.msgclass > last_sysmsg) {
			print_message(e, u, 2);

			if (config_beep)
				ui_beep();
		    
			play_sound(config_sound_sysmsg_file);
			last_sysmsg = e->event.msg.msgclass;
			sysmsg_write();
		}

		return;
	};
			
	if (u)
		add_send_nick(u->display);
	else
		add_send_nick(itoa(e->event.msg.sender));

	print_message(e, u, chat);

	if (config_beep && ((chat) ? config_beep_chat : config_beep_msg))
		ui_beep();

	play_sound((chat) ? config_sound_chat_file : config_sound_msg_file);

	tmp = log_escape(e->event.msg.message);
	put_log(e->event.msg.sender, "%s,%ld,%s,%ld,%ld,%s\n", (chat) ? "chatrecv" : "msgrecv", e->event.msg.sender, (u) ? u->display : "", time(NULL), e->event.msg.time, tmp);
	xfree(tmp);

	if (away && config_sms_away && config_sms_app && config_sms_number) {
		char *foo, sender[100];

		if (u)
			snprintf(sender, sizeof(sender), "%s/%u", u->display, u->uin);
		else
			snprintf(sender, sizeof(sender), "%u", e->event.msg.sender);

		if (strlen(e->event.msg.message) > config_sms_max_length)
			e->event.msg.message[config_sms_max_length] = 0;

		foo = format_string(format_find((chat) ? "sms_chat" : "sms_msg"), sender, e->event.msg.message);

		/* niech nie wysy�a sms�w, je�li brakuje format�w */
		if (strcmp(foo, ""))
			send_sms(config_sms_number, foo, 0);

		free(foo);
	}

	if (e->event.msg.formats_length > 0) {
		int i;
		
		gg_debug(GG_DEBUG_MISC, "[ekg received formatting info (len=%d):", e->event.msg.formats_length);
		for (i = 0; i < e->event.msg.formats_length; i++)
			gg_debug(GG_DEBUG_MISC, " %.2x", ((unsigned char*)e->event.msg.formats)[i]);
		gg_debug(GG_DEBUG_MISC, "]\n");
	}
}

/*
 * handle_ack()
 *
 * funkcja obs�uguje potwierdzenia wiadomo�ci.
 *
 *  - e - opis zdarzenia.
 *
 * nie zwraca niczego.
 */
void handle_ack(struct gg_event *e)
{
	char *tmp;
	int queued = (e->event.ack.status == GG_ACK_QUEUED);

	if (!e->event.ack.seq)	/* ignorujemy potwierdzenia ctcp */
		return;

	if (!config_display_ack)
		return;

	if (config_display_ack == 2 && queued)
		return;

	if (config_display_ack == 3 && !queued)
		return;

	tmp = queued ? "ack_queued" : "ack_delivered";
	print(tmp, format_user(e->event.ack.recipient));
}

/*
 * handle_common()
 *
 * ujednolicona obs�uga zmiany w userli�cie dla handle_status()
 * i handle_notify(). utrzymywanie tego samego kodu w dw�ch miejscach
 * jest kompletnie bez sensu.
 *  
 *  - uin - numer delikwenta,
 *  - status - nowy stan,
 *  - descr - nowy opis,
 *  - n - dodatki od gg_notify.
 */
static void handle_common(uin_t uin, int status, const char *descr, struct gg_notify_reply *n)
{
	struct userlist *u;
	struct status_table {
		int status;
		int event;
		char *log;
		char *format;
	};
	struct status_table st[] = {
		{ GG_STATUS_AVAIL, EVENT_AVAIL, "avail", "status_avail" },
		{ GG_STATUS_AVAIL_DESCR, EVENT_AVAIL, "avail", "status_avail_descr" },
		{ GG_STATUS_BUSY, EVENT_AWAY, "busy", "status_busy" },
		{ GG_STATUS_BUSY_DESCR, EVENT_AWAY, "busy", "status_busy_descr" },
		{ GG_STATUS_NOT_AVAIL, EVENT_NOT_AVAIL, "notavail", "status_not_avail" },
		{ GG_STATUS_NOT_AVAIL_DESCR, EVENT_NOT_AVAIL, "notavail", "status_not_avail_descr" },
		{ 0, 0, NULL, NULL },
	};
	struct status_table *s;
	int prev_status;

	/* je�li ignorujemy, nie wy�wietlaj */
	if (ignored_check(uin))
		return;
	
	/* nie pokazujemy nieznajomych */
	if (!(u = userlist_find(uin, NULL)))
		return;

	/* zapami�taj adres i port */
	if (n) {
		u->port = n->remote_port;
		u->ip.s_addr = n->remote_ip;
	}

	/* je�li status taki sam i ewentualnie opisy te same, ignoruj */
	if (!GG_S_D(status) && (u->status == status))
		return;
	if (GG_S_D(status) && (u->status == status) && u->descr && descr && !strcmp(u->descr, descr))
		return;

	/* usu� poprzedni opis */
	free(u->descr);
	u->descr = NULL;

	/* je�li stan z opisem, a opisu brak, wpisz pusty tekst */
	if (GG_S_D(status) && !descr)
		u->descr = xstrdup("");

	/* a je�li jest opis, to go zapami�taj */
	if (descr) {
		u->descr = xstrdup(descr);
		cp_to_iso(u->descr);
	}

	/* zapami�taj stary stan, ustaw nowy */
	prev_status = u->status;	
	u->status = status;

	for (s = st; s->status; s++) {
		/* je�li nie ten, sprawdzaj dalej */
		if (status != s->status)
			continue;

		/* je�li nie jest opisowy i taki sam, ignoruj */
		if (!GG_S_D(s->status) && prev_status == s->status)
			break;

		/* eventy */
	    	event_check(s->event, uin, NULL);

		/* zaloguj */
		if (config_log_status && !GG_S_D(s->status))
			put_log(uin, "status,%ld,%s,%s,%ld,%s\n", uin, u->display, inet_ntoa(u->ip), time(NULL), s->log);
		if (config_log_status && GG_S_D(s->status) && u->descr)
		    	put_log(uin, "status,%ld,%s,%s,%ld,%s (%s)\n", uin, u->display, inet_ntoa(u->ip), time(NULL), s->log, u->descr);

		/* jak dost�pny lub zaj�ty, dopiszmy do taba
		 * jak niedost�pny, usu�my */
		if (GG_S_A(s->status) && config_completion_notify) 
			add_send_nick(u->display);
		if (GG_S_B(s->status) && (config_completion_notify & 4))
			add_send_nick(u->display);
		if (GG_S_NA(s->status) && (config_completion_notify & 2))
			remove_send_nick(u->display);
		
		/* czy mamy wy�wietla� na ekranie? */
		if (!config_display_notify)
			break;
		
		if (config_display_notify == 2) {
			/* je�li na zaj�ty, ignorujemy */
			if (GG_S_B(s->status) && !GG_S_NA(prev_status))
				break;

			/* je�li na dost�pny i nie by� niedost�pny, ignoruj */
			if (GG_S_A(s->status) && !GG_S_NA(prev_status))
				break;
		}

		/* czy ukrywa� niedost�pnych */
		if (hide_notavail) {
			if (GG_S_NA(s->status))
				break;
			else
				hide_notavail = 0;
		}
			
		/* no dobra, poka� */
		print_window(u->display, 0, s->format, format_user(uin), (u->first_name) ? u->first_name : u->display, u->descr);

		/* daj zna� d�wi�kiem */
		if (config_beep && config_beep_notify)
			ui_beep();

		break;
	}
}

/*
 * handle_notify()
 *
 * funkcja obs�uguje list� obecnych.
 *
 *  - e - opis zdarzenia.
 */
void handle_notify(struct gg_event *e)
{
	struct gg_notify_reply *n;

	if (batch_mode)
		return;

	n = (e->type == GG_EVENT_NOTIFY) ? e->event.notify : e->event.notify_descr.notify;

	for (; n->uin; n++) {
		char *descr = (e->type == GG_EVENT_NOTIFY_DESCR) ? e->event.notify_descr.descr : NULL;
		
		handle_common(n->uin, n->status, descr, n);
	}
}

/*
 * handle_status()
 *
 * funkcja obs�uguje zmian� stanu ludzi z listy kontakt�w.
 *
 *  - e - opis zdarzenia.
 */
void handle_status(struct gg_event *e)
{
	if (batch_mode)
		return;

	handle_common(e->event.status.uin, e->event.status.status, e->event.status.descr, NULL);
}

/*
 * handle_failure()
 *
 * funkcja obs�uguje b��dy przy po��czeniu.
 *
 *  - e - opis zdarzenia.
 */
void handle_failure(struct gg_event *e)
{
	struct { int type; char *str; } reason[] = {
		{ GG_FAILURE_RESOLVING, "conn_failed_resolving" },
		{ GG_FAILURE_CONNECTING, "conn_failed_connecting" },
		{ GG_FAILURE_INVALID, "conn_failed_invalid" },
		{ GG_FAILURE_READING, "conn_failed_disconnected" },
		{ GG_FAILURE_WRITING, "conn_failed_disconnected" },
		{ GG_FAILURE_PASSWORD, "conn_failed_password" },
		{ GG_FAILURE_404, "conn_failed_404" },
		{ 0, NULL },
	};

	char *tmp = NULL;
	int i;

	for (i = 0; reason[i].type; i++) {
		if (reason[i].type == e->event.failure) {
			tmp = format_string(format_find(reason[i].str));
			break;
		}
	}

	print("conn_failed", (tmp) ? tmp : "?");
	xfree(tmp);

	list_remove(&watches, sess, 0);
	gg_logoff(sess);
	gg_free_session(sess);
	userlist_clear_status();
	sess = NULL;
	do_reconnect();
}

/*
 * handle_success()
 *
 * funkcja obs�uguje udane po��czenia.
 *
 *  - e - opis zdarzenia.
 */
void handle_success(struct gg_event *e)
{
	print("connected");
	ui_event("connected");

	userlist_send();

	/* je�li mieli�my zachowany stan i/lub opis, zr�b z niego u�ytek */
	if (away || private_mode) {
		if (!config_reason || !GG_S_D(config_status)) 
			gg_change_status(sess, config_status);
		else
			gg_change_status_descr(sess, config_status, config_reason);
	}

	if (batch_mode && batch_line) {
 		command_exec(NULL, batch_line);
 		free(batch_line);
 		batch_line = NULL;
 	}

	hide_notavail = 1;
}

/*
 * handle_event()
 *
 * funkcja obs�uguje wszystkie zdarzenia dotycz�ce danej sesji GG.
 *
 *  - e - opis zdarzenia.
 *
 * nie zwraca niczego.
 */
void handle_event(struct gg_session *s)
{
	struct gg_event *e;
	struct handler *h;

	if (!(e = gg_watch_fd(sess))) {
		print("conn_broken");
		list_remove(&watches, sess, 0);
		gg_free_session(sess);
		sess = NULL;
		ui_event("disconnected");
		do_reconnect();

		return;
	}

	for (h = handlers; h->type; h++)
		if (h->type == e->type)
			(h->handler)(e);

	gg_event_free(e);
}

/*
 * handle_search()
 *
 * funkcja obs�uguje zdarzenia dotycz�ce wyszukiwania u�ytkownik�w.
 *
 *  - e - opis zdarzenia.
 *
 * nie zwraca niczego.
 */
void handle_search(struct gg_http *h)
{
	struct gg_search_request *r;
	struct gg_search *s = NULL;
	int i;

	if (gg_search_watch_fd(h) || h->state == GG_STATE_ERROR) {
		print("search_failed", strerror(errno));
		free(h->user_data);
		list_remove(&watches, h, 0);
		gg_search_request_free((struct gg_search_request*) h->user_data);
		gg_search_free(h);
		return;
	}
	
	if (h->state != GG_STATE_DONE)
		return;

	gg_debug(GG_DEBUG_MISC, "++ gg_search()... done\n");

	if (!h || !(s = h->data) || !(s->count))
		print("search_not_found");

	for (i = 0; i < s->count; i++) {
		const char *active_format, *gender_format;
		char *name, *active, *gender;

		cp_to_iso(s->results[i].first_name);
		cp_to_iso(s->results[i].last_name);
		cp_to_iso(s->results[i].nickname);
		cp_to_iso(s->results[i].city);

		name = saprintf("%s %s", s->results[i].first_name, s->results[i].last_name);

		if (!(h->id & 1)) {
			switch (s->results[i].active) {
				case GG_STATUS_AVAIL:
					active_format = format_find("search_results_single_active");
					break;

				case GG_STATUS_BUSY:
					active_format = format_find("search_results_single_busy");
					break;
				default:
					active_format = format_find("search_results_single_inactive");
			}

			if (s->results[i].gender == GG_GENDER_FEMALE)
				gender_format = format_find("search_results_single_female");
			else if (s->results[i].gender == GG_GENDER_MALE)
				gender_format = format_find("search_results_single_male");
			else
				gender_format = format_find("search_results_single_male");
		} else {
			switch (s->results[i].active) {
				case GG_STATUS_AVAIL:
					active_format = format_find("search_results_multi_active");
					break;

				case GG_STATUS_BUSY:
					active_format = format_find("search_results_multi_busy");
					break;
				default:
					active_format = format_find("search_results_multi_inactive");
			}

			if (s->results[i].gender == GG_GENDER_FEMALE)
				gender_format = format_find("search_results_multi_female");
			else if (s->results[i].gender == GG_GENDER_MALE)
				gender_format = format_find("search_results_multi_male");
			else
				gender_format = format_find("search_results_multi_unknown");
		}

		active = format_string(active_format, s->results[i].nickname);
		gender = format_string(gender_format);

		print((h->id & 1) ? "search_results_multi" : "search_results_single", itoa(s->results[i].uin), (name) ? name : "", s->results[i].nickname, s->results[i].city, (s->results[i].born) ? itoa(s->results[i].born) : "-", gender, active);

		free(name);
		free(active);
		free(gender);
	}

	r = (void*) h->user_data;

	gg_debug(GG_DEBUG_MISC, "s->count = %d, start = 0x%.8x\n", s->count, r->start);
	if (s->count > 19 && (r->start & 0x80000000L)) {
		struct gg_http *h2;
		list_t l;
		int id = 1;
		
		r->start = s->results[19].uin | 0x80000000L;
		list_remove(&watches, h, 0);
		gg_free_search(h);
		
		for (l = watches; l; l = l->next)
		{
			struct gg_http *h3 = l->data;

			if (h3->type != GG_SESSION_SEARCH)
				continue;

			if (h3->id / 2 >= id)
				id = h3->id / 2 + 1;
		}
		if (!(h2 = gg_search(r, 1)))
		{
			print("search_failed", strerror(errno));
			gg_search_request_free(r);
			return;
		}
		h2->id = id;
		h2->user_data = (char*) r;
		list_add(&watches, h2, 0);
		return;
	}
	
	list_remove(&watches, h, 0);
	gg_search_request_free(r);
	gg_search_free(h);
}

/*
 * handle_pubdir()
 *
 * funkcja zajmuj�ca si� wszelkimi zdarzeniami http opr�cz szukania.
 *
 *  - h - delikwent.
 *
 * nie zwraca niczego.
 */
void handle_pubdir(struct gg_http *h)
{
	struct gg_pubdir *s = NULL;
	char *good = "", *bad = "";

	if (!h)
		return;
	
	switch (h->type) {
		case GG_SESSION_REGISTER:
			good = "register";
			bad = "register_failed";
			break;
		case GG_SESSION_PASSWD:
			good = "passwd";
			bad = "passwd_failed";
			break;
		case GG_SESSION_REMIND:
			good = "remind";
			bad = "remind_failed";
			break;
		case GG_SESSION_CHANGE:
			good = "change";
			bad = "change_failed";
			break;
	}

	if (gg_pubdir_watch_fd(h) || h->state == GG_STATE_ERROR) {
		print(bad, strerror(errno));
		goto fail;
	}
	
	if (h->state != GG_STATE_DONE)
		return;

	if (!(s = h->data) || !s->success) {
		print(bad, strerror(errno));
		goto fail;
	}

	if (h->type == GG_SESSION_PASSWD) {
		config_password = reg_password;
		reg_password = NULL;
	}

	if (h->type == GG_SESSION_REGISTER) {
		if (!s->uin) {
			print(bad);
			goto fail;
		}
		
		if (!config_uin && !config_password && reg_password) {
			config_uin = s->uin;
			config_password = reg_password;
			reg_password = NULL;
		}

		registered_today = 1;
	}
	
	print(good, itoa(s->uin));

fail:
	list_remove(&watches, h, 0);
	if (h->type == GG_SESSION_REGISTER || h->type == GG_SESSION_PASSWD) {
		free(reg_password);
		reg_password = NULL;
	}
	gg_free_pubdir(h);
}

/*
 * handle_userlist()
 *
 * funkcja zajmuj�ca si� zdarzeniami userlisty.
 *
 *  - h - delikwent.
 *
 * nie zwraca niczego.
 */
void handle_userlist(struct gg_http *h)
{
	char *format_ok, *format_error;
	
	if (!h)
		return;
	
	format_ok = (h->type == GG_SESSION_USERLIST_GET) ? "userlist_get_ok" : "userlist_put_ok";
	format_error = (h->type == GG_SESSION_USERLIST_GET) ? "userlist_get_error" : "userlist_put_error";

	if (h->callback(h) || h->state == GG_STATE_ERROR) {
		print(format_error, strerror(errno));
		list_remove(&watches, h, 0);
		h->destroy(h);
		return;
	}
	
	if (h->state != GG_STATE_DONE)
		return;

	print((h->data) ? format_ok : format_error);
		
	if (h->type == GG_SESSION_USERLIST_GET && h->data) {
		cp_to_iso(h->data);
		userlist_set(h->data);
		userlist_send();
		config_changed = 1;
	}

	list_remove(&watches, h, 0);
	h->destroy(h);
}

/*
 * handle_disconnect()
 *
 * funkcja obs�uguje ostrze�enie o roz��czeniu.
 *
 *  - e - opis zdarzenia.
 *
 * nie zwraca niczego.
 */
void handle_disconnect(struct gg_event *e)
{
	print("conn_broken");
	ui_event("disconnected");

	gg_logoff(sess);	/* a zobacz.. mo�e si� uda ;> */
	list_remove(&watches, sess, 0);
	userlist_clear_status();
	gg_free_session(sess);
	sess = NULL;	
	do_reconnect();
}

/*
 * find_transfer()
 *
 * znajduje struktur� ,,transfer'' dotycz�c� danego po��czenia.
 *
 *  - d - struktura gg_dcc, kt�rej szukamy.
 *
 * wska�nik do struktury ,,transfer'' lub NULL, je�li nie znalaz�.
 */
static struct transfer *find_transfer(struct gg_dcc *d)
{
	list_t l;

	for (l = transfers; l; l = l->next) {
		struct transfer *t = l->data;

		if (t->dcc == d)
			return t;
	}

	return NULL;
}

/*
 * remove_transfer()
 *
 * usuwa z listy transfer�w ten, kt�ry dotyczy podanego po��czenia dcc.
 *
 *  - d - po��czenie.
 *
 * nie zwraca nic.
 */
static void remove_transfer(struct gg_dcc *d)
{
	struct transfer *t = find_transfer(d);

	if (t) {
		free(t->filename);
		list_remove(&transfers, t, 1);
	}
}

/*
 * handle_dcc()
 *
 * funkcja zajmuje si� obs�ug� wszystkich zdarze� zwi�zanych z DCC.
 *
 *  - d - struktura danego po��czenia.
 *
 * nie zwraca niczego.
 */
void handle_dcc(struct gg_dcc *d)
{
	struct gg_event *e;
	struct transfer *t, tt;
	list_t l;
	char *p;

	event_check(EVENT_DCC, d->peer_uin, NULL);
	
	if (!(e = gg_dcc_watch_fd(d))) {
		print("dcc_error", strerror(errno));
		if (d->type != GG_SESSION_DCC_SOCKET) {
			remove_transfer(d);
			list_remove(&watches, d, 0);
			gg_free_dcc(d);
		}
		return;
	}

	switch (e->type) {
		case GG_EVENT_DCC_NEW:
			gg_debug(GG_DEBUG_MISC, "## GG_EVENT_DCC_CLIENT_NEW\n");
			list_add(&watches, e->event.dcc_new, 0);
			e->event.dcc_new = NULL;
			break;

		case GG_EVENT_DCC_CLIENT_ACCEPT:
			gg_debug(GG_DEBUG_MISC, "## GG_EVENT_DCC_CLIENT_ACCEPT\n");
			
			if (!userlist_find(d->peer_uin, NULL) || config_uin != d->uin) {
				gg_debug(GG_DEBUG_MISC, "## unauthorized client (uin=%ld), closing connection\n", d->peer_uin);
				list_remove(&watches, d, 0);
				gg_free_dcc(d);
				return;
			}
			break;	

		case GG_EVENT_DCC_CALLBACK:
		{
			int found = 0;
			
			gg_debug(GG_DEBUG_MISC, "## GG_EVENT_DCC_CALLBACK\n");
			
			for (l = transfers; l; l = l->next) {
				struct transfer *t = l->data;

				gg_debug(GG_DEBUG_MISC, "// transfer id=%d, uin=%d, type=%d\n", t->id, t->uin, t->type);

				if (t->uin == d->peer_uin && !t->dcc) {
					gg_debug(GG_DEBUG_MISC, "## found transfer, uin=%d, type=%d\n", d->peer_uin, t->type);
					t->dcc = d;
					gg_dcc_set_type(d, t->type);
					found = 1;
					break;
				}
			}
			
			if (!found) {
				gg_debug(GG_DEBUG_MISC, "## connection from %d not found\n", d->peer_uin);
				list_remove(&watches, d, 0);
				gg_dcc_free(d);
			}
			
			break;	
		}

		case GG_EVENT_DCC_NEED_FILE_INFO:
			gg_debug(GG_DEBUG_MISC, "## GG_EVENT_DCC_NEED_FILE_INFO\n");

			for (l = transfers; l; l = l->next) {
				struct transfer *t = l->data;

				if (t->dcc == d) {
					if (gg_dcc_fill_file_info(d, t->filename) == -1) {
						gg_debug(GG_DEBUG_MISC, "## gg_dcc_fill_file_info() failed (%s)\n", strerror(errno));
						print("dcc_open_error", t->filename);
						remove_transfer(d);
						list_remove(&watches, d, 0);
						gg_free_dcc(d);
						break;
					}
					break;
				}
			}
			break;
			
		case GG_EVENT_DCC_NEED_FILE_ACK:
			gg_debug(GG_DEBUG_MISC, "## GG_EVENT_DCC_NEED_FILE_ACK\n");
			/* �eby nie sprawdza�o, p�ki luser nie odpowie */
			list_remove(&watches, d, 0);

			if (!(t = find_transfer(d))) {
				tt.uin = d->peer_uin;
				tt.type = GG_SESSION_DCC_GET;
				tt.filename = NULL;
				tt.dcc = d;
				tt.id = transfer_id();
				if (!(t = list_add(&transfers, &tt, sizeof(tt)))) {
					gg_free_dcc(d);
					break;
				}
			}
			
			t->type = GG_SESSION_DCC_GET;
			t->filename = xstrdup(d->file_info.filename);

			for (p = d->file_info.filename; *p; p++)
				if (*p < 32 || *p == '\\' || *p == '/')
					*p = '_';

			print("dcc_get_offer", format_user(t->uin), t->filename, itoa(d->file_info.size), itoa(t->id));

			break;
			
		case GG_EVENT_DCC_NEED_VOICE_ACK:
			gg_debug(GG_DEBUG_MISC, "## GG_EVENT_DCC_NEED_VOICE_ACK\n");
#ifdef HAVE_VOIP
			/* �eby nie sprawdza�o, p�ki luser nie odpowie */
			list_remove(&watches, d, 0);

			if (!(t = find_transfer(d))) {
				tt.uin = d->peer_uin;
				tt.type = GG_SESSION_DCC_VOICE;
				tt.filename = NULL;
				tt.dcc = d;
				tt.id = transfer_id();
				if (!(t = list_add(&transfers, &tt, sizeof(tt)))) {
					gg_free_dcc(d);
					break;
				}
			}
			
			t->type = GG_SESSION_DCC_VOICE;

			print("dcc_voice_offer", format_user(t->uin), itoa(t->id));
#else
			list_remove(&watches, d, 0);
			remove_transfer(d);
			gg_free_dcc(d);
#endif
			break;

		case GG_EVENT_DCC_VOICE_DATA:
			gg_debug(GG_DEBUG_MISC, "## GG_EVENT_DCC_VOICE_DATA\n");

#ifdef HAVE_VOIP
			voice_open();
			voice_play(e->event.dcc_voice_data.data, e->event.dcc_voice_data.length, 0);
#endif
			break;
			
		case GG_EVENT_DCC_DONE:
			gg_debug(GG_DEBUG_MISC, "## GG_EVENT_DCC_DONE\n");

			if (!(t = find_transfer(d))) {
				gg_free_dcc(d);
				break;
			}

			print((t->dcc->type == GG_SESSION_DCC_SEND) ? "dcc_done_send" : "dcc_done_get", format_user(t->uin), t->filename);
			
			remove_transfer(d);
			list_remove(&watches, d, 0);
			gg_free_dcc(d);

			break;
			
		case GG_EVENT_DCC_ERROR:
			switch (e->event.dcc_error) {
				case GG_ERROR_DCC_HANDSHAKE:
					print("dcc_error_handshake", format_user(d->peer_uin));
					break;
				case GG_ERROR_DCC_NET:
					print("dcc_error_network", format_user(d->peer_uin));
					break;
				case GG_ERROR_DCC_REFUSED:
					print("dcc_error_refused", format_user(d->peer_uin));
					break;
				default:
					print("dcc_error_unknown", "");
			}

#ifdef HAVE_VOIP
			if (d->type == GG_SESSION_DCC_VOICE)
				voice_close();
#endif  /* HAVE_VOIP */

			remove_transfer(d);
			list_remove(&watches, d, 0);
			gg_free_dcc(d);

			break;
	}

	gg_event_free(e);
	
	return;
}

/*
 * handle_voice()
 *
 * obs�uga danych przychodz�cych z urz�dzenia wej�ciowego.
 *
 * brak.
 */
void handle_voice()
{
#ifdef HAVE_VOIP
	list_t l;
	struct gg_dcc *d = NULL;
	char buf[GG_DCC_VOICE_FRAME_LENGTH];
	
	for (l = transfers; l; l = l->next) {
		struct transfer *t = l->data;

		if (t->type == GG_SESSION_DCC_VOICE && t->dcc && (t->dcc->state == GG_STATE_READING_VOICE_HEADER || t->dcc->state == GG_STATE_READING_VOICE_SIZE || t->dcc->state == GG_STATE_READING_VOICE_DATA)) {
			d = t->dcc;
			break;
		}
	}

	/* p�ki nie mamy po��czenia, i tak czytamy z /dev/dsp */

	if (!d) {
		voice_record(buf, GG_DCC_VOICE_FRAME_LENGTH, 1);	/* XXX b��dy */
		return;
	} else {
		voice_record(buf, GG_DCC_VOICE_FRAME_LENGTH, 0);	/* XXX b��dy */
		gg_dcc_voice_send(d, buf, GG_DCC_VOICE_FRAME_LENGTH);	/* XXX b��dy */
	}
#endif /* HAVE_VOIP */
}
