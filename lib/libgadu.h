/* $Id$ */

/*
 *  (C) Copyright 2001 Wojtek Kaniewski <wojtekka@irc.pl>,
 *                     Robert J. Wo�ny <speedy@ziew.org>,
 *                     Arkadiusz Mi�kiewicz <misiek@pld.ORG.PL>
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

#ifndef __GG_LIBGG_H
#define __GG_LIBGG_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(sun) && !defined(INADDR_NONE)
  #define INADDR_NONE 0xffffffff
#endif

#include <sys/types.h>

/*
 * typ zmiennej okre�laj�cej numerek danej osoby.
 */
typedef unsigned long uin_t;

/*
 * og�lna struktura opisuj�ca r�ne sesje. przydatna w klientach.
 */
struct gg_common {
        int fd;                 /* podgl�dany deskryptor */
        int check;              /* sprawdzamy zapis czy odczyt */
        int state;              /* aktualny stan maszynki */
        int error;              /* kod b��du dla GG_STATE_ERROR */
	int type;		/* rodzaj sesji */
	int id;			/* identyfikator */
	int timeout;		/* sugerowany timeout w sekundach */
};

/*
 * struktura opisuj�ca dan� sesj�. tworzona przez gg_login().
 */
struct gg_session {
        int fd;         	/* podgl�dany deskryptor */
        int check;      	/* sprawdzamy zapis czy odczyt */
        int state;      	/* aktualny stan maszynki */
        int error;      	/* kod b��du dla GG_STATE_ERROR */
	int type;		/* rodzaj sesji. == GG_SESSION_GG */
	int id;			/* identyfikator */
	int timeout;		/* sugerowany timeout w sekundach */
	
        int async;      	/* czy po��czenie jest asynchroniczne */
	int pid;        	/* pid procesu resolvera */
	int port;       	/* port, z kt�rym si� ��czymy */
	int seq;        	/* numer sekwencyjny ostatniej wiadomo�ci */
	int last_pong;  	/* czas otrzymania ostatniego ping/pong */
	int last_event;		/* czas otrzymania ostatniego pakietu */

	unsigned int server_ip;	/* adres serwera */
	unsigned int client_ip;	/* adres klienta */
	int client_port;	/* port, na kt�rym klient s�ucha */

	uin_t uin;		/* numerek klienta */
	char *password;		/* i jego has�o. zwalniane automagicznie */
        
	int initial_status;	/* pocz�tkowy stan klienta */

	char *recv_buf;		/* bufor na otrzymywane pakiety */
	int recv_done;		/* ile ju� wczytano do bufora */
        int recv_left;		/* i ile jeszcze trzeba wczyta� */
};

/*
 * og�lna struktura opisuj�ca stan wszystkich operacji http.
 */
struct gg_http {
        int fd;                 /* podgl�dany deskryptor */
        int check;              /* sprawdzamy zapis czy odczyt */
        int state;              /* aktualny stan maszynki */
        int error;              /* kod b��du dla GG_STATE_ERROR */
 	int type;		/* rodzaj sesji. == GG_SESSION_HTTP */
	int id;			/* identyfikator */
	int timeout;		/* sugerowany timeout w sekundach */
	
        int async;              /* czy po��czenie asynchroniczne */
	int pid;                /* pid procesu resolvera */
	int port;               /* port, z kt�rym si� ��czymy */

        char *query;            /* bufor zapytania http */
        char *header;           /* bufor nag��wka */
        int header_size;        /* rozmiar wczytanego nag��wka */
        char *body;             /* bufor otrzymanych informacji */
        int body_size;          /* ilo�� informacji */

        void *data;             /* dane danej operacji http */

	char *user_data;	/* dane u�ytkownika */
};

/*
 * odpowiednik windowsowej struktury WIN32_FIND_DATA niezb�dnej przy
 * wysy�aniu plik�w.
 */
#define GG_MAX_PATH 276

struct gg_file_info {
	unsigned long mode;		/* dwFileAttributes */
	unsigned long ctime[2];		/* ftCreationTime */
	unsigned long atime[2];		/* ftLastAccessTime */
	unsigned long mtime[2];		/* ftLastWriteTime */
	unsigned long size_hi;		/* nFileSizeHigh */
	unsigned long size;		/* nFileSizeLow */
	unsigned long reserved0;	/* dwReserved0 */
	unsigned long reserved1;	/* dwReserved1 */
	char filename[GG_MAX_PATH];	/* cFileName */
};

