#
# File: tools/common/getmsgs.pl
#
# This is a perl script, used to automatically generate and check
# message files.
#

#
# If $ARGV[0] = "getmsg"
#	This goes through C source file(s), looking for printloc messages,
#	to generate an intermediate message file.
# If $ARGV[0] = "chkmsg"
#	This goes through the intermediate message files, checking for
#	conflicts/problems, and generates a file that something like 'gencat'
#	can be run on.
#

#
# $Copyright udi_reference:
# 
# 
#    Copyright (c) 1995-2001; Compaq Computer Corporation; Hewlett-Packard
#    Company; Interphase Corporation; The Santa Cruz Operation, Inc;
#    Software Technologies Group, Inc; and Sun Microsystems, Inc
#    (collectively, the "Copyright Holders").  All rights reserved.
# 
#    Redistribution and use in source and binary forms, with or without
#    modification, are permitted provided that the conditions are met:
# 
#            Redistributions of source code must retain the above
#            copyright notice, this list of conditions and the following
#            disclaimer.
# 
#            Redistributions in binary form must reproduce the above
#            copyright notice, this list of conditions and the following
#            disclaimers in the documentation and/or other materials
#            provided with the distribution.
# 
#            Neither the name of Project UDI nor the names of its
#            contributors may be used to endorse or promote products
#            derived from this software without specific prior written
#            permission.
# 
#    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#    "AS IS," AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#    HOLDERS OR ANY CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
#    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
#    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
#    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
#    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
#    TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
#    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
#    DAMAGE.
# 
#    THIS SOFTWARE IS BASED ON SOURCE CODE PROVIDED AS A SAMPLE REFERENCE
#    IMPLEMENTATION FOR VERSION 1.01 OF THE UDI CORE SPECIFICATION AND/OR
#    RELATED UDI SPECIFICATIONS. USE OF THIS SOFTWARE DOES NOT IN AND OF
#    ITSELF CONSTITUTE CONFORMANCE WITH THIS OR ANY OTHER VERSION OF ANY
#    UDI SPECIFICATION.
# 
# 
# $
#

# First, get data
$cont = 0;
$type = $crfile = "";
foreach $file (@ARGV) {
	if ($type eq "") {
		$type = $file;
		next;
	} elsif ($crfile eq "") {
		$crfile = $file;
		next;
	}

	if ($type eq "getmsg") {
		open(INFIL, "<$file");
		while (<INFIL>) {
			chomp;
			if ($cont) {
				$str .= $_;
				if (/\);/) {
					$str =~ s/\"[ \t]*\"//g;
					$str =~ s/^([^,]*,){2,2}//;
					$str =~ s/\",.*|\"\)\;.*/\"/g;
					$str =~ s/^[ \t]*//g;
					$str =~ s/^([0-9]+),[ \t]*/$1\t/g;
					push (@inp, "$str\n");
					$cont = 0;
				}
				next;
			}
			if (/printloc/ && !/^printloc/) {
				$str = $_;
				if (/\);/) {
					$str =~ s/\"[ \t]*\"//g;
					$str =~ s/^([^,]*,){2,2}//g;
					$str =~ s/\",.*|\"\)\;.*/\"/g;
					$str =~ s/^[ \t]*//g;
					$str =~ s/^([0-9]+),[ \t]*/$1\t/g;
					push (@inp, "$str\n");
				} else {
					$cont = 1;
				}
				next;
			}
			if (/printhloc/ && !/^printhloc/) {
				if (/\"/) {
					$str = $_;
					$str =~ s/.*printhloc[ \t]*\(//g;
					$str =~ s/\",.*|\"\).*/\"/g;
					$str =~ s/^[ \t]*//g;
					$str =~ s/^([0-9]+),[ \t]*/$1\t/g;
					push(@inp, "$str\n");
				}
			}
			if (/catgets/ && /\"/) {
				$str = $_;
				$str =~ s/.*catgets//g;
				$str =~ s/^([^,]*,){2,2}//g;
				$str =~ s/\",.*|\"\)\;.*|\"\)\).*|\"\),.*/\"/g;
				$str =~ s/^[ \t]*//g;
				$str =~ s/^([0-9]+),[ \t]*/$1\t/g;
				push (@inp, "$str\n");
			} else {
				if (/#define/ && /I18Nstr[0-9][0-9]/) {
					$str = $_;
					$str =~ s/^[^"]*"//g;
					$str =~ s/:.:/\t\"/g;
					push (@inp, "$str\n");
				}
			}
		}
		close(INFIL);
	} elsif ($type eq "chkmsg") {
		open(INFIL, "<$file");
		while (<INFIL>) {
			chomp;
			if (! /^\$ /) {
				$_ .= "|$file";
				push (@inp, $_);
			}
		}
		close(INFIL);
	}
}

# First, output the copyright file
open(INFIL, "<$crfile");
while (<INFIL>) {
	printf "\$ %s", $_;
}
close (INFIL);

# Then, output the messages
if ($type eq "chkmsg") {
	# First, miscellaneous info
	printf "\n\$quote \"\n";
	printf "\$set 1\n\n";

	# Then, the actual messages
	@inp = sort(@inp);
	$lmsgfil = ""; $error = 0;
	foreach (@inp) {
		($msgnum, $rest) = split("\t", $_, 2);
		($msgfmt, $junk) = split (/\|[^\|]*$/, $rest, 2);
		($junk, $msgfil) = split (/^.*\|/, $rest, 2);
		if ($lmsgfil) {
			if ($lmsgnum ne $msgnum) {
				$lmsgnum = $msgnum;
				$lmsgfmt = $msgfmt;
				$lmsgfil = $msgfil;
				printf "%s\t%s\n", $msgnum, $msgfmt;
			} elsif ($lmsgfmt ne $msgfmt) {
				printf stderr "Error: Differing messages:\n";
				printf stderr "    msgnum=%s, file=%s, msg=%s\n", $lmsgnum, $lmsgfil, $lmsgfmt;
				printf stderr "    msgnum=%s, file=%s, msg=%s\n", $msgnum, $msgfil, $msgfmt;
				$error = 1;
			}
		} else {
			$lmsgnum = $msgnum;
			$lmsgfmt = $msgfmt;
			$lmsgfil = $msgfil;
			printf "%s\t%s\n", $msgnum, $msgfmt;
		}
	}
	if ($error) {
		exit 1;
	}
} elsif ($type eq "getmsg") {
	@inp = sort(@inp);
	$lmsgnum = $lmsgfmt = "";
	foreach (@inp) {
		($msgnum, $msgfmt) = split("\t", $_, 2);
		if ($lmsgnum ne $msgnum || $msgfmt ne $msgfmt) {
			printf "%s", $_;
		}
		$lmsgnum = $msgnum;
		$lmsgfmt = $msgfmt;
	}
}
