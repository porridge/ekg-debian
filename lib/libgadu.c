/* $Id$ */

/*
 *  (C) Copyright 2001-2002 Wojtek Kaniewski <wojtekka@irc.pl>,
 *                          Robert J. Wo�ny <speedy@ziew.org>,
 *                          Arkadiusz Mi�kiewicz <misiek@pld.org.pl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License Version
 *  2.1 as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netdb.h>
#include <errno.h>
#ifndef _AIX
#  include <string.h>
#endif
#include <stdarg.h>
#include <pwd.h>
#include <time.h>
#ifdef sun
  #include <sys/filio.h>
#endif
#include "libgadu.h"
#include "config.h"

int gg_debug_level = 0;

int gg_dcc_port = 0;
unsigned long gg_dcc_ip = 0;

/*
 *  zmienne opisuj�ce parametry proxy http.
 */
char *gg_proxy_host = NULL;
int gg_proxy_port = 0;
int gg_proxy_enabled = 0;

#ifndef lint 
static char rcsid[]
#ifdef __GNUC__
__attribute__ ((unused))
#endif
= "$Id$";
#endif 

/*
 * fix32() // funkcja wewn�trzna
 *
 * dla maszyn big-endianowych zamienia kolejno�� bajt�w w ,,long''ach.
 */
unsigned long fix32(unsigned long x)
{
#ifndef WORDS_BIGENDIAN
	return x;
#else
	return (unsigned long)
		(((x & (unsigned long) 0x000000ffU) << 24) |
                 ((x & (unsigned long) 0x0000ff00U) << 8) |
                 ((x & (unsigned long) 0x00ff0000U) >> 8) |
                 ((x & (unsigned long) 0xff000000U) >> 24));
#endif		
}

/*
 * fix16() // funkcja wewn�trzna
 *
 * dla maszyn big-endianowych zamienia kolejno�� bajt�w w ,,short''ach.
 */
unsigned short fix16(unsigned short x)
{
#ifndef WORDS_BIGENDIAN
	return x;
#else
	return (unsigned short)
		(((x & (unsigned short) 0x00ffU) << 8) |
                 ((x & (unsigned short) 0xff00U) >> 8));
#endif
}

/* 
 * gg_login_hash() // funkcja wewn�trzna
 * 
 * liczy hash z has�a i danego seeda. p�ki co, nie ma wersji, kt�ra liczy
 * poprawnie na maszynach bigendianowych.
 * 
 *  - password - has�o do hashowania,
 *  - seed - warto�� podana przez serwer.
 *
 * zwraca hash.
 */
unsigned int gg_login_hash(const unsigned char *password, unsigned int seed)
{
#if 0
	unsigned int hash;

	for (hash = 1; *password; password++)
		hash *= (*password) + 1;
	hash *= seed;

	return hash;
#endif

	unsigned int x, y, z;

	y = seed;

	for (x = 0; *password; password++) {
		x = (x & 0xffffff00) | *password;
		y ^= x;
		y += x;
		x <<= 8;
		y ^= x;
		x <<= 8;
		y -= x;
		x <<= 8;
		y ^= x;

		z = y & 0x1F;
		y = (y << z) | (y >> (32 - z));
	}

	return y;
}

/*
 * gg_resolve() // funkcja wewn�trzna
 *
 * tworzy pipe'y, forkuje si� i w drugim procesie zaczyna resolvowa� 
 * podanego hosta. zapisuje w sesji deskryptor pipe'u. je�li co� tam
 * b�dzie gotowego, znaczy, �e mo�na wczyta� ,,struct in_addr''. je�li
 * nie znajdzie, zwraca INADDR_NONE.
 *
 *  - fd - wska�nik gdzie wrzuci� deskryptor,
 *  - pid - gdzie wrzuci� pid dzieciaka,
 *  - hostname - nazwa hosta do zresolvowania.
 *
 * zwraca 0 je�li uda�o si� odpali� proces lub -1 w przypadku b��du.
 */