/*
 * struktura opisuj�ca nas�uchuj�ce gniazdo po��cze� mi�dzy klientami.
 */
struct gg_dcc {
        int fd;                 /* podgl�dany deskryptor */
        int check;              /* sprawdzamy zapis czy odczyt */
        int state;              /* aktualny stan maszynki */
        int error;              /* kod b��du dla GG_STATE_ERROR */
	int type;		/* rodzaj sesji */
	int id;			/* identyfikator */
	int timeout;		/* sugerowany timeout w sekundach */

	int active;		/* czy to my si� ��czymy? */
	int port;		/* port, na kt�rym siedzi */
	uin_t uin;		/* uin klienta */
	uin_t peer_uin;		/* uin drugiej strony */
	int file_fd;		/* deskryptor pliku */
	int offset;		/* offset w pliku */
	int chunk_size;		/* rozmiar kawa�ka */
	int chunk_offset;	/* offset w aktualnym kawa�ku */
	struct gg_file_info file_info;
				/* informacje o pliku */
};

/*
 * rodzaje sesji.
 */
enum {
	GG_SESSION_GG = 1,	/* po��czenie z serwerem gg */
	GG_SESSION_HTTP,	/* og�lna sesja http */
	GG_SESSION_SEARCH,	/* szukanie */
	GG_SESSION_REGISTER,	/* rejestrowanie */
	GG_SESSION_REMIND,	/* przypominanie has�a */
	GG_SESSION_PASSWD,	/* zmiana has�a */
	GG_SESSION_CHANGE,	/* zmiana informacji o sobie */
	GG_SESSION_DCC,		/* og�lne po��czenie DCC */
	GG_SESSION_DCC_SOCKET,	/* nas�uchuj�cy socket */
	GG_SESSION_DCC_SEND,	/* wysy�anie pliku */
	GG_SESSION_DCC_GET,	/* odbieranie pliku */
	
	GG_SESSION_USER0 = 256,	/* zdefiniowana dla u�ytkownika */
	GG_SESSION_USER1,	/* j.w. */
	GG_SESSION_USER2,	/* j.w. */
	GG_SESSION_USER3,	/* j.w. */
};

/*
 * r�ne stany asynchronicznej maszynki.
 */
enum {
        /* wsp�lne */
        GG_STATE_IDLE = 0,		/* nie powinno wyst�pi�. */
        GG_STATE_RESOLVING,             /* wywo�a� gethostbyname() */
	GG_STATE_CONNECTING,            /* wywo�a� connect() */
	GG_STATE_READING_DATA,		/* czeka na dane http */
	GG_STATE_ERROR,			/* wyst�pi� b��d. kod w x->error */

        /* gg_session */
	GG_STATE_CONNECTING_GG,         /* wywo�a� connect() */
	GG_STATE_READING_KEY,           /* czeka na klucz */
	GG_STATE_READING_REPLY,         /* czeka na odpowied� */
	GG_STATE_CONNECTED,             /* po��czy� si� */

        /* gg_http */
	GG_STATE_READING_HEADER,	/* czeka na nag��wek http */
	GG_STATE_PARSING,               /* przetwarza dane */
	GG_STATE_DONE,                  /* sko�czy� */

	/* gg_dcc */
	GG_STATE_LISTENING,		/* czeka na po��czenia */
	GG_STATE_READING_UIN_1,		/* czeka na uin peera */
	GG_STATE_READING_UIN_2,		/* czeka na sw�j uin */
	GG_STATE_SENDING_ACK,		/* wysy�a potwierdzenie dcc */
	GG_STATE_READING_ACK,		/* czeka na potwierdzenie dcc */
	GG_STATE_READING_REQUEST,	/* czeka na komend� */
	GG_STATE_SENDING_REQUEST,	/* wysy�a komend� */
	GG_STATE_SENDING_FILE_INFO,	/* wysy�a informacje o pliku */
	GG_STATE_READING_PRE_FILE_INFO,	/* czeka na pakiet przed file_info */
	GG_STATE_READING_FILE_INFO,	/* czeka na informacje o pliku */
	GG_STATE_SENDING_FILE_ACK,	/* wysy�a potwierdzenie pliku */
	GG_STATE_READING_FILE_ACK,	/* czeka na potwierdzenie pliku */
	GG_STATE_SENDING_FILE_HEADER,	/* wysy�a nag��wek pliku */
	GG_STATE_READING_FILE_HEADER,	/* czeka na nag��wek */
	GG_STATE_GETTING_FILE,		/* odbiera plik */
	GG_STATE_SENDING_FILE,		/* wysy�a plik */
};

