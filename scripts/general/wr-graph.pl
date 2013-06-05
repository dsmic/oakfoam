#!/usr/bin/perl
# This takes a autosaved sgf from oakfoam and plots a winrate graph
# can be used in a script to produce a lot of them I hope
$num=1;
$handy=0;
$firstwhite=0;
$color="W";
open(tmpfile,'>data.tmp');
while (<>)
{
    $title=$ARGV;
    if ($firstwhite == 0 && /;B\[/)
        {$num=1; $handy++;}
    if ($firstwhite == 0 && /;W\[/)
	{$num=2; $firstwhite=1;}
    
    if (/C\[r:([^ ]+)/)
    {
    print tmpfile "$num $1\n";
    if (/;B\[/) {$color="B";}
    }
    if (/;B\[/ || /;W\[/) {$num++;}
}
close(tmpfile);
if ($handy==1) {$handy=0;}
$title="$title playing $color handy $handy";
open (GP, "|gnuplot -persist") or die "no gnuplot";
# force buffer to flush after each write
use FileHandle;
GP->autoflush(1);
print GP "set term x11;set title '$title'; plot 'data.tmp' with lines\n";
close GP


