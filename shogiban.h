//
//  shogiban.h
//  ShogiDBTool
//
//  Created by tosyama on 2016/2/1.
//  Copyright (c) 2016 tosyama. All rights reserved.
//

// Koma
enum Koma {
    EMP = 0, //空 1
    FU = 1, //歩 2
    KY = 2, //香車 4
    KE = 3, //桂馬 8
    GI = 4, //銀 16
    KI = 5, //金 32
    KA = 6, //角 64
    HI = 7, //飛車 128
    OU = 8, //玉 256
    KOMATYPE1 = 0x7, //成りを含まない駒種の判別
    NARI = 8, //成り
    NFU = NARI + FU, //と9 512
    NKY = NARI + KY, //成香10 1024
    NKE = NARI + KE, //成桂11 2048
    NGI = NARI + GI, //成銀12 4096
    UMA = NARI + KA, //馬14 16384
    RYU = NARI + HI, //龍15 32768
    KOMATYPE2 = 0xF, //成りを含む駒種の判別
    UWATE = 16,
    UFU = UWATE+FU, //17 131072
    UKY = UWATE+KY, //18 262144
    UKE = UWATE+KE, //19 524288
    UGI = UWATE+GI, //20 1048576
    UKI = UWATE+KI, //21 2097152
    UKA = UWATE+KA, //22 4194304
    UHI = UWATE+HI, //23 8388608
    UOU = UWATE+OU, //24 16777216
    UNFU = UWATE + NFU, //25
    UNKY = UWATE + NKY, //26
    UNKE = UWATE + NKE, //27
    UNGI = UWATE + NGI, //28
    UUMA = UWATE + UMA, //30
    URYU = UWATE + RYU, //31
};

#define F_KA_UMA        (64+16384)
#define F_HI_RYU        (128+32768)
#define F_KY_HI_RYU     (4+128+32768)
#define F_NAMAE_GOMA    (16+32+64+256+512+1024+2048+4096+16384+32768)
#define F_MAE_GOMA      (2+4+16+32+128+256+512+1024+2048+4096+16384+32768)
#define F_YOKO_GOMA     (32+128+256+512+1024+2048+4096+16384+32768)
#define F_NANAME_GOMA   (16+64+256+16384+32768)

// Shogi Ban (Kyokumen)
#define BanX    9
#define BanY    9
#define DaiN    8

typedef struct {
    Koma shogiBan[BanY][BanX];
    int komaDai[2][DaiN];
    int  ou_x, ou_y;
    int uou_x, uou_y;
} ShogiKykumen;

#define INNER_X(x)   (BanX-(x))
#define INNER_Y(y)   ((y)-1)

void resetShogiBan(ShogiKykumen *shogi);
void printKyokumen(FILE *f, ShogiKykumen *shogi);

Koma sashite1(ShogiKykumen *shogi, int from_x, int from_y, int to_x, int to_y, int nari);
void sashite2(ShogiKykumen *shogi, int uwate, Koma koma, int to_x, int to_y);
