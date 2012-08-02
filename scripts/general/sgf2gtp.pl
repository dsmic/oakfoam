#!/usr/bin/perl
$handicap=0;
while (<>)
{
  if (/HA\[(\d+)\]/) {print "Handicap game $1\n"; $handicap=1;};
  if (/SZ\[(\d+)\]/) {print "boardsize $1\n";};
  $tmp=$_;
  while ($tmp=~/;([WB])\[(\w)(\w)\](.*)/)
  {
   $m=uc($2);
   $m=~tr/ABCDEFGHIJKLMNOPQRSTUVWXY/ABCDEFGHJKLMNOPQRSTUVWXYZ/;
   $n=ord($3)-ord('a')+1;
   #print "$1 $m $n --- $4\n";
   if ($handicap==0) {print "play $1 $m$n\n";}
   $tmp=$4;
  }
}
