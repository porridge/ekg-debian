#!/usr/bin/perl
# prosty skrypt wyci�gaj�cy z ../src/themes.c wszystkie formaty pozwalaj�c
# na �atw� edycj�. ziew.
# $Id$

open(FOO, "../src/themes.c") || die("Nie wstan�, tak b�d� le�a�!");

while(<FOO>) {
	chomp;

	next if (!/\tadd_format\("/);

	s/.*add_format\("//;
	s/", "/ /;
	s/", 1\);//;

	print "$_\n";
}
