//
//	shogirule.cpp
//	ShogiDBTool
//
//	Created by tosyama on 2016/2/2.
//	Copyright (c) 2016 tosyama. All rights reserved.
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

#define min(a,b)	((a)<(b)?(a):(b))

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

inline void setBitsBB(BitBoard9x9 *b, const BitBoard9x9 *orb)
{
	b->topmid |= orb->topmid;
	b->bottom |= orb->bottom;
}

inline void setAndBitsBB(BitBoard9x9 *b, const BitBoard9x9 *andb)
{
	b->topmid &= andb->topmid;
	b->bottom &= andb->bottom;
}

const uint32_t GI_pttn = 0x00140007;
const uint32_t KI_pttn = 0x00080a07;
const uint32_t UMA_pttn = 0x00080a02;
const uint32_t RYU_pttn = 0x00140005;
const uint32_t OU_pttn = 0x001c0a07;
const uint32_t UGI_pttn = 0x001c0005;
const uint32_t UKI_pttn = 0x001c0a02;

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

//駒の効きの判定
inline int isKoma(Koma k, int koma_flag)
{
	return koma_flag & (1 << (k&KOMATYPE2));
}

#define NoPin		0
#define VertPin		1
#define HorizPin	2
#define LNanamePin	4
#define RNanamePin	8

int checkOute(BitBoard9x9 *outePosKikiB, int (*pinInfo)[BanX],
		Koma (*shogiBan)[BanX], int ou_x, int ou_y);
Sashite *createSashiteFU(Sashite *te, 
		Koma (*shogiBan)[BanX], int x, int y,
		int pin, int oute_num, BitBoard9x9 *outePosKikiB);
Sashite *createSashiteKY(Sashite *te,
		Koma (*shogiBan)[BanX], int x, int y,
		int pin, int oute_num, BitBoard9x9 *outePosKikiB, bool nari);
Sashite *createSashiteKE(Sashite *te, 
		Koma (*shogiBan)[BanX], int x, int y,
		int pin, int oute_num, BitBoard9x9 *outePosKikiB);
Sashite *createSashiteGI(Sashite *te,
		Koma (*shogiBan)[BanX], int x, int y,
		int pin, int oute_num, BitBoard9x9 *outePosKikiB);
Sashite *createSashiteKI(Sashite *te,
		Koma (*shogiBan)[BanX], int x, int y,
		int pin, int oute_num, BitBoard9x9 *outePosKikiB);
Sashite *createSashiteUMA(Sashite *te,
		Koma (*shogiBan)[BanX], int x, int y,
		int pin, int oute_num, BitBoard9x9 *outePosKikiB);
Sashite *createSashiteKA(Sashite *te,
		Koma (*shogiBan)[BanX], int x, int y,
		int pin, int oute_num, BitBoard9x9 *outePosKikiB, bool nari);
Sashite *createSashiteRYU(Sashite *te,
		Koma (*shogiBan)[BanX], int x, int y,
		int pin, int oute_num, BitBoard9x9 *outePosKikiB);
Sashite *createSashiteHI(Sashite *te,
		Koma (*shogiBan)[BanX], int x, int y,
		int pin, int oute_num, BitBoard9x9 *outePosKikiB, bool nari);
Sashite *createSashiteOU(Sashite *te, 
		Koma (*shogiBan)[BanX], int x, int y,
		BitBoard9x9 *noUwateKikiB);
Sashite *createSashiteUchi(Sashite *te, Koma k,
		Koma (*shogiBan)[BanX],
		int oute_num, BitBoard9x9 *outePosKikiB, int uchiLimit=0);
Sashite *createSashiteUchiFU(Sashite *te, Koma (*shogiBan)[BanX],
		unsigned int usedLine, int oute_num, BitBoard9x9 *outePosKikiB); 
