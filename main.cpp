//
//  main.cpp
//  ShogiDBTool
//
//  Created by tosyama on 2014/12/28.
//  Copyright (c) 2014 tosyama. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sqlite3.h>
#include <sys/time.h>
#include <iconv.h>

#define min(a,b)    ((a)<(b)?(a):(b))

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

enum SashiteType {
    SASHITE_EMP,
    SASHITE_IDOU,
    SASHITE_UCHI,
    SASHITE_RESULT,
};

enum Kisen {
    KISEN_MEIJIN,
    KISEN_RYUOU,
    KISEN_KISEI,
    KISEN_OUI,
    KISEN_OUZA,
    KISEN_KIOU,
    KISEN_OUSHOU,
    KISEN_JUNI,
    KISEN_NHK,
    KISEN_GINGA,
    KISEN_JUUDAN,
    KISEN_KUDAN,
    KISEN_SONOTA = 999
};

typedef union {
    int type;
    struct {    // SASHITE_IDOU
        int type;
        int from_x, from_y, to_x, to_y, nari;
        Koma torigoma;
    } idou;
    struct {    // SASHITE_UCHI
        int type;
        int uwate;
        int to_x, to_y;
        Koma koma;
    } uchi;
    struct {    // SASHITE_RESULT
        int type;
        int winner; // 0:先手, 1:後手(上手), 2: 千日手
    } result;
} Sashite;

#define BanX    9
#define BanY    9
#define DaiN    8

#define KykumenCodeLen  54

typedef struct {
    Koma shogiBan[BanY][BanX];
    int komaDai[2][DaiN];
    int  ou_x, ou_y;
    int uou_x, uou_y;
} ShogiKykumen;

#define KishiNameLen  25
typedef struct {
    char date[9];
    Kisen kisen;
    char sente[KishiNameLen];
    char gote[KishiNameLen];
    int tesuu;
    int result; // 0:先手, 1:後手(上手), 2: 千日手, 9:不明
    Sashite sashite[500];
} Kifu;

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

static void resetShogiBan(ShogiKykumen *shogi);
static void printKyokumen(FILE *f, ShogiKykumen *shogi);

static int sasu(ShogiKykumen *shogi, Sashite *s);
static Koma sashite1(ShogiKykumen *shogi, int from_x, int from_y, int to_x, int to_y, int nari);
static void sashite2(ShogiKykumen *shogi, int uwate, Koma koma, int to_x, int to_y);
static void temodoshi(ShogiKykumen *shogi, const Sashite *s);

static void createSashite(ShogiKykumen *shogi, int uwate, Sashite *s, int *n);

static int readKIF(const char *filename, Kifu* kifu);

static void createKyokumenCode(char code[], const ShogiKykumen *shogi, int rev=0);
static void loadKyokumenFromCode(ShogiKykumen *shogi, const char code[]);

static void createShogiDB(const char* filename);
static void insertShogiDB(const char* filename, Kifu* kifu);

static char komaStr[][5] = { "・","歩","香","桂","銀","金","角","飛","玉", "と", "杏", "圭", "全","　","馬","龍"};
static char kanSuji[][5] = { "〇","一","二","三","四","五","六","七","八","九"};
//static char sjisKanSuji[][5] = { "","\x88\xEA","\x93\xF1","\x8E\x4F","\x8E\x6C","\x8C\xDC","\x98\x5A","\x8E\xB5","\x94\xAA","\x8B\xE3"};

static int interactiveCUI(ShogiKykumen *shogi, Sashite *s);

#define INNER_X(x)   (BanX-(x))
#define INNER_Y(y)   ((y)-1)

