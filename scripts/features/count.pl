#!/usr/bin/perl
$A=0;
while (<>)
{
/(\d+)/;
$A+=$1;
print "$A $_"
}
print "$A\n";
