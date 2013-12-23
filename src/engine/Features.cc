#include "Features.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include "Parameters.h"
//#include "Pattern.h"
#include "Engine.h"

//moved here, so it is not parsed any time feature.h is included
const std::string FEATURES_DEFAULT=
  "pass:1 0.950848 \n"
  "pass:2 85.7124 \n"
  "capture:1 3.33649 \n"
  "capture:2 3.33649 \n" // duplicated prev value after adding level
  "capture:3 3.33649 \n" // duplicated prev value after adding level
  "capture:4 3.33649 \n" // duplicated prev value after adding level
  "capture:5 50.0 \n" // hand-picked, after adding level
  "capture:6 1000.0 \n" // hand-picked, after adding level
  "extension:1 4.03331 \n"
  "extension:2 0.1 \n" // hand-picked, after adding level
  "selfatari:1 0.05 \n" // hand-picked, after fixing bug
  "selfatari:2 0.05 \n" // hand-picked, after adding level choosen to be no change. Change it by setting param selfatari:2!!!
  "atari:1 1.66722 \n"
  "atari:2 2.99897 \n"
  "atari:3 0.5 \n" // hand-picked, after adding level
  "borderdist:1 1.29595 \n"
  "borderdist:2 1.06265 \n"
  "borderdist:3 1.82725 \n"
  "borderdist:4 1.37265 \n"
  "lastdist:1 1 \n"
  "lastdist:2 10.2626 \n"
  "lastdist:3 5.9774 \n"
  "lastdist:4 4.50125 \n"
  "lastdist:5 3.54012 \n"
  "lastdist:6 2.45481 \n"
  "lastdist:7 2.25542 \n"
  "lastdist:8 1.87933 \n"
  "lastdist:9 1.66716 \n"
  "lastdist:10 1.42354 \n"
  "secondlastdist:1 1 \n"
  "secondlastdist:2 1.60416 \n"
  "secondlastdist:3 1.49505 \n"
  "secondlastdist:4 1.54885 \n"
  "secondlastdist:5 1.30675 \n"
  "secondlastdist:6 1.10097 \n"
  "secondlastdist:7 1.13203 \n"
  "secondlastdist:8 1.04496 \n"
  "secondlastdist:9 1.0764 \n"
  "secondlastdist:10 1.01713 \n"
  "cfglastdist:1 2.70474 \n"
  "cfglastdist:2 3.28603 \n"
  "cfglastdist:3 3.57841 \n"
  "cfglastdist:4 1.99003 \n"
  "cfglastdist:5 1.96744 \n"
  "cfglastdist:6 1.52841 \n"
  "cfglastdist:7 1.36206 \n"
  "cfglastdist:8 1.11015 \n"
  "cfglastdist:9 1.04867 \n"
  "cfglastdist:10 0.90182 \n"
  "cfgsecondlastdist:1 4.11636 \n"
  "cfgsecondlastdist:2 3.38945 \n"
  "cfgsecondlastdist:3 2.62745 \n"
  "cfgsecondlastdist:4 2.07085 \n"
  "cfgsecondlastdist:5 1.89433 \n"
  "cfgsecondlastdist:6 1.68709 \n"
  "cfgsecondlastdist:7 1.49819 \n"
  "cfgsecondlastdist:8 1.35704 \n"
  "cfgsecondlastdist:9 1.25574 \n"
  "cfgsecondlastdist:10 1.08943 \n"
  "pattern3x3:0x0000 0.3169 \n"
  "pattern3x3:0x0001 0.178076 \n"
  "pattern3x3:0x0002 0.265096 \n"
  "pattern3x3:0x0004 0.028978 \n"
  "pattern3x3:0x0005 0.0113561 \n"
  "pattern3x3:0x0006 1.91773 \n"
  "pattern3x3:0x0008 0.244349 \n"
  "pattern3x3:0x0009 2.44077 \n"
  "pattern3x3:0x000a 0.0283378 \n"
  "pattern3x3:0x0011 0.0974626 \n"
  "pattern3x3:0x0012 0.395731 \n"
  "pattern3x3:0x0015 0.0152617 \n"
  "pattern3x3:0x0016 0.15065 \n"
  "pattern3x3:0x0019 4.49223 \n"
  "pattern3x3:0x001a 0.528035 \n"
  "pattern3x3:0x0022 0.515834 \n"
  "pattern3x3:0x0026 3.89694 \n"
  "pattern3x3:0x002a 0.00690897 \n"
  "pattern3x3:0x003f 0.00513808 \n"
  "pattern3x3:0x0044 0.000739214 \n"
  "pattern3x3:0x0045 0.00163146 \n"
  "pattern3x3:0x0046 1.17942 \n"
  "pattern3x3:0x0048 1.74618 \n"
  "pattern3x3:0x0049 1.39988 \n"
  "pattern3x3:0x004a 1.28847 \n"
  "pattern3x3:0x0050 0.0353479 \n"
  "pattern3x3:0x0051 0.0226909 \n"
  "pattern3x3:0x0052 1.60713 \n"
  "pattern3x3:0x0054 0.00353866 \n"
  "pattern3x3:0x0055 0.00110071 \n"
  "pattern3x3:0x0056 0.336042 \n"
  "pattern3x3:0x0058 5.39188 \n"
  "pattern3x3:0x0059 1.49302 \n"
  "pattern3x3:0x005a 5.38181 \n"
  "pattern3x3:0x0060 0.100118 \n"
  "pattern3x3:0x0061 0.0203204 \n"
  "pattern3x3:0x0062 1.61518 \n"
  "pattern3x3:0x0064 0.0331937 \n"
  "pattern3x3:0x0065 0.009892 \n"
  "pattern3x3:0x0066 3.41003 \n"
  "pattern3x3:0x0068 0.197171 \n"
  "pattern3x3:0x0069 0.27017 \n"
  "pattern3x3:0x006a 0.184645 \n"
  "pattern3x3:0x007f 0.0371996 \n"
  "pattern3x3:0x0088 0.00935251 \n"
  "pattern3x3:0x0089 1.20537 \n"
  "pattern3x3:0x008a 0.0020205 \n"
  "pattern3x3:0x0090 0.382538 \n"
  "pattern3x3:0x0091 2.11507 \n"
  "pattern3x3:0x0092 0.140235 \n"
  "pattern3x3:0x0094 0.358081 \n"
  "pattern3x3:0x0095 0.551784 \n"
  "pattern3x3:0x0096 0.341306 \n"
  "pattern3x3:0x0098 0.722389 \n"
  "pattern3x3:0x0099 2.21167 \n"
  "pattern3x3:0x009a 0.195355 \n"
  "pattern3x3:0x00a0 0.255404 \n"
  "pattern3x3:0x00a1 1.74019 \n"
  "pattern3x3:0x00a2 0.0399687 \n"
  "pattern3x3:0x00a4 4.80253 \n"
  "pattern3x3:0x00a5 5.05189 \n"
  "pattern3x3:0x00a6 0.976087 \n"
  "pattern3x3:0x00a8 0.00419701 \n"
  "pattern3x3:0x00a9 0.391538 \n"
  "pattern3x3:0x00aa 0.000701979 \n"
  "pattern3x3:0x00bf 0.114303 \n"
  "pattern3x3:0x0140 0.00461152 \n"
  "pattern3x3:0x0141 0.00561664 \n"
  "pattern3x3:0x0142 0.319524 \n"
  "pattern3x3:0x0144 0.00263963 \n"
  "pattern3x3:0x0145 0.00350956 \n"
  "pattern3x3:0x0146 0.0102804 \n"
  "pattern3x3:0x0148 4.03235 \n"
  "pattern3x3:0x0149 0.869131 \n"
  "pattern3x3:0x014a 1.45255 \n"
  "pattern3x3:0x0151 0.0027213 \n"
  "pattern3x3:0x0152 0.213133 \n"
  "pattern3x3:0x0155 0.00817308 \n"
  "pattern3x3:0x0156 0.0063072 \n"
  "pattern3x3:0x0159 0.439079 \n"
  "pattern3x3:0x015a 1.54966 \n"
  "pattern3x3:0x0162 0.907499 \n"
  "pattern3x3:0x0166 2.41041 \n"
  "pattern3x3:0x016a 0.805201 \n"
  "pattern3x3:0x0180 0.123449 \n"
  "pattern3x3:0x0181 1.15505 \n"
  "pattern3x3:0x0182 0.0761826 \n"
  "pattern3x3:0x0184 0.224645 \n"
  "pattern3x3:0x0185 0.195341 \n"
  "pattern3x3:0x0186 0.134097 \n"
  "pattern3x3:0x0188 0.147033 \n"
  "pattern3x3:0x0189 3.51817 \n"
  "pattern3x3:0x018a 0.0558392 \n"
  "pattern3x3:0x0190 0.107069 \n"
  "pattern3x3:0x0191 0.528108 \n"
  "pattern3x3:0x0192 0.0157457 \n"
  "pattern3x3:0x0194 0.058724 \n"
  "pattern3x3:0x0195 0.0840886 \n"
  "pattern3x3:0x0196 0.0332323 \n"
  "pattern3x3:0x0198 0.452776 \n"
  "pattern3x3:0x0199 1.76364 \n"
  "pattern3x3:0x019a 0.134929 \n"
  "pattern3x3:0x01a0 1.22623 \n"
  "pattern3x3:0x01a2 0.458491 \n"
  "pattern3x3:0x01a4 4.28042 \n"
  "pattern3x3:0x01a5 2.81771 \n"
  "pattern3x3:0x01a6 1.23787 \n"
  "pattern3x3:0x01a8 0.218194 \n"
  "pattern3x3:0x01a9 1.62469 \n"
  "pattern3x3:0x01aa 0.0605565 \n"
  "pattern3x3:0x0280 0.0418979 \n"
  "pattern3x3:0x0281 0.85538 \n"
  "pattern3x3:0x0282 0.0347472 \n"
  "pattern3x3:0x0284 2.56654 \n"
  "pattern3x3:0x0285 1.93112 \n"
  "pattern3x3:0x0286 1.71499 \n"
  "pattern3x3:0x0288 0.0023455 \n"
  "pattern3x3:0x0289 0.0208569 \n"
  "pattern3x3:0x028a 0.00285014 \n"
  "pattern3x3:0x0291 1.7667 \n"
  "pattern3x3:0x0292 0.508823 \n"
  "pattern3x3:0x0295 1.86092 \n"
  "pattern3x3:0x0296 2.09398 \n"
  "pattern3x3:0x0299 0.60295 \n"
  "pattern3x3:0x029a 0.0252708 \n"
  "pattern3x3:0x02a2 0.017232 \n"
  "pattern3x3:0x02a6 0.964446 \n"
  "pattern3x3:0x02aa 0.00678181 \n"
  "pattern3x3:0x0410 0.230134 \n"
  "pattern3x3:0x0411 0.0388674 \n"
  "pattern3x3:0x0412 0.31964 \n"
  "pattern3x3:0x0414 0.0210027 \n"
  "pattern3x3:0x0415 0.0163083 \n"
  "pattern3x3:0x0416 0.150827 \n"
  "pattern3x3:0x0418 1.72155 \n"
  "pattern3x3:0x041a 0.953434 \n"
  "pattern3x3:0x0420 0.381892 \n"
  "pattern3x3:0x0421 0.116461 \n"
  "pattern3x3:0x0422 0.538302 \n"
  "pattern3x3:0x0424 1.32866 \n"
  "pattern3x3:0x0425 0.238163 \n"
  "pattern3x3:0x0428 0.0917606 \n"
  "pattern3x3:0x0429 0.559717 \n"
  "pattern3x3:0x042a 0.0250187 \n"
  "pattern3x3:0x043f 0.0573189 \n"
  "pattern3x3:0x0454 0.0086989 \n"
  "pattern3x3:0x0455 0.00363242 \n"
  "pattern3x3:0x0456 0.125231 \n"
  "pattern3x3:0x0459 0.690122 \n"
  "pattern3x3:0x0460 0.050705 \n"
  "pattern3x3:0x0461 0.0156324 \n"
  "pattern3x3:0x0462 0.198086 \n"
  "pattern3x3:0x0464 0.0255843 \n"
  "pattern3x3:0x0465 0.00549858 \n"
  "pattern3x3:0x0466 1.81716 \n"
  "pattern3x3:0x0468 0.0498043 \n"
  "pattern3x3:0x0469 0.0861079 \n"
  "pattern3x3:0x046a 0.0555589 \n"
  "pattern3x3:0x047f 0.00545835 \n"
  "pattern3x3:0x04a0 1.93928 \n"
  "pattern3x3:0x04a2 0.269475 \n"
  "pattern3x3:0x04a8 0.34282 \n"
  "pattern3x3:0x04a9 1.16604 \n"
  "pattern3x3:0x04aa 0.0164883 \n"
  "pattern3x3:0x04bf 3.21992 \n"
  "pattern3x3:0x0501 0.0217577 \n"
  "pattern3x3:0x0502 0.0727257 \n"
  "pattern3x3:0x0504 0.0172506 \n"
  "pattern3x3:0x0505 0.0175791 \n"
  "pattern3x3:0x0506 0.0637121 \n"
  "pattern3x3:0x0508 0.385692 \n"
  "pattern3x3:0x050a 0.087971 \n"
  "pattern3x3:0x0511 0.0217302 \n"
  "pattern3x3:0x0512 0.0686765 \n"
  "pattern3x3:0x0515 0.0213077 \n"
  "pattern3x3:0x0518 0.47207 \n"
  "pattern3x3:0x051a 0.290982 \n"
  "pattern3x3:0x0521 0.316046 \n"
  "pattern3x3:0x0522 0.745774 \n"
  "pattern3x3:0x0524 0.274325 \n"
  "pattern3x3:0x0525 0.126076 \n"
  "pattern3x3:0x0528 0.485905 \n"
  "pattern3x3:0x052a 0.248595 \n"
  "pattern3x3:0x053f 0.142527 \n"
  "pattern3x3:0x0541 0.00765857 \n"
  "pattern3x3:0x0542 0.0639967 \n"
  "pattern3x3:0x0544 0.00341216 \n"
  "pattern3x3:0x0545 0.00464424 \n"
  "pattern3x3:0x0546 0.0130126 \n"
  "pattern3x3:0x0548 0.460714 \n"
  "pattern3x3:0x0549 0.325689 \n"
  "pattern3x3:0x054a 0.17622 \n"
  "pattern3x3:0x0550 0.0267714 \n"
  "pattern3x3:0x0551 0.00889929 \n"
  "pattern3x3:0x0552 0.0525095 \n"
  "pattern3x3:0x0554 0.00795041 \n"
  "pattern3x3:0x0555 0.00647691 \n"
  "pattern3x3:0x0556 0.00476828 \n"
  "pattern3x3:0x0558 0.342864 \n"
  "pattern3x3:0x0559 0.33371 \n"
  "pattern3x3:0x055a 0.543938 \n"
  "pattern3x3:0x0560 0.180048 \n"
  "pattern3x3:0x0561 0.113754 \n"
  "pattern3x3:0x0562 0.28632 \n"
  "pattern3x3:0x0564 0.0282755 \n"
  "pattern3x3:0x0565 0.00754627 \n"
  "pattern3x3:0x0566 1.39811 \n"
  "pattern3x3:0x0568 0.319348 \n"
  "pattern3x3:0x0569 0.651995 \n"
  "pattern3x3:0x056a 0.450457 \n"
  "pattern3x3:0x0582 0.554613 \n"
  "pattern3x3:0x0585 0.450115 \n"
  "pattern3x3:0x0588 1.20178 \n"
  "pattern3x3:0x0590 0.504903 \n"
  "pattern3x3:0x0592 0.462714 \n"
  "pattern3x3:0x0598 1.5994 \n"
  "pattern3x3:0x05a8 1.23717 \n"
  "pattern3x3:0x05aa 0.447367 \n"
  "pattern3x3:0x0601 0.119776 \n"
  "pattern3x3:0x0602 0.270185 \n"
  "pattern3x3:0x0605 0.319132 \n"
  "pattern3x3:0x0608 0.0795501 \n"
  "pattern3x3:0x0609 0.750543 \n"
  "pattern3x3:0x060a 0.0746398 \n"
  "pattern3x3:0x0611 1.11974 \n"
  "pattern3x3:0x0615 0.179119 \n"
  "pattern3x3:0x061a 1.08356 \n"
  "pattern3x3:0x0621 0.0289954 \n"
  "pattern3x3:0x0622 0.189035 \n"
  "pattern3x3:0x0625 0.10552 \n"
  "pattern3x3:0x062a 0.0162428 \n"
  "pattern3x3:0x0641 0.0463225 \n"
  "pattern3x3:0x0642 0.227544 \n"
  "pattern3x3:0x0644 0.0867799 \n"
  "pattern3x3:0x0645 0.00903169 \n"
  "pattern3x3:0x0646 3.2691 \n"
  "pattern3x3:0x0648 0.0797801 \n"
  "pattern3x3:0x0649 0.245266 \n"
  "pattern3x3:0x064a 0.140442 \n"
  "pattern3x3:0x0651 0.318974 \n"
  "pattern3x3:0x0654 0.147294 \n"
  "pattern3x3:0x0655 0.0312209 \n"
  "pattern3x3:0x0660 0.0412666 \n"
  "pattern3x3:0x0661 0.0169536 \n"
  "pattern3x3:0x0662 0.240616 \n"
  "pattern3x3:0x0664 0.112055 \n"
  "pattern3x3:0x0665 0.0237484 \n"
  "pattern3x3:0x0669 0.0400603 \n"
  "pattern3x3:0x066a 0.00685103 \n"
  "pattern3x3:0x0682 0.117269 \n"
  "pattern3x3:0x0688 0.00857485 \n"
  "pattern3x3:0x0689 0.997543 \n"
  "pattern3x3:0x0698 0.0704319 \n"
  "pattern3x3:0x06a0 0.451521 \n"
  "pattern3x3:0x06a2 0.0981353 \n"
  "pattern3x3:0x0820 0.707226 \n"
  "pattern3x3:0x0821 1.4019 \n"
  "pattern3x3:0x0822 0.50197 \n"
  "pattern3x3:0x0824 3.26627 \n"
  "pattern3x3:0x0825 0.888087 \n"
  "pattern3x3:0x0828 0.058823 \n"
  "pattern3x3:0x0829 0.441991 \n"
  "pattern3x3:0x082a 0.0379323 \n"
  "pattern3x3:0x083f 0.0202856 \n"
  "pattern3x3:0x086a 0.343363 \n"
  "pattern3x3:0x08a8 0.00912074 \n"
  "pattern3x3:0x08a9 0.190676 \n"
  "pattern3x3:0x08aa 0.00738604 \n"
  "pattern3x3:0x08bf 0.00287895 \n"
  "pattern3x3:0x0902 0.291647 \n"
  "pattern3x3:0x0904 0.0190386 \n"
  "pattern3x3:0x0905 0.0115569 \n"
  "pattern3x3:0x0906 0.118479 \n"
  "pattern3x3:0x0908 0.872731 \n"
  "pattern3x3:0x090a 0.209398 \n"
  "pattern3x3:0x0911 0.0220698 \n"
  "pattern3x3:0x0912 0.155025 \n"
  "pattern3x3:0x0915 0.0202021 \n"
  "pattern3x3:0x0918 0.807992 \n"
  "pattern3x3:0x091a 0.271327 \n"
  "pattern3x3:0x0922 1.67042 \n"
  "pattern3x3:0x0925 0.850954 \n"
  "pattern3x3:0x0928 0.949061 \n"
  "pattern3x3:0x092a 0.215518 \n"
  "pattern3x3:0x0944 0.0314109 \n"
  "pattern3x3:0x0946 1.86487 \n"
  "pattern3x3:0x0951 0.0328674 \n"
  "pattern3x3:0x0964 0.22672 \n"
  "pattern3x3:0x0982 0.0343041 \n"
  "pattern3x3:0x0984 0.0760261 \n"
  "pattern3x3:0x0985 0.0790672 \n"
  "pattern3x3:0x0986 0.065093 \n"
  "pattern3x3:0x0988 0.0514482 \n"
  "pattern3x3:0x0989 1.2025 \n"
  "pattern3x3:0x098a 0.0347708 \n"
  "pattern3x3:0x0991 0.275986 \n"
  "pattern3x3:0x0992 0.0175108 \n"
  "pattern3x3:0x0995 0.0170564 \n"
  "pattern3x3:0x0996 0.0329883 \n"
  "pattern3x3:0x0998 0.232903 \n"
  "pattern3x3:0x099a 0.0350457 \n"
  "pattern3x3:0x09a0 0.494273 \n"
  "pattern3x3:0x09a2 0.231487 \n"
  "pattern3x3:0x09a8 0.139931 \n"
  "pattern3x3:0x09aa 0.0223557 \n"
  "pattern3x3:0x0a02 0.209958 \n"
  "pattern3x3:0x0a05 0.258416 \n"
  "pattern3x3:0x0a08 0.0190405 \n"
  "pattern3x3:0x0a09 0.374975 \n"
  "pattern3x3:0x0a0a 0.0202473 \n"
  "pattern3x3:0x0a11 1.54064 \n"
  "pattern3x3:0x0a12 1.14157 \n"
  "pattern3x3:0x0a15 0.347784 \n"
  "pattern3x3:0x0a18 0.502752 \n"
  "pattern3x3:0x0a1a 0.238572 \n"
  "pattern3x3:0x0a21 0.114062 \n"
  "pattern3x3:0x0a22 0.0704645 \n"
  "pattern3x3:0x0a25 0.37631 \n"
  "pattern3x3:0x0a2a 0.00352149 \n"
  "pattern3x3:0x0a3f 0.102764 \n"
  "pattern3x3:0x0a44 0.611397 \n"
  "pattern3x3:0x0a4a 0.377419 \n"
  "pattern3x3:0x0a54 0.757207 \n"
  "pattern3x3:0x0a55 0.221629 \n"
  "pattern3x3:0x0a61 0.220529 \n"
  "pattern3x3:0x0a64 0.569516 \n"
  "pattern3x3:0x0a82 0.0369822 \n"
  "pattern3x3:0x0a84 0.658369 \n"
  "pattern3x3:0x0a85 0.771024 \n"
  "pattern3x3:0x0a86 0.551099 \n"
  "pattern3x3:0x0a88 0.00306949 \n"
  "pattern3x3:0x0a89 0.0614102 \n"
  "pattern3x3:0x0a8a 0.00401333 \n"
  "pattern3x3:0x0a91 0.582888 \n"
  "pattern3x3:0x0a92 0.289868 \n"
  "pattern3x3:0x0a94 0.816247 \n"
  "pattern3x3:0x0a95 0.900427 \n"
  "pattern3x3:0x0a96 1.03464 \n"
  "pattern3x3:0x0a98 0.0359749 \n"
  "pattern3x3:0x0a99 0.627937 \n"
  "pattern3x3:0x0a9a 0.0569129 \n"
  "pattern3x3:0x0aa0 0.0191593 \n"
  "pattern3x3:0x0aa1 0.182586 \n"
  "pattern3x3:0x0aa2 0.00966921 \n"
  "pattern3x3:0x0aa4 0.596881 \n"
  "pattern3x3:0x0aa5 0.774611 \n"
  "pattern3x3:0x0aa6 0.594122 \n"
  "pattern3x3:0x0aa8 0.00711843 \n"
  "pattern3x3:0x0aa9 0.0480371 \n"
  "pattern3x3:0x0aaa 0.00557634 \n"
  "pattern3x3:0x0cff 0.00032485 \n"
  "pattern3x3:0x0dc3 0.00742523 \n"
  "pattern3x3:0x0dc7 0.00286252 \n"
  "pattern3x3:0x0dd3 0.00474466 \n"
  "pattern3x3:0x0dd7 0.0046897 \n"
  "pattern3x3:0x0ddb 1.73068 \n"
  "pattern3x3:0x0de3 0.159344 \n"
  "pattern3x3:0x0de7 0.682544 \n"
  "pattern3x3:0x0deb 0.50659 \n"
  "pattern3x3:0x0dff 0.0136076 \n"
  "pattern3x3:0x0ec3 0.0184223 \n"
  "pattern3x3:0x0ecb 0.05268 \n"
  "pattern3x3:0x0ed3 0.43726 \n"
  "pattern3x3:0x0ed7 0.419727 \n"
  "pattern3x3:0x0edb 0.201201 \n"
  "pattern3x3:0x0ee3 0.0124976 \n"
  "pattern3x3:0x0ee7 0.815867 \n"
  "pattern3x3:0x0eeb 0.00457665 \n"
  "pattern3x3:0x0eff 0.00637201 \n"
  "pattern3x3:0x1145 0.00859099 \n"
  "pattern3x3:0x1146 0.00653497 \n"
  "pattern3x3:0x1149 0.160687 \n"
  "pattern3x3:0x114a 0.174622 \n"
  "pattern3x3:0x1151 0.00483099 \n"
  "pattern3x3:0x1152 0.00922301 \n"
  "pattern3x3:0x1155 0.00913994 \n"
  "pattern3x3:0x1156 0.00182422 \n"
  "pattern3x3:0x115a 0.486391 \n"
  "pattern3x3:0x1166 0.445257 \n"
  "pattern3x3:0x116a 0.425063 \n"
  "pattern3x3:0x117f 0.00937591 \n"
  "pattern3x3:0x1192 0.0433714 \n"
  "pattern3x3:0x1198 0.207299 \n"
  "pattern3x3:0x11a8 0.210865 \n"
  "pattern3x3:0x12a2 0.207523 \n"
  "pattern3x3:0x1511 0.0070791 \n"
  "pattern3x3:0x1515 0.004155 \n"
  "pattern3x3:0x151a 0.172494 \n"
  "pattern3x3:0x1522 0.184912 \n"
  "pattern3x3:0x1525 0.008737 \n"
  "pattern3x3:0x152a 0.167984 \n"
  "pattern3x3:0x153f 0.00279046 \n"
  "pattern3x3:0x1556 0.0052047 \n"
  "pattern3x3:0x1559 0.272446 \n"
  "pattern3x3:0x155a 0.296748 \n"
  "pattern3x3:0x1564 0.0113644 \n"
  "pattern3x3:0x1565 0.00524931 \n"
  "pattern3x3:0x1566 0.254614 \n"
  "pattern3x3:0x1569 0.269794 \n"
  "pattern3x3:0x156a 0.337026 \n"
  "pattern3x3:0x157f 0.00516138 \n"
  "pattern3x3:0x1598 0.488391 \n"
  "pattern3x3:0x159a 0.335405 \n"
  "pattern3x3:0x15a2 0.337938 \n"
  "pattern3x3:0x15a4 0.825033 \n"
  "pattern3x3:0x15a8 0.413559 \n"
  "pattern3x3:0x15aa 0.272505 \n"
  "pattern3x3:0x15bf 0.61856 \n"
  "pattern3x3:0x1605 0.112348 \n"
  "pattern3x3:0x160a 0.0742834 \n"
  "pattern3x3:0x1615 0.170202 \n"
  "pattern3x3:0x1625 0.100749 \n"
  "pattern3x3:0x1688 0.456461 \n"
  "pattern3x3:0x16a2 0.225546 \n"
  "pattern3x3:0x1925 0.431621 \n"
  "pattern3x3:0x192a 0.357106 \n"
  "pattern3x3:0x193f 0.0678004 \n"
  "pattern3x3:0x1964 0.054997 \n"
  "pattern3x3:0x1965 0.588212 \n"
  "pattern3x3:0x197f 0.385651 \n"
  "pattern3x3:0x19a8 0.279044 \n"
  "pattern3x3:0x19aa 0.238129 \n"
  "pattern3x3:0x19bf 0.837744 \n"
  "pattern3x3:0x1a55 0.746873 \n"
  "pattern3x3:0x1a88 0.187071 \n"
  "pattern3x3:0x1a98 0.991644 \n"
  "pattern3x3:0x1a9a 0.443495 \n"
  "pattern3x3:0x1aa2 0.266415 \n"
  "pattern3x3:0x2292 0.032048 \n"
  "pattern3x3:0x2295 0.68353 \n"
  "pattern3x3:0x2296 0.488173 \n"
  "pattern3x3:0x2299 5.06803 \n"
  "pattern3x3:0x22a2 0.0040633 \n"
  "pattern3x3:0x2615 0.671149 \n"
  "pattern3x3:0x261a 0.162436 \n"
  "pattern3x3:0x2621 0.403009 \n"
  "pattern3x3:0x2625 0.488191 \n"
  "pattern3x3:0x263f 0.0572528 \n"
  "pattern3x3:0x2655 0.25863 \n"
  "pattern3x3:0x2661 0.187202 \n"
  "pattern3x3:0x2665 0.244659 \n"
  "pattern3x3:0x2669 0.282843 \n"
  "pattern3x3:0x266a 0.236048 \n"
  "pattern3x3:0x267f 0.988047 \n"
  "pattern3x3:0x26a2 0.114944 \n"
  "pattern3x3:0x26a9 2.86798 \n"
  "pattern3x3:0x2a22 0.0521432 \n"
  "pattern3x3:0x2a25 0.0990866 \n"
  "pattern3x3:0x2a2a 0.00764493 \n"
  "pattern3x3:0x2a3f 0.0613183 \n"
  "pattern3x3:0x2a69 0.193024 \n"
  "pattern3x3:0x2a7f 0.136573 \n"
  "pattern3x3:0x4412 0.0959079 \n"
  "pattern3x3:0x4415 0.0430546 \n"
  "pattern3x3:0x4416 0.0437664 \n"
  "pattern3x3:0x441a 0.715564 \n"
  "pattern3x3:0x4422 0.251677 \n"
  "pattern3x3:0x442a 0.0440514 \n"
  "pattern3x3:0x443f 0.146389 \n"
  "pattern3x3:0x4455 0.0617397 \n"
  "pattern3x3:0x4456 0.0578129 \n"
  "pattern3x3:0x445a 1.1686 \n"
  "pattern3x3:0x4461 0.0169009 \n"
  "pattern3x3:0x4462 0.189238 \n"
  "pattern3x3:0x4469 0.129627 \n"
  "pattern3x3:0x446a 0.110912 \n"
  "pattern3x3:0x447f 0.047141 \n"
  "pattern3x3:0x44a2 0.482304 \n"
  "pattern3x3:0x44aa 0.0854113 \n"
  "pattern3x3:0x4551 0.00598202 \n"
  "pattern3x3:0x4552 0.00957494 \n"
  "pattern3x3:0x4555 0.00907403 \n"
  "pattern3x3:0x4556 0.00111853 \n"
  "pattern3x3:0x4559 0.317888 \n"
  "pattern3x3:0x455a 0.33933 \n"
  "pattern3x3:0x4562 0.236354 \n"
  "pattern3x3:0x4566 0.494648 \n"
  "pattern3x3:0x456a 0.194412 \n"
  "pattern3x3:0x457f 0.0107105 \n"
  "pattern3x3:0x4592 0.292962 \n"
  "pattern3x3:0x4595 0.286039 \n"
  "pattern3x3:0x45a2 0.557932 \n"
  "pattern3x3:0x45aa 0.541364 \n"
  "pattern3x3:0x46a2 0.321522 \n"
  "pattern3x3:0x4822 0.761504 \n"
  "pattern3x3:0x482a 0.0948601 \n"
  "pattern3x3:0x483f 0.152583 \n"
  "pattern3x3:0x48a9 0.628689 \n"
  "pattern3x3:0x48bf 0.260251 \n"
  "pattern3x3:0x4912 0.103385 \n"
  "pattern3x3:0x4916 0.101734 \n"
  "pattern3x3:0x491a 0.248908 \n"
  "pattern3x3:0x4922 0.539241 \n"
  "pattern3x3:0x4925 0.73811 \n"
  "pattern3x3:0x492a 0.223941 \n"
  "pattern3x3:0x493f 0.0752281 \n"
  "pattern3x3:0x4965 0.50935 \n"
  "pattern3x3:0x4992 0.0139547 \n"
  "pattern3x3:0x4995 0.0590654 \n"
  "pattern3x3:0x4996 0.0378748 \n"
  "pattern3x3:0x499a 0.0450371 \n"
  "pattern3x3:0x49a2 0.274389 \n"
  "pattern3x3:0x49aa 0.040071 \n"
  "pattern3x3:0x4a22 0.267839 \n"
  "pattern3x3:0x4aa2 0.0847343 \n"
  "pattern3x3:0x4aa9 0.230171 \n"
  "pattern3x3:0x4cff 0.0146683 \n"
  "pattern3x3:0x4dd3 0.00557727 \n"
  "pattern3x3:0x4dd7 0.00295181 \n"
  "pattern3x3:0x4ddb 0.739046 \n"
  "pattern3x3:0x4de3 0.174653 \n"
  "pattern3x3:0x4de7 0.637262 \n"
  "pattern3x3:0x4deb 0.43421 \n"
  "pattern3x3:0x4dff 0.00613481 \n"
  "pattern3x3:0x4ed3 1.5475 \n"
  "pattern3x3:0x4ed7 0.84672 \n"
  "pattern3x3:0x4edb 0.614248 \n"
  "pattern3x3:0x4ee3 0.434605 \n"
  "pattern3x3:0x4eeb 0.126502 \n"
  "pattern3x3:0x4eff 0.245109 \n"
  "pattern3x3:0x5556 0.00217732 \n"
  "pattern3x3:0x555a 0.200083 \n"
  "pattern3x3:0x5566 0.275902 \n"
  "pattern3x3:0x556a 0.218245 \n"
  "pattern3x3:0x559a 0.16585 \n"
  "pattern3x3:0x55a2 0.46363 \n"
  "pattern3x3:0x55a5 0.705113 \n"
  "pattern3x3:0x55aa 0.197865 \n"
  "pattern3x3:0x55bf 0.189138 \n"
  "pattern3x3:0x56a2 0.354039 \n"
  "pattern3x3:0x56aa 0.142943 \n"
  "pattern3x3:0x593f 0.0405295 \n"
  "pattern3x3:0x5965 0.708688 \n"
  "pattern3x3:0x597f 0.444543 \n"
  "pattern3x3:0x59aa 0.141683 \n"
  "pattern3x3:0x5a22 0.929311 \n"
  "pattern3x3:0x5aa2 0.327799 \n"
  "pattern3x3:0x5aaa 0.142505 \n"
  "pattern3x3:0x5ee3 0.577096 \n"
  "pattern3x3:0x5eff 0.210335 \n"
  "pattern3x3:0x66a2 0.758969 \n"
  "pattern3x3:0x66aa 2.3224 \n"
  "pattern3x3:0x6a22 0.139143 \n"
  "pattern3x3:0x6a2a 0.0578737 \n"
  "pattern3x3:0x6a3f 0.415501 \n"
  "pattern3x3:0x882a 0.0704346 \n"
  "pattern3x3:0x883f 0.143601 \n"
  "pattern3x3:0x88aa 0.0470172 \n"
  "pattern3x3:0x88bf 0.0459199 \n"
  "pattern3x3:0x89aa 0.259133 \n"
  "pattern3x3:0x8aa2 0.0379365 \n"
  "pattern3x3:0x8aa6 0.374706 \n"
  "pattern3x3:0x8aaa 0.038405 \n"
  "pattern3x3:0x8abf 0.0181794 \n"
  "pattern3x3:0x8cff 0.00423266 \n"
  "pattern3x3:0x8de3 0.212511 \n"
  "pattern3x3:0x8de7 0.642001 \n"
  "pattern3x3:0x8deb 0.550659 \n"
  "pattern3x3:0x8dff 0.0572516 \n"
  "pattern3x3:0x8ee3 0.0127361 \n"
  "pattern3x3:0x8ee7 0.479957 \n"
  "pattern3x3:0x8eeb 0.0110146 \n"
  "pattern3x3:0x8eff 0.00451208 \n"
  "pattern3x3:0x9eeb 0.34918 \n"
  "pattern3x3:0x9eff 0.249719 \n"
  ;