// 手の生成
void createSashiteAll(ShogiKykumen *shogi, Sashite *s, int *n)
{
	Koma (*shogiBan)[BanX] = shogi->shogiBan;
	int (*komaDai)[DaiN] = shogi->komaDai;
	*n = 0;
	Sashite *cs = s;	
	
	int ou_x = shogi->ou_x;
	int ou_y = shogi->ou_y;
	int oute_num = 0;
	BitBoard9x9 outePosKikiB = { 0 };	 // 王手ゴマの位置を記録
	int pinInfo[BanY][BanX] = { 0 };	// pinされている駒の向きを記録
	
	oute_num = checkOute(&outePosKikiB, pinInfo, shogiBan, ou_x, ou_y); 
	
	BitBoard9x9 uwateKikiB = {0}; // 相手駒の効き 王は壁にしない
	unsigned int usedLineFU = 0;

	// まずは盤上の駒移動から
	int to_y, to_x;
	Koma to_k;
	for (int y=0; y<BanY; y++) {
		for(int x=0; x<BanX; x++) {
			Koma k = shogiBan[y][x];
			switch (k) {
				case FU:
					usedLineFU |= (1u << x);
					cs=createSashiteFU(cs, shogiBan, x, y,
							pinInfo[y][x], oute_num, &outePosKikiB);
					break;
				case KY:
					cs=createSashiteKY(cs, shogiBan, x, y,
							pinInfo[y][x], oute_num, &outePosKikiB, k==RYU);
					break;
				case KE:
					cs=createSashiteKE(cs, shogiBan, x, y,
							pinInfo[y][x], oute_num, &outePosKikiB);
					break;
				case GI:
					cs=createSashiteGI(cs, shogiBan, x, y,
							pinInfo[y][x], oute_num, &outePosKikiB);
					break;
				case KI:
				case NFU:
				case NKY:
				case NKE:
				case NGI: 
					cs=createSashiteKI(cs, shogiBan, x, y,
							pinInfo[y][x], oute_num, &outePosKikiB);
					break;
				
				case UMA: // 十字4箇所だけ
					cs=createSashiteUMA(cs, shogiBan, x, y,
							pinInfo[y][x], oute_num, &outePosKikiB);
				case KA: // 馬と共通。馬のときは成りなし
					cs=createSashiteKA(cs, shogiBan, x, y,
							pinInfo[y][x], oute_num, &outePosKikiB, k==UMA);
					break;
				
				case RYU: // 斜め4つだけ
					cs=createSashiteRYU(cs, shogiBan, x, y,
							pinInfo[y][x], oute_num, &outePosKikiB);

				case HI: // 竜と共通。竜のときは成りなし
					cs=createSashiteHI(cs, shogiBan, x, y, pinInfo[y][x], oute_num, &outePosKikiB, k==RYU);
					break;
				
				// 上手の効きと位置の記録
				case UFU:
					assert(y < (BanY-1));
					setBitBB(&uwateKikiB, x, y+1);
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
					if (x<(BanX-1)) setBitBB(&uwateKikiB, x+1, y+2);
					break;

				case UGI:
					setBitsBB(&uwateKikiB, x, y, UGI_pttn);
					break;
				
				case UKI:
				case UNFU:
				case UNKY:
				case UNKE:
				case UNGI:
					setBitsBB(&uwateKikiB, x, y, UKI_pttn);
					break;

				case UUMA:
					setBitsBB(&uwateKikiB, x, y, UMA_pttn);

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
								setBitBB(&uwateKikiB, to_x, to_y);	// 効きの記録
								if ((to_k != EMP)&&(to_k!=OU)) break;
							}
						}
					}
					break;
				case URYU:
					setBitsBB(&uwateKikiB, x, y, RYU_pttn);

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
					setBitsBB(&uwateKikiB, x, y, OU_pttn);
					break;

				default:
					break;
			}
		}
	}

	//王の移動 相手の効きがある場合は移動できない
	{
		BitBoard9x9 noUwateKiki;
		noUwateKiki.topmid = ~uwateKikiB.topmid;
		noUwateKiki.bottom = ~uwateKikiB.bottom;
		cs=createSashiteOU(cs, shogiBan, ou_x, ou_y, &noUwateKiki);
	}
							
	if (komaDai[0][FU])
		cs = createSashiteUchiFU(cs, shogiBan, usedLineFU, oute_num, &outePosKikiB);
	if (komaDai[0][KY])
		cs = createSashiteUchi(cs, KY, shogiBan, oute_num, &outePosKikiB, 1);
	if (komaDai[0][KE])
		cs = createSashiteUchi(cs, KE, shogiBan, oute_num, &outePosKikiB, 2);
	if (komaDai[0][GI])
		cs = createSashiteUchi(cs, GI, shogiBan, oute_num, &outePosKikiB);
	if (komaDai[0][KI])
		cs = createSashiteUchi(cs, KI, shogiBan, oute_num, &outePosKikiB);
	if (komaDai[0][KA])
		cs = createSashiteUchi(cs, KA, shogiBan, oute_num, &outePosKikiB);
	if (komaDai[0][HI])
		cs = createSashiteUchi(cs, HI, shogiBan, oute_num, &outePosKikiB);
	*n = cs-s;
}

