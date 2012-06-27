#!/usr/bin/perl
$drop=15.0; #this drop will be marked by drop! (or drop? in case there was an undo before)
$lastp=0.0;
$move=1;
$gamename="";
while (<>)
{
    if (/FEINER: Undo requested, request will be granted/)
    {
    $undo="  (undo done during game)";
    print $_;
    }
    if (/un:/)
      {print "\t $_";}
    if (/rd:/)
      {print "\t $_";}
    if (/pv:/)
      {print "\t $_";}
    if (/(.+) com.gokgs.client.gtp.a <init>/)
      {
      $gamename="$1 - ";
      print "$1 - ";}
    if (/Starting game as (.+) against (.+)/) 
      { $undo="";
        $gamename=$gamename."$2 $1";
        $lastp=0.0;
        $move=1;
        print "$2 $1\n";
        }
    if (/r:([\d\.]+)%/)
      { if ($1+$drop<$lastp)
          {
          if ($undo eq "")
            {print "drop!\t$gamename -- move: $move$undo\n";}
            else
            {print "drop?\t$gamename -- move: $move$undo\n";}
          }
        $lastp=$1;
#        print "$1%\t";
        }
    if (/Got successful response to command \"play (.) (.+)\"/ )
      {
      $move++;
      print "\t$1$2\n"};
    if (/to command \"genmove (.)\": = (.+)/ ) 
      {
      $move++;
      print "$lastp%\t$1$2\n"};
}