void Features::loadGammaDefaults() 
{ this->loadGammaString(FEATURES_DEFAULT); 
}

Features::Features(Parameters *prms) : params(prms)
{
  patterngammas=new Pattern::ThreeByThreeGammas();
  patternids=new Pattern::ThreeByThreeGammas();
  
  for (int i=0;i<PASS_LEVELS;i++)
    gammas_pass[i]=1.0;
  for (int i=0;i<CAPTURE_LEVELS;i++)
    gammas_capture[i]=1.0;
  for (int i=0;i<EXTENSION_LEVELS;i++)
    gammas_extension[i]=1.0;
  for (int i=0;i<SELFATARI_LEVELS;i++)
    gammas_selfatari[i]=1.0;
  for (int i=0;i<ATARI_LEVELS;i++)
    gammas_atari[i]=1.0;
  for (int i=0;i<BORDERDIST_LEVELS;i++)
    gammas_borderdist[i]=1.0;
  for (int i=0;i<LASTDIST_LEVELS;i++)
    gammas_lastdist[i]=1.0;
  for (int i=0;i<SECONDLASTDIST_LEVELS;i++)
    gammas_secondlastdist[i]=1.0;
  for (int i=0;i<CFGLASTDIST_LEVELS;i++)
    gammas_cfglastdist[i]=1.0;
  for (int i=0;i<CFGSECONDLASTDIST_LEVELS;i++)
    gammas_cfgsecondlastdist[i]=1.0;

  circdict=new Pattern::CircularDictionary();
  circlevels = new std::map<Pattern::Circular,unsigned int>();
  circstrings = new std::vector<std::string>();
  circgammas = new std::vector<float>();
  
  circpatternsize=0;
  num_circmoves=0;
  num_circmoves_not=0;
}

