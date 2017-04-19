//
//  kyokumencode.cpp
//  ShogiDBTool
//
//  Created by tosyama on 2014/12/28.
//  Copyright (c) 2016 tosyama. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "shogiban.h"
#include "kyokumencode.h"

void createAreaKyokumenCode(char code[], const ShogiKyokumen *shogi)
{
    const Koma (*shogiBan)[BanX] = shogi->shogiBan;
    const int (*komaDai)[DaiN] = shogi->komaDai;
    
    const char banJouCodeTbl[][10] = {
        "!#$&()*+-","./0123456","789:;<=>?",
        "@ABCDEFGH","IJKLMNOPQ","RSTUVWXYZ",
        "_abcdefgh","ijklmnopq","rstuvwxyz"};
    const int komaDaiCode = '~';
    const int komaNashiCode = ' ';
	
    char *uKomaPos[DaiN] = {NULL};
    int uKomaNum[DaiN] = {0};

	char *komaPos[DaiN] = {NULL};
	char codeBuf[39];

	for (int i=0; i<KyokumenCodeLen-1; i++) {
		code[i] = '^';
	}
	code[KyokumenCodeLen-1] = 0;

	uKomaPos[0] = &code[0];
	uKomaPos[KI] = &code[3]; uKomaPos[GI] = &code[9];
	uKomaPos[HI] = &code[14]; uKomaPos[KA] = &code[17];
	uKomaPos[KE] = &code[21]; uKomaPos[KY] = &code[27];
	uKomaPos[FU] = &code[35];

	komaPos[0] = &codeBuf[0];
	komaPos[KI] = &codeBuf[1]; komaPos[GI] = &codeBuf[5];
	komaPos[HI] = &codeBuf[9]; komaPos[KA] = &codeBuf[11];
	komaPos[KE] = &codeBuf[13]; komaPos[KY] = &codeBuf[17];
	komaPos[FU] = &codeBuf[21];

	for (int yy=0; yy<BanY; yy+=(BanY/3))
	for (int xx=0; xx<BanX; xx+=(BanX/3)) {
		for(int y=yy; y<yy+(BanY/3); y++)
		for(int x=xx; x<xx+(BanX/3); x++) {
			Koma k = shogiBan[y][x];
			if (k!=EMP) {
				int k_ind = k&KOMATYPE1;
				char banJouCode = banJouCodeTbl[y][x];
				if (k & UWATE) {
					*uKomaPos[k_ind]=banJouCode;
					uKomaPos[k_ind]++;	
					uKomaNum[k_ind]++;
				} else {
					*komaPos[k_ind]=banJouCode;
					komaPos[k_ind]++;
				}
			}
		}
	}

	//OU
	memcpy(uKomaPos[0],&codeBuf[0],komaPos[0]-&codeBuf[0]);

	memcpy(uKomaPos[KI],&codeBuf[1],komaPos[KI]-&codeBuf[1]);
	memcpy(uKomaPos[GI],&codeBuf[5],komaPos[GI]-&codeBuf[5]);
	memcpy(uKomaPos[HI],&codeBuf[9],komaPos[HI]-&codeBuf[9]);

}
 
