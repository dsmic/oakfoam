#ifndef DEF_OAKFOAM_FEATURES_H
#define DEF_OAKFOAM_FEATURES_H

#define PASS_LEVELS 2
#define CAPTURE_LEVELS 3
#define EXTENSION_LEVELS 2
#define SELFATARI_LEVELS 2
#define ATARI_LEVELS 3
#define BORDERDIST_LEVELS 4
#define LASTDIST_LEVELS 10
#define SECONDLASTDIST_LEVELS 10
#define CFGLASTDIST_LEVELS 10
#define CFGSECONDLASTDIST_LEVELS 10

#include <string>
#include <set>
#include <map>

const std::string FEATURES_DEFAULT=
  "pass:1 0.950848 \n"
  "pass:2 85.7124 \n"
  "capture:1 3.33649 \n"
  "capture:2 50.0 \n" // hand-picked, after adding level
  "capture:3 1000.0 \n" // hand-picked, after adding level
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

#include "Go.h"
//from "Parameters.h":
class Parameters;
//from "Pattern.h":
namespace Pattern
{
  class ThreeByThreeGammas;
  class CircularDictionary;
};

/** ELO Features.
 * Class to manage extracting, learning, and using feature weights.
 *
 * Typically, a move's gamma value is determined using these features.
 * This is done by checking if each of the feature classes match for the move.
 * Feature classes can have multiple levels.
 * When there are multiple feature levels that match, the largest level is used.
 * The gamma value for the move is the multiple of the matching features' gamma values.
 *
 * Feature level descriptions:
 *  - PASS
 *    - 1: The move is a pass.
 *    - 2: The move and previous move are passes.
 *  - CAPTURE
 *    - 1: The move is a capture.
 *    - 2: The move is a capture of a group adjacent to another group in atari.
 *    - 3: The move is a capture of a group, of 10 or more stones, adjacent to another group in atari.
 *  - EXTENSION
 *    - 1: The move extends a group in atari.
 *    - 2: The move extends a group in atari and a working ladder.
 *  - SELFATARI
 *    - 1: The move is a self-atari.
 *  - ATARI
 *    - 1: The move is an atari.
 *    - 2: The move is an atari and there is an active ko.
 *    - 3: The move is an atari on a group in a broken ladder.
 *  - BORDERDIST
 *    - x: The move is x away from the border.
 *  - LASTDIST
 *    - x: The move is x away from the last move (Manhattan distance).
 *  - SECONDLASTDIST
 *    - x: The move is x away from the second last move (Manhattan distance).
 *  - CFGLASTDIST
 *    - x: The move is x away from the last move (CFG distance).
 *  - CFGSECONDLASTDIST
 *    - x: The move is x away from the second last move (CFG distance).
 *  - PATTERN3X3
 *    - x: The move has a 3x3 pattern hash of x.
 */
class Features
{
  public:
    /** The different classes of features. */
    enum FeatureClass
    {
      PASS,
      CAPTURE,
      EXTENSION,
      SELFATARI,
      ATARI,
      BORDERDIST,
      LASTDIST,
      SECONDLASTDIST,
      CFGLASTDIST,
      CFGSECONDLASTDIST,
      PATTERN3X3,
      INVALID
    };
    
    /** Create a Features object with the global parameters. */
    Features(Parameters *prms);
    ~Features();
    
    /** Check for a feature match and return the matched level.
     * The @p move is check for a match against @p featclass on @p board.
     * A return value of zero, means that the feature was not matched.
     */
    unsigned int matchFeatureClass(Features::FeatureClass featclass, Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, Go::Move move, bool checkforvalidmove=true) const;
    /** Return the gamma weight for a specific feature and level. */
    float getFeatureGamma(Features::FeatureClass featclass, unsigned int level) const;
    /** Return the weight for a move.
     * The weight for a move is the product of matching feature weights for that move.
     */
    float getMoveGamma(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, Go::Move move, bool checkforvalidmove=true, bool withcircularpatterns=true) const;
    bool learnMoveGamma(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, Go::Move move, int learn_diff);
    /** Return the total of all gammas for the moves on a board. */
    float getBoardGamma(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, Go::Color col) const;
    /** Return the total of all gammas for the moves on a board and each move's weight in @p gammas. */
    float getBoardGammas(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, Go::Color col, Go::ObjectBoard<float> *gammas) const;
    /** Return the human-readable name for a feature class. */
    std::string getFeatureClassName(Features::FeatureClass featclass) const;
    /** Return the feature class, given a name. */
    Features::FeatureClass getFeatureClassFromName(std::string name) const;
    /** Set the gamma value for a specific feature and level. */
    bool setFeatureGamma(Features::FeatureClass featclass, unsigned int level, float gamma);
    void learnFeatureGamma(Features::FeatureClass featclass, unsigned int level, int learn_diff) const;
    
    /** Return a string of all the matching features for a move. */ 
    std::string getMatchingFeaturesString(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, Go::Move move, bool pretty=true) const;
    /** Return a list of all valid features and levels. */
    std::string getFeatureIdList() const;
    
    /** Load a gamma value from a line. */
    bool loadGammaLine(std::string line);
    /** Load a file of gamma values. */
    bool loadGammaFile(std::string filename);
    bool saveGammaFile(std::string filename);
    bool loadCircFile(std::string filename,int numlines);
    bool loadCircFileNot(std::string filename,int numlines);
    bool saveCircValueFile(std::string filename);
    bool loadCircValueFile(std::string filename);
    /** Load a number of lines of gamma values. */
    bool loadGammaString(std::string lines);
    /** Load the default gamma values. */
    void loadGammaDefaults() { this->loadGammaString(FEATURES_DEFAULT); };
    
    /** Return the CFG distances for the last and second last moves on a board. */
    void computeCFGDist(Go::Board *board, Go::ObjectBoard<int> **cfglastdist, Go::ObjectBoard<int> **cfgsecondlastdist);

    Pattern::CircularDictionary *circdict; 
    bool isCircPattern(std::string circpattern) const;
    float valueCircPattern(std::string circpattern) const;
    void learnCircPattern(std::string circpattern,float delta);
    int getCircSize () {return circpatternsize;}
    Pattern::ThreeByThreeGammas* getPatternGammas() {return patterngammas;}
    
  private:
    Parameters *const params;
    Pattern::ThreeByThreeGammas *patterngammas;
    Pattern::ThreeByThreeGammas *patternids;
    float gammas_pass[PASS_LEVELS];
    float gammas_capture[CAPTURE_LEVELS];
    float gammas_extension[EXTENSION_LEVELS];
    float gammas_selfatari[SELFATARI_LEVELS];
    float gammas_atari[ATARI_LEVELS];
    float gammas_borderdist[BORDERDIST_LEVELS];
    float gammas_lastdist[LASTDIST_LEVELS];
    float gammas_secondlastdist[SECONDLASTDIST_LEVELS];
    float gammas_cfglastdist[CFGLASTDIST_LEVELS];
    float gammas_cfgsecondlastdist[CFGSECONDLASTDIST_LEVELS];
    
    float *getStandardGamma(Features::FeatureClass featclass) const;
    void updatePatternIds();

    std::map<std::string,long int> circpatterns;
    std::map<std::string,long int> circpatternsnot;
    std::map<std::string,float> circpatternvalues;
    int circpatternsize;
    long int num_circmoves;
    long int num_circmoves_not;
};

#endif
