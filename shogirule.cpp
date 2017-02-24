//
//  shogirule.cpp
//  ShogiDBTool
//
//  Created by tosyama on 2016/2/2.
//  Copyright (c) 2016 tosyama. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "shogiban.h"
#include "kyokumencode.h"
#include "sashite.h"
#include "kifu.h"
#include "shogidb.h"
#include "shogirule.h"

#define min(a,b)    ((a)<(b)?(a):(b))

// work用 bitBoard
typedef struct {
    uint64_t topmid;
    uint32_t bottom;
} BitBoard9x9;

inline void clearBB(BitBoard9x9 *b){
    b->topmid = 0;
    b->bottom = 0;
}

inline void setBitBB(BitBoard9x9 *b, int x, int y){
	assert(x>=0 && x<BanX && y>=0 && y<BanY);
    if (y<6) b->topmid |= ((uint64_t)1 << (BanX*y + x));
    else b->bottom |= (1u << (BanX*(y-6) + x));
}

inline void setBitBB(BitBoard9x9 *b, int n){
	assert(n>=0 && n<81);
    if (n<54) b->topmid |= ((uint64_t)1 << n);
    else b->bottom |= (1u << (n-54));
}

inline void setBitBB(BitBoard9x9 *b, const BitBoard9x9 *orb)
{
	b->topmid |= orb->topmid;
	b->bottom |= orb->bottom;
}

inline void setBitsBB(BitBoard9x9 *b, int x, int y, uint32_t pattern)
{
	int n=BanX*y + x-10; 
	if (x==0) pattern &= 0xf7fbfdfeu;
	else if (x==8) pattern &= 0xdfeff7fbu;
	if (n>=0) {
		if (n<64) b->topmid |= ((uint64_t)pattern << n);
	} else if (n>-64)
		b->topmid |= (((uint64_t)pattern) >> (-n));
	n-=54;
	if (n>=0) {
		if(n<32) b->bottom |= (pattern << n);
	} else if (n>-32)
		b->bottom |= (pattern >> (-n));
}

inline int getBitBB(const BitBoard9x9 *b, int x, int y){
    if (y<6) return (int)((uint64_t)1 & (b->topmid >> (BanX*y + x)));
    else return (int)((uint32_t)1 & (b->bottom >> (BanX*(y-6) + x)));
}

void printBB(FILE *f, const BitBoard9x9 *b)
{
    for (int y=0; y<BanY; y++) {
        for (int x=0; x<BanX; x++) {
            if(getBitBB(b,x,y)) fprintf(f, "1"); else fprintf(f, "0");
        }
        fprintf(f, "\n");
    }
}


// 番外の場合はEMPを返す
inline Koma getKoma(Koma (*s)[BanX], int x, int y)
{
    if (y<0 || y>=BanY || x<0 || x>=BanX) return EMP;
    return s[y][x];
}
inline Koma getKomaTop(Koma (*s)[BanX], int x, int y) { return (y<0) ? EMP : s[y][x]; }
inline Koma getKomaTopL(Koma (*s)[BanX], int x, int y) { return (y<0 || x <0) ? EMP : s[y][x]; }
inline Koma getKomaTopR(Koma (*s)[BanX], int x, int y) { return (y<0 || x >=BanX) ? EMP : s[y][x]; }
inline Koma getKomaL(Koma (*s)[BanX], int x, int y) { return (x<0) ? EMP : s[y][x]; }
inline Koma getKomaR(Koma (*s)[BanX], int x, int y) { return (x>=BanX) ? EMP : s[y][x]; }
inline Koma getKomaBot(Koma (*s)[BanX], int x, int y) { return (y>=BanY) ? EMP : s[y][x]; }
inline Koma getKomaBotL(Koma (*s)[BanX], int x, int y) { return (y>=BanY || x <0) ? EMP : s[y][x]; }
inline Koma getKomaBotR(Koma (*s)[BanX], int x, int y) { return (y>=BanY || x >=BanX) ? EMP : s[y][x]; }

//駒の効きの判定
// 桂馬の効きか
inline int isRangeKE(Koma k, int uwate)
{
    if (uwate) return (k==UKE); else return (k==KE);
}

inline int isKoma(Koma k, int koma_flag, int uwate)
{
    if (uwate && !(k&UWATE)) return 0;
    return koma_flag & (1 << (k&KOMATYPE2));
}

//現在の位置から盤外の位置を示すフラグ
#define OutF2    1
#define OutF1    2
#define OutB     4
#define OutL    8
#define OutR    16

