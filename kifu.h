//
//  kifu.h
//  ShogiDBTool
//
//  Created by tosyama on 2014/12/28.
//  Copyright (c) 2016 tosyama. All rights reserved.
//

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

int readKIF(const char *filename, Kifu* kifu);