Features::~Features()
{
  delete patterngammas;
  delete patternids;
  delete circdict;
  delete circgammas;
  delete circstrings;
  delete circlevels;
}

unsigned int Features::matchFeatureClass(Features::FeatureClass featclass, Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, Go::Move move, bool checkforvalidmove) const
{
  if ((featclass!=Features::PASS && move.isPass()) || move.isResign())
    return 0;
  
  if (checkforvalidmove && !board->validMove(move))
    return 0;

  if (!params->features_tactical && (featclass!=Features::PATTERN3X3) && (featclass!=Features::CIRCPATT))
    return 0; // disable tactical features
  
  switch (featclass)
  {
    case Features::PASS:
    {
      if (!move.isPass())
        return 0;
      else if (board->getPassesPlayed()==0)
        return 1;
      else
        return 2;
    }
    case Features::CAPTURE:
    {
      if (board->isCapture(move))
      {
        Go::Color col=move.getColor();
        Go::Color othercol=Go::otherColor(col);
        int pos=move.getPosition();
        int size=board->getSize();

        // check if adjacent group of captured group is in atari
        foreach_adjacent(pos,p,{
          if (board->inGroup(p))
          {
            Go::Group *group=board->getGroup(p);
            if (group->getColor()!=col && group->inAtari()) // captured group
            {
              Go::list_int *adjacentgroups=group->getAdjacentGroups();
              for(Go::list_int::iterator iter=adjacentgroups->begin();iter!=adjacentgroups->end();++iter)
              {
                if (board->inGroup((*iter)) && board->getGroup((*iter))->inAtari())
                {
                  if (group->numOfStones()>=10)
                    return 6;
                  else
                    return 5;
                }
              }
            }
          }
        });

        // check for re-capture
        if (board->isLastCapture() && board->getLastMove().isNormal())
        {
          int lastpos = board->getLastMove().getPosition();
          foreach_adjacent(pos,p,{
            if (board->inGroup(p))
            {
              Go::Group *group=board->getGroup(p);
              if (group->getColor()!=col && group->inAtari()) // captured group
              {
                if (board->getGroup(lastpos)==group)
                  return 4;
              }
            }
          });
        }

        // check if this prevents a connection
        foreach_adjacent(pos,p,{
          if (board->inGroup(p))
          {
            Go::Group *group=board->getGroup(p);
            if (group->getColor()!=col && !group->inAtari()) // another group
            {
              if (board->isExtension(Go::Move(othercol,pos)))
                return 3;
            }
          }
        });

        // check if in ladder
        if (params->features_ladders)
        {
          Go::Color col=move.getColor();
          int pos=move.getPosition();
          int size=board->getSize();
          foreach_adjacent(pos,p,{
            if (board->inGroup(p) && board->getColor(p)==col)
            {
              Go::Group *group=board->getGroup(p);
              if (board->isLadder(group) && !board->isProbableWorkingLadder(group))
                return 2;
            }
          });
        }

        return 1;
      }
      else
        return 0;
    }
    case Features::EXTENSION:
    {
      if (board->isExtension(move))
      {
        if (params->features_ladders)
        {
          Go::Color col=move.getColor();
          int pos=move.getPosition();
          int size=board->getSize();
          foreach_adjacent(pos,p,{
            if (board->inGroup(p) && board->getColor(p)==col)
            {
              Go::Group *group=board->getGroup(p);
              if (board->isLadder(group) && board->isProbableWorkingLadder(group))
                return 2;
            }
          });
        }
        return 1;
      }
      else
        return 0;
    }
    case Features::SELFATARI:
    {
      //of size uses the same parameters as in the playouts!!!!! 
      if (params->playout_avoid_selfatari_size>0 && board->isSelfAtariOfSize(move,params->playout_avoid_selfatari_size,params->playout_avoid_selfatari_complex))
        return 2;
      else
      {
        if (board->isSelfAtari(move))
          return 1;
        else
          return 0;
      }
    }
    case Features::ATARI:
    {
      if (board->isAtari(move))
      {
        if (params->features_ladders)
        {
          Go::Color col=move.getColor();
          int pos=move.getPosition();
          int size=board->getSize();
          foreach_adjacent(pos,p,{
            if (board->inGroup(p) && board->getColor(p)!=col)
            {
              Go::Group *group=board->getGroup(p);
              if (board->isLadderAfter(group,move) && !board->isProbableWorkingLadderAfter(group,move))
                return 3;
            }
          });
        }
        if (board->isCurrentSimpleKo())
          return 2;
        else
          return 1;
      }
      else
        return 0;
    }
    case Features::BORDERDIST:
    {
      int dist=board->getDistanceToBorder(move.getPosition());
      
      if (dist<BORDERDIST_LEVELS)
        return (dist+1);
      else
        return 0;
    }
    case Features::LASTDIST:
    {
      if (params->features_history_agnostic)
        return 0;
      if (board->getLastMove().isResign() || (!params->features_pass_no_move_for_lastdist && board->getLastMove().isPass()))
        return 0;
      //this returned 0 in case of pass before. This lead playing bad after a pass, as it
      //tends to let the opponent play at an other place, which might be bad.
      if (board->getLastMove().isPass())
      {
        if (board->getSecondLastMove().isPass() || board->getSecondLastMove().isResign())
          return 0;
        int dist=board->getCircularDistance(move.getPosition(),board->getSecondLastMove().getPosition());
        
        int maxdist=SECONDLASTDIST_LEVELS;
        if (params->features_only_small)
          maxdist=3;
        
        if (dist<=maxdist)
          return dist;
        else
          return 0;
      }
      
      int dist=board->getCircularDistance(move.getPosition(),board->getLastMove().getPosition());
      
      int maxdist=LASTDIST_LEVELS;
      if (params->features_only_small)
        maxdist=3;
      
      if (dist<=maxdist)
        return dist;
      else
        return 0;
    }
    case Features::SECONDLASTDIST:
    {
      if (params->features_history_agnostic)
        return 0;
      if (board->getSecondLastMove().isPass() || board->getSecondLastMove().isResign())
        return 0;
      //ignore second last, if last was pass
      if (params->features_pass_no_move_for_lastdist && board->getLastMove().isPass())
        return 0;
      
      int dist=board->getCircularDistance(move.getPosition(),board->getSecondLastMove().getPosition());
      
      int maxdist=SECONDLASTDIST_LEVELS;
      if (params->features_only_small)
        maxdist=3;
      
      if (dist<=maxdist)
        return dist;
      else
        return 0;
    }
    case Features::CFGLASTDIST:
    {
      if (params->features_history_agnostic)
        return 0;
      if (board->getLastMove().isPass() || board->getLastMove().isResign())
        return 0;
      
      if (cfglastdist==NULL)
        return 0;
      
      int dist=cfglastdist->get(move.getPosition());
      if (dist==-1)
        return 0;
      
      if (dist<=CFGLASTDIST_LEVELS)
        return dist;
      else
        return 0;
    }
    case Features::CFGSECONDLASTDIST:
    {
      if (params->features_history_agnostic)
        return 0;
      if (board->getLastMove().isPass() || board->getLastMove().isResign())
        return 0;
      
      if (cfgsecondlastdist==NULL)
        return 0;
      
      int dist=cfgsecondlastdist->get(move.getPosition());
      if (dist==-1)
        return 0;
      
      if (dist<=CFGSECONDLASTDIST_LEVELS)
        return dist;
      else
        return 0;
    }
    case Features::PATTERN3X3:
    {
      Go::Color col=move.getColor();
      int pos=move.getPosition();
      
      unsigned int hash=Pattern::ThreeByThree::makeHash(board,pos);
      if (col==Go::WHITE)
        hash=Pattern::ThreeByThree::invert(hash);
      hash=Pattern::ThreeByThree::smallestEquivalent(hash);
      
      return hash;
    }
    case Features::CIRCPATT:
    {
      Go::Color col=move.getColor();
      int pos=move.getPosition();
      
      Pattern::Circular pattcirc = Pattern::Circular(circdict,board,pos,PATTERN_CIRC_MAXSIZE);
      if (col == Go::WHITE)
        pattcirc.invert();
      pattcirc.convertToSmallestEquivalent(circdict);

      for (int s=PATTERN_CIRC_MAXSIZE;s>=3;s--)
      {
        Pattern::Circular pc = pattcirc.getSubPattern(circdict,s);
        if (circlevels->count(pc)>0)
          return (*circlevels)[pc];
      }
      return 0;
    }
    default:
      return 0;
  }
}

