.\"                                      Hey, EMACS: -*- nroff -*-
.TH EKGSEARCH 1 "19 lipca 2002"
.\" Please adjust this date whenever revising the manpage.

.SH NAZWA
ekgsearch \- wyszukuje osoby w publicznym katalogu Gadu-Gadu
.SH SK�ADNIA
.B ekgsearch
.RI [ opcje ] \  ...
.br
.SH OPIS
Ta strona podr�cznika opisuje kr�tko dzia�anie programu
.B ekgsearch
\.
Zosta�a ona napisana dla dystrybucji Debian, poniewa� oryginalny program nie
posiada strony podr�cznika.
.PP
\fBekgsearch\fP to program umo�liwiaj�cy wyszukiwanie os�b w publicznym
katalogu Gadu-Gadu.
.SH OPCJE
Poni�sze opcje umo�liwiaj� wyszukiwanie os�b wed�ug r�nych kryteri�w.
.TP
.B \-h, \-\-help
Powoduje wypisanie kr�tkiej pomocy.
.TP
.B \-u, \-\-uin \fInumerek\fP
Szukaj osoby o numerze \fInumerek\fP.
.TP
.B \-f, \-\-first \fIimi�\fP
Szukaj os�b o imieniu \fIimi�\fP.
.TP
.B \-l, \-\-last \fInazwisko\fP
Szukaj os�b o nazwisku \fInazwisko\fP.
.TP
.B \-n, \-\-nick \fIpseudonim\fP
Szukaj os�b o pseudonimie \fIpseudonim\fP.
.TP
.B \-c, \-\-city \fImiasto\fP
Szukaj os�b z miasta \fImiasto\fP.
.TP
.B \-b, \-\-born \fImin:max\fP
Szukaj os�b urodzonych przed dat� \fImax\fP, ale po \fImin\fP.
.TP
.B \-p, \-\-phone \fItelefon\fP
Szukaj osoby kt�rej numer telefonu to \fItelefon\fP.
.TP
.B \-e, \-\-email \fIe\-mail\fP
Szukaj osoby kt�rej adres e\-mail to \fIe\-mail\fP.
.TP
.B \-a, \-\-active
Szukaj tylko aktywnych u�ytkownik�w.
.TP
.B \-F, \-\-female
Szukaj tylko os�b p�ci �e�skiej
.TP
.B \-M, \-\-male
Szukaj tylko os�b p�ci m�skiej.
.TP
.B \-s, \-\-start \fInumer\fP
Pokazuj tylko osoby kt�rych numer GG jest wi�kszy od lub r�wny \fInumer\fP.
.TP
.B \-\-all
Poka� wszystkie pasuj�ce osoby, nie tylko 20 pierwszych.
.TP
.B \-\-debug
Pokazuj informacje diagnostyczne.
.SH PATRZ TE�
.BR ekg (1).
.br
.SH AUTHOR
Piotr Wysocki <wysek@linux.bydg.org>.
.br
Ta strona podr�cznika zosta�a napisana przez Marcina Owsianego <porridge@debian.org>
dla systemu Debian GNU/Linux (ale mo�e by� u�ywana w innych).