int gg_resolve(int *fd, int *pid, const char *hostname)
{
	int pipes[2], res;
	struct in_addr a;

	gg_debug(GG_DEBUG_FUNCTION, "** gg_resolve(..., \"%s\");\n", hostname);
	
	if (!fd | !pid) {
		errno = EFAULT;
		return -1;
	}

	if (pipe(pipes) == -1)
		return -1;

	if ((res = fork()) == -1)
		return -1;

	if (!res) {
		if ((a.s_addr = inet_addr(hostname)) == INADDR_NONE) {
			struct hostent *he;
		
			if (!(he = gethostbyname(hostname)))
				a.s_addr = INADDR_NONE;
			else
				memcpy((char*) &a, he->h_addr, sizeof(a));
		}

		write(pipes[1], &a, sizeof(a));

		exit(0);
	}

	close(pipes[1]);

	*fd = pipes[0];
	*pid = res;

	return 0;
}

/*
 * gg_recv_packet() // funkcja wewn�trzna
 *
 * odbiera jeden pakiet gg i zwraca wska�nik do niego. pami�� po nim
 * wypada�oby uwolni�.
 *
 *  - sock - po��czony socket.
 *
 * je�li wyst�pi� b��d, zwraca NULL. reszta w errno.
 */
void *gg_recv_packet(struct gg_session *sess)
{
	struct gg_header h;
	char *buf = NULL;
	int ret = 0, offset, size = 0;

	gg_debug(GG_DEBUG_FUNCTION, "** gg_recv_packet(...);\n");
	
	if (!sess) {
		errno = EFAULT;
		return NULL;
	}

	if (sess->recv_left < 1) {
		int tries = 10;
		
		while (ret != sizeof(h)) {
			ret = read(sess->fd, &h, sizeof(h));
			gg_debug(GG_DEBUG_MISC, "-- header recv(..., %d) = %d\n", sizeof(h), ret);
			if (ret < sizeof(h)) {
				if (errno != EINTR || !--tries) {
					gg_debug(GG_DEBUG_MISC, "-- errno = %d (%s)\n", errno, strerror(errno));
					return NULL;
				}
			}
		}

		h.type = fix32(h.type);
		h.length = fix32(h.length);
	} else {
		memcpy(&h, sess->recv_buf, sizeof(h));
	}

	/* jakie� sensowne limity na rozmiar pakietu */
	if (h.length < 0 || h.length > 65535) {
		gg_debug(GG_DEBUG_MISC, "-- invalid packet length (%d)\n", h.length);
		errno = ERANGE;
		return NULL;
	}

	if (sess->recv_left > 0) {
		gg_debug(GG_DEBUG_MISC, "-- resuming last gg_recv_packet()\n");
		size = sess->recv_left;
		offset = sess->recv_done;
		buf = sess->recv_buf;
	} else {
		if (!(buf = malloc(sizeof(h) + h.length + 1))) {
			gg_debug(GG_DEBUG_MISC, "-- not enough memory\n");
			return NULL;
		}

		memcpy(buf, &h, sizeof(h));

		offset = 0;
		size = h.length;
	}

	while (size > 0) {
		ret = read(sess->fd, buf + sizeof(h) + offset, size);
		gg_debug(GG_DEBUG_MISC, "-- body recv(..., %d) = %d\n", size, ret);
		if (ret > -1 && ret <= size) {
			offset += ret;
			size -= ret;
		} else if (ret == -1) {	
			gg_debug(GG_DEBUG_MISC, "-- errno = %d (%s)\n", errno, strerror(errno));
			if (errno == EAGAIN) {
				gg_debug(GG_DEBUG_MISC, "-- %d bytes received, %d left\n", offset, size);
				sess->recv_buf = buf;
				sess->recv_left = size;
				sess->recv_done = offset;
				return NULL;
			}
			if (errno != EINTR) {
//				errno = EINVAL;
				free(buf);
				return NULL;
			}
		}
	}

	sess->recv_left = 0;

	if ((gg_debug_level & GG_DEBUG_DUMP)) {
		int i;

		gg_debug(GG_DEBUG_DUMP, ">> received packet (type=%.2x):", h.type);
		for (i = 0; i < sizeof(h) + h.length; i++) 
			gg_debug(GG_DEBUG_DUMP, " %.2x", (unsigned char) buf[i]);
		gg_debug(GG_DEBUG_DUMP, "\n");
	}

	return buf;
}

