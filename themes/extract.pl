#!/usr/bin/perl
# prosty skrypt wyci�gaj�cy z ../src/themes.c wszystkie formaty pozwalaj�c
# na �atw� edycj�. ziew.
# $Id$

open(FOO, "../src/themes.c") || die("Nie wstan�, tak b�d� le�a�!");

while(<FOO>) {
	chomp;

	next if (!/\tformat_add\("/);

	s/.*format_add\("//;
	s/", "/ /;
	s/", 1\);//;

	print "$_\n";
}
