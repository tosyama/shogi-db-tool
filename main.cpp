//
//  main.cpp
//  ShogiDBTool
//
//  Created by tosyama on 2014/12/28.
//  Copyright (c) 2016 tosyama. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "shogiban.h"
#include "kyokumencode.h"
#include "sashite.h"
#include "kifu.h"
#include "shogidb.h"
#include "shogirule.h"

#include "shogigame.h"
static int interactiveCUI(ShogiKyokumen *shogi, Sashite *s);

static char komaStr[][5] = { "・","歩","香","桂","銀","金","角","飛","玉", "と", "杏", "圭", "全","　","馬","龍"};
FILE *shg_log = NULL;

int main(int argc, const char * argv[]) {
	ShogiGame shg;
	printf("%s\n", shg.currentKyCode());
	shg.move(2,7,2,6,false);
	shg.print(1);
	printf("%s\n", shg.currentKyCode());
	return 0;

    Kifu kifu;
    ShogiKyokumen shogi;
    char code[KyokumenCodeLen];
    shg_log = fopen("shogidbtool.log", "w");

    resetShogiBan(&shogi);
    if (argc==2 && strlen(argv[1])==(KyokumenCodeLen-1)) {
		loadKyokumenFromCode(&shogi, argv[1]);
	}
    Sashite s[200];
    int n=0;
    createSashiteAll(&shogi, s, &n);
    
    Sashite si[200];
    si[0].type = SASHITE_RESULT;
    int i=0;
    int cmd;
    while ((cmd = interactiveCUI(&shogi, &si[i]))) {
        i+=cmd;
        if (i<=0) {i=0; si[0].type = SASHITE_RESULT;}
        else {si[i]=si[i-1];}
        createSashiteAll(&shogi, s, &n);
        fprintf(shg_log, "手の数: %d\n", n);

		printKyokumen(shg_log, &shogi);
        for (int j=0; j<n; j++) {
			if (s[j].type == SASHITE_IDOU) {
				int x = s[j].idou.from_x;
				int y = s[j].idou.from_y;
				fprintf(shg_log, "%.2d %s(%d,%d)>(%d,%d)%s\n",
						j+1,
						komaStr[shogi.shogiBan[y][x]],
						STD_X(x), STD_Y(y),
						STD_X(s[j].idou.to_x), STD_Y(s[j].idou.to_y),
						s[j].idou.nari?"+":"");
			} else if (s[j].type == SASHITE_UCHI)
				fprintf(shg_log, "%.2d %s>(%d,%d)\n",
						j+1,
						komaStr[s[j].uchi.koma],
						STD_X(s[j].uchi.to_x), STD_Y(s[j].uchi.to_y));
        }

    }
    readKIF("test.kif", &kifu);
/*    Sashite* sashite = kifu.sashite;
    
    int i = 0;
    while (sasu(&shogi, &sashite[i]) < 0) {
        printKyokumen(&shogi);
        createKyokumenCode(code, &shogi, i%2);
        printf("code %d: %s\n", i, code);
        i++;
    }
    
    if (sashite[i].result.winner == 0) {
        printf("%d手で先手の勝ち\n", kifu.tesuu);
    } else if (sashite[i].result.winner == 1) {
        printf("%d手で後手の勝ち\n", kifu.tesuu);
    } else if (sashite[i].result.winner == 2) {
        printf("千日手\n");
    } else {
        assert(0);
    }

    createShogiDB("shogiDb.db");
    char fname[256];
    for (int i=1; i<=10; i++) {
        sprintf(fname, "/Users/tos/Documents/Dev/ShogiDBTool/KifuData/%08d.kif",i);
        printf("%s\n", fname);
        if (readKIF(fname, &kifu) > 0)
            insertShogiDB("shogiDb.db", &kifu);
    } */
    fclose(shg_log);
    return 0;
}

static int interactiveCUI(ShogiKyokumen *shogi, Sashite *s)
{
    Koma (*shogiBan)[BanX] = shogi->shogiBan;
	int (*komaDai)[DaiN] = shogi->komaDai;
    char buf[80];
    int k, fx, fy, tx, ty;
    printKyokumen(stdout, shogi);
    
    while (1) {
        printf("move:1-9 1-9 1-9 1-9 + uchi:[^v]1-7 1-9 1-9\nundo:-1, show cd:s(v) print:p, quit:q,\n>");
		fflush(stdout);
        if(fgets(buf,80,stdin)) {
            if (buf[0] >= '1' && buf[0] <= '9') { // move
                fx = buf[0] - '0'; fy = buf[1] - '0';
                tx = buf[2] - '0'; ty = buf[3] - '0';

                if(fx >= 1 && fx <=9 && fy >= 1 && fy <= 9 && tx >=1 && tx <= 9 && ty >= 1 && ty <= 9) {
                    s->type = SASHITE_IDOU;
                    s->idou.from_x = INNER_X(fx);
                    s->idou.from_y = INNER_Y(fy);
                    s->idou.to_x = INNER_X(tx);
                    s->idou.to_y = INNER_Y(ty);
                    
                    if(buf[4] == '+')
                        s->idou.nari = 1;
                    else
                        s->idou.nari = 0;
                    
                    if (shogiBan[s->idou.from_y][s->idou.from_x] != EMP) {
                        sasu(shogi,s);
                        return 1;
                    }
                }
            } else if (buf[0] == '^' || buf[0] == 'v') {
				int n = buf[1] - '0';
				tx = buf[2] - '0';
				ty = buf[3] - '0';
				if (n>=1 && n<=7 && tx >=1 && tx <= 9 && ty >= 1 && ty <= 9) {
					int u = buf[0] == 'v' ? 1 : 0;
					for (k=1;k<DaiN;k++) {
						if (komaDai[u][k]>0 && (--n)==0) {
							if (shogiBan[INNER_Y(ty)][INNER_X(tx)]==EMP) {
								s->type = SASHITE_UCHI;
								s->uchi.uwate = u;
								s->uchi.to_x = INNER_X(tx);
								s->uchi.to_y = INNER_Y(ty);
								s->uchi.koma = (Koma)k;
								sasu(shogi,s);
								return 1;
							} else break;
						}
					}
				}
            } else if (buf[0] == '-') {
				if (s->type != SASHITE_RESULT) {
					temodoshi(shogi, s);
					return -1;
				}
            } else if (buf[0] == 'q') { // end
                return 0;
            } else if (buf[0] == 'p') { // print
                printKyokumen(stdout, shogi);
            } else if (buf[0] == 's') { // show code
				char code[KyokumenCodeLen];
				createKyokumenCode(code, shogi, (buf[1]=='v') ? 1 : 0);
				printf("%s\n", code);
			} else if (buf[0] == 'd') { // delete
                fx = buf[1] - '0'; fy = buf[2] - '0';
                if(fx >= 1 && fx <=9 && fy >= 1 && fy <=9) {
					fx = INNER_X(fx);
					fy = INNER_Y(fy);
					if ((k=shogiBan[fy][fx])!=EMP) {
						if (k==OU) shogi->ou_x = shogi->ou_y = NonPos;	
						if (k==UOU) shogi->uou_x = shogi->uou_y = NonPos;	
						shogiBan[fy][fx]=EMP;
						printf("Deleted.\n");
						return -999;
					}
				}
			}
        } else {
            return 0;
        }
    }
    
    return 0;
}