float Features::getFeatureGamma(Features::FeatureClass featclass, unsigned int level) const
{
  if (level==0 && featclass!=Features::PATTERN3X3)
    return 1.0;
  
  if (featclass==Features::PATTERN3X3)
  {
    if (patterngammas->hasGamma(level))
      return patterngammas->getGamma(level);
    else
      return 1.0;
  }
  else if (featclass==Features::CIRCPATT)
  {
    if (circgammas->size()>level)
      return (*circgammas)[level];
    else
      return 1.0;
  }
  else
  {
    float *gammas=this->getStandardGamma(featclass);
    if (gammas!=NULL)
    {
      float gamma=gammas[level-1];
      if (gamma>0)
        return gamma;
      else
        return 1.0;
    }
    else
      return 1.0;
  }
}

int Features::learnFeatureGammaC(Features::FeatureClass featclass, unsigned int level, float learn_diff)
{
  if (level==0 && featclass!=Features::PATTERN3X3)
    return 0;

  
  if (featclass==Features::PATTERN3X3)
  {
    if (patterngammas->hasGamma(level))
    {
      //patterngammas->learnGamma(level,learn_diff);
      return 1;
    }
    else
      return 0;
  }
  else if (featclass==Features::CIRCPATT)
  {
    if (circgammas->size()>level)
      return 1; //(*circgammas)[level];
    else
      return 0;
  }
  else
  {
    float *gammas=this->getStandardGamma(featclass);
    if (gammas!=NULL)
    {
      if (gammas[level-1]>0)
      {
        //gammas[level-1]+=learn_diff;
        //if (gammas[level-1]<=0.0) 
        //  gammas[level-1]=0.001;
        return 1;
      }
      else
        return 0;
    }
    else
      return 0;
  }
}