inline int existsOuteGomaInLine(
		BitBoard9x9 *outePosKikiB, int (*pinInfo)[BanX],
		Koma (*shogiBan)[BanX], int x, int y,
		int incx, int incy, int maxloop,
		int direct, int indirect, int pin)
{
	if (maxloop==0) return 0;

	BitBoard9x9 workBB = {0};
	setBitBB(&workBB, x, y);
	for (int i=0; i<maxloop; i++) {
		x+=incx; y+=incy;
		Koma k = shogiBan[y][x];
		setBitBB(&workBB, x, y);
		if (k != EMP) {
			if (k & UWATE) {
				if (isKoma(k, i==0 ? direct : indirect)){
					setBitsBB(outePosKikiB,&workBB);
					return 1;
				}
				return 0;
			}
			// check pinned.
			int xx = x; int yy = y;
			for (int j=i+1;j<maxloop;j++) {
				xx+=incx; yy+=incy;
				k=shogiBan[yy][xx];
				if (k != EMP) {
					if ((k&UWATE)&&isKoma(k, indirect)){
						pinInfo[y][x] = pin;
					}
					return 0;
				}
			}
			return 0;
		}
	}
	return 0;
}

int checkOute(BitBoard9x9 *outePosKikiB, int (*pinInfo)[BanX],
		Koma (*shogiBan)[BanX], int ou_x, int ou_y)
{
	int oute_num = 0;
	if (ou_y >= 2) {
		if (ou_x >= 1 && shogiBan[ou_y-2][ou_x-1]==UKE) {
			setBitBB(outePosKikiB, ou_x-1, ou_y-2);
			oute_num++;
		} else if (ou_x <= (BanX-2) && shogiBan[ou_y-2][ou_x+1]==UKE) {
			setBitBB(outePosKikiB, ou_x+1, ou_y-2);
			oute_num++;
		}
	}
	oute_num += existsOuteGomaInLine(outePosKikiB, pinInfo, shogiBan, ou_x, ou_y, -1, -1, min(ou_x,ou_y), F_NAMAE_GOMA, F_KA_UMA, LNanamePin); 
	oute_num += existsOuteGomaInLine(outePosKikiB, pinInfo, shogiBan, ou_x, ou_y, 1, 1, min(BanX-1-ou_x,BanY-1-ou_y), F_NANAME_GOMA, F_KA_UMA, LNanamePin); 
	oute_num += existsOuteGomaInLine(outePosKikiB, pinInfo, shogiBan, ou_x, ou_y, 0, -1, ou_y, F_MAE_GOMA, F_KY_HI_RYU, VertPin); 
	oute_num += existsOuteGomaInLine(outePosKikiB, pinInfo, shogiBan, ou_x, ou_y, 0, 1, BanY-1-ou_y, F_YOKO_GOMA, F_HI_RYU, VertPin); 
	oute_num += existsOuteGomaInLine(outePosKikiB, pinInfo, shogiBan, ou_x, ou_y, 1, -1, min(BanX-1-ou_x,ou_y), F_NAMAE_GOMA, F_KA_UMA, RNanamePin); 
	oute_num += existsOuteGomaInLine(outePosKikiB, pinInfo, shogiBan, ou_x, ou_y, -1, 1, min(ou_x,BanY-1-ou_y), F_NANAME_GOMA, F_KA_UMA, RNanamePin); 
	oute_num += existsOuteGomaInLine(outePosKikiB, pinInfo, shogiBan, ou_x, ou_y, -1, 0, ou_x, F_YOKO_GOMA, F_HI_RYU, HorizPin); 
	oute_num += existsOuteGomaInLine(outePosKikiB, pinInfo, shogiBan, ou_x, ou_y, 1, 0, BanX-1-ou_x, F_YOKO_GOMA, F_HI_RYU, HorizPin); 
	return oute_num;
}