/*
 * dla zachowania kompatybilno�ci wstecz. w wersji 1.0 b�dzie usuni�te. oby.
 */
#define GG_STATE_CONNECTING_HTTP GG_STATE_CONNECTING
#define GG_STATE_WRITING_HTTP GG_STATE_READING_DATA
#define GG_STATE_WAITING_FOR_KEY GG_STATE_READING_KEY
#define GG_STATE_SENDING_KEY GG_STATE_READING_REPLY
#define GG_STATE_FINISHED GG_STATE_DONE

/*
 * co proces klienta powinien sprawdza� w deskryptorach?
 */
enum {
	GG_CHECK_NONE = 0,		/* nic. nie powinno wyst�pi� */
	GG_CHECK_WRITE = 1,		/* sprawdzamy mo�liwo�� zapisu */
	GG_CHECK_READ = 2,		/* sprawdzamy mo�liwo�� odczytu */
};

struct gg_session *gg_login(uin_t uin, char *password, int async);
void gg_free_session(struct gg_session *sess);
void gg_logoff(struct gg_session *sess);
int gg_change_status(struct gg_session *sess, int status);
int gg_send_message(struct gg_session *sess, int msgclass, uin_t recipient, unsigned char *message);
int gg_send_message_ctcp(struct gg_session *sess, int msgclass, uin_t recipient, unsigned char *message, int message_len);
int gg_ping(struct gg_session *sess);


enum {
	GG_EVENT_NONE = 0,
	GG_EVENT_MSG,
	GG_EVENT_NOTIFY,
	GG_EVENT_STATUS,
	GG_EVENT_ACK,
	GG_EVENT_PONG,
	GG_EVENT_CONN_FAILED,
	GG_EVENT_CONN_SUCCESS,
	GG_EVENT_DISCONNECT,

	GG_EVENT_DCC_NEW,		/* nowe po��czenie mi�dzy klientami */
	GG_EVENT_DCC_ERROR,		/* b��d */
	GG_EVENT_DCC_DONE,		/* sko�czy� */
	GG_EVENT_DCC_CLIENT_ACCEPT,	/* moment akceptacji klienta */
	GG_EVENT_DCC_NEED_FILE_INFO,	/* trzeba wype�ni� file_info */
	GG_EVENT_DCC_NEED_FILE_ACK,	/* czeka na potwierdzenie */
};

/*
 * nied�ugo si� tego pozb�d� na rzecz sensownej obs�ugi b��d�w. --w
 */
enum {
	GG_FAILURE_RESOLVING = 1,
	GG_FAILURE_CONNECTING,
	GG_FAILURE_INVALID,
	GG_FAILURE_READING,
	GG_FAILURE_WRITING,
	GG_FAILURE_PASSWORD,
	GG_FAILURE_404,
};

/*
 * rodzaje b��d�w, na razie u�ywane przez http. bez rozczulania si� nad
 * powodami. klient powie, �e albo nie znalaz� hosta, albo nie m�g� si�
 * po��czy�, albo nie m�g� wys�a�, albo nie m�g� odebrac. i tyle. jak
 * kto� b�dzie chcia�, to b�dzie m�g� sprawdzi� errno. ale po co?
 */
enum {
	GG_ERROR_RESOLVING = 1,		/* b��d znajdowania hosta */
	GG_ERROR_CONNECTING,		/* b��d �aczenia si� */
	GG_ERROR_READING,		/* b��d odczytu */
	GG_ERROR_WRITING,		/* b��d wysy�ania */

	GG_ERROR_DCC_HANDSHAKE,		/* b��d negocjacji */
	GG_ERROR_DCC_FILE,		/* b��d odczytu/zapisu pliku */
	GG_ERROR_DCC_EOF,		/* plik si� sko�czy�? */
	GG_ERROR_DCC_NET,		/* b��d wysy�ania/odbierania */
};

/*
 * struktura opisuj�ca rodzaj zdarzenia. wychodzi z gg_watch_fd() lub
 * z gg_dcc_watch_fd()
 */
