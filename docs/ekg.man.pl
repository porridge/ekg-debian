.TH EKG n "11 listopada 2002" 
.SH NAZWA
ekg \- Eksperymentalny Klient Gadu-Gadu
.SH SK�ADNIA
.B ekg [
.BI opcje
.B ]
.B [
.BI komendy
.B ]

.SH OPIS
.B ekg
Jest to eksperymentalny klient Gadu-Gadu.

.SH OPCJE
.TP
.BI \-h\ [\-\-help]
Wy�wietla list� mo�liwych opcji.
.TP
.BI \-u\ username\ [\-\-user\ username]
Pozwala na uruchomienie ekg z innym ,,u�ytkownikiem''
Standardowo konfiguracja ekg znajduje sie w katalogu ~/.gg (chyba, �e
zdefiniowana jest zmienna �rodowiskowa $CONFGI_DIR - wtedy w ~/$CONFIG_DIR/gg/).
Ta opcja pozwala na stworzenie struktury podkatalog�w z oddzielnymi
konfiguracjami.
.TP
.BI \-t\ theme\ [\-\-theme\ theme]
�aduje opis wygl�du z podanego pliku.
.TP
.BI \-c\ plik\ [\-\-control-pipe\ plik]
Tworzy nazwany potok (ang. named pipe) umo�liwiaj�cy sterowanie klientem.
.TP
.BI \-n\ [\-\-no-auto]
Nie ��cz si� automagicznie. Po uruchomieniu nie ��czy si� automatycznie 
z serwerami gg. 
.TP
.BI \-a\ [\-\-away[=opis]]
Po po��czeniu zmienia stan na ,,zaj�ty''.
.TP
.BI \-b\ [\-\-back[=opis]]
Po po��czeniu zmienia stan na ,,dost�pny''.
.TP 
.BI \-i\ [\-\-invisible[=opis]]
Po po��czeniu zmienia stan na ,,niewidoczny''.
.TP
.BI \-p\ [\-\-private]
Po po��czeniu w��cza tryb ,,tylko dla przyjaci�''.
.TP
.BI \-d\ [\-\-debug]
W��cza tryb ,,debug'' - jest to specjalny tryb, pokazuj�cy, co robi 
program w danej chwili.
.TP
.BI \-v\ [\-\-version]
Wy�wietla wersj� klienta i biblioteki libgadu.
.TP
.BI \-f\ nazwa\ [\-\-frontend\ nazwa]
Wybiera podany interfejs u�ytkownika.