inline Sashite *createSashiteRange(
	Sashite *te,
	Koma (*shogiBan)[BanX],
	int x, int y,
	int oute_num,
	BitBoard9x9 *outePosKikiB,
	int (*range)[2],
	int rs, int re,
	bool nari,
	int nari_limit = -1)
{
	if(oute_num>=2) return te;
	
	for (int r=rs; r<re; r++) {
		int to_x = x+range[r][0];
		int to_y = y+range[r][1];
		
		// 盤外チェック
		if (to_x<0 || to_x>=BanX) continue;
		if (to_y<0 || to_y>=BanY) continue;
		
		Koma to_k = shogiBan[to_y][to_x];
		
		if (to_k == EMP || (to_k & UWATE)) {// 味方がいなければ進める
			if (oute_num == 1 && !getBitBB(outePosKikiB, to_x, to_y)) continue;
			if (to_y > nari_limit) {
				te->type = SASHITE_IDOU;
				te->idou.to_y = to_y;
				te->idou.to_x = to_x;
				te->idou.from_y = y;
				te->idou.from_x = x;
				te->idou.nari = 0;
				te++;
			}
			// 成りの指手
			if (!nari && (to_y < 3 || y < 3)) {
				te->type = SASHITE_IDOU;
				te->idou.to_y = to_y;
				te->idou.to_x = to_x;
				te->idou.from_y = y;
				te->idou.from_x = x;
				te->idou.nari = 1;
				te++;
			}
		}
	}
	return te;
}

inline Sashite *createSashiteTobiGoma( Sashite *te,
	Koma (*shogiBan)[BanX], int x, int y,
	int pin,
	int oute_num, BitBoard9x9 *outePosKikiB,
	bool nari,
	int (*scanInf)[5], int scan_num,
	int nari_limit = -1)
{
	// {x移動,y移動,終了条件x,終了条件y,有効pin}

	for (int r=0; r<scan_num; r++) {
		int inc_x = scanInf[r][0];
		int inc_y = scanInf[r][1];
		int end_x = scanInf[r][2];
		int end_y = scanInf[r][3];
		bool canmove = (oute_num<2 && (pin==NoPin || pin==scanInf[r][4]));
		bool stoploop = false;
		
		int to_x = x+inc_x;
		int to_y = y+inc_y;
		for (;
				to_x != end_x && to_y != end_y && !stoploop;
				to_x+=inc_x, to_y+=inc_y) {
			Koma to_k = shogiBan[to_y][to_x];

			if (to_k != EMP) {
				if (to_k & UWATE) stoploop = true;
				else break; // exit loop
			}
			
			if (canmove) {
				if (oute_num == 1 && !getBitBB(outePosKikiB, to_x, to_y)) continue; 
				if (to_y > nari_limit) {
					te->type = SASHITE_IDOU;
					te->idou.to_y = to_y;
					te->idou.to_x = to_x;
					te->idou.from_y = y;
					te->idou.from_x = x;
					te->idou.nari = 0;
					te++;
				}
				// 成りの指手
				if ((to_y < 3 || y < 3) && !nari) {
					te->type = SASHITE_IDOU;
					te->idou.to_y = to_y;
					te->idou.to_x = to_x;
					te->idou.from_y = y;
					te->idou.from_x = x;
					te->idou.nari = 1;
					te++;
				}
			}
		}
	}
	return te;
}