struct gg_event {
        int type;
        union {
		/* dotycz�ce gg_session */
		struct {
                        uin_t sender;
			int msgclass;
			time_t time;
                        unsigned char *message;
			/* konferencyjne */
			int recipients_count;
			uin_t *recipients;
                } msg;
                struct gg_notify_reply *notify;
                struct {
			uin_t uin;
			unsigned long status;
		} status;
                struct {
                        uin_t recipient;
                        int status;
                        int seq;
                } ack;
		int failure;

		/* dotycz�ce gg_dcc */
		struct gg_dcc *dcc_new;
		int dcc_error;
        } event;
};

struct gg_event *gg_watch_fd(struct gg_session *sess);
void gg_free_event(struct gg_event *e);

int gg_notify(struct gg_session *sess, uin_t *userlist, int count);
int gg_add_notify(struct gg_session *sess, uin_t uin);
int gg_remove_notify(struct gg_session *sess, uin_t uin);


/*
 * OBS�UGA HTTP
 */

struct gg_http *gg_http_connect(char *hostname, int port, int async, char *method, char *path, char *header);
int gg_http_watch_fd(struct gg_http *h);
void gg_http_stop(struct gg_http *h);
void gg_free_http(struct gg_http *h);

/* 
 * SZUKANIE U�YTKOWNIK�W
 */

/*
 * struktura opisuj�ca kryteria wyszukiwania. argument gg_search().
 */
struct gg_search_request {
	int active;		/* czy ma szuka� tylko aktywnych? */

	/* mode 0 */
	char *nickname;		/* pseudonim */
	char *first_name;	/* imi� */
	char *last_name;	/* nazwisko */
	char *city;		/* miasto */
	int gender;		/* p�e� */
	int min_birth;		/* urodzony od roku... */
	int max_birth;		/* urodzony do roku... */
	
	/* mode 1 */
	char *email;		/* adres e-mail */

	/* mode 2 */
	char *phone;		/* numer telefonu */
	
	/* mode 3 */
	uin_t uin;		/* numerek */
};

/*
 * struktura opisuj�ca rezultat wyszukiwania. pole gg_http.
 */
struct gg_search {
	int count;				/* ilo�� znalezionych */
	struct gg_search_result *results;	/* tabelka z nimi */
};

/*
 * pojedynczy rezultat wyszukiwania.
 */
struct gg_search_result {
	uin_t uin;		/* numerek */
	char *first_name;	/* imi� */
	char *last_name;	/* nazwisko */
	char *nickname;		/* pseudonim */
	int born;		/* rok urodzenia */
	int gender;		/* p�e� */
	char *city;		/* miasto */
	int active;		/* czy jest aktywny */
};

#define GG_GENDER_NONE 0	/* nie podano lub bez znaczenia */
#define GG_GENDER_FEMALE 1	/* kobieta */
#define GG_GENDER_MALE 2	/* m�czyzna */

struct gg_http *gg_search(struct gg_search_request *r, int async);
int gg_search_watch_fd(struct gg_http *f);
void gg_free_search(struct gg_http *f);

struct gg_search_request *gg_search_request_mode_0(char *nickname, char *first_name, char *last_name, char *city, int gender, int min_birth, int max_birth, int active);
struct gg_search_request *gg_search_request_mode_1(char *email, int active);
struct gg_search_request *gg_search_request_mode_2(char *phone, int active);
struct gg_search_request *gg_search_request_mode_3(uin_t uin, int active);

/*
 * OPERACJE NA KATALOGU PUBLICZNYM
 */

struct gg_pubdir {
	int success;		/* czy si� uda�o */
	uin_t uin;		/* otrzymany numerek. 0 je�li b��d */
};

struct gg_modify {
	char *first_name;	/* imi� */
	char *last_name;	/* nazwisko */
	char *nickname;		/* pseudonim */
	char *email;		/* email */
	int born;		/* rok urodzenia */
	int gender;		/* p�e� */
	char *city;		/* miasto */
};

struct gg_http *gg_register(char *email, char *password, int async);
#define gg_register_watch_fd gg_pubdir_watch_fd
#define gg_free_register gg_free_pubdir

struct gg_http *gg_remind_passwd(uin_t uin, int async);
#define gg_remind_passwd_watch_fd gg_pubdir_watch_fd
#define gg_free_remind_passwd gg_free_pubdir

struct gg_http *gg_change_passwd(uin_t uin, char *passwd, char *newpasswd, char *newemail, int async);
#define gg_change_passwd_watch_fd gg_pubdir_watch_fd
#define gg_free_change_passwd gg_free_pubdir