/*
 * gg_send_packet() // funkcja wewn�trzna
 *
 * konstruuje pakiet i wysy�a go w do serwera.
 *
 *  - sock - po��czony socket,
 *  - type - typ pakietu,
 *  - payload_1
 *  - payload_length_1
 *  - payload_2
 *  - payload_length_2
 *  - ...
 *  - ...
 *  - NULL - ko�cowym parametr (konieczny!)
 *
 * je�li posz�o dobrze, zwraca 0. w przypadku b��du -1. je�li errno=ENOMEM,
 * zabrak�o pami�ci. inaczej by� b��d przy wysy�aniu pakietu. dla errno=0
 * nie wys�ano ca�ego pakietu.
 */
int gg_send_packet(int sock, int type, ...)
{
	struct gg_header *h;
	char *tmp;
	int tmp_length;
	void *payload;
	int payload_length;
	va_list ap;
	int res;

	gg_debug(GG_DEBUG_FUNCTION, "** gg_send_packet(0x%.2x, ...)\n", type);

	tmp_length = 0;

	if (!(tmp = malloc(sizeof(struct gg_header)))) {
		gg_debug(GG_DEBUG_MISC, "-- not enough memory\n");
		return -1;
	}

	h = (struct gg_header*) tmp;
	h->type = fix32(type);
	h->length = fix32(0);

	va_start(ap, type);

	payload = va_arg(ap, void *);

	while (payload) {
		payload_length = va_arg(ap, int);

		if (payload_length < 0)
			gg_debug(GG_DEBUG_MISC, "-- invalid payload length\n");
	
		if (!(tmp = realloc(tmp, sizeof(struct gg_header) + tmp_length + payload_length))) {
                        gg_debug(GG_DEBUG_MISC, "-- not enough memory\n");
                        return -1;
                }
		
		memcpy(tmp + sizeof(struct gg_header) + tmp_length, payload, payload_length);
		tmp_length += payload_length;

		payload = va_arg(ap, void *);
	}

	va_end(ap);

	h = (struct gg_header*) tmp;
	h->length = fix32(tmp_length);

	if ((gg_debug_level & GG_DEBUG_DUMP)) {
                int i;
		
                gg_debug(GG_DEBUG_DUMP, "%%%% sending packet (type=%.2x):", fix32(h->type));
                for (i = 0; i < sizeof(struct gg_header) + fix32(h->length); i++)
                        gg_debug(GG_DEBUG_DUMP, " %.2x", (unsigned char) tmp[i]);
                gg_debug(GG_DEBUG_DUMP, "\n");
        }
	
	tmp_length += sizeof(struct gg_header);
	
	if ((res = write(sock, tmp, tmp_length)) < tmp_length) {
		gg_debug(GG_DEBUG_MISC, "-- write() failed. res = %d, errno = %d (%s)\n", res, errno, strerror(errno));
		free(tmp);
		return -1;
	}
	
	free(tmp);	
	return 0;
}

/*
 * gg_session_callback() // funkcja wewn�trzna
 *
 * wywo�ywany z gg_session->callback, wykonuje gg_watch_fd() i pakuje
 * do gg_session->event jego wynik.
 */
static int gg_session_callback(struct gg_session *s)
{
	if (!s) {
		errno = EINVAL;
		return -1;
	}

	return ((s->event = gg_watch_fd(s)) != NULL) ? 0 : -1;
}

/*
 * gg_login()
 *
 * rozpoczyna procedur� ��czenia si� z serwerem. reszt� obs�guje si� przez
 * gg_watch_fd.
 *
 *  - info - struktura opisuj�ca pocz�tkowy stan. wymagane pola: uin,
 *    password.
 *
 * UWAGA! program musi obs�u�y� SIGCHLD, je�li ��czy si� asynchronicznie,
 * �eby zrobi� pogrzeb zmar�emu procesowi resolvera.
 *
 * w przypadku b��du zwraca NULL, je�li idzie dobrze (async) albo posz�o
 * dobrze (sync), zwr�ci wska�nik do zaalokowanej struktury `gg_session'.
 */
struct gg_session *gg_login(const struct gg_login_params *p)
{
	struct gg_session *sess = NULL;
	char *hostname;
	int port;

	gg_debug(GG_DEBUG_FUNCTION, "** gg_login(uin=%u, async=%d, ...);\n", p->uin, p->async);

	if (!(sess = malloc(sizeof(*sess)))) {
		gg_debug(GG_DEBUG_MISC, "-- not enough memory\n");
		goto fail;
	}

	memset(sess, 0, sizeof(*sess));

