//
//  shogirule.cpp
//  ShogiDBTool
//
//  Created by tosyama on 2016/2/2.
//  Copyright (c) 2016 tosyama. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
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
    int64_t topmid;
    int32_t bottom;
} BitBoard9x9;

inline void clearBB(BitBoard9x9 *b){
    b->topmid = 0;
    b->bottom = 0;
}

inline void setBitBB(BitBoard9x9 *b, int x, int y){
    if (y<6) b->topmid |= ((int64_t)1 << (BanX*y + x));
    else b->bottom |= (1 << (BanX*(y-6) + x));
}

inline void setBitBB(BitBoard9x9 *b, int n){
    if (n<54) b->topmid |= ((int64_t)1 << n);
    else b->bottom |= (1 << (n-54));
}

inline int getBitBB(const BitBoard9x9 *b, int x, int y){
    if (y<6) return (int)((int64_t)1 & (b->topmid >> (BanX*y + x)));
    else return (int)((int64_t)1 & (b->bottom >> (BanX*(y-6) + x)));
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

// 手の生成
void createSashite(ShogiKykumen *shogi, int uwate, Sashite *s, int *n)
{
    Koma (*shogiBan)[BanX] = shogi->shogiBan;
    int (*komaDai)[DaiN] = shogi->komaDai;
    *n = 0;
    Sashite *cs = s;
    Koma teban = uwate ? UWATE : (Koma)0;
    
    static int OU_range[8][2] = {{-1,-1}, {0,-1}, {1,-1}, {-1,0},{1,0},{-1,1},{0,1},{1,1}};
    static int KI_range[6][2] = {{-1,-1}, {0,-1}, {1,-1}, {-1,0},{1,0},{0,1}};
    static int GI_range[5][2] = {{-1,-1}, {0,-1}, {1,-1}, {-1,1},{1,1}};
    static int KE_range[2][2] = {{-1,-2}, {1,-2}};
    static int UMA_range[4][2] = {{0,-1}, {-1,0}, {1,0},{0,1}};
    static int RYU_range[4][2] = {{-1,-1}, {1,-1}, {-1,1},{1,1}};
    
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
    BitBoard9x9 outePosB;    // 王手ゴマの位置を記録
    BitBoard9x9 kabePosB;   // pinされている駒の位置を記録
    
    clearBB(&outePosB);
    clearBB(&kabePosB);
    {
        if (!(bangai & OutF2L) && isRangeKE(shogiBan[ou_y-2][ou_x-1],1)) {   // 桂王手の検出
            setBitBB(&outePosB, ou_x-1, ou_y-2);
            printf("左桂王手\n");
        } if (!(bangai & OutF2R) && isRangeKE(shogiBan[ou_y-2][ou_x+1],1)) {    // 桂王手の検出
            setBitBB(&outePosB, ou_x+1, ou_y-2);
            printf("右桂王手\n");
        }

        // {検出移動幅, ループ数, 直接王手駒, 間接王手駒}
        int  scanInf[8][4] = {
            {BanX*-1-1, min(ou_x,ou_y),F_NAMAE_GOMA,F_KA_UMA},
            {BanX*-1,ou_y,F_MAE_GOMA,F_KY_HI_RYU},
            {BanX*-1+1, min(BanX-1-ou_x,ou_y),F_NAMAE_GOMA,F_KA_UMA},
            {-1,ou_x,F_YOKO_GOMA,F_HI_RYU},
            {1,BanX-1-ou_x,F_YOKO_GOMA,F_HI_RYU},
            {BanX*1-1,min(ou_x,BanY-1-ou_y),F_NANAME_GOMA,F_KA_UMA},
            {BanX*1,BanY-1-ou_y,F_YOKO_GOMA,F_HI_RYU},
            {BanX*1+1,min(BanX-1-ou_x,BanY-1-ou_y),F_NANAME_GOMA,F_KA_UMA}
        };
        
        Koma *ou_pos = &shogiBan[ou_y][ou_x];
        
        for(int i=0; i<8; i++) {
            if (scanInf[i][1]>0) {
                int inc = scanInf[i][0];
                Koma *k = ou_pos + inc;
                if(*k == EMP) { // 空 間接王手の検索
                    Koma *end_pos = ou_pos + (inc*(scanInf[i][1]+1));
                    Koma koma_flag = (Koma)scanInf[i][3];
                    for(k+=inc;k!=end_pos; k+=inc) {
                        if (*k != EMP) {
                            if ((*k & UWATE) != teban) { // 相手駒
                                if(isKoma(*k, koma_flag, 1)) {
                                    printf("間接王手%d\n", i);
                                    setBitBB(&outePosB, (int)(k-&shogiBan[0][0]));
                                }
                            } else { // 味方駒 壁駒になってないか相手駒を追加検索
                                for (Koma *kk=k+inc; kk!=end_pos; kk+=inc) {
                                    if (*kk != EMP) {
                                        if (isKoma(*kk, koma_flag, 1)) {
                                            printf("壁駒%d\n", i);
                                            setBitBB(&kabePosB, (int)(k-&shogiBan[0][0]));
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
                        setBitBB(&outePosB, (int)(k-&shogiBan[0][0]));
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
                            }
                            break;
                        }
                    }
                }
            }
        }

    }
    
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
                        if (to_k == EMP || to_k & UWATE) {// 味方がいなければ進める
                            if (getBitBB(&kabePosB, x, y) && ou_x!=x) continue;   // 壁駒の場合は王と縦と並んでる以外は動かせない
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
                    break;

                case KY: break;
                    {
                        to_x = x;
                        for (to_y=y-1; to_y>=0; to_y--) {
                            to_k = shogiBan[to_y][to_x];
                            setBitBB(&tebanKikiB, to_x, to_y);  // 効きの記録
                            
                            if (to_k == EMP || (to_k & UWATE) != teban) {// 味方がいなければ進める
                                if (getBitBB(&kabePosB, x, y) && ou_x!=x) {// 壁駒の場合は王と縦と並んでる以外は動かせない
                                    if (to_k != EMP) break;
                                    else continue;  //効きを記録するためループ継続
                                }
                                if (to_y > 0) {
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
                                if (to_k != EMP) break;
                            } else {
                                break;
                            }
                        }
                    }
                    break;
                case KE: break;
                    for (int r=0; r<2; r++) {
                        to_x = x+KE_range[r][0];
                        to_y = y+KE_range[r][1];
                        
                        to_k = shogiBan[to_y][to_x];
                        
                        // 盤外チェック
                        if (to_x<0 || to_x>=BanX) continue;
                        if (to_y<0 || to_y>=BanY) continue;

                        setBitBB(&tebanKikiB, to_x, to_y);  // 効きの記録

                        if (to_k == EMP || (to_k & UWATE) != teban) {// 味方がいなければ進める
                            if (getBitBB(&kabePosB, x, y)) continue;    // 壁駒の場合は動けない
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
                    break;
                case GI: break;
                    for (int r=0; r<5; r++) {
                        to_x = x+GI_range[r][0];
                        to_y = y+GI_range[r][1];
                        
                        // 盤外チェック
                        if (to_x<0 || to_x>=BanX) continue;
                        if (to_y<0 || to_y>=BanY) continue;
                        
                        to_k = shogiBan[to_y][to_x];
                        setBitBB(&tebanKikiB, to_x, to_y);  // 効きの記録
                        
                        if (to_k == EMP || (to_k & UWATE) != teban) {// 味方がいなければ進める
                            if (getBitBB(&kabePosB, x, y)) {   // 壁駒の場合は判断が必要
                                if (ou_x == x) { // 縦以外動いたらダメ
                                    if (GI_range[r][0] != 0) continue;
                                } else if (ou_y == y) { // 横以外動いたらダメ
                                    if (GI_range[r][1] != 0) continue;
                                } else if ((ou_x<x && ou_y<y) || (ou_x>x && ou_y>y)) {    // "\" 以外動いたらダメ
                                    if (GI_range[r][0] != GI_range[r][1]) continue;
                                } else {    // "/" 以外動いたらダメ
                                    if ((GI_range[r][0]+GI_range[r][1])) continue;
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
                    break;
                case KI: break;
                    for (int r=0; r<6; r++) {
                        to_x = x+KI_range[r][0];
                        to_y = y+KI_range[r][1];
                        
                        // 盤外チェック
                        if (to_x<0 || to_x>=BanX) continue;
                        if (to_y<0 || to_y>=BanY) continue;
                        
                        to_k = shogiBan[to_y][to_x];
                        setBitBB(&tebanKikiB, to_x, to_y);  // 効きの記録

                        if (to_k == EMP || (to_k & UWATE) != teban) {// 味方がいなければ進める
                            if (getBitBB(&kabePosB, x, y)) {   // 壁駒の場合は判断が必要
                                if (ou_x == x) { // 縦以外動いたらダメ
                                    if (KI_range[r][0] != 0) continue;
                                } else if (ou_y == y) { // 横以外動いたらダメ
                                    if (KI_range[r][1] != 0) continue;
                                } else if ((ou_x<x && ou_y<y) || (ou_x>x && ou_y>y)) {    // "\" 以外動いたらダメ
                                    if (KI_range[r][0] != KI_range[r][1]) continue;
                                } else {    // "/" 以外動いたらダメ
                                    if ((KI_range[r][0]+KI_range[r][1])) continue;
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
                    break;
                
                case UMA: break;  // 十字4箇所だけ
                    for (int r=0; r<4; r++) {
                        to_x = x+UMA_range[r][0];
                        to_y = y+UMA_range[r][1];
                        
                        // 盤外チェック
                        if (to_x<0 || to_x>=BanX) continue;
                        if (to_y<0 || to_y>=BanY) continue;
                        
                        to_k = shogiBan[to_y][to_x];
                        setBitBB(&tebanKikiB, to_x, to_y);  // 効きの記録
                        
                        if (to_k == EMP || (to_k & UWATE) != teban) {// 味方がいなければ進める
                            if (getBitBB(&kabePosB, x, y)) {   // 壁駒の場合は判断が必要
                                if (ou_x == x) {
                                    if(x != to_x) continue;
                                } else if (ou_y == y) {
                                    if(y != to_y) continue;
                                } else { //斜め壁なので基本動けない
                                    continue;
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
                        if (to_k != EMP) break;
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
                                if (to_k != EMP) break;
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
                                if (to_k != EMP) break;
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
    for (int r=0; r<8; r++) {
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
    }
                        
    // 予定: 打ち
    // 予定: 効きから打ちふ詰めも検出
    
    *n = te_num;
}