struct gg_http *gg_change_pubdir(uin_t uin, char *passwd, struct gg_modify *modify, int async);
#define gg_change_pubdir_watch_fd gg_pubdir_watch_fd
#define gg_free_change_pubdir gg_free_pubdir

int gg_pubdir_watch_fd(struct gg_http *f);
void gg_free_pubdir(struct gg_http *f);

/*
 * FUNKCJE DOTYCZ�CE KOMUNIKACJI MI�DZY KLIENTAMI
 */

int gg_dcc_port;

int gg_dcc_request(struct gg_session *sess, uin_t uin);

struct gg_dcc *gg_dcc_send_file(unsigned long ip, unsigned short port, uin_t my_uin, uin_t peer_uin);
int gg_dcc_fill_file_info(struct gg_dcc *d, char *filename);

struct gg_dcc *gg_dcc_create_socket(uin_t uin, unsigned int port);
#define gg_free_dcc_socket gg_free_dcc
struct gg_event *gg_dcc_watch_fd(struct gg_dcc *d);

void gg_free_dcc(struct gg_dcc *c);

/*
 * je�li chcemy sobie podebugowa�, wystarczy ustawi� `gg_debug_level'.
 * niestety w miar� przybywania wpis�w `gg_debug(...)' nie chcia�o mi
 * si� ustawia� odpowiednich leveli, wi�c wi�kszo�� sz�a do _MISC.
 */

extern int gg_debug_level;

#define GG_DEBUG_NET 1
#define GG_DEBUG_TRAFFIC 2
#define GG_DEBUG_DUMP 4
#define GG_DEBUG_FUNCTION 8
#define GG_DEBUG_MISC 16

void gg_debug(int level, char *format, ...);

/*
 * Pare ma�ych zmiennych do obs�ugi "http proxy"
 */
 
extern int gg_http_use_proxy;
extern char *gg_http_proxy_host;
extern int gg_http_proxy_port;

/*
 * -------------------------------------------------------------------------
 * poni�ej znajduj� si� wewn�trzne sprawy biblioteki. zwyk�y klient nie
 * powinien ich w og�le rusza�, bo i nie ma po co. wszystko mo�na za�atwi�
 * procedurami wy�szego poziomu, kt�rych definicje znajduj� si� na pocz�tku
 * tego pliku.
 * -------------------------------------------------------------------------
 */

int gg_resolve(int *fd, int *pid, char *hostname);
void gg_debug(int level, char *format, ...);
char *gg_alloc_sprintf(char *format, ...);
char *gg_get_line(char **ptr);
int gg_connect(void *addr, int port, int async);
void gg_read_line(int sock, char *buf, int length);
void gg_chomp(char *line);
char *gg_urlencode(char *str);
int gg_http_hash(char *format, ...);
unsigned long fix32(unsigned long x);
unsigned short fix16(unsigned short x);

#define GG_APPMSG_HOST "appmsg.gadu-gadu.pl"
#define GG_APPMSG_PORT 80
#define GG_PUBDIR_HOST "pubdir.gadu-gadu.pl"
#define GG_PUBDIR_PORT 80
#define GG_REGISTER_HOST "register.gadu-gadu.pl"
#define GG_REGISTER_PORT 80
#define GG_REMIND_HOST "retr.gadu-gadu.pl"
#define GG_REMIND_PORT 80

#define GG_DEFAULT_PORT 8074
#define GG_HTTPS_PORT 443
#define GG_HTTP_USERAGENT "Mozilla/4.7 [en] (Win98; I)"

#define GG_CLIENT_VERSION 0x11
#define GG_DEFAULT_TIMEOUT 30

#define GG_DEFAULT_DCC_PORT 1550