	if (!p->password || !p->uin) {
		gg_debug(GG_DEBUG_MISC, "-- invalid arguments (!password || !uin)\n");
		errno = EINVAL;
		goto fail;
	}

	if (!(sess->password = strdup(p->password))) {
		gg_debug(GG_DEBUG_MISC, "-- not enough memory\n");
		goto fail;
	}

	sess->uin = p->uin;
	sess->state = GG_STATE_RESOLVING;
	sess->check = GG_CHECK_READ;
	sess->timeout = GG_DEFAULT_TIMEOUT;
	sess->async = p->async;
        sess->type = GG_SESSION_GG;
	sess->initial_status = p->status;
	sess->callback = gg_session_callback;
	sess->destroy = gg_free_session;
	sess->port = (p->server_port) ? p->server_port : GG_DEFAULT_PORT;
	sess->server_addr = p->server_addr;
	
	if (gg_proxy_enabled) {
		hostname = gg_proxy_host;
		sess->proxy_port = port = gg_proxy_port;
	} else {
		hostname = GG_APPMSG_HOST;
		port = GG_APPMSG_PORT;
	};

	if (!p->async) {
		struct in_addr a;

		if (!p->server_addr || !p->server_port) {
			if ((a.s_addr = inet_addr(hostname)) == INADDR_NONE) {
				struct hostent *he;
	
				if (!(he = gethostbyname(hostname))) {
					gg_debug(GG_DEBUG_MISC, "-- host %s not found\n", hostname);
					goto fail;
				} else
					memcpy((char*) &a, he->h_addr, sizeof(a));
			}
		} else {
			a.s_addr = p->server_addr;
			port = p->server_port;
		}

		if ((sess->fd = gg_connect(&a, port, 0)) == -1) {
			gg_debug(GG_DEBUG_MISC, "-- connection failed\n");
			goto fail;
		}

		sess->state = GG_STATE_CONNECTING_HUB;

		while (sess->state != GG_STATE_CONNECTED) {
			struct gg_event *e;

			if (!(e = gg_watch_fd(sess))) {
				gg_debug(GG_DEBUG_MISC, "-- some nasty error in gg_watch_fd()\n");
				goto fail;
			}

			if (e->type == GG_EVENT_CONN_FAILED) {
				errno = EACCES;
				gg_debug(GG_DEBUG_MISC, "-- could not login\n");
				gg_free_event(e);
				goto fail;
			}

			gg_free_event(e);
		}

		return sess;
	}
	
	if (!sess->server_addr || gg_proxy_enabled) {
		if (gg_resolve(&sess->fd, &sess->pid, hostname)) {
			gg_debug(GG_DEBUG_MISC, "-- resolving failed (errno=%d, %s), something serious\n", errno, strerror(errno));
			goto fail;
		}
	} else {
		if ((sess->fd = gg_connect(&sess->server_addr, sess->port, sess->async)) == -1) {
			gg_debug(GG_DEBUG_MISC, "-- direct connection failed (errno=%d, %s)\n", errno, strerror(errno));
			goto fail;
		}
		sess->state = GG_STATE_CONNECTING_GG;
		sess->check = GG_CHECK_WRITE;
	}

	return sess;

fail:
	free(sess);
	return NULL;
}

/* 
 * gg_free_session()
 *
 * pr�buje zamkn�� po��czenia i zwalnia pami�� zajmowan� przez sesj�.
 *
 *  - sess - opis sesji.
 *
 * nie zwraca niczego, bo i po co?
 */
void gg_free_session(struct gg_session *sess)
{
	if (!sess)
		return;

	/* XXX dopisa� zwalnianie i zamykanie wszystkiego, bo mog�o zosta� */

	free(sess->password);
	free(sess);
}

/*
 * gg_change_status()
 *
 * zmienia status u�ytkownika. przydatne do /away i /busy oraz /quit.
 *
 *  - sess - opis sesji,
 *  - status - nowy status u�ytkownika.
 *
 * je�li wys�a� pakiet zwraca 0, je�li nie uda�o si�, zwraca -1.
 */
int gg_change_status(struct gg_session *sess, int status)
{
	struct gg_new_status p;

	if (!sess) {
		errno = EFAULT;
		return -1;
	}

	if (sess->state != GG_STATE_CONNECTED) {
		errno = ENOTCONN;
		return -1;
	}

	gg_debug(GG_DEBUG_FUNCTION, "** gg_change_status(..., %d);\n", status);

	p.status = fix32(status);

	sess->status = status;

	return gg_send_packet(sess->fd, GG_NEW_STATUS, &p, sizeof(p), NULL);
}