void Features::learnFeatureGammaMoves(Features::FeatureClass featclass, Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, std::map<float,Go::Move,std::greater<float> > ordervalue, std::map<int,float> move_gamma, float sum_gammas)
{

  std::map<float,Go::Move>::iterator it;
  std::map<long int,bool> used_moves_levels;
  int win=1;
  for (it=ordervalue.begin();it!=ordervalue.end();++it)
  {
    Go::Move move_1=it->second;
    int level_1=matchFeatureClass(featclass,board,cfglastdist,cfgsecondlastdist,move_1,true);
    float C_iM_gamma_i=0;
    //float gammaHERE=0;
    std::map<float,Go::Move>::iterator it_int;
    for (it_int=it;it_int!=ordervalue.end();++it_int)
    {
      Go::Move move=it_int->second;
      int level=matchFeatureClass(featclass,board,cfglastdist,cfgsecondlastdist,move,true);
      if (level==level_1 && !used_moves_levels.count(PATTERN_3x3_GAMMAS*move.getPosition()+level))
      {
        switch (win) //dummy to use the break statement and keep the code closer to other parts
        {
          default:
            if (level==0 && featclass!=Features::PATTERN3X3)
              break;
            if (featclass==Features::PATTERN3X3)
            {
              //used patterns should not be done by featurelevels_used, to much ram?!
              if (patterngammas->hasGamma(level))
              {
                //gammaHERE=patterngammas->getGamma(level);
                C_iM_gamma_i+=move_gamma.find(it_int->second.getPosition())->second;
                break;
              }
              else
                break;
            }
            else if (featclass==Features::CIRCPATT)
            {
              if ((int)circgammas->size()>level)
              {
                C_iM_gamma_i+=move_gamma.find(it_int->second.getPosition())->second;
                break;
              }
              else
                break;
            }
            else
            {
              float *gammas=this->getStandardGamma(featclass);
              if (gammas!=NULL)
              {
                if (gammas[level-1]>0)
                {
                  //gammaHERE=gammas[level-1];
                  C_iM_gamma_i+=move_gamma.find(it_int->second.getPosition())->second;
                  break;
                }
                else
                  break;
              }
              else
                break;
            }
        }

#if (1L<<16!=PATTERN_3x3_GAMMAS)
#error PATTERN_3x3_GAMMAS not 1L<<16!!! may be a problem
#endif
        used_moves_levels.insert(std::make_pair(PATTERN_3x3_GAMMAS*move.getPosition()+level,true));
      }
    }

    // Now C_iM_gamma_i has now the value of docs C_iM times gamma_i !!
    // and sum_gamma should be correct for incremental Taylor expansion of min max formula
    float diff_gamma_i=0;
    if (C_iM_gamma_i!=0)
    {
      if (it==ordervalue.begin())
      {
        //this was a win
        diff_gamma_i=1-C_iM_gamma_i/sum_gammas;
        //fprintf(stderr,"win feature %d level %d win sum_gammas %f C_iM_gamma_i %f gammaHERE %f diff_gamma %f\n",featclass,level_1,sum_gammas,C_iM_gamma_i,gammaHERE,diff_gamma_i);
        
      }
      else
      {
        //this was a loss
        diff_gamma_i=-C_iM_gamma_i/sum_gammas;
        //fprintf(stderr,"lost feature %d level %d win sum_gammas %f C_iM_gamma_i %f gammaHERE %f diff_gamma %f\n",featclass,level_1,sum_gammas,C_iM_gamma_i,gammaHERE,diff_gamma_i);
        
      }
    }

    //Now the gamma has to be changed
    {
      switch (win) //dummy to use the break statement and keep the code closer to other parts
      {
        default:
          if (level_1==0 && featclass!=Features::PATTERN3X3)
            break;
          if (featclass==Features::PATTERN3X3)
          {
            //used patterns should not be done by featurelevels_used, to much ram?!
            if (patterngammas->hasGamma(level_1))
            {
              patterngammas->learnGamma(level_1,diff_gamma_i*params->mm_learn_delta);
              break;
            }
            else
              break;
          }
          else if (featclass==Features::CIRCPATT)
          {
            if ((int)circgammas->size()>level_1)
            {
              float g=(*circgammas)[level_1];
              g+=diff_gamma_i*params->mm_learn_delta;
              if (g<0.001) g=0.001;
              (*circgammas)[level_1]=g;
              break;
            }
            else
              break;
          }
          else
          {
            float *gammas=this->getStandardGamma(featclass);
            if (gammas!=NULL)
            {
              if (gammas[level_1-1]>0)
              {
                gammas[level_1-1]+=diff_gamma_i*params->mm_learn_delta;
                break;
              }
              else
                break;
            }
            else
              break;
          }
      }
    }
  }
  return;
}
  
void Features::learnFeatureGamma(Features::FeatureClass featclass, unsigned int level, float learn_diff)
{
  if (level==0 && featclass!=Features::PATTERN3X3)
      return;
  
  if (featclass==Features::PATTERN3X3)
  {
    if (patterngammas->hasGamma(level))
    {
      patterngammas->learnGamma(level,learn_diff);
      return;
    }
    else
      return;
  }
  else if (featclass==Features::CIRCPATT)
  {
    if (circgammas->size()>level)
    {
      float g=(*circgammas)[level];
      g+=learn_diff;
      if (g<0.001) g=0.001;
      (*circgammas)[level]=g;
      return;
    }
    else
      return;
  }
  else
  {
    float *gammas=this->getStandardGamma(featclass);
    if (gammas!=NULL)
    {
      if (gammas[level-1]>0)
      {
        gammas[level-1]+=learn_diff;
        if (gammas[level-1]<=0.0) 
          gammas[level-1]=0.001;
        return;
      }
      else
        return;
    }
    else
      return;
  }
}

