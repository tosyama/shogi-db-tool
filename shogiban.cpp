#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "shogiban.h"

static char komaStr[][5] = { "・","歩","香","桂","銀","金","角","飛","玉", "と", "杏", "圭", "全","　","馬","龍"};
static char kanSuji[][5] = { "〇","一","二","三","四","五","六","七","八","九"};

void resetShogiBan(ShogiKyokumen *shogi)
{
    int x, k;
    Koma (*shogiBan)[BanX] = shogi->shogiBan;
    int (*komaDai)[DaiN] = shogi->komaDai;
    
    shogiBan[0][0] = shogiBan[0][8] = UKY;
    shogiBan[0][1] = shogiBan[0][7] = UKE;
    shogiBan[0][2] = shogiBan[0][6] = UGI;
    shogiBan[0][3] = shogiBan[0][5] = UKI;
    shogiBan[0][4] = UOU;
    shogi->uou_x = 4; shogi->uou_y = 0;

    shogiBan[1][0] = shogiBan[1][2] = shogiBan[1][3] = shogiBan[1][4] =
    shogiBan[1][5] = shogiBan[1][6] = shogiBan[1][8] = EMP;
    shogiBan[1][1] = UHI;
    shogiBan[1][7] = UKA;
    
    for (x=0; x<BanX; x++) shogiBan[2][x] = UFU;
    for (x=0; x<BanX; x++) shogiBan[3][x] = EMP;
    for (x=0; x<BanX; x++) shogiBan[4][x] = EMP;
    for (x=0; x<BanX; x++) shogiBan[5][x] = EMP;
    for (x=0; x<BanX; x++) shogiBan[6][x] = FU;

    shogiBan[7][0] = shogiBan[7][2] = shogiBan[7][3] = shogiBan[7][4] =
    shogiBan[7][5] = shogiBan[7][6] = shogiBan[7][8] = EMP;
    shogiBan[7][1] = KA;
    shogiBan[7][7] = HI;

    shogiBan[8][0] = shogiBan[8][8] = KY;
    shogiBan[8][1] = shogiBan[8][7] = KE;
    shogiBan[8][2] = shogiBan[8][6] = GI;
    shogiBan[8][3] = shogiBan[8][5] = KI;
    shogiBan[8][4] = OU;
    shogi->ou_x = 4; shogi->ou_y = 8;
    
    for (k=0;k<DaiN;k++) komaDai[0][k] = 0;
    for (k=0;k<DaiN;k++) komaDai[1][k] = 0;
}

void copyKyokumenInversely(ShogiKyokumen *dst, const ShogiKyokumen *src)
{
	Koma *dk = dst->shogiBan[0];
	const Koma *src_ban = src->shogiBan[0];
	const Koma *sk = src_ban + (BanY*BanX);

	for (; sk>=src_ban; dk++, sk--) {
		Koma k=*sk;
		if (k != EMP) {
			*dk =static_cast<Koma>(k & UWATE ? k & KOMATYPE2 : k | UWATE);
		} else {
			*dk = EMP;
		}
	}

	memcpy(dst->komaDai[0],src->komaDai[1],sizeof(dst->komaDai[0]));
	memcpy(dst->komaDai[1],src->komaDai[0],sizeof(dst->komaDai[0]));

	if (src->ou_x != NonPos) {
		dst->uou_x = BanX - src->ou_x;
		dst->uou_y = BanY - src->ou_y;
	} else {
		dst->uou_x = dst->uou_y = NonPos;
	}

	if (src->uou_x != NonPos) {
		dst->ou_x = BanX - src->uou_x;
		dst->ou_y = BanY - src->uou_y;
	} else {
		dst->ou_x = dst->ou_y = NonPos;
	}
}


void printKyokumen(FILE *f, ShogiKyokumen *shogi)
{
    Koma (*shogiBan)[BanX] = shogi->shogiBan;
    int (*komaDai)[DaiN] = shogi->komaDai;
    
    fprintf(f, "後手の持駒：");
    int nashi = 1;
    for (int k=0; k<DaiN; k++) {
        if (komaDai[1][k] == 1) {
            fprintf(f, "%s ", komaStr[k?k:OU]);
            nashi = 0;
        } else if (komaDai[1][k]>1) {
            fprintf(f, "%s%d ", komaStr[k?k:OU], komaDai[1][k]);
            nashi = 0;
        }
    }
    if (nashi) fprintf(f, "なし");
    
    fprintf(f, "\n");
    fprintf(f, "  ９ ８ ７ ６ ５ ４ ３ ２ １\n"
           "+---------------------------+\n");

    for (int y=0; y<9; y++) {
        fprintf(f, "|");
        for (int x=0; x<9; x++) {
            fprintf(f, "%s", (shogiBan[y][x] & UWATE) ? "v" : " ");
            fprintf(f, "%s", komaStr[shogiBan[y][x] & KOMATYPE2]);
        }
        fprintf(f, "|%s",kanSuji[y+1]);
        fprintf(f, "\n");
    }
    fprintf(f, "+---------------------------+\n");
    fprintf(f, "先手の持駒：");
    nashi = 1;
    for (int k=0; k<DaiN; k++) {
        if (komaDai[0][k] == 1) {
            fprintf(f, "%s ", komaStr[k?k:OU]);
            nashi = 0;
        } else if (komaDai[0][k]>1) {
            fprintf(f, "%s%d ", komaStr[k?k:OU], komaDai[0][k]);
            nashi = 0;
        }
    }
    if (nashi) fprintf(f, "なし");
    
    fprintf(f, "\n");
}

Koma sashite1(ShogiKyokumen *shogi, int from_x, int from_y, int to_x, int to_y, int nari)
{
    Koma (*shogiBan)[BanX] = shogi->shogiBan;
    int (*komaDai)[DaiN] = shogi->komaDai;
    
    Koma k1, k2;
    k1 = shogiBan[from_y][from_x];
    k2 = shogiBan[to_y][to_x];
    
    assert(k1 != EMP);
    if (k1 == OU) { // 王の位置は常に記録
        shogi->ou_x = to_x; shogi->ou_y = to_y;
    } else if (k1 == UOU) {
        shogi->uou_x = to_x; shogi->uou_y = to_y;
    }

    if (k2 != EMP) {
		if (k2==OU) {
			shogi->ou_x = shogi->ou_y = NonPos;
            komaDai[0][0]++;
		} else if (k2 == UOU) {
			shogi->uou_x = shogi->uou_y = NonPos;
            komaDai[1][0]++;
		} else if (k1 & UWATE) {
            komaDai[1][k2 & KOMATYPE1]++;
        } else {
            komaDai[0][k2 & KOMATYPE1]++;
        }
    }
    
    if (nari) {
		assert((k1 & KOMATYPE1)!=KI && (k1 & KOMATYPE1)!=OU);
        shogiBan[to_y][to_x] = (Koma)(k1 ^ NARI);
    }
    else shogiBan[to_y][to_x] = k1;
    shogiBan[from_y][from_x] = EMP;
    
    return k2;  //手を戻すときに使用するため
}

void sashite2(ShogiKyokumen *shogi, int uwate, Koma koma, int to_x, int to_y)
{
    Koma (*shogiBan)[BanX] = shogi->shogiBan;
    int (*komaDai)[DaiN] = shogi->komaDai;
    
    assert(komaDai[uwate][koma] > 0); // 駒を持ってること
    assert(shogiBan[to_y][to_x] == EMP);
    
    if (uwate) shogiBan[to_y][to_x] = (Koma) (koma + UWATE);
    else shogiBan[to_y][to_x] = koma;

    komaDai[uwate][koma]--;
}

