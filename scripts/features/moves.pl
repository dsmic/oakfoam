#!/usr/bin/perl
while (<>)
{
 if (/^play +(.) +(\w+)/)
   {print "$1 $2\n";};
}