float Features::getMoveGamma(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, DecisionTree::GraphCollection *graphs, Go::Move move, bool checkforvalidmove, bool usecircularpatterns) const
{
  float g=1.0;
  
  if (checkforvalidmove && !board->validMove(move))
    return 0;
  
  g*=this->getFeatureGamma(Features::PASS,this->matchFeatureClass(Features::PASS,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::CAPTURE,this->matchFeatureClass(Features::CAPTURE,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::EXTENSION,this->matchFeatureClass(Features::EXTENSION,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::SELFATARI,this->matchFeatureClass(Features::SELFATARI,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::ATARI,this->matchFeatureClass(Features::ATARI,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::BORDERDIST,this->matchFeatureClass(Features::BORDERDIST,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::LASTDIST,this->matchFeatureClass(Features::LASTDIST,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::SECONDLASTDIST,this->matchFeatureClass(Features::SECONDLASTDIST,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::CFGLASTDIST,this->matchFeatureClass(Features::CFGLASTDIST,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::CFGSECONDLASTDIST,this->matchFeatureClass(Features::CFGSECONDLASTDIST,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::PATTERN3X3,this->matchFeatureClass(Features::PATTERN3X3,board,cfglastdist,cfgsecondlastdist,move,false));
  if (circlevels->size()>0)
    g*=this->getFeatureGamma(Features::CIRCPATT,this->matchFeatureClass(Features::CIRCPATT,board,cfglastdist,cfgsecondlastdist,move,false));

  if (params->features_dt_use)
  {
    float w = DecisionTree::getCollectionWeight(params->engine->getDecisionTrees(),graphs,move);
    if (w != -1)
      g *= w;
  }

  if (params->uct_factor_circpattern>0.0 &&move.isNormal() && usecircularpatterns)
  {
    Pattern::Circular pattcirc=Pattern::Circular(circdict,board,move.getPosition(),circpatternsize);
    if (move.getColor()==Go::WHITE)
            pattcirc.invert();
    pattcirc.convertToSmallestEquivalent(circdict);
    if (this->valueCircPattern(pattcirc.toString(circdict))>0.0)
    {
     //fprintf(stderr,"found pattern %f %s (stones %d)\n",params->test_p1,pattcirc.toString(circdict).c_str(),pattcirc.countStones(circdict));
     //fprintf(stderr,"found pattern %f %s (stones %d)\n",this->valueCircPattern(pattcirc.toString(circdict)),pattcirc.toString(circdict).c_str(),pattcirc.countStones(circdict));
     g*=1.0+exp(-0.5*circpatternsize)*params->uct_factor_circpattern * this->valueCircPattern(pattcirc.toString(circdict)); 
    }
    for (int j=circpatternsize-1;j>=params->uct_circpattern_minsize;j--)
    {
      Pattern::Circular tmp=pattcirc.getSubPattern(circdict,j);
      tmp.convertToSmallestEquivalent(circdict);
      std::string tmpPattString=tmp.toString(circdict);
      if (this->valueCircPattern(tmpPattString)>0.0)
      {
       //fprintf(stderr,"found pattern %f %s (stones %d)\n",this->valueCircPattern(tmpPattString),tmpPattString.c_str(),tmp.countStones(circdict));
       g*=1.0+exp(-0.5*j)*params->uct_factor_circpattern * this->valueCircPattern(tmpPattString); //params->uct_factor_circpattern_exponent
      }
    }
  }

  if (params->uct_simple_pattern_factor!=1.0)
  {
    unsigned int pattern=Pattern::ThreeByThree::makeHash(board,move.getPosition ());
    if (move.getColor()==Go::WHITE)
      pattern=Pattern::ThreeByThree::invert(pattern);
   if (params->engine->getPatternTable()->isPattern (pattern))
      g*=params->uct_simple_pattern_factor;
  }

  if ((params->uct_atari_unprune!=1.0 || params->uct_atari_unprune_exp!=0.0 || params->uct_danger_value!=0.0) && move.isNormal() && !board->isSelfAtariOfSize(move,2))
  {
    int size=params->board_size;
    int StonesInAtari=0;
    float DangerValue=0.0;
    Go::Color col=move.getColor();
    int pos=move.getPosition();
    foreach_adjacent(pos,p,{
      if (board->inGroup(p))
      {
        Go::Group *group=board->getGroup(p);
        if (col!=group->getColor() && group->isOneOfTwoLiberties(pos))
          StonesInAtari+=group->numOfStones();
        if (col!=group->getColor())
          DangerValue+=params->uct_danger_value*group->numOfStones()/group->numOfPseudoLiberties();
      }
    });
    //set gamma
    //fprintf(stderr,"StonesInAtari %d move %s\n",StonesInAtari,move.toString(size).c_str());
    if (StonesInAtari>0)
      g*=params->uct_atari_unprune*pow(StonesInAtari,params->uct_atari_unprune_exp);
    g*=1+DangerValue;
  }

  return g;
}

int Features::learnMoveGammaC(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, Go::Move move,float learn_diff)
{
  int C=0;
  if (!board->validMove(move))
    return 0;
  
  C+=this->learnFeatureGammaC(Features::PASS,this->matchFeatureClass(Features::PASS,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  C+=this->learnFeatureGammaC(Features::CAPTURE,this->matchFeatureClass(Features::CAPTURE,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  C+=this->learnFeatureGammaC(Features::EXTENSION,this->matchFeatureClass(Features::EXTENSION,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  C+=this->learnFeatureGammaC(Features::SELFATARI,this->matchFeatureClass(Features::SELFATARI,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  C+=this->learnFeatureGammaC(Features::ATARI,this->matchFeatureClass(Features::ATARI,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  C+=this->learnFeatureGammaC(Features::BORDERDIST,this->matchFeatureClass(Features::BORDERDIST,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  C+=this->learnFeatureGammaC(Features::LASTDIST,this->matchFeatureClass(Features::LASTDIST,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  C+=this->learnFeatureGammaC(Features::SECONDLASTDIST,this->matchFeatureClass(Features::SECONDLASTDIST,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  C+=this->learnFeatureGammaC(Features::CFGLASTDIST,this->matchFeatureClass(Features::CFGLASTDIST,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  C+=this->learnFeatureGammaC(Features::CFGSECONDLASTDIST,this->matchFeatureClass(Features::CFGSECONDLASTDIST,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  C+=this->learnFeatureGammaC(Features::PATTERN3X3,this->matchFeatureClass(Features::PATTERN3X3,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);

  if (params->uct_factor_circpattern>0.0 &&move.isNormal())
  {
    Pattern::Circular pattcirc=Pattern::Circular(circdict,board,move.getPosition(),circpatternsize);
    if (move.getColor()==Go::WHITE)
            pattcirc.invert();
    pattcirc.convertToSmallestEquivalent(circdict);
    fprintf(stderr,"found pattern %f %s (stones %d)\n",this->valueCircPattern(pattcirc.toString(circdict)),pattcirc.toString(circdict).c_str(),pattcirc.countStones(circdict));
    if (this->valueCircPattern(pattcirc.toString(circdict))>0.0)
    {
     //fprintf(stderr,"found pattern %f %s (stones %d)\n",params->test_p1,pattcirc.toString(circdict).c_str(),pattcirc.countStones(circdict));
     //fprintf(stderr,"found pattern %f %s (stones %d)\n",this->valueCircPattern(pattcirc.toString(circdict)),pattcirc.toString(circdict).c_str(),pattcirc.countStones(circdict));
     //this->learnCircPattern(pattcirc.toString(circdict),params->mm_learn_delta*learn_diff);
     C++;
    }
    for (int j=circpatternsize-1;j>=params->uct_circpattern_minsize;j--)
    {
      Pattern::Circular tmp=pattcirc.getSubPattern(circdict,j);
      tmp.convertToSmallestEquivalent(circdict);
      std::string tmpPattString=tmp.toString(circdict);
      if (this->valueCircPattern(tmpPattString)>0.0)
      {
       //fprintf(stderr,"found pattern %f %s (stones %d)\n",this->valueCircPattern(tmpPattString),tmpPattString.c_str(),tmp.countStones(circdict));
       //this->learnCircPattern(tmpPattString,params->mm_learn_delta*learn_diff); //params->uct_factor_circpattern_exponent
        C++;
      }
    }
  }

  return C;
}

bool Features::learnMovesGamma(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, std::map<float,Go::Move,std::greater<float> > ordervalue, std::map<int,float> move_gamma, float sum_gammas)
{
  //the formula used is documented in the docs directory IncrementalMMAlgorithm.lyx and pdf
  this->learnFeatureGammaMoves(Features::PASS,board,cfglastdist,cfgsecondlastdist,ordervalue,move_gamma,sum_gammas);
  this->learnFeatureGammaMoves(Features::CAPTURE,board,cfglastdist,cfgsecondlastdist,ordervalue,move_gamma,sum_gammas);
  this->learnFeatureGammaMoves(Features::EXTENSION,board,cfglastdist,cfgsecondlastdist,ordervalue,move_gamma,sum_gammas);
  this->learnFeatureGammaMoves(Features::SELFATARI,board,cfglastdist,cfgsecondlastdist,ordervalue,move_gamma,sum_gammas);
  this->learnFeatureGammaMoves(Features::ATARI,board,cfglastdist,cfgsecondlastdist,ordervalue,move_gamma,sum_gammas);
  this->learnFeatureGammaMoves(Features::BORDERDIST,board,cfglastdist,cfgsecondlastdist,ordervalue,move_gamma,sum_gammas);
  this->learnFeatureGammaMoves(Features::LASTDIST,board,cfglastdist,cfgsecondlastdist,ordervalue,move_gamma,sum_gammas);
  this->learnFeatureGammaMoves(Features::SECONDLASTDIST,board,cfglastdist,cfgsecondlastdist,ordervalue,move_gamma,sum_gammas);
  this->learnFeatureGammaMoves(Features::CFGLASTDIST,board,cfglastdist,cfgsecondlastdist,ordervalue,move_gamma,sum_gammas);
  this->learnFeatureGammaMoves(Features::CFGSECONDLASTDIST,board,cfglastdist,cfgsecondlastdist,ordervalue,move_gamma,sum_gammas);
  this->learnFeatureGammaMoves(Features::PATTERN3X3,board,cfglastdist,cfgsecondlastdist,ordervalue,move_gamma,sum_gammas);

  std::map<float,Go::Move>::iterator it;
  std::map<long int,bool> used_moves_levels;
  for (it=ordervalue.begin();it!=ordervalue.end();++it)
  {
    //Go::Move move_1=it->second;
    float C_iM_gamma_i[PATTERN_CIRC_MAXSIZE];
    std::string stringHERE[PATTERN_CIRC_MAXSIZE];
    for (int i=0;i<PATTERN_CIRC_MAXSIZE;i++)
    {
      C_iM_gamma_i[i]=0;
      //gammaHERE[i]=0;
    }
    std::map<float,Go::Move>::iterator it_int;
    for (it_int=it;it_int!=ordervalue.end();++it_int)
    {
      Go::Move move=it_int->second;
      if (params->uct_factor_circpattern>0.0 &&move.isNormal())
      {
        Pattern::Circular pattcirc=Pattern::Circular(circdict,board,move.getPosition(),circpatternsize);
        if (move.getColor()==Go::WHITE)
                pattcirc.invert();
        pattcirc.convertToSmallestEquivalent(circdict);
        if (this->valueCircPattern(pattcirc.toString(circdict))>0.0)
        {
         C_iM_gamma_i[circpatternsize]+=move_gamma.find(it_int->second.getPosition())->second;
         //gammaHERE[circpatternsize]=this->valueCircPattern(pattcirc.toString(circdict));
         stringHERE[circpatternsize]=pattcirc.toString(circdict);
        }
        for (int j=circpatternsize-1;j>=params->uct_circpattern_minsize;j--)
        {
          Pattern::Circular tmp=pattcirc.getSubPattern(circdict,j);
          tmp.convertToSmallestEquivalent(circdict);
          std::string tmpPattString=tmp.toString(circdict);
          if (this->valueCircPattern(tmpPattString)>0.0)
          {
           C_iM_gamma_i[j]+=move_gamma.find(it_int->second.getPosition())->second;
           //gammaHERE[j]=this->valueCircPattern(tmpPattString);
           stringHERE[j]=tmpPattString;
          }
        }
      }
    }
    //C_iM_gamma_i is C_iM*gamma_i with respect to docs name!!!
    for (int i=0;i<=circpatternsize;i++)
    {
      if (C_iM_gamma_i[i]!=0)
      {
        float diff_gamma_i;
        if (it==ordervalue.begin())
        {
          //this was a win
          diff_gamma_i=1-C_iM_gamma_i[i]/sum_gammas;
        }
        else
        {
          //this was a loss
          diff_gamma_i=-C_iM_gamma_i[i]/sum_gammas;
        }
        this->learnCircPattern(stringHERE[i],params->mm_learn_delta*diff_gamma_i); 
      }
    }
  }

  return true;
}

bool Features::learnMoveGamma(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, Go::Move move,float learn_diff)
{
  if (!board->validMove(move))
    return 0;
  int number_of_features_used=learnMoveGammaC(board,cfglastdist,cfgsecondlastdist,move,0);
  if (number_of_features_used>0)
    learn_diff/=number_of_features_used;
  
  this->learnFeatureGamma(Features::PASS,this->matchFeatureClass(Features::PASS,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  this->learnFeatureGamma(Features::CAPTURE,this->matchFeatureClass(Features::CAPTURE,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  this->learnFeatureGamma(Features::EXTENSION,this->matchFeatureClass(Features::EXTENSION,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  this->learnFeatureGamma(Features::SELFATARI,this->matchFeatureClass(Features::SELFATARI,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  this->learnFeatureGamma(Features::ATARI,this->matchFeatureClass(Features::ATARI,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  this->learnFeatureGamma(Features::BORDERDIST,this->matchFeatureClass(Features::BORDERDIST,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  this->learnFeatureGamma(Features::LASTDIST,this->matchFeatureClass(Features::LASTDIST,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  this->learnFeatureGamma(Features::SECONDLASTDIST,this->matchFeatureClass(Features::SECONDLASTDIST,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  this->learnFeatureGamma(Features::CFGLASTDIST,this->matchFeatureClass(Features::CFGLASTDIST,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  this->learnFeatureGamma(Features::CFGSECONDLASTDIST,this->matchFeatureClass(Features::CFGSECONDLASTDIST,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);
  this->learnFeatureGamma(Features::PATTERN3X3,this->matchFeatureClass(Features::PATTERN3X3,board,cfglastdist,cfgsecondlastdist,move),params->mm_learn_delta*learn_diff);

  if (params->uct_factor_circpattern>0.0 &&move.isNormal())
  {
    Pattern::Circular pattcirc=Pattern::Circular(circdict,board,move.getPosition(),circpatternsize);
    if (move.getColor()==Go::WHITE)
            pattcirc.invert();
    pattcirc.convertToSmallestEquivalent(circdict);
    fprintf(stderr,"found pattern %f %s (stones %d)\n",this->valueCircPattern(pattcirc.toString(circdict)),pattcirc.toString(circdict).c_str(),pattcirc.countStones(circdict));
    if (this->valueCircPattern(pattcirc.toString(circdict))>0.0)
    {
     //fprintf(stderr,"found pattern %f %s (stones %d)\n",params->test_p1,pattcirc.toString(circdict).c_str(),pattcirc.countStones(circdict));
     fprintf(stderr,"found pattern %f %s (stones %d)\n",this->valueCircPattern(pattcirc.toString(circdict)),pattcirc.toString(circdict).c_str(),pattcirc.countStones(circdict));
     this->learnCircPattern(pattcirc.toString(circdict),params->mm_learn_delta*learn_diff); 
    }
    for (int j=circpatternsize-1;j>=params->uct_circpattern_minsize;j--)
    {
      Pattern::Circular tmp=pattcirc.getSubPattern(circdict,j);
      tmp.convertToSmallestEquivalent(circdict);
      std::string tmpPattString=tmp.toString(circdict);
      if (this->valueCircPattern(tmpPattString)>0.0)
      {
       fprintf(stderr,"found pattern %f %s (stones %d)\n",this->valueCircPattern(tmpPattString),tmpPattString.c_str(),tmp.countStones(circdict));
       this->learnCircPattern(tmpPattString,params->mm_learn_delta*learn_diff); //params->uct_factor_circpattern_exponent
      }
    }
  }

  return true;
}

float Features::getBoardGamma(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, DecisionTree::GraphCollection *graphs, Go::Color col) const
{
  float total=0;
  
  for (int p=0;p<board->getPositionMax();p++)
  {
    Go::Move move=Go::Move(col,p);
    if (board->validMove(move))
      total+=this->getMoveGamma(board,cfglastdist,cfgsecondlastdist,graphs,move,false);
  }
  
  Go::Move passmove=Go::Move(col,Go::Move::PASS);
  total+=this->getMoveGamma(board,cfglastdist,cfgsecondlastdist,graphs,passmove,false);
  
  return total;
}

float Features::getBoardGammas(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, DecisionTree::GraphCollection *graphs, Go::Color col, Go::ObjectBoard<float> *gammas) const
{
  float total=0;
  
  for (int p=0;p<board->getPositionMax();p++)
  {
    Go::Move move=Go::Move(col,p);
    if (board->validMove(move))
    {
      float gamma=this->getMoveGamma(board,cfglastdist,cfgsecondlastdist,graphs,move,false);;
      gammas->set(p,gamma);
      total+=gamma;
    }
  }
  
  {
    Go::Move move=Go::Move(col,Go::Move::PASS);
    float gamma=this->getMoveGamma(board,cfglastdist,cfgsecondlastdist,graphs,move,false);;
    gammas->set(0,gamma);
    total+=gamma;
  }
  
  return total;
}

std::string Features::getFeatureClassName(Features::FeatureClass featclass) const
{
  switch (featclass)
  {
    case Features::PASS:
      return "pass";
    case Features::CAPTURE:
      return "capture";
    case Features::EXTENSION:
      return "extension";
    case Features::ATARI:
      return "atari";
    case Features::SELFATARI:
      return "selfatari";
    case Features::BORDERDIST:
      return "borderdist";
    case Features::LASTDIST:
      return "lastdist";
    case Features::SECONDLASTDIST:
      return "secondlastdist";
    case Features::CFGLASTDIST:
      return "cfglastdist";
    case Features::CFGSECONDLASTDIST:
      return "cfgsecondlastdist";
    case Features::PATTERN3X3:
      return "pattern3x3";
    case Features::CIRCPATT:
      return "circpatt";
    default:
      return "";
  }
}

Features::FeatureClass Features::getFeatureClassFromName(std::string name) const
{
  if (name=="pass")
    return Features::PASS;
  else if (name=="capture")
    return Features::CAPTURE;
  else if (name=="extension")
    return Features::EXTENSION;
  else if (name=="atari")
    return Features::ATARI;
  else if (name=="selfatari")
    return Features::SELFATARI;
  else if (name=="borderdist")
    return Features::BORDERDIST;
  else if (name=="lastdist")
    return Features::LASTDIST;
  else if (name=="secondlastdist")
    return Features::SECONDLASTDIST;
  else if (name=="cfglastdist")
    return Features::CFGLASTDIST;
  else if (name=="cfgsecondlastdist")
    return Features::CFGSECONDLASTDIST;
  else if (name=="pattern3x3")
    return Features::PATTERN3X3;
  else if (name=="circpatt")
    return Features::CIRCPATT;
  else
    return Features::INVALID;
}

bool Features::loadGammaFile(std::string filename)
{
  std::ifstream fin(filename.c_str());
  
  if (!fin)
    return false;
  
  std::string line;
  while (std::getline(fin,line))
  {
    if (!this->loadGammaLine(line))
    {
      fin.close();
      return false;
    }
  }
  
  fin.close();
  
  return true;
}

bool Features::saveGammaFile(std::string filename)
{
  std::ofstream fout(filename.c_str());
  
  if (!fout)
    return false;
  
  unsigned int i;
  for (i=0;i<PASS_LEVELS;i++) fout<<"pass:"<<i+1<<" "<<gammas_pass[i]<<" \n";
  for (i=0;i<CAPTURE_LEVELS;i++) fout<<"capture:"<<i+1<<" "<<gammas_capture[i]<<" \n";
  for (i=0;i<EXTENSION_LEVELS;i++) fout<<"extension:"<<i+1<<" "<<gammas_extension[i]<<" \n";
  for (i=0;i<SELFATARI_LEVELS;i++) fout<<"selfatari:"<<i+1<<" "<<gammas_selfatari[i]<<" \n";
  for (i=0;i<ATARI_LEVELS;i++) fout<<"atari:"<<i+1<<" "<<gammas_atari[i]<<" \n";
  for (i=0;i<BORDERDIST_LEVELS;i++) fout<<"borderdist:"<<i+1<<" "<<gammas_borderdist[i]<<" \n";
  for (i=0;i<LASTDIST_LEVELS;i++) fout<<"lastdist:"<<i+1<<" "<<gammas_lastdist[i]<<" \n";
  for (i=0;i<SECONDLASTDIST_LEVELS;i++) fout<<"secondlastdist:"<<i+1<<" "<<gammas_secondlastdist[i]<<" \n";
  for (i=0;i<CFGLASTDIST_LEVELS;i++) fout<<"cfglastdist:"<<i+1<<" "<<gammas_cfglastdist[i]<<" \n";
  for (i=0;i<CFGSECONDLASTDIST_LEVELS;i++) fout<<"cfgsecondlastdist:"<<i+1<<" "<<gammas_cfgsecondlastdist[i]<<" \n";
    
  for (i=0;i<PATTERN_3x3_GAMMAS;i++) if (patterngammas->getGamma (i)>0) {fout<<"pattern3x3:0x"<<std::hex<<std::setw(4)<<std::setfill('0')<<i<<" "<<patterngammas->getGamma (i)<<" \n";};
  std::map<unsigned int,std::string>::iterator it;
  for (i=0;i<circstrings->size();i++)
  {
    //int circid=it->first;
    fout<<"circpatt:"<<(*circstrings)[i]<<" "<<(*circgammas)[i]<<" \n";
  }
  fout.close();
  
  return true;
}

bool Features::saveGammaFileInline(std::string filename)
{
  std::ofstream fout(filename.c_str());
  
  if (!fout)
    return false;
  
  unsigned int i;
/*
   for (i=0;i<PASS_LEVELS;i++) fout<<"pass:"<<i+1<<" "<<gammas_pass[i]<<" \n";
  for (i=0;i<CAPTURE_LEVELS;i++) fout<<"capture:"<<i+1<<" "<<gammas_capture[i]<<" \n";
  for (i=0;i<EXTENSION_LEVELS;i++) fout<<"extension:"<<i+1<<" "<<gammas_extension[i]<<" \n";
  for (i=0;i<SELFATARI_LEVELS;i++) fout<<"selfatari:"<<i+1<<" "<<gammas_selfatari[i]<<" \n";
  for (i=0;i<ATARI_LEVELS;i++) fout<<"atari:"<<i+1<<" "<<gammas_atari[i]<<" \n";
  for (i=0;i<BORDERDIST_LEVELS;i++) fout<<"borderdist:"<<i+1<<" "<<gammas_borderdist[i]<<" \n";
  for (i=0;i<LASTDIST_LEVELS;i++) fout<<"lastdist:"<<i+1<<" "<<gammas_lastdist[i]<<" \n";
  for (i=0;i<SECONDLASTDIST_LEVELS;i++) fout<<"secondlastdist:"<<i+1<<" "<<gammas_secondlastdist[i]<<" \n";
  for (i=0;i<CFGLASTDIST_LEVELS;i++) fout<<"cfglastdist:"<<i+1<<" "<<gammas_cfglastdist[i]<<" \n";
  for (i=0;i<CFGSECONDLASTDIST_LEVELS;i++) fout<<"cfgsecondlastdist:"<<i+1<<" "<<gammas_cfgsecondlastdist[i]<<" \n";
    
  for (i=0;i<PATTERN_3x3_GAMMAS;i++) if (patterngammas->getGamma (i)>0) {fout<<"pattern3x3:0x"<<std::hex<<std::setw(4)<<std::setfill('0')<<i<<" "<<patterngammas->getGamma (i)<<" \n";};
*/
  std::map<unsigned int,std::string>::iterator it;
  //fout<<"std::map<Pattern::Circular,int> m={";
  fout<<"std::vector<std::string> m={""\n";
  for (i=1;i<circstrings->size();i++)
  {
/*would do the map, but is much too slow for compiling (hours)

     std::string p[PATTERN_CIRC_MAXSIZE+1];
    std::istringstream iss((*circstrings)[i]);
    getline(iss,p[PATTERN_CIRC_MAXSIZE],':'); //size
    for (int i=0;i<PATTERN_CIRC_MAXSIZE;i++)
    {
      p[i]="00000000";
      getline(iss,p[i],':'); //size
    }
    if (i!=1)
      fout<<"     ,";
    fout<<"\n{Pattern::Circular((boost::uint32_t[]){";
    for (int i=0;i<PATTERN_CIRC_MAXSIZE-1;i++)
      fout<<"0x"<<p[i]<<",";
    fout<<"0x"<<p[PATTERN_CIRC_MAXSIZE-1]<<"},"<<p[PATTERN_CIRC_MAXSIZE]<<"),"<<i<<"}\n";
    
    fout<<"//circpatt:"<<(*circstrings)[i]<<" "<<(*circgammas)[i]<<" \n";
*/
    if (i!=1)
      fout<<",\n";
    fout<<"\""<<(*circstrings)[i]<<"\"";
  }
  fout<<"};";
  fout.close();
  
  return true;
}

bool Features::loadCircFile(std::string filename,int numlines)
{
  std::ifstream fin(filename.c_str());
  
  if (!fin)
    return false;
  num_circmoves=0;
  std::string line;
  circpatterns.clear();
  circpatternsize=0;
  int n=0;
  while (std::getline(fin,line)&&(numlines==0||n<numlines))
  {
    int strpos = line.find(":");
    int numpos = line.find(" ");
    long int timesfound = atol(line.substr(0,numpos).c_str());
    circpatternsize=atoi(line.substr(numpos,strpos).c_str());
    circpatterns.insert(std::make_pair(line.substr(numpos+1),timesfound));
    //create small patterns and insert them
    Pattern::Circular tmpPattern=Pattern::Circular(circdict,line.substr(numpos+1));
    for (int j=circpatternsize-1;j>1;j--)
    {
      Pattern::Circular tmp=tmpPattern.getSubPattern(circdict,j);
      tmp.convertToSmallestEquivalent(circdict);
      std::string tmpPattString=tmp.toString(circdict);
      if (circpatterns.count(tmpPattString))
      {
        //long vor=circpatterns.find(tmpPattString)->second;
        circpatterns.find(tmpPattString)->second+=timesfound;
        //long nach=circpatterns.find(tmpPattString)->second;
        //fprintf(stderr,"played %s %ld %ld\n",tmpPattString.c_str(),vor,nach);
      }
      else
        circpatterns.insert(std::make_pair(tmpPattString,timesfound));
    }
    n++;
    num_circmoves+=timesfound;
    //fprintf(stderr,"line %s\n",line.c_str());
    //fprintf(stderr,"%d %s %d %ld %s\n",n,line.substr(numpos+1).c_str(),circpatternsize,timesfound,tmpPattern.toString(circdict).c_str());
  }
  
  fin.close();
  
  return true;
}

bool Features::loadCircFileNot(std::string filename,int numlines)
{
  std::ifstream fin(filename.c_str());
  
  if (!fin)
    return false;
  num_circmoves_not=0;
  std::string line;
  circpatternsnot.clear();
  circpatternsize=0;
  int n=0;
  while (std::getline(fin,line)&&(numlines==0||n<numlines))
  {
    int strpos = line.find(":");
    int numpos = line.find(" ");
    long int timesfound = atol(line.substr(0,numpos).c_str());
    circpatternsize=atoi(line.substr(numpos,strpos).c_str());
    circpatternsnot.insert(std::make_pair(line.substr(numpos+1),timesfound));
    //create small patterns and insert them
    Pattern::Circular tmpPattern=Pattern::Circular(circdict,line.substr(numpos+1));
    for (int j=circpatternsize-1;j>1;j--)
    {
      Pattern::Circular tmp=tmpPattern.getSubPattern(circdict,j);
      tmp.convertToSmallestEquivalent(circdict);
      std::string tmpPattString=tmp.toString(circdict);
      if (circpatternsnot.count(tmpPattString))
      {
        //long vor=circpatternsnot.find(tmpPattString)->second;
        circpatternsnot.find(tmpPattString)->second+=timesfound;
        //long nach=circpatternsnot.find(tmpPattString)->second;
        //fprintf(stderr,"not played %s %ld %ld\n",tmpPattString.c_str(),vor,nach);
      }
      else
        circpatternsnot.insert(std::make_pair(tmpPattString,timesfound));
    }
    n++;
    num_circmoves_not+=timesfound;
    //fprintf(stderr,"%d %s %d %ld\n",n,line.substr(numpos+1).c_str(),circpatternsize,timesfound);
  }
  
  fin.close();
  
  return true;
}

bool Features::saveCircValueFile(std::string filename)
{
  
  if (!circpatternvalues.empty())
  {
    std::ofstream fout(filename.c_str());
    if (!fout)
      return false;
    std::map<std::string,float>::iterator it;
    for (it=circpatternvalues.begin();it!=circpatternvalues.end();++it)
    {
      float v=valueCircPattern(it->first);
      if (v!=0)
      {
        //fprintf(stderr,"%s %f\n",it->first.c_str(),v);;
        fout<<it->first<<" "<<v<<"\n";
      }
    }
    fout.close();
    return true;
  }
  if ((circpatterns.empty()||circpatternsnot.empty()))
    return false;
  std::ofstream fout(filename.c_str());
  if (!fout)
    return false;
  std::map<std::string,long int>::iterator it;
  for (it=circpatterns.begin();it!=circpatterns.end();++it)
  {
    float v=valueCircPattern(it->first);
    if (v!=0)
    {
      circpatternvalues.insert(std::make_pair(it->first,v));
      fout<<it->first<<" "<<v<<"\n";
    }
  }
  fout.close();
  return true;
}

bool Features::loadCircValueFile(std::string filename)
{
  circpatternvalues.clear();
  std::ifstream fin(filename.c_str());
  if (!fin)
    return false;
  std::string line;
  circpatternsize=0;
  while (std::getline(fin,line))
  {
    int strpos = line.find(":");
    int numpos = line.find(" ");
    int tmp = atoi(line.substr(0,strpos).c_str()); //sorted, so that the biggest are last
    if (tmp>circpatternsize) 
    {
      fprintf(stderr,"circpatternsize now %d\n",tmp);
      circpatternsize=tmp;
    }
    float v=atof(line.substr(numpos+1).c_str());
    circpatternvalues.insert(std::make_pair(line.substr(0,numpos),v));
  }
  fin.close();
  return true;
}
bool Features::loadGammaString(std::string lines)
{
  std::istringstream iss(lines);
  
  std::string line;
  while (getline(iss,line,'\n'))
  {
    if (!this->loadGammaLine(line))
      return false;
  }
  
  return true;
}

bool Features::loadGammaLine(std::string line)
{
  std::string id,classname,levelstring,gammastring;
  Features::FeatureClass featclass;
  unsigned int level;
  float gamma;
  
  std::transform(line.begin(),line.end(),line.begin(),::tolower);
  
  std::istringstream issline(line);
  if (!getline(issline,id,' '))
    return false;
  //fprintf(stderr,"debug: %s %s\n",line.c_str(),gammastring.c_str());
  if (!getline(issline,gammastring,' '))
    return false;
  
  std::istringstream issid(id);
  if (!getline(issid,classname,':'))
    return false;
  if (!getline(issid,levelstring,' '))
    return false;
  
  if (levelstring.find(':') != std::string::npos)
  {
    Pattern::Circular pc = Pattern::Circular(circdict,levelstring);

    if (circlevels->count(pc)>0)
      level = (*circlevels)[pc];
    else
    {
      level = circlevels->size()+1;
      (*circlevels)[pc] = level;
      circstrings->resize(level+1);
      circgammas->resize(level+1);
      (*circstrings)[level] = pc.toString(circdict);
    }
  }
  else if (levelstring.at(0)=='0' && levelstring.at(1)=='x')
  {
    level=0;
    std::string hex="0123456789abcdef";
    for (unsigned int i=2;i<levelstring.length();i++)
    {
      level=level*16+(unsigned int)hex.find(levelstring.at(i),0);
    }
  }
  else
  {
    std::istringstream isslevel(levelstring);
    if (!(isslevel >> level))
      return false;
  }
  
  std::istringstream issgamma(gammastring);
  if (!(issgamma >> gamma))
    return false;
  
  featclass=this->getFeatureClassFromName(classname);
  if (featclass==Features::INVALID)
    return false;
  
  return this->setFeatureGamma(featclass,level,gamma);
}

float *Features::getStandardGamma(Features::FeatureClass featclass) const
{
  switch (featclass)
  {
    case Features::PASS:
      return (float *)gammas_pass;
    case Features::CAPTURE:
      return (float *)gammas_capture;
    case Features::EXTENSION:
      return (float *)gammas_extension;
    case Features::ATARI:
      return (float *)gammas_atari;
    case Features::SELFATARI:
      return (float *)gammas_selfatari;
    case Features::BORDERDIST:
      return (float *)gammas_borderdist;
    case Features::LASTDIST:
      return (float *)gammas_lastdist;
    case Features::SECONDLASTDIST:
      return (float *)gammas_secondlastdist;
    case Features::CFGLASTDIST:
      return (float *)gammas_cfglastdist;
    case Features::CFGSECONDLASTDIST:
      return (float *)gammas_cfgsecondlastdist;
    default:
      return NULL;
  }
}

bool Features::setFeatureGamma(Features::FeatureClass featclass, unsigned int level, float gamma)
{
  if (featclass==Features::CIRCPATT)
  {
    (*circgammas)[level] = gamma;
    return true;
  }
  else if (featclass==Features::PATTERN3X3)
  {
    patterngammas->setGamma(level,gamma);
    this->updatePatternIds();
    return true;
  }
  else
  {
    float *gammas=this->getStandardGamma(featclass);
    if (gammas!=NULL)
    {
      gammas[level-1]=gamma;
      return true;
    }
    else
      return false;
  }
}

std::string Features::getMatchingFeaturesString(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, DecisionTree::GraphCollection *graphs, Go::Move move, bool pretty) const
{
  std::ostringstream ss;
  unsigned int level,base;
  
  base=0;
  
  level=this->matchFeatureClass(Features::PASS,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" pass:"<<level;
    else
      ss<<" "<<(level-1);
  }
  base+=PASS_LEVELS;
  
  level=this->matchFeatureClass(Features::CAPTURE,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" capture:"<<level;
    else
      ss<<" "<<(base+level-1);
  }
  base+=CAPTURE_LEVELS;
  
  level=this->matchFeatureClass(Features::EXTENSION,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" extension:"<<level;
    else
      ss<<" "<<(base+level-1);
  }
  base+=EXTENSION_LEVELS;
  
  level=this->matchFeatureClass(Features::SELFATARI,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" selfatari:"<<level;
    else
      ss<<" "<<(base+level-1);
  }
  base+=SELFATARI_LEVELS;
  
  level=this->matchFeatureClass(Features::ATARI,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" atari:"<<level;
    else
      ss<<" "<<(base+level-1);
  }
  base+=ATARI_LEVELS;
  
  level=this->matchFeatureClass(Features::BORDERDIST,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" borderdist:"<<level;
    else
      ss<<" "<<(base+level-1);
  }
  base+=BORDERDIST_LEVELS;
  
  level=this->matchFeatureClass(Features::LASTDIST,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" lastdist:"<<level;
    else
      ss<<" "<<(base+level-1);
  }
  base+=LASTDIST_LEVELS;
  
  level=this->matchFeatureClass(Features::SECONDLASTDIST,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" secondlastdist:"<<level;
    else
      ss<<" "<<(base+level-1);
  }
  base+=SECONDLASTDIST_LEVELS;
  
  level=this->matchFeatureClass(Features::CFGLASTDIST,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" cfglastdist:"<<level;
    else
      ss<<" "<<(base+level-1);
  }
  base+=CFGLASTDIST_LEVELS;
  
  level=this->matchFeatureClass(Features::CFGSECONDLASTDIST,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" cfgsecondlastdist:"<<level;
    else
      ss<<" "<<(base+level-1);
  }
  base+=CFGSECONDLASTDIST_LEVELS;
  
  level=this->matchFeatureClass(Features::PATTERN3X3,board,cfglastdist,cfgsecondlastdist,move);
  if (patterngammas->hasGamma(level) && !move.isPass() && !move.isResign())
  {
    if (pretty)
      ss<<" pattern3x3:0x"<<std::hex<<std::setw(4)<<std::setfill('0')<<level;
    else
      ss<<" "<<(int)patternids->getGamma(level);
  }
  base+=patternids->getCount();

  level=this->matchFeatureClass(Features::CIRCPATT,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0 && !move.isPass() && !move.isResign())
  {
    if (pretty)
      ss<<" circpatt:"<<(*circstrings)[level];
    else
      ss<<" "<<(base+level-1);
  }
  base+=circlevels->size();

  if (params->features_dt_use)
  {
    std::list<int> *ids = DecisionTree::getCollectionLeafIds(params->engine->getDecisionTrees(),graphs,move);
    if (ids != NULL)
    {
      for (std::list<int>::iterator iter=ids->begin();iter!=ids->end();++iter)
      {
        int i = base+(*iter);
        if (pretty)
          ss<<" dt:"<<i;
        else
          ss<<" "<<i;
      }
      delete ids;
    }
  }
  
  return ss.str();
}

std::string Features::getFeatureIdList() const
{
  std::ostringstream ss;
  unsigned int id=0;
  
  for (unsigned int level=1;level<=PASS_LEVELS;level++)
    ss<<(id++)<<" pass:"<<level<<"\n";
  
  for (unsigned int level=1;level<=CAPTURE_LEVELS;level++)
    ss<<(id++)<<" capture:"<<level<<"\n";
  
  for (unsigned int level=1;level<=EXTENSION_LEVELS;level++)
    ss<<(id++)<<" extension:"<<level<<"\n";
  
  for (unsigned int level=1;level<=SELFATARI_LEVELS;level++)
    ss<<(id++)<<" selfatari:"<<level<<"\n";
  
  for (unsigned int level=1;level<=ATARI_LEVELS;level++)
    ss<<(id++)<<" atari:"<<level<<"\n";
  
  for (unsigned int level=1;level<=BORDERDIST_LEVELS;level++)
    ss<<(id++)<<" borderdist:"<<level<<"\n";
  
  for (unsigned int level=1;level<=LASTDIST_LEVELS;level++)
    ss<<(id++)<<" lastdist:"<<level<<"\n";
  
  for (unsigned int level=1;level<=SECONDLASTDIST_LEVELS;level++)
    ss<<(id++)<<" secondlastdist:"<<level<<"\n";
  
  for (unsigned int level=1;level<=CFGLASTDIST_LEVELS;level++)
    ss<<(id++)<<" cfglastdist:"<<level<<"\n";
  
  for (unsigned int level=1;level<=CFGSECONDLASTDIST_LEVELS;level++)
    ss<<(id++)<<" cfgsecondlastdist:"<<level<<"\n";
  
  for (unsigned int level=0;level<PATTERN_3x3_GAMMAS;level++)
  {
    if (patterngammas->hasGamma(level))
    {
      if (id==patternids->getGamma(level))
        ss<<std::dec<<(id++)<<" pattern3x3:0x"<<std::hex<<std::setw(4)<<std::setfill('0')<<level<<"\n";
      else
        fprintf(stderr,"WARNING! pattern id mismatch");
    }
  }

  for (unsigned int level=1;level<=circlevels->size();level++)
    ss<<(id++)<<" circpatt:"<<(*circstrings)[level]<<"\n";
  
  return ss.str();
}

void Features::updatePatternIds()
{
  unsigned int id=PASS_LEVELS+CAPTURE_LEVELS+EXTENSION_LEVELS+SELFATARI_LEVELS+ATARI_LEVELS+BORDERDIST_LEVELS+LASTDIST_LEVELS+SECONDLASTDIST_LEVELS+CFGLASTDIST_LEVELS+CFGSECONDLASTDIST_LEVELS;
  
  for (unsigned int level=0;level<PATTERN_3x3_GAMMAS;level++)
  {
    if (patterngammas->hasGamma(level))
      patternids->setGamma(level,id++);
  }
}

void Features::computeCFGDist(Go::Board *board, Go::ObjectBoard<int> **cfglastdist, Go::ObjectBoard<int> **cfgsecondlastdist)
{
  if (!board->getLastMove().isPass() && !board->getLastMove().isResign())
    *cfglastdist=board->getCFGFrom(board->getLastMove().getPosition(),CFGLASTDIST_LEVELS);
  if (!board->getSecondLastMove().isPass() && !board->getSecondLastMove().isResign())
    *cfgsecondlastdist=board->getCFGFrom(board->getSecondLastMove().getPosition(),CFGSECONDLASTDIST_LEVELS);
}


float Features::valueCircPattern(std::string circpattern) const
{
  //use ready circular pattern values, if availible
  if (!circpatternvalues.empty())
  {
    if (circpatternvalues.count(circpattern))
      return circpatternvalues.find(circpattern)->second;
    return 0;
  }
  //not strip the patternsize anymore!!!!
  //int strpos = circpattern.find(":");
    
 //fprintf(stderr,"not %s %ld\n",circpattern.c_str(),circpatternsnot.count(circpattern));
 //fprintf(stderr,"played %s %ld\n",circpattern.c_str(),circpatterns.count(circpattern));
  //in the not played database the circ pattern is not contained, therefore if it is played
  //it is set to factor 1 (count allways gives 1 or 0 in map)

//this is managed by test_p7 at the moment
//  if (!circpatternsnot.count(circpattern))
//    return 0;
  
  std::map<std::string,long int>::const_iterator it;
  it=circpatterns.find(circpattern);
  if (it==circpatterns.end())
    return 0;
  //both exist
  long int num_played=it->second;
  long int num_not_played=0;
  it=circpatternsnot.find(circpattern);
  if (it!=circpatternsnot.end()) num_not_played=it->second;
  float ratio=float(num_played)/(num_not_played+20)*params->uct_factor_circpattern_exponent;
  if (ratio>1.0) ratio=1.0;
  //fprintf(stderr,"valueCircPattern %ld %ld %f\n",num_played,num_not_played,ratio);
  return ratio;
}

void Features::learnCircPattern(std::string circpattern,float delta)
{
  //use ready circular pattern values, if availible
  if (!circpatternvalues.empty())
  {
    if (circpatternvalues.count(circpattern))
    {
      float v=circpatternvalues.find(circpattern)->second+delta;
      //should not be necessary if learning is ok
      //if (v>1) v=1;
      if (v<0.0001) v=0.0001;
      circpatternvalues.find(circpattern)->second=v;
    }
  }
}

bool Features::isCircPattern(std::string circpattern) const
{
  //strip the patternsize
  int strpos = circpattern.find(":");
    
// fprintf(stderr,"%s %d\n",circpattern.substr(strpos+1).c_str(),circpatterns.count(circpattern.substr(strpos+1)));
  return circpatterns.count(circpattern.substr(strpos+1));
}

bool Features::hasCircPattern(Pattern::Circular *pc)
{
  return (circlevels->count(*pc)>0);
}

