#!/usr/bin/perl
# This script uses the regression test output files .html (not .out.html) and modifies the .tst files
# resulting in a .new.tst file
# the new file has marked all tests with * (expected to fail), which failed in the test.
#
# usage: copy the .html files to the directory containing the .tst files
# ls -1 *.html|./create_expected.pl
#
# The script relies on some espects of the html file, eg the colors behind the fail FAIL.
# If it does not work anymore, somebody might have polished the colors:)
#
# 2014, Detlef Schmicker

while (<>)
{
print "$_\n";
$filename=$_;
if (!($filename=~/.html/)) {die "no htmlfile $filename\n";};
open(HTML,"< $filename") || die "can not open $filename\n";
$line="";
undef %result;
 while (<HTML>)
 {
  if ($line ne "")
   {
    if (/bgcolor="#e0e0e0">fail</) {$result{$line}="*"; print "$line $result{$line}\n"; $line=""};
    if (/bgcolor="#ff5454">FAIL</) {$result{$line}="*"; print "$line $result{$line}\n"; $line=""};
   }
  if (/#(\d+)">/) {$line=$1;}
 }
close HTML;
#debugging only
#foreach $element (keys %result) 
# {
#  print "$element $result{$element}\n";
# }
$filenameOut=$filename;	
$filename=~ s/\.html/\.tst/;
$filenameOut=~ s/\.html/\.new.tst/;
print "$filename $filenameOut\n";
open(TST,"< $filename") || die "can not open $filename\n";
open(NEWTST,"> $filenameOut") || die "can not open $filenameOut\n";
while (<TST>)
 {
  if (/^(.*)$/) {$line=$1;};
  if (/^(\d+)/) {$linenr=$1;};
  if (/^(#\?.+)\*$/) {$line=$1;};
  if (/^#\?(.*)$/) {$line="$line$result{$linenr}";};
  print NEWTST "$line\n";
 }
}