void createKyokumenCode(char code[], const  ShogiKyokumen *shogi, int rev)
{
    const Koma (*shogiBan)[BanX] = shogi->shogiBan;
    const int (*komaDai)[DaiN] = shogi->komaDai;
    
    static const char banJouCodeTbl[9][10] = {
        "!#$&()*+-","./0123456","789:;<=>?",
        "@ABCDEFGH","IJKLMNOPQ","RSTUVWXYZ",
        "_abcdefgh","ijklmnopq","rstuvwxyz"
    };
    static const char revBanJouCode[9][10] = {
        "zyxwvutsr","qponmlkji","hgfedcba_",
        "ZYXWVUTSR","QPONMLKJI","HGFEDCBA@",
        "?>=<;:987","6543210/.","-+*)(&$#!",
    };
    
    const char (*banJouCode)[10] = banJouCodeTbl;
    if (rev) banJouCode = revBanJouCode;
    
    char komaDaiCode = '~';
    char komaNashiCode = ' ';
    
    // 駒情報
    int allKomaNum[DaiN] = {/*OU*/2,/*FU*/18,/*KY*/4,/*KE*/4,/*GI*/4,/*KI*/4,/*KA*/2,/*HI*/2};
    int uKomaNum[DaiN] = {0};
    int komaNum[DaiN] = {0};
    char uPosBuf[40], posBuf[40];
    char *uKomaPos[DaiN] = {NULL};
    char *komaPos[DaiN] = {NULL};
    int uNariBuf[40], nariBuf[40];
    int *uKomaNari[DaiN];
    int *komaNari[DaiN];

    for (int i=0; i<40; i++) {
        uPosBuf[i] = posBuf[i] = 0;
        uNariBuf[i] = nariBuf[i] = 0;
    }
    for (int i=0, n=0;i<DaiN;i++) {
        uKomaPos[i] = &uPosBuf[n];
        komaPos[i] = &posBuf[n];
        uKomaNari[i] = &uNariBuf[n];
        komaNari[i] = &nariBuf[n];
        n += allKomaNum[i];
    }

    // 盤の駒の位置の検索
    int begin_y=0, end_y=BanY, step_y=1;
    int begin_x=0, end_x=BanX, step_x=1;
    if (rev) {
        begin_y=BanY-1; end_y=-1; step_y=-1;
        begin_x=BanX-1; end_x=-1; step_x=-1;
    };
    
    for (int y=begin_y; y!=end_y; y+=step_y)
        for (int x=begin_x; x!=end_x; x+=step_x) {
            if (shogiBan[y][x] != EMP) {
                {
                    Koma k = shogiBan[y][x];
                    Koma k1 = (Koma)(k & KOMATYPE1);
					
                    if (((k & UWATE) && !rev) || (!(k & UWATE) && rev)) {
                        uKomaPos[k1][uKomaNum[k1]] = banJouCode[y][x];
                        if (k & NARI) uKomaNari[k1][uKomaNum[k1]] = 1;
                        uKomaNum[k1]++;
                    } else {
                        komaPos[k1][komaNum[k1]] = banJouCode[y][x];
                        if (k & NARI) komaNari[k1][komaNum[k1]] = 1;
                        komaNum[k1]++;
                    }
                }
            }
        }
    
    // 駒台の検索
    {
        int u = 1, s=0;
        if (rev) {u=0; s=1;}
        for (int k=0; k<DaiN; k++) {
            for (int i=0; i<komaDai[u][k]; i++) uKomaPos[k][uKomaNum[k]++] = komaDaiCode;
            for (int i=0; i<komaDai[s][k]; i++) komaPos[k][komaNum[k]++] = komaDaiCode;
        }
    }
    // 駒が盤上/駒台にもない
    for (int k=1; k<DaiN; k++) {
        for (int i=(uKomaNum[k]+komaNum[k]); i<allKomaNum[k]; i++) komaPos[k][komaNum[k]++] = komaNashiCode;
    }
    
    //-- コード生成 ---//
    Koma  codeKomas[7] = {KI, GI, HI, KA, KE, KY, FU};
    int codePos = 0;
    // 王
    code[codePos++] = uKomaNum[0]?uKomaPos[0][0]:komaNashiCode;
    code[codePos++] = komaNum[0]?komaPos[0][0]:komaNashiCode;
    
    for (int c=0; c<7; c++) {
        Koma k = codeKomas[c];
        int numPos, nariPos;
        switch (k) {
            case KI:
                numPos = codePos++;
                nariPos = -1;
                break;
            case GI:
            case KE:
            case KY:
                numPos = codePos++;
                nariPos = codePos++;
                break;
            case HI:
            case KA:
                numPos = nariPos = codePos++;
                break;
            case FU:
                numPos = codePos++;
                nariPos = codePos;
                codePos += 3;
                break;
            default:
                assert(0);
                break;
        }
        
        int unum = uKomaNum[k];
        int nariCode = 0;
        for (int i=0, n=0; i<allKomaNum[k]; i++) {
            if (i<unum) {
                if(uKomaNari[k][i]) nariCode |= 1<<i;
                code[codePos++] = uKomaPos[k][i];
            } else {
                if(komaNari[k][n]) nariCode |= 1<<i;
                code[codePos++] = komaPos[k][n];
                n++;
            }
        }
        if (numPos == nariPos) {
            nariCode = (nariCode << 2) | unum;
            code[numPos] = (nariCode < 10) ? (nariCode + '0') : (nariCode-10+'A');
        } else if (k==FU) {
            code[numPos] = (unum < 10) ? (unum + '0') : (unum-10+'A');
            int b = 0x3F & nariCode;
            code[nariPos] = (b < 10) ? (b + '0') : ((b < 36) ? (b-10+'A') : (b-36+'a'));
            b = (0xFC0 & nariCode) >> 6;
            code[nariPos+1] = (b < 10) ? (b + '0') : ((b < 36) ? (b-10+'A') : (b-36+'a'));
            b = (0x3F000 & nariCode) >> 12;
            code[nariPos+2] =  (b < 10) ? (b + '0') : ((b < 36) ? (b-10+'A') : (b-36+'a'));
        }else {
            code[numPos] = (unum < 10) ? (unum + '0') : (unum-10+'A');
            if (nariPos > 0) code[nariPos] = (nariCode < 10) ? (nariCode + '0') : (nariCode-10+'A');
        }
        
    }
    code[codePos] = '\0';
    assert(codePos == (KyokumenCodeLen-1));
}

