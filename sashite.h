//
//  sashite.h
//  ShogiDBTool
//
//  Created by tosyama on 2014/12/28.
//  Copyright (c) 2016 tosyama. All rights reserved.
//

#define MAX_LEGAL_SASHITE	593

enum SashiteType {
    SASHITE_EMP,
    SASHITE_IDOU,
    SASHITE_UCHI,
    SASHITE_RESULT
};

typedef union {
    unsigned int type : 2;
    struct {    // SASHITE_IDOU
        unsigned int type : 2;
        unsigned int from_x : 5;
		unsigned int from_y : 5;
		unsigned int to_x : 5;
		unsigned int to_y : 5;
		unsigned int nari : 1;
		unsigned int : 0;
        Koma torigoma;
    } idou;
    struct {    // SASHITE_UCHI
        unsigned int type : 2;
        unsigned int uwate : 1;
		unsigned int koma: 9;
        unsigned int to_x : 5;
		unsigned int to_y : 5;
    } uchi;
    struct {    // SASHITE_RESULT
        unsigned int type : 2;
        unsigned int winner : 2; // 0:先手, 1:後手(上手), 2: 千日手
    } result;
	struct {
		unsigned int from : 12;
		unsigned int to : 10;
	} data;
} Sashite;


int sasu(ShogiKykumen *shogi, Sashite *s);
void temodoshi(ShogiKykumen *shogi, const Sashite *s);