Sashite *createSashiteFU(
	Sashite *te,
	Koma (*shogiBan)[BanX],
	int x, int y,
	int pin,
	int oute_num,
	BitBoard9x9 *outePosKikiB)
{
	static int FU_range[1][2] = {{0,-1}};
	assert(y>0);

	int rs,re;
	switch(pin) {
		case VertPin: rs=0;re=1; break;
		case HorizPin: 
		case LNanamePin:
		case RNanamePin: 
			 return te;
		default: rs=0;re=1; break;
	}

	return te=createSashiteRange(te,
			shogiBan, x, y, oute_num, outePosKikiB,
			FU_range, rs, re, false, 0);
}

Sashite *createSashiteKY(
	Sashite *te,
	Koma (*shogiBan)[BanX],
	int x, int y,
	int pin,
	int oute_num,
	BitBoard9x9 *outePosKikiB,
	bool nari)
{
	// {x移動,y移動,終了条件x,終了条件y,有効pin}
	int scanInf[1][5] = { {0,-1,-1,-1,VertPin}};
	return createSashiteTobiGoma(
		te,
		shogiBan, x, y, pin,
		oute_num, outePosKikiB, nari,
		scanInf, 1, 0);
}

Sashite *createSashiteKE(
	Sashite *te,
	Koma (*shogiBan)[BanX],
	int x, int y,
	int pin,
	int oute_num,
	BitBoard9x9 *outePosKikiB)
{
	static int KE_range[2][2] = {{-1,-2}, {1,-2}};
	assert(y>1);

	if (pin!=NoPin) return te;

	return te=createSashiteRange(te,
			shogiBan, x, y, oute_num, outePosKikiB,
			KE_range, 0, 2, false, 1);
}

Sashite *createSashiteGI(
	Sashite *te,
	Koma (*shogiBan)[BanX],
	int x, int y,
	int pin,
	int oute_num,
	BitBoard9x9 *outePosKikiB)
{
	static int GI_range[5][2] = {{0,-1}, {-1,-1}, {1,1}, {1,-1}, {-1,1}};
	
	int rs,re;
	switch(pin) {
		case VertPin: rs=0;re=1; break;
		case HorizPin: return te;
		case LNanamePin :rs=1;re=3; break;
		case RNanamePin: rs=3;re=5; break;
		default: rs=0;re=5; break;
	}

	return te=createSashiteRange(te,
			shogiBan, x, y, oute_num, outePosKikiB,
			GI_range, rs, re, false);
}

Sashite *createSashiteKI(
	Sashite *te,
	Koma (*shogiBan)[BanX],
	int x, int y,
	int pin,
	int oute_num,
	BitBoard9x9 *outePosKikiB)
{
	static int KI_range[6][2] = {{0,-1},{0,1},{-1,0},{1,0},{-1,-1},{1,-1}};

	int rs,re;
	switch(pin) {
		case VertPin: rs=0;re=2; break;
		case HorizPin: rs=2;re=4; break;
		case LNanamePin :rs=4;re=5; break;
		case RNanamePin: rs=5;re=6; break;
		default: rs=0;re=6; break;
	}
	return te=createSashiteRange(te,
			shogiBan, x, y, oute_num, outePosKikiB,
			KI_range, rs, re, true);
}