struct gg_header {
	unsigned long type;		/* typ pakietu */
	unsigned long length;		/* d�ugo�� reszty pakietu */
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
;

#define GG_WELCOME 0x0001

struct gg_welcome {
	unsigned long key;		/* klucz szyfrowania has�a */
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
;
	
#define GG_LOGIN 0x000c

struct gg_login {
	uin_t uin;			/* tw�j numerek */
	unsigned long hash;		/* hash has�a */
	unsigned long status;		/* status na dzie� dobry */
	unsigned long version;		/* moja wersja klienta */
	unsigned long local_ip;		/* m�j adres ip */
	unsigned short local_port;	/* port, na kt�rym s�ucham */
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
;

#define GG_LOGIN_OK 0x0003

#define GG_LOGIN_FAILED 0x0009

#define GG_NEW_STATUS 0x0002

#define GG_STATUS_NOT_AVAIL 0x0001	/* roz��czony */
#define GG_STATUS_AVAIL 0x0002		/* dost�pny */
#define GG_STATUS_BUSY 0x0003		/* zaj�ty */
#define GG_STATUS_INVISIBLE 0x0014	/* niewidoczny (GG 4.6) */

#define GG_STATUS_FRIENDS_MASK 0x8000	/* tylko dla znajomych (GG 4.6) */

struct gg_new_status {
	unsigned long status;			/* na jaki zmieni�? */
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
;

#define GG_NOTIFY 0x0010
	
struct gg_notify {
	uin_t uin;		/* numerek danej osoby */
	char dunno1;		/* == 3 */
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
;
	
#define GG_NOTIFY_REPLY 0x000c	/* tak, to samo co GG_LOGIN */
	
struct gg_notify_reply {
	uin_t uin;			/* numerek */
	unsigned long status;		/* status danej osoby */
	unsigned long remote_ip;	/* adres ip delikwenta */
	unsigned short remote_port;	/* port, na kt�rym s�ucha klient */
	unsigned long version;		/* wersja klienta */
	unsigned short dunno2;		/* znowu port? */
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
;

#define GG_ADD_NOTIFY 0x000d
#define GG_REMOVE_NOTIFY 0x000e
	
struct gg_add_remove {
	uin_t uin;		/* numerek */
	char dunno1;		/* == 3 */
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
;

#define GG_STATUS 0x0002

struct gg_status {
	uin_t uin;			/* numerek */
	unsigned long status;		/* nowy stan */
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
;
	
#define GG_SEND_MSG 0x000b

#define GG_CLASS_QUEUED 0x0001
#define GG_CLASS_OFFLINE GG_CLASS_QUEUED
#define GG_CLASS_MSG 0x0004
#define GG_CLASS_CHAT 0x0008
#define GG_CLASS_CTCP 0x0010

#define GG_MSG_MAXSIZE 2000

struct gg_send_msg {
	unsigned long recipient;
	unsigned long seq;
	unsigned long msgclass;
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
;

struct gg_msg_recipients {
	char flag;
	unsigned int count;
}
#ifdef __GNUC__
 __attribute__ ((packed))
#endif
;

#define GG_SEND_MSG_ACK 0x0005

#define GG_ACK_DELIVERED 0x0002
#define GG_ACK_QUEUED 0x0003
#define GG_ACK_NOT_DELIVERED 0x0006
	
struct gg_send_msg_ack {
	unsigned long status;
	unsigned long recipient;
	unsigned long seq;
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
;

#define GG_RECV_MSG 0x000a
	
struct gg_recv_msg {
	unsigned long sender;
	unsigned long seq;
	unsigned long time;
	unsigned long msgclass;
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
;

#define GG_PING 0x0008
	
#define GG_PONG 0x0007

#define GG_DISCONNECTING 0x000b

/*
 * pakiety, sta�e, struktury dla DCC
 */

struct gg_dcc_small_packet {
	unsigned long type;		/* rodzaj pakietu */
};

struct gg_dcc_big_packet {
	unsigned long type;		/* rodzaj pakietu */
	unsigned long dunno1;		/* niewiadoma */
	unsigned long dunno2;		/* niewiadoma */
};

/*
 * p�ki co, nie znamy dok�adnie protoko�u. nie wiemy, co czemu odpowiada.
 * nazwy s� niepowa�ne i tymczasowe.
 */
#define GG_DCC_WANT_FILE 0x0003		/* peer chce plik */
#define GG_DCC_HAVE_FILE 0x0001		/* wi�c mu damy */
#define GG_DCC_HAVE_FILEINFO 0x0003	/* niech ma informacje o pliku */
#define GG_DCC_GIMME_FILE 0x0006	/* peer jest pewny */
#define GG_DCC_CATCH_FILE 0x0002	/* wysy�amy plik */

#define GG_DCC_FILEATTR_READONLY 0x0020

#define GG_DCC_TIMEOUT_SEND 1800	/* 30 minut */
#define GG_DCC_TIMEOUT_GET 1800		/* 30 minut */
#define GG_DCC_TIMEOUT_FILE_ACK 300	/* 5 minut */

#ifdef __cplusplus
}
#endif

#endif /* __GG_LIBGG_H */

/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: notnil
 * End:
 *
 * vim: shiftwidth=8:
 */
