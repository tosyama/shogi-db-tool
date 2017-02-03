//
//  sashite.cpp
//  ShogiDBTool
//
//  Created by tosyama on 2014/12/28.
//  Copyright (c) 2016 tosyama. All rights reserved.
//

#include <stdio.h>
#include <assert.h>
#include "shogiban.h"
#include "sashite.h"

int sasu(ShogiKykumen *shogi, Sashite *s)
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

void temodoshi(ShogiKykumen *shogi, const Sashite *s)
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