Sashite *createSashiteUMA(
	Sashite *te,
	Koma (*shogiBan)[BanX],
	int x, int y,
	int pin,
	int oute_num,
	BitBoard9x9 *outePosKikiB)
{
	static int UMA_range[4][2] = {{0,-1}, {0,1}, {-1,0}, {1,0}};
	
	int rs,re;
	switch(pin) {
		case VertPin: rs=0;re=2; break;
		case HorizPin: rs=2;re=4; break;
		case LNanamePin:
		case RNanamePin:
		  return te;
		default: rs=0;re=4; break;
	}
	return te=createSashiteRange(te,
			shogiBan, x, y, oute_num, outePosKikiB,
			UMA_range, rs, re, true);
}

Sashite *createSashiteKA(
	Sashite *te,
	Koma (*shogiBan)[BanX],
	int x, int y,
	int pin,
	int oute_num,
	BitBoard9x9 *outePosKikiB,
	bool nari)
{
	// {x移動,y移動,終了条件x,終了条件y,有効pin}
	int scanInf[4][5] = {
		{-1,-1,-1,-1,LNanamePin},{1,1,BanX,BanY,LNanamePin},
		{1,-1,BanX,-1,RNanamePin},{-1,1,-1,BanY,RNanamePin}
	};
	return createSashiteTobiGoma(
		te,
		shogiBan, x, y, pin,
		oute_num, outePosKikiB, nari,
		scanInf, 4
		);
}

Sashite *createSashiteRYU(
	Sashite *te,
	Koma (*shogiBan)[BanX],
	int x, int y,
	int pin,
	int oute_num,
	BitBoard9x9 *outePosKikiB)
{
	static int RYU_range[4][2] = {{-1,-1}, {1,1}, {1,-1}, {-1,1}};

	int rs,re;
	switch(pin) {
		case VertPin: 
		case HorizPin: 
			return te;
		case LNanamePin: rs=0;re=2; break;
		case RNanamePin: rs=2;re=4; break;
		default: rs=0;re=4; break;
	}
	return te=createSashiteRange(te,
			shogiBan, x, y, oute_num, outePosKikiB,
			RYU_range, rs, re, true);
}

Sashite *createSashiteHI(
	Sashite *te,
	Koma (*shogiBan)[BanX],
	int x, int y,
	int pin,
	int oute_num,
	BitBoard9x9 *outePosKikiB,
	bool nari)
{
	// {x移動,y移動,終了条件x,終了条件y,有効pin}
	int scanInf[4][5] = {
		{0,-1,-1,-1,VertPin},{0,1,-1,BanY,VertPin},
		{-1,0,-1,-1,HorizPin},{1,0,BanX,-1,HorizPin}
	};
	return createSashiteTobiGoma(
		te,
		shogiBan, x, y, pin,
		oute_num, outePosKikiB, nari,
		scanInf, 4
	);
}

Sashite *createSashiteOU(Sashite *te,
		Koma (*shogiBan)[BanX], int x, int y,
		BitBoard9x9 *noUwateKikiB)
{
	static int OU_range[8][2] = {{0,-1},{0,1},{-1,0},{1,0},{-1,-1},{1,1},{1,-1},{-1,1}};

	return te=createSashiteRange(te,
			shogiBan, x, y, 1, noUwateKikiB,
			OU_range, 0, 8, true);
}

