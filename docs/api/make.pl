#!/usr/bin/perl
#
# prosty skrypt tworz�cy dokumentacj� API libgadu z dost�pnych �r�de�.
#
# $Id$

open(H, ">ref.functions.html");

print H "<html>\n<head>\n<meta http-equiv=\"Content-type\" content=\"text/html; charset=iso-8859-2\">\n<link rel=stylesheet href=\"ref.css\" type=\"text/css\">\n</head>\n<body>\n";

for $i (glob("../../lib/*.[ch]")) {
	open(F, $i);

	print "Analizuj� plik $i\n";

	while (<F>) {
		chomp();
		next if (!/^\/\*/);

		$_ = <F>;
		chomp();
		next if (!/^ \* gg_.*\(\)/);

		s/^ \* //;
		s/\(.*//;

		$name = $_;
		print "  Funkcja $name\n";
		$descr = "";
		$p_num = 0;
		%p_descr = ();
		%p_type = ();
		$result = "";
		
		<F>;
		
		while (1) {
			# wczytaj lini�.
			$_ = <F>;
			chomp();

			# usu� komentarz.
			s/^ \* *//;
			
			# je�li zacz�y si� parametry, spadaj.
			last if (/^-/ || /^\/$/);
			
			if (/^$/) {
				$descr .= "<p> ";
			} else {
				$descr .= "$_ ";
			}
		}

		$descr =~ s/<p> $//;

		$descr = uc_my($descr);

		while (1) {
			# je�li koniec parametr�w, wyjd�.
			last if (/^$/ || /^\/$/);

			# pocz�tek opisu parametru?
			if (/^- ([^ -]+) - (.*)/) {
				$last_p = "$p_num $1";
				$p_descr{$last_p} = $2;
				$p_num++;
			} elsif (/^- ([a-zA-Z0-9_]+)/) {
				$last_p = "$p_num $1";
				$p_descr{$last_p} = "";
				$last_p = $1;
			} else {
				$p_descr{$last_p} .= "$_ ";
			}

			# czytaj nast�pn� lini�.
			$_ = <F>;
			chomp();
			s/^ \* *//;
		}

		while (!/^\/$/) {
			if ($_ ne "") {
				$result .= "$_ ";
			}

			$_ = <F>;
			chomp();
			s/^ \* *//;
		}

		if (!$result) {
			$result = "Brak.";
		}

		if ($result =~ /0, -1, errno/) {
			$result = "0 je�li operacja si� powiod�a, -1 w przypadku b��du (kod b��du w zmiennej <tt>errno</tt>.)";
		}
		if ($result =~ /0, -1/) {
			$result = "0 je�li operacja si� powiod�a, -1 w przypadku b��du.";
		}

		$result = uc_my($result);

		$_ = <F>;
		chomp();

		$decl = $_;
		
		s/^[^(]*\(//;
		s/\) *$//;

		foreach (split(/ *, */)) {
			s/^ *//;
			s/ *$//;

			if (/([a-zA-Z0-9_]+)$/) {
				$p_name = $1;
				$_ =~ s/$p_name$//;
				$p_type{$p_name} = $_;
			}
		}

		print H "<a name=\"$name\">\n";
		print H "<div class=func>\n$name\n</div>\n";
		print H "<div class=header>\nDzia�anie:\n</div>\n";
		print H "<div class=desc>\n$descr\n</div>\n";

		print H "<div class=header>\nDeklaracja:\n</div>\n";
		print H "<div class=decl>\n$decl\n</div>\n";
		if (%p_descr) {
			print H "<div class=header>\nParametry:\n</div>\n";
			print H "<div class=params>\n<table cellspacing=1 border=0 class=params>\n";

			foreach $i (sort keys %p_descr) {
				$name = $i;
				$name =~ s/^[0-9]* //g;
				$type = $p_type{$name};
				print H "<tr><td class=paramname>$type<i>$name</i></td><td class=paramdescr>" . $p_descr{$i} . "</td></tr>\n";
			}
		}

		print H "</table>\n</div>\n";
		print H "<div class=header>\nZwracana warto��:\n</div>\n";
		print H "<div class=result>\n$result\n</div>\n";
	}

	close(F);
	
}

print H "</body>\n</html>\n";
close(H);

sub uc_char($)
{
	my ($ch) = @_;

	$ch =~ y/a-z����󶿼/A-Z��ʣ�Ӧ��/;

	return $ch;
}

sub uc_my()
{
	my ($str) = @_;

	$str =~ s/ +/ /g;

	$str =~ s/^(.)/uc_char($1)/eg;
	$str =~ s/\. ([a-z����󶿼])/sprintf(". %s", uc_char($1))/eg;
	$str =~ s/\"([^"]*)\"/"<tt>$1<\/tt>"/g;
	$str =~ s/\'([^']*)\'/'<tt>$1<\/tt>'/g;
	$str =~ s/([a-zA-Z0-9_]+\(\))/<tt>$1<\/tt>/g;
	$str =~ s/(gg_[a-zA-Z0-9_]+)\(\)/<a href="#$1">$1()<\/a>/g;
	$str =~ s/(GG_[A-Z0-9_]+)/<tt>$1<\/tt>/g;
	$str =~ s/NULL/<tt>NULL<\/tt>/g;

	return $str;
}
