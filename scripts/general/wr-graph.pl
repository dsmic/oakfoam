#!/usr/bin/perl
# This takes a autosaved sgf from oakfoam and plots a winrate graph
# can be used in a script to produce a lot of them I hope
$num=1;
$firstwhite=0;
open(tmpfile,'>data.tmp');
while (<>)
{
    $title=$ARGV;
    if ($firstwhite == 0 && /;B\[/)
        {$num=1;}
    if ($firstwhite == 0 && /;W\[/)
	{$num=2; $firstwhite=1;}
    
    if (/C\[r:([^ ]+)/)
    {
    print tmpfile "$num $1\n";
    }
    if (/;B\[/ || /;W\[/) {$num++;}
}
close(tmpfile);
open (GP, "|gnuplot -persist") or die "no gnuplot";
# force buffer to flush after each write
use FileHandle;
GP->autoflush(1);
print GP "set term x11;set title '$title'; plot 'data.tmp' with lines\n";
close GP