Sashite *createSashiteUchi(Sashite *te, Koma k,
		Koma (*shogiBan)[BanX], 
		int oute_num, BitBoard9x9 *outePosKikiB,
		int uchiLimit)
{
	if (oute_num >= 2) return te;
	for (int y=uchiLimit; y<BanY; y++)
		for (int x=0; x<BanX; x++) {
			if (shogiBan[y][x]==EMP) {
				if (oute_num==1 && !getBitBB(outePosKikiB,x,y))
					continue;
				te->type = SASHITE_UCHI;
				te->uchi.uwate = 0;
				te->uchi.to_y = y;
				te->uchi.to_x = x;
				te->uchi.koma = k;
				te++;
			}
		}
	return te;
}

inline bool existsKikiGomaInLine(
		Koma (*shogiBan)[BanX], int x, int y,
		int incx, int incy, int maxloop,
		int direct, int indirect, int allow_pin,
		int (*uPinInfo)[BanX])
{
	if (maxloop==0) return false;
	for (int i=1; i<maxloop; i++) {
		x+=incx; y+=incy;
		if (shogiBan[y][x] != EMP) {
			return ((shogiBan[y][x]&UWATE)
					&& isKoma(shogiBan[y][x],i==0?direct:indirect)
					&& !(uPinInfo[y][x]&(~allow_pin)));
		}
	}
	return false;
}

inline void setKikiLine(
		BitBoard9x9 *kikiB, int (*uPinInfo)[BanX],
		Koma (*shogiBan)[BanX], int x, int y,
		int incx, int incy, int maxloop,
		int pin)
{
	for (int i=0; i<maxloop; i++) {
		x += incx; y+= incy;
		setBitBB(kikiB, x, y);
		Koma k = shogiBan[y][x];
		if (k != EMP) {
			if (k & UWATE) { 
				int px = x, py = y;
				for (int j=i+1; j<maxloop; j++) {
					x += incx; y+= incy;
					k = shogiBan[y][x];
					if (k != EMP) {
						if (k == UOU) uPinInfo[py][px] = pin;
						return;
					}
				}
			}
			return;
		}
	}
}

inline void createEscapeArea(
	BitBoard9x9 *escapeAreaB, int (*uPinInfo)[BanX],
	Koma (*shogiBan)[BanX])
{
	for (int y=0; y<BanY; y++)
		for (int x=0; x<BanX; x++) {
			switch(shogiBan[y][x]) {
				case EMP: break;
				case FU: setBitBB(escapeAreaB, x, y-1); break;
				case KY: setKikiLine(escapeAreaB, uPinInfo, shogiBan, x, y, 0, -1, y, VertPin); break;
				case KE:
					if (x>=1) setBitBB(escapeAreaB, x-1, y-2);
					if (x<=BanX-2) setBitBB(escapeAreaB, x+1, y-2);
					break;
				case GI: setBitsBB(escapeAreaB, x, y, GI_pttn); break;
				case KI: case NFU: case NKY: case NKE: case NGI:
						 setBitsBB(escapeAreaB, x, y, KI_pttn); break;
				case UMA: setBitsBB(escapeAreaB, x, y, UMA_pttn);
				case KA: setKikiLine(escapeAreaB, uPinInfo, shogiBan, x, y, -1, -1, min(x,y), LNanamePin);
						 setKikiLine(escapeAreaB, uPinInfo, shogiBan, x, y, 1, 1, min(BanX-1-x, BanY-1-y), LNanamePin);
						 setKikiLine(escapeAreaB, uPinInfo, shogiBan, x, y, 1, -1, min(BanX-1-x, y), RNanamePin);
						 setKikiLine(escapeAreaB, uPinInfo, shogiBan, x, y, -1, 1, min(x, BanY-1-y), RNanamePin);
						 break;
				case RYU: setBitsBB(escapeAreaB, x, y, RYU_pttn);
				case HI: setKikiLine(escapeAreaB, uPinInfo, shogiBan, x, y, 0, -1, y, VertPin);
						 setKikiLine(escapeAreaB, uPinInfo, shogiBan, x, y, 0, 1, BanY-1-y, VertPin);
						 setKikiLine(escapeAreaB, uPinInfo, shogiBan, x, y, -1, 0, x, HorizPin);
						 setKikiLine(escapeAreaB, uPinInfo, shogiBan, x, y, 1, 0, BanX-1-x, HorizPin);
						 break;

				case OU: setBitsBB(escapeAreaB, x, y, OU_pttn); break;
				default: setBitBB(escapeAreaB, x, y); break;
			}
		}
}

