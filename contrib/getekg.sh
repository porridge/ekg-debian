#!/bin/bash
# getekg.sh - skrypt �ci�gaj�cy zawsze najnowsz� wersj� EKG.
# Autor: Arim.
# Ostatnie modyfikacja: 28/05/2002 14:28

if [ "$1" == "" ]; then 
	echo "Podaj zmienne (\"--prefix=/usr\" itp.) dla configure'a!"; 
	exit 1; 
fi
echo "Wchodz� do katalogu tymczasowego."
cd /tmp
echo -n "�ci�gam najnowsz� wersj�."
wget -o wget.log http://dev.null.pl/ekg/ekg-current.tar.gz
rm wget.log
echo -n " Rozpakowuj�..."
gunzip ekg-current.tar.gz
tar xvf ekg-current.tar > /dev/null
rm ekg-current.tar
cd `ls -1 | grep ekg-`
echo -n " Konfiguruj�..."
./configure $* > /dev/null
echo " Instaluj�..."
make install > /dev/null
cd ..
rm -r `ls -1 | grep ekg-`
echo "Zrobione."
