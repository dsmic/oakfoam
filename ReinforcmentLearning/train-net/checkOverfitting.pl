#!/usr/bin/perl
$iter=0;
$nnn=0.0001;
$sum1=0;
$sum4=0;
$sum6=0;
$sumacc=0;
while (<>) {
  if (/Iteration (\d+)/) {$iter=$1;}
	if (/Train net output #\d: accloss1 = (\S+)/) { $loss1=$1; }
  if (/Train net output #\d: accloss4 = (\S+)/) { $loss4=$1; }
  if (/Train net output #\d: accloss6 = (\S+)/) { $loss6=$1; }
  if (/Train net output #\d: accu.* = (\S+)/) { $acc=$1; $sumacc+=$acc; $sum1+=$loss1; $sum4+=$loss4; $sum6+=$loss6; $nnn+=1}

	if (/Test net output #\d: accloss1 = (\S+)/) { $loss1=$1; }
	if (/Test net output #\d: accloss3 = (\S+)/) { $loss3=$1; }
	if (/Test net output #\d: accloss4 = (\S+)/) { $loss4=$1; }
	if (/Test net output #\d: accloss6 = (\S+)/) { $loss6=$1; } #print "$iter $loss $acc $loss1 $loss4 $loss5 $loss6\n";}
  if (/Test net output #\d: accu.* = (\S+)/) { $acc=$1; printf "testing at iteration %6d accurency: %.4f policy loss: %.4f value smooth: %.4f value: %.4f --train mean acc: %.4f pl: %.4f vs: %.4f v: %.4f\n",$iter,$acc,$loss1,$loss4,$loss6,$sumacc/$nnn,$sum1/$nnn,$sum4/$nnn,$sum6/$nnn;}
}
$meanacc=$sumacc/$nnn;
$mean1=$sum1/$nnn;
$mean4=$sum4/$nnn;
$mean6=$sum6/$nnn;
printf "mean of training            accurency: %.4f policy loss: %.4f value smooth: %.4f value: %.4f\n",$meanacc,$mean1,$mean4,$mean6;