#define OutF2L  9
#define OutF2R  17
#define OutF1L  10
#define OutF1R  18
#define OutBL   12
#define OutBR   20

enum PinInfo {
    NoPin = 0, VertPin, HorizPin, LNanamePin, RNanamePin
    
};


Sashite *createSashiteUMA(Sashite *te, BitBoard9x9 *tebanKikiB, Koma (*shogiBan)[BanX], int x, int y,
		int (*kabegomaInfo)[BanX], int oute_num, BitBoard9x9 *outePosKikiB);

// 手の生成
void createSashite(ShogiKykumen *shogi, int uwate, Sashite *s, int *n)
{
    Koma (*shogiBan)[BanX] = shogi->shogiBan;
    int (*komaDai)[DaiN] = shogi->komaDai;
    *n = 0;
    Sashite *cs = s;
    Koma teban = uwate ? UWATE : (Koma)0;
    
    static int OU_range[8][2] = {{-1,-1}, {0,-1}, {1,-1}, {-1,0},{1,0},{-1,1},{0,1},{1,1}};
    static int KI_range[6][2] = {{0,-1},{0,1},{-1,0},{1,0},{-1,-1},{1,-1}};
    static int GI_range[5][2] = {{0,-1}, {-1,-1}, {1,1}, {1,-1}, {-1,1}};
    static int KE_range[2][2] = {{-1,-2}, {1,-2}};
    static int UMA_range[4][2] = {{0,-1}, {-1,0}, {1,0},{0,1}};
    static int RYU_range[4][2] = {{-1,-1}, {1,-1}, {-1,1},{1,1}};
    
    //      0001 0
	// 000 0001 11
	// 00 0000 111
	// 0 0000 0101

	static uint32_t GI_pttn = 0x00140007;
	static uint32_t KI_pttn = 0x00080a07;

    // ↓あまりいらないかも
    static int bangaiInfo[BanY][BanX] = {
        {11,3,3, 3,3,3, 3,3,19},{9,1,1, 1,1,1, 1,1,17},{8,0,0, 0,0,0, 0,0,16},
        {8,0,0, 0,0,0, 0,0,16}, {8,0,0, 0,0,0, 0,0,16}, {8,0,0, 0,0,0, 0,0,16},
        {8,0,0, 0,0,0, 0,0,16}, {8,0,0, 0,0,0, 0,0,16}, {12,4,4, 4,4,4, 4,4,20}
    };
    
    // 予定: 直接王手の検出。自ゴマは直接王手ゴマをとるか、王が移動するしかない
    // 予定: 間接王手の効きの検出。効きに入るか王手ゴマを取るか(二重王手の場合不可)、王が移動
    // 予定: 自王位置から、間接王手と壁となる自ゴマの位置をマーク
    int ou_x = shogi->ou_x;
    int ou_y = shogi->ou_y;
    int bangai = bangaiInfo[ou_y][ou_x];
	int oute_num = 0;
    BitBoard9x9 outePosKikiB;    // 王手ゴマの位置を記録
    BitBoard9x9 kabePosB;   // pinされている駒の位置を記録
    int kabegomaInfo[BanY][BanX] = {    // pinされている駒のと向きを記録
        {0,0,0, 0,0,0, 0,0,0}, {0,0,0, 0,0,0, 0,0,0}, {0,0,0, 0,0,0, 0,0,0},
        {0,0,0, 0,0,0, 0,0,0}, {0,0,0, 0,0,0, 0,0,0}, {0,0,0, 0,0,0, 0,0,0},
        {0,0,0, 0,0,0, 0,0,0}, {0,0,0, 0,0,0, 0,0,0}, {0,0,0, 0,0,0, 0,0,0},
    };
    int ukabegomaInfo[BanY][BanX] = {    // pinされている駒のと向きを記録
        {0,0,0, 0,0,0, 0,0,0}, {0,0,0, 0,0,0, 0,0,0}, {0,0,0, 0,0,0, 0,0,0},
        {0,0,0, 0,0,0, 0,0,0}, {0,0,0, 0,0,0, 0,0,0}, {0,0,0, 0,0,0, 0,0,0},
        {0,0,0, 0,0,0, 0,0,0}, {0,0,0, 0,0,0, 0,0,0}, {0,0,0, 0,0,0, 0,0,0},
    };
    
    clearBB(&outePosKikiB);
    clearBB(&kabePosB);
    {
        if (!(bangai & OutF2L) && isRangeKE(shogiBan[ou_y-2][ou_x-1],1)) {   // 桂王手の検出
            setBitBB(&outePosKikiB, ou_x-1, ou_y-2);
			oute_num++;
            printf("左桂王手\n");
        } if (!(bangai & OutF2R) && isRangeKE(shogiBan[ou_y-2][ou_x+1],1)) {    // 桂王手の検出
            setBitBB(&outePosKikiB, ou_x+1, ou_y-2);
			oute_num++;
            printf("右桂王手\n");
        }

        // {検出移動幅, ループ数, 直接王手駒, 間接王手駒, Pinタイプ}
        int  scanInf[8][5] = {
            {BanX*-1-1, min(ou_x,ou_y),F_NAMAE_GOMA,F_KA_UMA, LNanamePin},
            {BanX*-1,ou_y,F_MAE_GOMA,F_KY_HI_RYU, VertPin},
            {BanX*-1+1, min(BanX-1-ou_x,ou_y),F_NAMAE_GOMA,F_KA_UMA, RNanamePin},
            {-1,ou_x,F_YOKO_GOMA,F_HI_RYU, HorizPin},
            {1,BanX-1-ou_x,F_YOKO_GOMA,F_HI_RYU, HorizPin},
            {BanX*1-1,min(ou_x,BanY-1-ou_y),F_NANAME_GOMA,F_KA_UMA, RNanamePin},
            {BanX*1,BanY-1-ou_y,F_YOKO_GOMA,F_HI_RYU, VertPin},
            {BanX*1+1,min(BanX-1-ou_x,BanY-1-ou_y),F_NANAME_GOMA,F_KA_UMA, LNanamePin}
        };
        
        Koma *ou_pos = &shogiBan[ou_y][ou_x];
        
        for(int i=0; i<8; i++) {
            if (scanInf[i][1]>0) {
                int inc = scanInf[i][0];
                Koma *k = ou_pos + inc;
				BitBoard9x9 workKikiB;    // 王手ゴマの位置を記録
                if(*k == EMP) { // 空 間接王手の検索
                    Koma *end_pos = ou_pos + (inc*(scanInf[i][1]+1));
                    Koma koma_flag = (Koma)scanInf[i][3];
					clearBB(&workKikiB);
					setBitBB(&workKikiB,(int)(k-&shogiBan[0][0]));
                    for(k+=inc;k!=end_pos; k+=inc) {
						setBitBB(&workKikiB,(int)(k-&shogiBan[0][0]));
                        if (*k != EMP) {
                            if ((*k & UWATE) != teban) { // 相手駒
                                if(isKoma(*k, koma_flag, 1)) {
                                    printf("間接王手%d\n", i);
									setBitBB(&outePosKikiB, &workKikiB);
									oute_num++;
                                }
                            } else { // 味方駒 壁駒になってないか相手駒を追加検索
                                for (Koma *kk=k+inc; kk!=end_pos; kk+=inc) {
                                    if (*kk != EMP) {
                                        if (isKoma(*kk, koma_flag, 1)) {
                                            printf("壁駒%d\n", i);
                                            setBitBB(&kabePosB, (int)(k-&shogiBan[0][0]));
											int n = (int)(k-&shogiBan[0][0]);
											*(&kabegomaInfo[0][0]+n) = scanInf[i][4];
                                        }
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                    }
                } else if ((*k & UWATE) != teban) { // 相手駒 直接王手の判別
                    if(isKoma(*k, scanInf[i][2], 1)) {
                        setBitBB(&outePosKikiB, (int)(k-&shogiBan[0][0]));
						oute_num++;
                        printf("直接王手%d\n", i);
                    }
                } else { //味方 壁駒になってないか相手駒を追加検索
                    Koma *end_pos = ou_pos + scanInf[i][1];
                    Koma koma_flag = (Koma)scanInf[i][3];
                    
                    for (Koma *kk=k+inc; kk!=end_pos; kk+=inc) {
                        if (*kk != EMP) {
                            if (isKoma(*kk, koma_flag, 1)) {
                                printf("壁駒%d\n", i);
                                setBitBB(&kabePosB, (int)(k-&shogiBan[0][0]));
								int n = (int)(k-&shogiBan[0][0]);
								*(&kabegomaInfo[0][0]+n) = scanInf[i][4];
                            }
                            break;
                        }
                    }
                }
            }
        }

    }
    printf("outen: %d\n", oute_num);
    /*for (int y=0; y<BanY; y++) {
        for (int x=0; x<BanX; x++) {
			printf("%d", kabegomaInfo[y][x]);
        }
        printf("\n");
    }*/
    
    BitBoard9x9 tebanKikiB;
    clearBB(&tebanKikiB);
    BitBoard9x9 uwateKikiB;
    clearBB(&uwateKikiB);


    // まずは盤上の駒移動から
    // 予定: 壁ゴマの位置にある場合は、移動制限
    // 予定: 駒の効きも記録
    // 予定: 成対応必要
    int te_num = 0;
    int to_y, to_x;
    Koma to_k;
    for (int y=0; y<BanY; y++) {
        for(int x=0; x<BanX; x++) {
            Koma k = shogiBan[y][x];
            switch (k) {
                case FU: break;
                    {
                        to_y = y-1; //ルール上、-1にならない。
                        to_k = shogiBan[to_y][x];
                        setBitBB(&tebanKikiB, x, to_y); // 効きの記録
						if (oute_num > 1) continue;
						if (oute_num == 1 && !getBitBB(&outePosKikiB, x, to_y)) continue;

                        if (to_k == EMP || to_k & UWATE) {// 味方がいなければ進める
                            if (kabegomaInfo[y][x]==NoPin
									|| kabegomaInfo[y][x]==VertPin) {
								if (y != 1) { // 成らずの指手
									cs->type = SASHITE_IDOU;
									cs->idou.to_y = to_y;
									cs->idou.from_y = y;
									cs->idou.to_x = cs->idou.from_x = x;
									cs->idou.nari = 0;
									cs++;
									te_num++;
								}
								// 成りの指手
								if (y < 4) {
									cs->type = SASHITE_IDOU;
									cs->idou.to_y = to_y;
									cs->idou.from_y = y;
									cs->idou.to_x = cs->idou.from_x = x;
									cs->idou.nari = 1;
									cs++;
									te_num++;
								}
							}
                        }
                    }
                    break;

                case KY: break; 
                    {
						bool canmove=(oute_num<2 && 
								(kabegomaInfo[y][x]==NoPin
									|| kabegomaInfo[y][x]==VertPin));
						bool ext = false;
                        for (to_y=y-1; to_y>=0 && !ext; to_y--) {
                            to_k = shogiBan[to_y][x];
                            setBitBB(&tebanKikiB, x, to_y);  // 効きの記録
							if (to_k != EMP) 
								if (to_k & UWATE){
									ext=true; // exit loop
									// check to_k is pinned.
									for (int yy=to_y-1; yy>=0; yy--) {
										if (shogiBan[yy][x] != EMP) {
											if (shogiBan[yy][x]==UOU)
												ukabegomaInfo[to_y][x]=VertPin;
											break;
										}
									}
								} else break;
							if (canmove) {
								if (oute_num == 1 && !getBitBB(&outePosKikiB, x, to_y)) continue; 
                                if (to_y > 0) {
                                    cs->type = SASHITE_IDOU;
                                    cs->idou.to_y = to_y;
                                    cs->idou.to_x = x;
                                    cs->idou.from_y = y;
                                    cs->idou.from_x = x;
                                    cs->idou.nari = 0;
                                    cs++;
                                    te_num++;
                                }
                                // 成りの指手
                                if (to_y < 3) {
                                    cs->type = SASHITE_IDOU;
                                    cs->idou.to_y = to_y;
                                    cs->idou.to_x = x;
                                    cs->idou.from_y = y;
                                    cs->idou.from_x = x;
                                    cs->idou.nari = 1;
                                    cs++;
                                    te_num++;
                                }
							}
                        }
                    }
                    break;
                case KE: break;
					{
						bool cantmove=(oute_num>=2 || 
								(kabegomaInfo[y][x]!=NoPin));
						for (int r=0; r<2; r++) {
							to_x = x+KE_range[r][0];
							to_y = y+KE_range[r][1];
							
							// 盤外チェック
							if (to_x<0 || to_x>=BanX) continue;
							if (to_y<0 || to_y>=BanY) continue;
							setBitBB(&tebanKikiB, to_x, to_y);

							if (cantmove) continue;
							to_k = shogiBan[to_y][to_x];

							if (to_k == EMP || (to_k & UWATE)) {// 味方がいなければ進める
								if (oute_num == 1 && !getBitBB(&outePosKikiB, to_x, to_y)) continue; 
								if (to_y > 1) {
									cs->type = SASHITE_IDOU;
									cs->idou.to_y = to_y;
									cs->idou.to_x = to_x;
									cs->idou.from_y = y;
									cs->idou.from_x = x;
									cs->idou.nari = 0;
									cs++;
									te_num++;
								}
								// 成りの指手
								if (to_y < 3) {
									cs->type = SASHITE_IDOU;
									cs->idou.to_y = to_y;
									cs->idou.to_x = to_x;
									cs->idou.from_y = y;
									cs->idou.from_x = x;
									cs->idou.nari = 1;
									cs++;
									te_num++;
								}
							}
						}
					}
                    break;
                case GI: break;
					{
						setBitsBB(&tebanKikiB, x, y, GI_pttn);
						if(oute_num>=2 || kabegomaInfo[y][x]==HorizPin) break;
						int rs,re;
						switch(kabegomaInfo[y][x]) {
							case VertPin: rs=0;re=1; break;
							case LNanamePin :rs=1;re=3; break;
							case RNanamePin: rs=3;re=5; break;
							default: rs=0;re=5; break;
						}
						for (int r=rs; r<re; r++) {
							to_x = x+GI_range[r][0];
							to_y = y+GI_range[r][1];
							
							// 盤外チェック
							if (to_x<0 || to_x>=BanX) continue;
							if (to_y<0 || to_y>=BanY) continue;
							
							to_k = shogiBan[to_y][to_x];
							
							if (to_k == EMP || (to_k & UWATE) ) {// 味方がいなければ進める
								if (oute_num == 1 && !getBitBB(&outePosKikiB, to_x, to_y)) continue; 
								cs->type = SASHITE_IDOU;
								cs->idou.to_y = to_y;
								cs->idou.to_x = to_x;
								cs->idou.from_y = y;
								cs->idou.from_x = x;
								cs->idou.nari = 0;
								cs++;
								te_num++;
								
								// 成りの指手
								if (to_y < 3 || y<=2) {
									cs->type = SASHITE_IDOU;
									cs->idou.to_y = to_y;
									cs->idou.to_x = to_x;
									cs->idou.from_y = y;
									cs->idou.from_x = x;
									cs->idou.nari = 1;
									cs++;
									te_num++;
								}
								
							}
						}
					}
                    break;
                case KI:
				case NFU:
				case NKY:
				case NKE:
				case NGI: break;
					{
						setBitsBB(&tebanKikiB, x, y, KI_pttn);
						int rs,re;
						switch(kabegomaInfo[y][x]) {
							case VertPin: rs=0;re=2; break;
							case HorizPin: rs=2;re=4; break;
							case LNanamePin :rs=4;re=5; break;
							case RNanamePin: rs=5;re=6; break;
							default: rs=0;re=6; break;
						}
						for (int r=rs; r<re; r++) {
							to_x = x+KI_range[r][0];
							to_y = y+KI_range[r][1];
							
							// 盤外チェック
							if (to_x<0 || to_x>=BanX) continue;
							if (to_y<0 || to_y>=BanY) continue;
							
							to_k = shogiBan[to_y][to_x];

							if (to_k == EMP || (to_k & UWATE)) {// 味方がいなければ進める
								if (oute_num == 1 && !getBitBB(&outePosKikiB, to_x, to_y)) continue; 
								cs->type = SASHITE_IDOU;
								cs->idou.to_y = to_y;
								cs->idou.to_x = to_x;
								cs->idou.from_y = y;
								cs->idou.from_x = x;
								cs->idou.nari = 0;
								cs++;
								te_num++;
							}
						}
					}
					break;
                
                case UMA:  // 十字4箇所だけ
					cs=createSashiteUMA(cs, &tebanKikiB, shogiBan, x, y,
						kabegomaInfo, oute_num, &outePosKikiB);
				case KA: break; // 馬と共通。馬のときは成りなし
                    {
                        // {x移動,y移動,終了条件x,終了条件y}
                        int scanInf[4][4] = {{-1,-1,-1,-1},{1,-1,BanX,-1},{-1,1,-1,BanY},{1,1,BanX,BanY}};
                        for (int r=0; r<4; r++) {
                            int inc_x = scanInf[r][0];
                            int inc_y = scanInf[r][1];
                            int end_x = scanInf[r][2];
                            int end_y = scanInf[r][3];
                            
                            
                            for (to_x = x+inc_x,to_y = y+inc_y; to_x != end_x && to_y != end_y; to_x+=inc_x, to_y+=inc_y) {
                                to_k = shogiBan[to_y][to_x];
                                setBitBB(&tebanKikiB, to_x, to_y);  // 効きの記録
                                
                                if (to_k == EMP || (to_k & UWATE) != teban) {// 味方がいなければ進める
                                    if (getBitBB(&kabePosB, x, y)) {// 壁駒の場合は王と同じ斜め向きなら可
                                        if (ou_x == x || ou_y == y) { // 王が横か縦の場合は斜めに動けない
                                            continue;
                                        } else if ((ou_x<x && ou_y<y) || (ou_x>x && ou_y>y)) {    // "\" 以外動いたらダメ
                                            if (inc_x != inc_y) {
                                                if (to_k != EMP) break;
                                                else continue;
                                            }
                                        } else {
                                            if (inc_x == inc_y) {
                                                if (to_k != EMP) break;
                                                else continue;
                                            };
                                        }
                                    }
                                        
                                    cs->type = SASHITE_IDOU;
                                    cs->idou.to_y = to_y;
                                    cs->idou.to_x = to_x;
                                    cs->idou.from_y = y;
                                    cs->idou.from_x = x;
                                    cs->idou.nari = 0;
                                    cs++;
                                    te_num++;
                                    
                                    // 成りの指手
                                    if ((to_y < 3 || y < 3) && k==KA) {
                                        cs->type = SASHITE_IDOU;
                                        cs->idou.to_y = to_y;
                                        cs->idou.to_x = to_x;
                                        cs->idou.from_y = y;
                                        cs->idou.from_x = x;
                                        cs->idou.nari = 1;
                                        cs++;
                                        te_num++;
                                    }
                                    if (to_k != EMP) break;
                                } else {
                                    break;
                                }
                            }
                        }
                    }
                    break;
                
                case RYU: break; // 斜め4つだけ
                    for (int r=0; r<4; r++) {
                        to_x = x+RYU_range[r][0];
                        to_y = y+RYU_range[r][1];
                        
                        // 盤外チェック
                        if (to_x<0 || to_x>=BanX) continue;
                        if (to_y<0 || to_y>=BanY) continue;
                        
                        to_k = shogiBan[to_y][to_x];
                        setBitBB(&tebanKikiB, to_x, to_y);  // 効きの記録

                        if (to_k == EMP || (to_k & UWATE) != teban) {// 味方がいなければ進める
                            if (getBitBB(&kabePosB, x, y)) {   // 壁駒の場合は判断が必要
                                if (ou_x == x || ou_y == y) { // 横か縦の場合は斜めに動いたらダメ
                                    continue;
                                } else if ((ou_x<x && ou_y<y) || (ou_x>x && ou_y>y)) {    // "\" 以外動いたらダメ
                                    if (RYU_range[r][0] != RYU_range[r][1]) continue;
                                } else {    // "/" 以外動いたらダメ
                                    if ((RYU_range[r][0]+RYU_range[r][1])) continue;
                                }
                            }
                            setBitBB(&tebanKikiB, to_x, to_y);
                            cs->type = SASHITE_IDOU;
                            cs->idou.to_y = to_y;
                            cs->idou.to_x = to_x;
                            cs->idou.from_y = y;
                            cs->idou.from_x = x;
                            cs->idou.nari = 0;
                            cs++;
                            te_num++;
                        }

                    }
                case HI: break;  // 竜と共通。竜のときは成りなし
                    {
                        // {移動幅, 終了条件}, {変数}　// メモ: 最適化が効きにくいかも
                        int scanInf1[4][2] = {{-1,-1},{-1,-1},{1,BanY},{1,BanX}};
                        int *scanInf2[4] = {&to_y, &to_x, &to_y, &to_x};
                        
                        for (int r=0; r<4; r++) {
                            int* to_xy =scanInf2[r];
                            int inc =scanInf1[r][0], end =scanInf1[r][1];
                            to_x = x;
                            to_y = y;
                            (*to_xy)+= inc;
                            for (;*to_xy != end;(*to_xy)+=inc) {
                                to_k = shogiBan[to_y][to_x];
                                setBitBB(&tebanKikiB, to_x, to_y);  // 効きの記録
                                
                                if (to_k == EMP || (to_k & UWATE) != teban) {// 味方がいなければ進める
                                    if (getBitBB(&kabePosB, x, y)) {// 壁駒の場合は王と縦と並んでて縦移動、もしくは横に並んでて横移動なら可
                                        if (ou_x == x) {
                                            if(x != to_x) {
                                                if (to_k != EMP) break;
                                                else continue;
                                            }
                                        } else if (ou_y == y) {
                                            if(y != to_y) {
                                                if (to_k != EMP) break;
                                                else continue;
                                            }
                                        } else { //斜め壁なので基本動けない
                                            if (to_k != EMP) break;
                                            else continue;
                                        }
                                    }
                                    
                                    cs->type = SASHITE_IDOU;
                                    cs->idou.to_y = to_y;
                                    cs->idou.to_x = to_x;
                                    cs->idou.from_y = y;
                                    cs->idou.from_x = x;
                                    cs->idou.nari = 0;
                                    cs++;
                                    te_num++;
                                    
                                    // 成りの指手
                                    if ((to_y < 3 || y < 3) && k==HI) {
                                        cs->type = SASHITE_IDOU;
                                        cs->idou.to_y = to_y;
                                        cs->idou.to_x = to_x;
                                        cs->idou.from_y = y;
                                        cs->idou.from_x = x;
                                        cs->idou.nari = 1;
                                        cs++;
                                        te_num++;
                                    }
                                    if (to_k != EMP) break;
                                } else {
                                    break;
                                }
                            }
                        }
                        
                    }
                    break;

                // 上手の効き記録
                    
                case UFU: 
                    to_y = y+1;
                    setBitBB(&uwateKikiB, x, to_y);
                    break;
                    
                case UKY: 
                    for (to_y=y+1; to_y<BanY; to_y++) {
                        to_k = shogiBan[to_y][x];
                        setBitBB(&uwateKikiB, x, to_y);
                        if ((to_k != EMP)&&(to_k!=OU)) break;
                    }
                    break;
                
                case UKE:
                    if (x>0) setBitBB(&uwateKikiB, x-1, y+2);
                    if (x<BanX) setBitBB(&uwateKikiB, x+1, y+2);
                    break;

				case UGI:
                    for (int r=0; r<5; r++) {
                        to_x = x-GI_range[r][0];
                        to_y = y-GI_range[r][1];
                        
                        // 盤外チェック
                        if (to_x<0 || to_x>=BanX) continue;
                        if (to_y<0 || to_y>=BanY) continue;
                        
                        setBitBB(&uwateKikiB, to_x, to_y);
                    }
                    break;
                
				case UKI:
				case UNFU:
				case UNKY:
				case UNKE:
				case UNGI:
                    for (int r=0; r<6; r++) {
                        to_x = x-KI_range[r][0];
                        to_y = y-KI_range[r][1];
                        
                        // 盤外チェック
                        if (to_x<0 || to_x>=BanX) continue;
                        if (to_y<0 || to_y>=BanY) continue;
                        
                        setBitBB(&uwateKikiB, to_x, to_y);
                    }
                    break;

				case UUMA:
                    for (int r=0; r<4; r++) {
                        to_x = x-UMA_range[r][0];
                        to_y = y-UMA_range[r][1];
                        
                        // 盤外チェック
                        if (to_x<0 || to_x>=BanX) continue;
                        if (to_y<0 || to_y>=BanY) continue;
                        
                        setBitBB(&uwateKikiB, to_x, to_y);
                    }

				case UKA:
                    {
                        // {x移動,y移動,終了条件x,終了条件y}
                        int scanInf[4][4] = {{-1,-1,-1,-1},{1,-1,BanX,-1},{-1,1,-1,BanY},{1,1,BanX,BanY}};
                        for (int r=0; r<4; r++) {
                            int inc_x = scanInf[r][0];
                            int inc_y = scanInf[r][1];
                            int end_x = scanInf[r][2];
                            int end_y = scanInf[r][3];
                            
                            
                            for (to_x = x+inc_x,to_y = y+inc_y; to_x != end_x && to_y != end_y; to_x+=inc_x, to_y+=inc_y) {
                                to_k = shogiBan[to_y][to_x];
                                setBitBB(&uwateKikiB, to_x, to_y);  // 効きの記録
                                if ((to_k != EMP)&&(to_k!=OU)) break;
							}
						}
					}
                 
                    break;
                case URYU:
                    for (int r=0; r<4; r++) {
                        to_x = x+RYU_range[r][0];
                        to_y = y+RYU_range[r][1];
                        
                        // 盤外チェック
                        if (to_x<0 || to_x>=BanX) continue;
                        if (to_y<0 || to_y>=BanY) continue;
                        
                        to_k = shogiBan[to_y][to_x];
                        setBitBB(&uwateKikiB, to_x, to_y);  // 効きの記録
					}

               case UHI:
                    {
                        // {移動幅, 終了条件}, {変数}　// メモ: 最適化が効きにくいかも
                        int scanInf1[4][2] = {{-1,-1},{-1,-1},{1,BanY},{1,BanX}};
                        int *scanInf2[4] = {&to_y, &to_x, &to_y, &to_x};
                        
                        for (int r=0; r<4; r++) {
                            int* to_xy =scanInf2[r];
                            int inc =scanInf1[r][0], end =scanInf1[r][1];
                            to_x = x;
                            to_y = y;
                            (*to_xy)+= inc;
                            for (;*to_xy != end;(*to_xy)+=inc) {
                                to_k = shogiBan[to_y][to_x];
                                setBitBB(&uwateKikiB, to_x, to_y);
                                if ((to_k != EMP)&&(to_k!=OU)) break;
							}
						}
					}
                    break;            

				case UOU:
                    for (int r=0; r<8; r++) {
                        to_x = x-OU_range[r][0];
                        to_y = y-OU_range[r][1];
                        
                        // 盤外チェック
                        if (to_x<0 || to_x>=BanX) continue;
                        if (to_y<0 || to_y>=BanY) continue;
                        
                        setBitBB(&uwateKikiB, to_x, to_y);
                    }
                    break;

				default:
                    break;
            }
        }
    }

    //王の移動 相手の効きがある場合は移動できない
    /*for (int r=0; r<8; r++) {
        to_x = ou_x+OU_range[r][0];
        to_y = ou_y+OU_range[r][1];
                        
        // 盤外チェック
        if (to_x<0 || to_x>=BanX) continue;
        if (to_y<0 || to_y>=BanY) continue;
        
        to_k = shogiBan[to_y][to_x];
        setBitBB(&tebanKikiB, to_x, to_y);  // 効きの記録
                        
        if (to_k == EMP || (to_k & UWATE) != teban) {// 味方がいなければ進める
            if (getBitBB(&uwateKikiB, to_x, to_y)) continue; // 相手の効きあれば進めない
            cs->type = SASHITE_IDOU;
            cs->idou.to_y = to_y;
            cs->idou.to_x = to_x;
            cs->idou.from_y = ou_y;
            cs->idou.from_x = ou_x;
            cs->idou.nari = 0;
            cs++;
            te_num++;
        }
    }*/
                        
    // 予定: 打ち
    // 予定: 効きから打ちふ詰めも検出
	printBB(stdout, &tebanKikiB);    
   /* for (int y=0; y<BanY; y++) {
        for (int x=0; x<BanX; x++) {
			printf("%d", ukabegomaInfo[y][x]);
        }
        printf("\n");
    }*/
    
    *n = cs-s;
}

Sashite *createSashiteUMA(
	Sashite *te,
	BitBoard9x9 *tebanKikiB,
	Koma (*shogiBan)[BanX],
	int x, int y,
	int (*kabegomaInfo)[BanX],
	int oute_num,
	BitBoard9x9 *outePosKikiB)
{
    const uint32_t UMA_pttn = 0x00080a02;
    static int UMA_range[4][2] = {{0,-1}, {0,1}, {-1,0}, {1,0}};

    setBitsBB(tebanKikiB, x, y, UMA_pttn);
    if(oute_num>=2) return te;
	
	int rs,re;
	switch(kabegomaInfo[y][x]) {
		case VertPin: rs=0;re=2; break;
		case HorizPin: rs=2;re=4; break;
		case LNanamePin :
		case RNanamePin:
            return te;
		default: rs=0;re=4; break;
	}
	for (int r=rs; r<re; r++) {
        int to_x = x+UMA_range[r][0];
        int to_y = y+UMA_range[r][1];
        
        // 盤外チェック
        if (to_x<0 || to_x>=BanX) continue;
        if (to_y<0 || to_y>=BanY) continue;
        
        Koma to_k = shogiBan[to_y][to_x];
        
        if (to_k == EMP || (to_k & UWATE)) {// 味方がいなければ進める
            if (oute_num == 1 && !getBitBB(outePosKikiB, to_x, to_y)) continue;
            
            te->type = SASHITE_IDOU;
            te->idou.to_y = to_y;
            te->idou.to_x = to_x;
            te->idou.from_y = y;
            te->idou.from_x = x;
            te->idou.nari = 0;
            te++;
        }
    }
    return te;
}