/*
 * gg_change_status_descr()
 *
 * zmienia status u�ytkownika. przydatne do /away i /busy oraz /quit.
 *
 *  - sess - opis sesji,
 *  - status - nowy status u�ytkownika,
 *  - descr - opis statusu.
 *
 * je�li wys�a� pakiet zwraca 0, je�li nie uda�o si�, zwraca -1.
 */
int gg_change_status_descr(struct gg_session *sess, int status, const char *descr)
{
	struct gg_new_status p;

	if (!sess || !descr) {
		errno = EFAULT;
		return -1;
	}

	if (sess->state != GG_STATE_CONNECTED) {
		errno = ENOTCONN;
		return -1;
	}

	gg_debug(GG_DEBUG_FUNCTION, "** gg_change_status_descr(..., %d, \"%s\");\n", status, descr);

	p.status = fix32(status);

	sess->status = status;

	return gg_send_packet(sess->fd, GG_NEW_STATUS, &p, sizeof(p), descr, strlen(descr), NULL);
}

/*
 * gg_logoff()
 *
 * wylogowuje u�ytkownika i zamyka po��czenie.
 *
 *  - sock - deskryptor socketu.
 *
 * nie zwraca b��d�w. skoro si� �egnamy, to olewamy wszystko.
 */
void gg_logoff(struct gg_session *sess)
{
	if (!sess)
		return;

	gg_debug(GG_DEBUG_FUNCTION, "** gg_logoff(...);\n");

	if (sess->status != GG_STATUS_NOT_AVAIL && sess->status != GG_STATUS_NOT_AVAIL_DESCR)
		gg_change_status(sess, GG_STATUS_NOT_AVAIL);
	
	if (sess->fd != -1) {
		shutdown(sess->fd, 2);
		close(sess->fd);
	}
}

/*
 * gg_send_message_ctcp() // TO BE GONE
 *
 * wysy�a wiadomo�� do innego u�ytkownika. zwraca losowy numer
 * sekwencyjny, kt�ry mo�na ola� albo wykorzysta� do potwierdzenia.
 *
 *  - sess - opis sesji,
 *  - msgclass - rodzaj wiadomo�ci,
 *  - recipient - numer adresata,
 *  - message - tre�� wiadomo�ci,
 *  - message_len - d�ugo��.
 *
 * w przypadku b��du zwraca -1, inaczej numer sekwencyjny.
 */
int gg_send_message_ctcp(struct gg_session *sess, int msgclass, uin_t recipient, const unsigned char *message, int message_len)
{
	struct gg_send_msg s;

	if (!sess) {
		errno = EFAULT;
		return -1;
	}
	
	if (sess->state != GG_STATE_CONNECTED) {
		errno = ENOTCONN;
		return -1;
	}

	gg_debug(GG_DEBUG_FUNCTION, "** gg_send_message_ctcp(..., %d, %u, \"...\");\n", msgclass, recipient);

	s.recipient = fix32(recipient);
	s.seq = fix32(0);
	s.msgclass = fix32(msgclass);
	
	return gg_send_packet(sess->fd, GG_SEND_MSG, &s, sizeof(s), message, message_len, NULL);
}

/*
 * gg_send_message()
 *
 * wysy�a wiadomo�� do innego u�ytkownika. zwraca losowy numer
 * sekwencyjny, kt�ry mo�na ola� albo wykorzysta� do potwierdzenia.
 *
 *  - sess - opis sesji,
 *  - msgclass - rodzaj wiadomo�ci,
 *  - recipient - numer adresata,
 *  - message - tre�� wiadomo�ci.
 *
 * w przypadku b��du zwraca -1, inaczej numer sekwencyjny.
 */
int gg_send_message(struct gg_session *sess, int msgclass, uin_t recipient, const unsigned char *message)
{
	struct gg_send_msg s;

	if (!sess) {
		errno = EFAULT;
		return -1;
	}
	
	if (sess->state != GG_STATE_CONNECTED) {
		errno = ENOTCONN;
		return -1;
	}

	gg_debug(GG_DEBUG_FUNCTION, "** gg_send_message(..., %d, %u, \"...\");\n", msgclass, recipient);

	s.recipient = fix32(recipient);
	if (!sess->seq)
		sess->seq = 0x01740000 | (rand() & 0xffff);
	s.seq = fix32(sess->seq);
	s.msgclass = fix32(msgclass);
	sess->seq += (rand() % 0x300) + 0x300;
	
	if (gg_send_packet(sess->fd, GG_SEND_MSG, &s, sizeof(s), message, strlen(message) + 1, NULL) == -1)
		return -1;

	return fix32(s.seq);
}

