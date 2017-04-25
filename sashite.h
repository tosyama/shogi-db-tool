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
        unsigned int from_x : 4;
		unsigned int from_y : 4;
		unsigned int to_x : 4;
		unsigned int to_y : 4;
		unsigned int nari : 1;
        unsigned int torigoma : 6;
    } idou;
    struct {    // SASHITE_UCHI
        unsigned int type : 2;
        unsigned int uwate : 1;
		unsigned int koma: 7;
        unsigned int to_x : 4;
		unsigned int to_y : 4;
    } uchi;
    struct {    // SASHITE_RESULT
        unsigned int type : 2;
        unsigned int winner : 2; // 0:先手, 1:後手(上手), 2: 千日手
    } result;
	struct {
		unsigned int from : 10;
		unsigned int to : 8;
	} data;
} Sashite;


int sasu(ShogiKyokumen *shogi, Sashite *s);
void temodoshi(ShogiKyokumen *shogi, const Sashite *s);

int extractSashie(const Sashite **start, Sashite target, const Sashite *array, int n); 
