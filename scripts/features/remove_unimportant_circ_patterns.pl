#!/usr/bin/perl
#from a trained.gamma file the circular patterns with gammas between 1.0-$parameter and 1.0+parameter are removed
#use: ./remove_unimportant_circ_patterns.pl trained.gamma 0.3 
$num_args=$#ARGV+1;
#print STDERR "$num_args";
if ($num_args!=2) {print STDERR "\nA ratio to remove should have been provided!!!!\n"; $p=0.01;}
else
{
$p=$ARGV[1];
}
$file_name=$ARGV[0];
open ($file, '<',$file_name) or die $!;
#print "parameter ",$p,"\n";
print STDERR "Circular patterns between ",(1.0-$p)," and ",(1.0+$p)," are removed!!\n";
while (<$file>)
{
 if (/^(circpatt:.+) (.+)/)
   {if ($2<1.0-$p || $2>1.0+$p) {print "$1 $2\n";}}
 else
   {print "$_";}
}