FILE *logf = NULL;
int main(int argc, const char * argv[]) {
    Kifu kifu;
    ShogiKykumen shogi;
    char code[KykumenCodeLen];
    logf = fopen("shogidbtool.log", "w");

    resetShogiBan(&shogi);
    sashite1(&shogi, 1, 1, 3, 7, 1);
    
    Sashite s[200];
    int n;
    createSashite(&shogi, 0, s, &n);
    
    Sashite si[200];
    si[0].type = SASHITE_RESULT;
    int i=0;
    int cmd;
    while ((cmd = interactiveCUI(&shogi, &si[i]))) {
        i+=cmd;
        if (i<0) {i=0; si[0].type = SASHITE_RESULT;}
        if (cmd>0) {si[i]=si[i-1];}
        createSashite(&shogi, 0, s, &n);
        fprintf(logf, "手の数: %d\n", n);

        for (int j=0; j<n; j++) {
            sasu(&shogi, &s[j]);
            printKyokumen(logf, &shogi);
            temodoshi(&shogi, &s[j]);
        }

    }
/*    readKIF("test.kif", &kifu);
    Sashite* sashite = kifu.sashite;
    
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
    fclose(logf);
    return 0;
}

static int interactiveCUI(ShogiKykumen *shogi, Sashite *s)
{
    Koma (*shogiBan)[BanX] = shogi->shogiBan;
    char buf[80];
    int fx, fy, tx, ty;
    printKyokumen(stdout, shogi);
    
    while (1) {
        printf("move:1-9 1-9 1-9 1-9 + uchi:[11-17 or 21-27] 1-0 1-9\ntemodoshi:-1, print: p end:0,\n>");
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
            } else if (buf[0] == '-') {
                temodoshi(shogi, s);
                return -1;
            } else if (buf[0] == '0') { // end
                return 0;
            } else if (buf[0] == 'p') { // print
                printKyokumen(stdout, shogi);
            }
        } else {
            return 0;
        }
    }
    
    return 0;
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

void resetShogiBan(ShogiKykumen *shogi)
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

void printKyokumen(FILE *f, ShogiKykumen *shogi)
{
    Koma (*shogiBan)[BanX] = shogi->shogiBan;
    int (*komaDai)[DaiN] = shogi->komaDai;
    
    fprintf(f, "後手の持駒：");
    int nashi = 1;
    for (int k=FU; k<DaiN; k++) {
        if (komaDai[1][k] == 1) {
            fprintf(f, "%s ", komaStr[k]);
            nashi = 0;
        } else if (komaDai[1][k]>1) {
            fprintf(f, "%s%d ", komaStr[k], komaDai[1][k]);
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
    for (int k=FU; k<DaiN; k++) {
        if (komaDai[0][k] == 1) {
            fprintf(f, "%s ", komaStr[k]);
            nashi = 0;
        } else if (komaDai[0][k]>1) {
            fprintf(f, "%s%d ", komaStr[k], komaDai[0][k]);
            nashi = 0;
        }
    }
    if (nashi) fprintf(f, "なし");
    
    fprintf(f, "\n");
}

static int sasu(ShogiKykumen *shogi, Sashite *s)
{
    if (s->type == SASHITE_IDOU) {
        s->idou.torigoma = sashite1(shogi, s->idou.from_x, s->idou.from_y, s->idou.to_x, s->idou.to_y, s->idou.nari);
        
    } else if(s->type == SASHITE_UCHI) {
        sashite2(shogi, s->uchi.uwate, s->uchi.koma, s->uchi.to_x, s->uchi.to_y);
    
    } else if(s->type == SASHITE_RESULT) {
        return s->result.winner;
    
    }else {
        assert(0);
    }
    
    return -1;
}

static Koma sashite1(ShogiKykumen *shogi, int from_x, int from_y, int to_x, int to_y, int nari)
{
    Koma (*shogiBan)[BanX] = shogi->shogiBan;
    int (*komaDai)[DaiN] = shogi->komaDai;
    
    Koma k1, k2;
    k1 = shogiBan[from_y][from_x];
    k2 = shogiBan[to_y][to_x];
    
    assert(k1 != EMP);
    
    if (k2 != EMP) {
        if (k1 & UWATE) {
            komaDai[1][k2 & KOMATYPE1]++;
        } else {
            komaDai[0][k2 & KOMATYPE1]++;
        }
    }
    
    if (nari) {
        assert(!(k1 & NARI));   // 成りが指定されている場合は成り駒でないこと
        shogiBan[to_y][to_x] = (Koma)(k1 + NARI);
    }
    else shogiBan[to_y][to_x] = k1;
    shogiBan[from_y][from_x] = EMP;
    
    if (k1 == OU) { // 王の位置は常に記録
        shogi->ou_x = to_x; shogi->ou_y = to_y;
    } else if (k1 == UOU) {
        shogi->uou_x = to_x; shogi->uou_y = to_y;
    }
    return k2;  //手を戻すときに使用するため
}

static void sashite2(ShogiKykumen *shogi, int uwate, Koma koma, int to_x, int to_y)
{
    Koma (*shogiBan)[BanX] = shogi->shogiBan;
    int (*komaDai)[DaiN] = shogi->komaDai;
    
    assert(komaDai[uwate][koma] > 0); // 駒を持ってること
    assert(shogiBan[to_y][to_x] == EMP);
    
    if (uwate) shogiBan[to_y][to_x] = (Koma) (koma + UWATE);
    else shogiBan[to_y][to_x] = koma;

    komaDai[uwate][koma]--;
}

static void temodoshi(ShogiKykumen *shogi, const Sashite *s)
{
    Koma (*shogiBan)[BanX] = shogi->shogiBan;
    int (*komaDai)[DaiN] = shogi->komaDai;

    if (s->type == SASHITE_IDOU) {
        Koma torik = s->idou.torigoma;
        Koma k =shogiBan[s->idou.to_y][s->idou.to_x];
        assert(k != EMP);
        if(s->idou.nari) k = (Koma)(k ^ NARI);
        shogiBan[s->idou.from_y][s->idou.from_x] = k;
        shogiBan[s->idou.to_y][s->idou.to_x] = torik;
        if (torik != EMP) {
            int dai_uwate = (torik&UWATE) ? 0 : 1;
            assert(komaDai[dai_uwate][torik&KOMATYPE1] > 0);
            komaDai[dai_uwate][torik&KOMATYPE1]--;
        }
        
    } else if (s->type == SASHITE_UCHI) {
        assert(shogiBan[s->uchi.to_y][s->uchi.to_x] != EMP);
        shogiBan[s->uchi.to_y][s->uchi.to_x] = EMP;
        komaDai[s->uchi.uwate][s->uchi.koma & KOMATYPE1]++;
    } else if (s->type == SASHITE_RESULT) {
        // do nothing
    } else {
        assert(0);
    }
}

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
static void createSashite(ShogiKykumen *shogi, int uwate, Sashite *s, int *n)
{
    Koma (*shogiBan)[BanX] = shogi->shogiBan;
    int (*komaDai)[DaiN] = shogi->komaDai;
    *n = 0;
    Sashite *cs = s;
    Koma teban = uwate ? UWATE : (Koma)0;
    
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
                    
                case UFU: break;
                    to_y = y+1;
                    setBitBB(&uwateKikiB, x, to_y);
                    break;
                    
                case UKY:  break;
                    for (to_y=y+1; to_y<BanY; to_y++) {
                        to_k = shogiBan[to_y][x];
                        setBitBB(&uwateKikiB, x, to_y);
                        if (to_k != EMP) break;
                    }
                    break;
                
                case UKE:  break;
                    if (x>0) setBitBB(&uwateKikiB, x-1, y+2);
                    if (x<BanX) setBitBB(&uwateKikiB, x+1, y+2);
                    break;

				case UGI: break;
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
				case UNGI: break;
                    for (int r=0; r<6; r++) {
                        to_x = x-KI_range[r][0];
                        to_y = y-KI_range[r][1];
                        
                        // 盤外チェック
                        if (to_x<0 || to_x>=BanX) continue;
                        if (to_y<0 || to_y>=BanY) continue;
                        
                        setBitBB(&uwateKikiB, to_x, to_y);
                    }
                    break;

				case UUMA: break;
                    for (int r=0; r<4; r++) {
                        to_x = x-UMA_range[r][0];
                        to_y = y-UMA_range[r][1];
                        
                        // 盤外チェック
                        if (to_x<0 || to_x>=BanX) continue;
                        if (to_y<0 || to_y>=BanY) continue;
                        
                        setBitBB(&uwateKikiB, to_x, to_y);
                    }

				case UKA: break;
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

				default:
                    break;
            }
        }
    }
    
    printBB(logf, &uwateKikiB);
	fprintf(logf, "\n");

    // 予定: 打ち
    // 予定: 効きから打ちふ詰めも検出
    
    *n = te_num;
}

static char* chomp(char *str)
{
    for (int i=0; i<1024; i++) {
        if (str[i] == '\0' || str[i]=='\n' || str[i]=='\r') {
            str[i] = '\0';
            break;
        }
    }
    return str;
}

static void sjis2utf8(char *utf8_str, const char *sjis_str, size_t len)
{
    iconv_t ic;
    char *pi = (char*)sjis_str;
    char *po = utf8_str;
    size_t ilen = strlen(sjis_str);
    ic = iconv_open("UTF-8", "SJIS");
    assert(ic);

    iconv(ic,&pi,&ilen,&po,&len);
    *po = '\0';
    iconv_close(ic);
}

#define KIF_HEAD    0
#define KIF_SASHITE 1
#define KIF_RESULT  2

static int readKIF(const char *filename, Kifu *kifu)
{
    Sashite* sashite = kifu->sashite;
    char buf[256];
    FILE *f = fopen(filename, "r");
    if (!f) return 0;
    
    int i = 0;
    int to_x = -1;
    int to_y = -1;
    int fm_x = -1;
    int fm_y = -1;
    int nari = 0;
    int status = KIF_HEAD;
    int uwate = 0;
    
    sashite[0].type = SASHITE_EMP;
    kifu->date[0]='\0';
    kifu->kisen = KISEN_SONOTA;
    kifu->gote[0]='\0';
    kifu->sente[0]='\0';
    kifu->tesuu = 0;
    kifu->result = 9;
    
    
    
    while (fgets(buf, 256, f)) {
        if(buf[0]=='*') continue; // コメント
        
        if (status == KIF_HEAD) { // ヘッダ情報
            if (strncmp("   1 ", buf, 4) == 0) {
                status = KIF_SASHITE;
            } else if (buf[0]=='\x8A' && buf[8]=='\x81'){ // 開(8A4A)始日時:(8146)
                kifu->date[0] = buf[10];kifu->date[1] = buf[11];kifu->date[2] = buf[12];kifu->date[3] = buf[13];
                assert(buf[14]=='/');
                kifu->date[4] = buf[15];kifu->date[5] = buf[16];
                assert(buf[17]=='/');
                kifu->date[6] = buf[18];kifu->date[7] = buf[19];
                kifu->date[8] = '\0';
                printf("%s ",kifu->date);
                
            } else if (buf[0]=='\x8A' && buf[4]=='\x81') { // 棋(8AFA)戦:
                char *kisen_str = &buf[6];
                chomp(kisen_str);
                if (strcmp(kisen_str, "\x96\xbc\x90\x6c\x90\xed") == 0) { // 名人戦
                    kifu->kisen = KISEN_MEIJIN;
                    printf("名人戦 ");
                } else if(strstr(kisen_str, "\x97\xb3\x89\xa4\x90\xed")) { //竜王戦
                    kifu->kisen = KISEN_RYUOU;
                    printf("竜王戦 ");
                } else if(strstr(kisen_str,"\x8a\xfb\x90\xb9\x90\xed")) { // 棋聖戦
                    kifu->kisen = KISEN_KISEI;
                    printf("棋聖戦 ");
                } else if(strstr(kisen_str,"\x89\xa4\x88\xca\x90\xed")) { // 王位戦
                    kifu->kisen = KISEN_OUI;
                    printf("王位戦 ");
                } else if(strstr(kisen_str,"\x89\xa4\x8d\xc0\x90\xed")) { // 王座戦
                    kifu->kisen = KISEN_OUZA;
                    printf("王座戦 ");
                } else if(strstr(kisen_str,"\x8a\xfb\x89\xa4\x90\xed")) { // 棋王戦
                    kifu->kisen = KISEN_KIOU;
                    printf("棋王戦 ");
                } else if(strstr(kisen_str,"\x89\xa4\x8f\xab\x90\xed")) { // 王将戦
                    kifu->kisen = KISEN_OUSHOU;
                    printf("王将戦 ");
                } else if(strstr(kisen_str,"\x8f\x87\x88\xca\x90\xed")) { // 順位戦
                    kifu->kisen = KISEN_JUNI;
                    printf("順位戦 ");
                } else if(strstr(kisen_str,"\x82\x6d\x82\x67\x82\x6a\x94\x74")) { // NHK杯
                    kifu->kisen = KISEN_NHK;
                    printf("NHK杯 ");
                } else if(strcmp(kisen_str,"\x8b\xe2\x89\xcd\x90\xed")) { // 銀河戦
                    kifu->kisen = KISEN_NHK;
                    printf("銀河戦 ");
                } else if(strcmp(kisen_str,"\x8f\x5c\x92\x69\x90\xed")) { // 十段戦
                    kifu->kisen = KISEN_JUUDAN;
                    printf("十段戦 ");
                } else if(strcmp(kisen_str,"\x8b\xe3\x92\x69\x90\xed")) { // 九段戦
                    kifu->kisen = KISEN_JUUDAN;
                    printf("九段戦 ");
                } else {
                    kifu->kisen = KISEN_SONOTA;
                    printf("その他の棋戦 ");
                }
            } else if (buf[0]=='\x8E' && buf[6]=='\x81') { // 手(8EE8)合割:
                // 基本　平(95BD)手
                assert(buf[8]=='\x95' && buf[9]=='\xBD');
            } else if (buf[0]=='\x8C' && buf[4]=='\x81') { // 後(8CE3)手:
                sjis2utf8(kifu->gote, chomp(&buf[6]), KishiNameLen);
                printf("%s ", kifu->gote);
            } else if (buf[0]=='\x90' && buf[1]=='\xE6' && buf[4]=='\x81') { // 先(90E6)手:
                sjis2utf8(kifu->sente, chomp(&buf[6]), KishiNameLen);
                printf("%s ", kifu->sente);
            }
        }
        
        if (status == KIF_SASHITE) {
            if (buf[5]=='\x82' || (buf[5]=='\x93' && buf[6]=='\xAF')) {
                if (buf[6]=='\xAF') { //同
                } else {
                    to_x = buf[6] - '\x4F';
                    
                    switch (buf[8]) {
                        case '\xEA': to_y = 1; break;
                        case '\xF1': to_y = 2; break;
                        case '\x4F': to_y = 3; break;
                        case '\x6C': to_y = 4; break;
                        case '\xDC': to_y = 5; break;
                        case '\x5A': to_y = 6; break;
                        case '\xB5': to_y = 7; break;
                        case '\xAA': to_y = 8; break;
                        case '\xE3': to_y = 9; break;
                        default:
                            assert(0);
                            break;
                    }
                }
                
                // 駒種
                // 歩 95E0 香 8D81 桂 8C6A 銀 8BE2 金 8BF0 角 8A70 飛 94F2 玉 8BCA 王 89A4
                // と 82C6 杏 88C7 圭 8C5C 全 9153 竜 97B3 龍 97B4 馬 946E
                // 成 90AC 打 91C5
                Koma km = EMP;
                int from_ind = 11;

                switch (buf[10]) {
                    case '\xE0': {
                        km = (buf[9]=='\x95')? FU : KI;
                        break;
                    }
                    case '\x81': km = KY; break;
                    case '\x6A': km = KE; break;
                    case '\xE2': km = GI; break;
                    case '\x70': km = KA; break;
                    case '\xF2': km = HI; break;
                    case '\xCA': case '\xA4': km = OU; break;
                    case '\xC6': km = NFU; break;
                    case '\xC7': km = NKY; break;
                    case '\x5C': km = NKE; break;
                    case '\x53': km = NGI; break;
                    case '\xB3': case '\xB4': km = RYU; break;
                    case '\x6E': km = UMA; break;
                    case '\xAC':    // 成
                        {
                            switch (buf[12]) {
                                case '\xE2': km=NGI; break;
                                case '\x81': km = NKY; break;
                                case '\x6A': km = NKE; break;
                                default:
                                    assert(0);
                                    break;
                            }
                            from_ind+=2;
                        }
                        break;
                    default:
                        assert(0);
                        break;
                }

                if (buf[from_ind] == '\x91') { // 打ち
                    sashite[i].uchi.type = SASHITE_UCHI;
                    sashite[i].uchi.to_x = INNER_X(to_x);
                    sashite[i].uchi.to_y = INNER_Y(to_y);
                    sashite[i].uchi.koma = km;
                    sashite[i].uchi.uwate = uwate;
                    
                } else {
                    nari = 0;
                    if (buf[from_ind] == '\x90') { // 成り
                        nari = 1;
                        from_ind += 2;
                    }
                    
                    assert(buf[from_ind]=='(');
                    fm_x = buf[from_ind+1] - '0';
                    fm_y = buf[from_ind+2] - '0';
                    
                    sashite[i].idou.type = SASHITE_IDOU;
                    sashite[i].idou.from_x = INNER_X(fm_x);
                    sashite[i].idou.from_y = INNER_Y(fm_y);
                    sashite[i].idou.to_x = INNER_X(to_x);
                    sashite[i].idou.to_y = INNER_Y(to_y);
                    sashite[i].idou.nari = nari;
                }
                i++;
                uwate = !uwate;
                
            } else {
                status = KIF_RESULT;
                kifu->tesuu = i;
            }
        }
        
        if (status == KIF_RESULT) {
            if (strstr(buf, "\x90\xe6\x8e\xe8")) { //先手
                sashite[i].type = SASHITE_RESULT;
                kifu->result = sashite[i].result.winner = 0;
                i++;
                break;
                
            } else if (strstr(buf, "\x8c\xe3\x8e\xe8")) { //後手
                sashite[i].type = SASHITE_RESULT;
                kifu->result = sashite[i].result.winner = 1;
                i++;
                break;
            } else if (strstr(buf, "\x90\xe7\x93\xfa\x8e\xe8")) { //千日手
                sashite[i].type = SASHITE_RESULT;
                kifu->result = sashite[i].result.winner = 2;
                i++;
                break;
            }

        }
    }
    sashite[i].type = SASHITE_EMP;

    fclose(f);
    
    return kifu->tesuu;
}

static void createKyokumenCode(char code[], const  ShogiKykumen *shogi, int rev)
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
    int allKomaNum[DaiN+1] = {/*EMP*/0,/*FU*/18,/*KY*/4,/*KE*/4,/*GI*/4,/*KI*/4,/*KA*/2,/*HI*/2,/*OU*/2};
    int uKomaNum[DaiN+1] = {0,0,0,0,0,0,0,0,0};
    int komaNum[DaiN+1] = {0,0,0,0,0,0,0,0,0};
    char uPosBuf[40], posBuf[40];
    char *uKomaPos[DaiN+1] = {NULL};
    char *komaPos[DaiN+1] = {NULL};
    int uNariBuf[40], nariBuf[40];
    int *uKomaNari[DaiN+1];
    int *komaNari[DaiN+1];

    for (int i=0; i<40; i++) {
        uPosBuf[i] = posBuf[i] = 0;
        uNariBuf[i] = nariBuf[i] = 0;
    }
    for (int i=1, n=0;i<DaiN+1;i++) {
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
                    if (k1==0) k1 = OU;
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
        for (int k=1; k<DaiN; k++) {
            for (int i=0; i<komaDai[u][k]; i++) uKomaPos[k][uKomaNum[k]++] = komaDaiCode;
            for (int i=0; i<komaDai[s][k]; i++) komaPos[k][komaNum[k]++] = komaDaiCode;
        }
    }
    
    // 駒が盤上/駒台にもない
    for (int k=1; k<=DaiN; k++) {
        for (int i=(uKomaNum[k]+komaNum[k]); i<allKomaNum[k]; i++) komaPos[k][komaNum[k]++] = komaNashiCode;
    }
    
    //-- コード生成 ---//
    Koma  codeKomas[7] = {KI, GI, HI, KA, KE, KY, FU};
    int codePos = 0;
    // 王
    code[codePos++] = uKomaPos[OU][0];
    code[codePos++] = komaPos[OU][0];
    
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
    assert(codePos == (KykumenCodeLen-1));
}