bool checkUchiFUZume(Koma (*shogiBan)[BanX], int x, int y)
{
	int uPinInfo[BanY][BanX] = {0};

	// 王手の判定
	if (shogiBan[y-1][x] != UOU) return false;

	// 王の逃げ場チェック
	BitBoard9x9 escapeAreaB={0};
	BitBoard9x9 ou_range={0};
	assert(shogiBan[y][x]==EMP);
	shogiBan[y][x]=FU;
	createEscapeArea(&escapeAreaB, uPinInfo, shogiBan);
	shogiBan[y][x]=EMP;
	setBitsBB(&ou_range, x, y-1, OU_pttn);
	setAndBitsBB(&escapeAreaB, &ou_range);
	if (escapeAreaB.topmid != ou_range.topmid || escapeAreaB.bottom != ou_range.bottom)
		return false; // 逃げ場あり

	// 打った歩が取られるか？
	if (existsKikiGomaInLine(shogiBan,x,y,-1,-1,min(x,y),F_NAMAE_GOMA,F_KA_UMA,LNanamePin,uPinInfo)) return false;
	if (existsKikiGomaInLine(shogiBan,x,y,1,-1,min(BanX-1-x,y),F_NAMAE_GOMA,F_KA_UMA,RNanamePin,uPinInfo)) return false;
	if (existsKikiGomaInLine(shogiBan,x,y,-1,0,x,F_YOKO_GOMA,F_HI_RYU,HorizPin,uPinInfo)) return false;
	if (existsKikiGomaInLine(shogiBan,x,y,1,0,BanX-1-x,F_YOKO_GOMA,F_HI_RYU,HorizPin,uPinInfo)) return false;
	if (existsKikiGomaInLine(shogiBan,x,y,-1,1,min(x,BanY-1-y),F_NANAME_GOMA,F_KA_UMA,RNanamePin,uPinInfo)) return false;
	if (existsKikiGomaInLine(shogiBan,x,y,1,1,min(BanX-1-x,BanY-1-y),F_NANAME_GOMA,F_KA_UMA,LNanamePin,uPinInfo)) return false;
	if (existsKikiGomaInLine(shogiBan,x,y,0,1,BanY-1-y,F_YOKO_GOMA,F_HI_RYU,VertPin,uPinInfo)) return false;
	if (y >= 2) {
		if (x >= 1 && shogiBan[y-2][x-1]==UKE) {
			if (uPinInfo[y-2][x-1]==NoPin)
				return false;
		} else if (x <= 7 && shogiBan[y-2][x+1]==UKE) {
			if (uPinInfo[y-2][x+1]==NoPin)
				return false;
		}
	}

	return true;
}

Sashite *createSashiteUchiFU(Sashite *te, 
		Koma (*shogiBan)[BanX], unsigned int usedLine,
		int oute_num, BitBoard9x9 *outePosKikiB)
{
	if (oute_num >= 2) return te;
	for (int x=0; x<BanX; x++) {
		if(1u &(usedLine >> x)) continue;
		for (int y=1; y<BanY; y++) {
			if (shogiBan[y][x]==EMP) {
				if (oute_num==1 && !getBitBB(outePosKikiB,x,y))
					continue;
				if (!checkUchiFUZume(shogiBan, x, y)) {
					te->type = SASHITE_UCHI;
					te->uchi.uwate = 0;
					te->uchi.koma = FU;
					te->uchi.to_y = y;
					te->uchi.to_x = x;
					te++;
				}
			}
		}
	}
	return te;
}