void loadKyokumenFromCode(ShogiKyokumen *shogi, const char code[])
{
    Koma (*shogiBan)[BanX] = shogi->shogiBan;
    int (*komaDai)[DaiN] = shogi->komaDai;
    int banjouPos[] = { 0,-1,1,2,-1,3,-1,4,5,6,7,-1,8,  //!#$&()*+-
                        9,10,11,12,13,14,15,16,17,      //./0123456
                        18,19,20,21,22,23,24,25,26,     //789:;<=>?
                        27,28,29,30,31,32,33,34,35,     //@ABCDEFGH
                        36,37,38,39,40,41,42,43,44,     //IJKLMNOPQ
                        45,46,47,48,49,50,51,52,53,     //RSTUVWXYZ
                        -1,-1,-1,-1,54,-1,55,56,57,58,59,60,61,62, //_abcdefgh
                        63,64,65,66,67,68,69,70,71,     //ijklmnopq
                        72,73,74,75,76,77,78,79,80      //rstuvwxyz
    };
    int allKomaNum[DaiN+1] = {/*EMP*/0,/*FU*/18,/*KY*/4,/*KE*/4,/*GI*/4,/*KI*/4,/*KA*/2,/*HI*/2,/*OU*/2};

    for (int y=0; y<BanY; y++)
        for (int x=0; x<BanX; x++) shogiBan[y][x] = EMP;
    for (int n=0;n<2;n++)
        for (int k=0;k<DaiN;k++) komaDai[n][k] = 0;
    
    // 王
    if (code[0] != ' ' && code[0] != '~') {
        assert(code[0]>='!' && code[0]<'~');
        int p = banjouPos[code[0]-'!'];
        assert(p != -1);
        
        shogiBan[(shogi->uou_y = p/BanX)][(shogi->uou_x = p%BanX)] = UOU;
    } else {
		if (code[0]=='~') komaDai[1][0]++;
		shogi->uou_x = NonPos;
		shogi->uou_y = NonPos;
	}
    if (code[1] != ' ' && code[1]<'~') {
        int p = banjouPos[code[1]-'!'];
        shogiBan[(shogi->ou_y = p/BanX)][(shogi->ou_x = p%BanX)] = OU;
    } else {
		if (code[1]=='~') komaDai[0][0]++;
		shogi->ou_x = NonPos;
		shogi->ou_y = NonPos;
	}
    
    //その他
    Koma  codeKomas[7] = {KI, GI, HI, KA, KE, KY, FU};
    int codePos = 2;
    
    for (int c=0; c<7; c++) {
        Koma k = codeKomas[c];
        int nari = 0;
        int unum = 0;
        switch (k) {
            case KI:
                unum = code[codePos++]-'0';
                break;
                
            case GI:
            case KE:
            case KY:
                unum = code[codePos++]-'0';
                nari = (code[codePos] <= '9') ? code[codePos]-'0' : code[codePos]-'A'+10;
                codePos++;
                break;
            case HI:
            case KA:
            {
                int temp;
                temp = (code[codePos] <= '9') ? code[codePos]-'0' : code[codePos]-'A'+10;
                unum = temp & 0x3;
                nari = temp >> 2;
                codePos++;
            }
                break;
            case FU:
            {
                int temp;
                unum = (code[codePos] <= '9') ? code[codePos]-'0' : code[codePos]-'A'+10;
                codePos++;
                nari = (code[codePos] <= '9') ? code[codePos]-'0'
                    : ((code[codePos] <= 'Z') ? code[codePos]-'A'+10 : code[codePos]-'a'+36);
                
                codePos++;
                temp =(code[codePos] <= '9') ? code[codePos]-'0'
                : ((code[codePos] <= 'Z') ? code[codePos]-'A'+10 : code[codePos]-'a'+36);
                nari |= (temp << 6);
                
                codePos++;
                temp =(code[codePos] <= '9') ? code[codePos]-'0'
                : ((code[codePos] <= 'Z') ? code[codePos]-'A'+10 : code[codePos]-'a'+36);
                nari |= (temp << 12);
                codePos++;
            }
                
            default:
                break;
        }
        for (int i=0; i<allKomaNum[k];i++) {
            assert((code[codePos]>='!' && code[codePos]<='~')
					|| code[codePos]==' ');
			if (code[codePos] == ' ') {
				// Skip
            } else if (code[codePos] == '~') {
                if (i<unum) komaDai[1][k]++;
                else komaDai[0][k]++;
            } else {
                int p = banjouPos[code[codePos]-'!'];
                Koma k2 = k;
                if (nari & (1 << i)) k2 = (Koma)(k2 + NARI);
                
                assert(p != -1);
                shogiBan[p/BanX][p%BanX] = (i<unum) ? (Koma)(k2+UWATE) : k2;
            }
            codePos++;
        }
    }
}