static void loadKyokumenFromCode(ShogiKykumen *shogi, const char code[])
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
    if (code[0] != ' ') {
        assert(code[0]>='!' && code[0]<='~');
        int p = banjouPos[code[0]-'!'];
        assert(p != -1);
        
        shogiBan[(shogi->uou_y = p/BanX)][(shogi->uou_x = p%BanX)] = UOU;
        
    }
    if (code[1] != ' ') {
        int p = banjouPos[code[1]-'!'];
        shogiBan[(shogi->ou_y = p/BanX)][(shogi->ou_x = p%BanX)] = OU;
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
            assert(code[codePos]>='!' && code[codePos]<='~');
            if (code[codePos] == '~') {
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

static void createShogiDB(const char* filename)
{
    remove(filename);
    
    sqlite3 *db = NULL;
    int ret = sqlite3_open(filename, &db);
    assert(ret == SQLITE_OK);
    
    // テーブルの作成
    ret = sqlite3_exec(db,
                       "create table KYOKUMEN_ID_MST ("
                       "KY_CODE TEXT PRIMARY KEY, "
                       "KY_ID INTEGER);"
                       , NULL, NULL, NULL);
    assert(ret == SQLITE_OK);
    
    // これは好みに応じてユーザがつくる。
    ret = sqlite3_exec(db,
                       "create table KYOKUMEN_INF ("
                       "KY_ID INTEGER PRIMARY KEY, "
                       "WIN_NUM INTEGER, "
                       "LOSE_NUM INTEGER, "
                       "SENNICHI_NUM INTEGER, "
                       "SCORE INTEGER"
                       ");"
                       , NULL, NULL, NULL);
    assert(ret == SQLITE_OK);
    
    ret = sqlite3_exec(db,
                       "create table KIF_INF ("
                       "KIF_ID INTEGER PRIMARY KEY, "
                       "KIF_DATE TEXT, "
                       "GOTE TEXT, "
                       "SENTE TEXT"
                       ");"
                       , NULL, NULL, NULL);
    assert(ret == SQLITE_OK);
    
    ret = sqlite3_exec(db,
                       "create table KYOKUMEN_KIF_INF ("
                       "KY_ID INTEGER, "
                       "KIF_ID INTEGER, "
                       "TESUU INTEGER"
                       ");"
                       , NULL, NULL, NULL);
    assert(ret == SQLITE_OK);
    
    sqlite3_close(db);
}

static void insertShogiDB(const char* filename, Kifu* kifu)
{
    sqlite3 *db = NULL;
    int ret = sqlite3_open(filename, &db);
    assert(ret == SQLITE_OK);

    
    {    // sqlの準備
        ret = sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
        assert(ret == SQLITE_OK);
    }

    // 新棋譜IDの取得
    int kif_id = 0;
    {
        sqlite3_stmt *selSql = NULL;
        
        ret = sqlite3_prepare(db, "select count(*) from KIF_INF;", -1, &selSql, NULL);
        assert(ret == SQLITE_OK);
        sqlite3_reset(selSql);
        ret = sqlite3_step(selSql);
        assert(ret == SQLITE_ROW);
        kif_id = sqlite3_column_int(selSql, 0) + 1;
        
        sqlite3_finalize(selSql);
    }
    
    // 棋譜情報の格納
    {
        sqlite3_stmt *insInfSql = NULL;
        ret = sqlite3_prepare(db, "insert into KIF_INF (KIF_ID, KIF_DATE, GOTE, SENTE) values(?,?,?,?)", -1, &insInfSql, NULL);
        assert(ret == SQLITE_OK);
        sqlite3_reset(insInfSql);
        sqlite3_bind_int(insInfSql, 1, kif_id);
        sqlite3_bind_text(insInfSql, 2, kifu->date, 0, SQLITE_TRANSIENT);
        sqlite3_bind_text(insInfSql, 3, kifu->gote, 0, SQLITE_TRANSIENT);
        sqlite3_bind_text(insInfSql, 4, kifu->sente, 0, SQLITE_TRANSIENT);
        
        while ((ret = sqlite3_step(insInfSql)) == SQLITE_BUSY)
            ;
        assert(ret == SQLITE_DONE);
        
        sqlite3_finalize(insInfSql);
    }

    int max_ky_id = 0;
    // 新局面IDの取得
    {
        sqlite3_stmt *selSql = NULL;

        ret = sqlite3_prepare(db, "select count(*) from KYOKUMEN_ID_MST;", -1, &selSql, NULL);
        assert(ret == SQLITE_OK);
    
        sqlite3_reset(selSql);
        ret = sqlite3_step(selSql);
        assert(ret == SQLITE_ROW);
        
        max_ky_id = sqlite3_column_int(selSql, 0);
        sqlite3_finalize(selSql);
        
        max_ky_id++;
    }

    Sashite *sashite =kifu->sashite;
    assert(sashite[kifu->tesuu].type == SASHITE_RESULT);
    
    ShogiKykumen shogi;
    int ky_id = 0;
    int uwate = 0;
    char ky_code[KykumenCodeLen];
    resetShogiBan(&shogi);
    
    for (int i=0; i<500; i++) {
        // 指手実施
        if (sasu(&shogi, &sashite[i]) >= 0) break;
        createKyokumenCode(ky_code, &shogi, uwate);

        //同一局面の検索
        {
            sqlite3_stmt *selSql = NULL;
            
            ret = sqlite3_prepare(db, "select KY_ID from KYOKUMEN_ID_MST where KY_CODE=?;", -1, &selSql, NULL);
            assert(ret == SQLITE_OK);
            sqlite3_reset(selSql);
            sqlite3_bind_text(selSql, 1, ky_code, KykumenCodeLen, SQLITE_TRANSIENT);
            if (SQLITE_ROW == sqlite3_step(selSql)) {
                ky_id = sqlite3_column_int(selSql, 0);
            } else {
                ky_id = -1;
            }
            sqlite3_finalize(selSql);
        }
        
        int doukyokumen = 0;
        if (ky_id > 0) {
            printf("*");
            
            // 千日手等、自分の棋譜の同局面か確認。
            sqlite3_stmt *selSql = NULL;
            
            ret = sqlite3_prepare(db, "select * from KYOKUMEN_KIF_INF where KY_ID=? and KIF_ID=?;", -1, &selSql, NULL);
            assert(ret == SQLITE_OK);
            sqlite3_reset(selSql);
            sqlite3_bind_int(selSql, 1, ky_id);
            sqlite3_bind_int(selSql, 2, kif_id);
            if (SQLITE_ROW == sqlite3_step(selSql)) {
                // 同棋譜に同局面あり;
                printf("-");
                doukyokumen = 1;
            }
            sqlite3_finalize(selSql);
            
        } else {    // 同一局面なし, データ挿入
            // idのみ
            sqlite3_stmt *insMstSql = NULL;
            ret = sqlite3_prepare(db, "insert into KYOKUMEN_ID_MST (KY_CODE, KY_ID) values(?,?);", -1, &insMstSql, NULL);
            assert(ret == SQLITE_OK);
            sqlite3_reset(insMstSql);
            sqlite3_bind_text(insMstSql, 1, ky_code, KykumenCodeLen, SQLITE_TRANSIENT);
            sqlite3_bind_int(insMstSql, 2, max_ky_id);

            printf("+");

            while ((ret = sqlite3_step(insMstSql)) == SQLITE_BUSY)
                ;
            assert(ret == SQLITE_DONE);
            
            sqlite3_finalize(insMstSql);
            
            ky_id = max_ky_id;
            max_ky_id++;
        }
        
        // 局面と棋譜の紐付け
        if (!doukyokumen) {
            sqlite3_stmt *insMstSql = NULL;
            ret = sqlite3_prepare(db, "insert into KYOKUMEN_KIF_INF (KY_ID,KIF_ID,TESUU) values(?,?,?);", -1, &insMstSql, NULL);
            assert(ret == SQLITE_OK);
            sqlite3_reset(insMstSql);
            sqlite3_bind_int(insMstSql, 1, ky_id);
            sqlite3_bind_int(insMstSql, 2, kif_id);
            sqlite3_bind_int(insMstSql, 3, i);
            
            while ((ret = sqlite3_step(insMstSql)) == SQLITE_BUSY)
                ;
            assert(ret == SQLITE_DONE);
            
            sqlite3_finalize(insMstSql);
        } else {
            // 手数を後のものにアップデート
            sqlite3_stmt *updMstSql = NULL;
            ret = sqlite3_prepare(db, "update KYOKUMEN_KIF_INF set TESUU=? where KY_ID=? and KIF_ID=?;", -1, &updMstSql, NULL);
            assert(ret == SQLITE_OK);
            sqlite3_reset(updMstSql);
            sqlite3_bind_int(updMstSql, 1, i);
            sqlite3_bind_int(updMstSql, 2, ky_id);
            sqlite3_bind_int(updMstSql, 3, kif_id);
            
            while ((ret = sqlite3_step(updMstSql)) == SQLITE_BUSY)
                ;
            assert(ret == SQLITE_DONE);
            
            sqlite3_finalize(updMstSql);
        }

        uwate = (!uwate) ? 1 : 0;
    }
    
    printf("\n");
    printKyokumen(stdout, &shogi);
    loadKyokumenFromCode(&shogi, ky_code);
    printKyokumen(stdout, &shogi);

    // 同一棋譜でないか念のため投了図でチェック
    {
        sqlite3_stmt *selSql = NULL;
        
        ret = sqlite3_prepare(db, "select count(*) from KYOKUMEN_KIF_INF where KY_ID=?;", -1, &selSql, NULL);
        assert(ret == SQLITE_OK);
        sqlite3_reset(selSql);
        sqlite3_bind_int(selSql, 1, ky_id);
        if (SQLITE_ROW == sqlite3_step(selSql)) {
            assert(sqlite3_column_int(selSql, 0) == 1);
            // 同じ棋譜を登録しようとしている可能性あり
        } else {
            assert(0);
        }
        sqlite3_finalize(selSql);
    }

    {
        ret = sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
        assert(ret == SQLITE_OK);
    }
    sqlite3_close(db);
}
