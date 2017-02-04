//
//  sashite.h
//  ShogiDBTool
//
//  Created by tosyama on 2014/12/28.
//  Copyright (c) 2016 tosyama. All rights reserved.
//

enum SashiteType {
    SASHITE_EMP,
    SASHITE_IDOU,
    SASHITE_UCHI,
    SASHITE_RESULT
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


int sasu(ShogiKykumen *shogi, Sashite *s);
void temodoshi(ShogiKykumen *shogi, const Sashite *s);