/*
 * gg_ping()
 *
 * wysy�a do serwera pakiet typu yeah-i'm-still-alive.
 *
 *  - sess - zgadnij.
 *
 * je�li nie powiod�o si� wys�anie pakietu, zwraca -1. otherwise 0.
 */
int gg_ping(struct gg_session *sess)
{
	if (!sess) {
		errno = EFAULT;
		return -1;
	}

	if (sess->state != GG_STATE_CONNECTED) {
		errno = ENOTCONN;
		return -1;
	}

	gg_debug(GG_DEBUG_FUNCTION, "** gg_ping(...);\n");

	return gg_send_packet(sess->fd, GG_PING, NULL);
}

/*
 * gg_notify()
 *
 * wysy�a serwerowi list� ludk�w, za kt�rymi t�sknimy.
 *
 *  - sess - identyfikator sesji,
 *  - userlist - wska�nik do tablicy numer�w,
 *  - count - ilo�� numerk�w.
 *
 * je�li uda�o si�, zwraca 0. je�li b��d, dostajemy -1.
 */
int gg_notify(struct gg_session *sess, uin_t *userlist, int count)
{
	struct gg_notify *n;
	uin_t *u;
	int i, res = 0;

	if (!sess) {
		errno = EFAULT;
		return -1;
	}
	
	if (sess->state != GG_STATE_CONNECTED) {
		errno = ENOTCONN;
		return -1;
	}

	gg_debug(GG_DEBUG_FUNCTION, "** gg_notify(..., %d);\n", count);
	
	if (!userlist || !count)
		return 0;
	
	if (!(n = (struct gg_notify*) malloc(sizeof(*n) * count)))
		return -1;
	
	for (u = userlist, i = 0; i < count; u++, i++) { 
		n[i].uin = fix32(*u);
		n[i].dunno1 = 3;
	}
	
	if (gg_send_packet(sess->fd, GG_NOTIFY, n, sizeof(*n) * count, NULL) == -1)
		res = -1;

	free(n);

	return res;
}

/*
 * gg_add_notify()
 *
 * dodaje w locie do listy ukochanych dany numerek.
 *
 *  - sess - identyfikator sesji,
 *  - uin - numerek ukochanej.
 *
 * je�li uda�o si� wys�a�, daje 0. inaczej -1.
 */
int gg_add_notify(struct gg_session *sess, uin_t uin)
{
	struct gg_add_remove a;

	if (!sess) {
		errno = EFAULT;
		return -1;
	}

	if (sess->state != GG_STATE_CONNECTED) {
		errno = ENOTCONN;
		return -1;
	}
	
	gg_debug(GG_DEBUG_FUNCTION, "** gg_add_notify(..., %u);\n", uin);
	
	a.uin = fix32(uin);
	a.dunno1 = 3;
	
	return gg_send_packet(sess->fd, GG_ADD_NOTIFY, &a, sizeof(a), NULL);
}

/*
 * gg_remove_notify()
 *
 * w locie usuwa z listy zainteresowanych.
 *
 *  - sess - id sesji,
 *  - uin - numerek.
 *
 * zwraca -1 je�li by� b��d, 0 je�li si� uda�o wys�a� pakiet.
 */
int gg_remove_notify(struct gg_session *sess, uin_t uin)
{
	struct gg_add_remove a;

	if (!sess) {
		errno = EFAULT;
		return -1;
	}

	if (sess->state != GG_STATE_CONNECTED) {
		errno = ENOTCONN;
		return -1;
	}

	gg_debug(GG_DEBUG_FUNCTION, "** gg_remove_notify(..., %u);\n", uin);
	
	a.uin = fix32(uin);
	a.dunno1 = 3;
	
	return gg_send_packet(sess->fd, GG_REMOVE_NOTIFY, &a, sizeof(a), NULL);
}

/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: notnil
 * End:
 *
 * vim: shiftwidth=8:
 */
