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

int sasu(ShogiKyokumen *shogi, Sashite *s)
{
    if (s->type == SASHITE_IDOU) {
        s->idou.torigoma = sashite1(shogi, s->idou.from_x, s->idou.from_y, s->idou.to_x, s->idou.to_y, s->idou.nari);
        
    } else if(s->type == SASHITE_UCHI) {
        sashite2(shogi, s->uchi.uwate, (Koma)s->uchi.koma, s->uchi.to_x, s->uchi.to_y);
    
    } else if(s->type == SASHITE_RESULT) {
        return s->result.winner;
    
    }else {
        assert(0);
    }
    
    return -1;
}

void temodoshi(ShogiKyokumen *shogi, const Sashite *s)
{
    Koma (*shogiBan)[BanX] = shogi->shogiBan;
    int (*komaDai)[DaiN] = shogi->komaDai;

    if (s->type == SASHITE_IDOU) {
        Koma torik = (Koma)s->idou.torigoma;
        Koma k =shogiBan[s->idou.to_y][s->idou.to_x];
        if(s->idou.nari) k = (Koma)(k ^ NARI);
        shogiBan[s->idou.from_y][s->idou.from_x] = k;
        shogiBan[s->idou.to_y][s->idou.to_x] = torik;
        if (torik != EMP) {
			int dai_uwate;
			if(torik==OU) {
				shogi->ou_x = s->idou.to_x;
				shogi->ou_y = s->idou.to_y;
				dai_uwate = 0;
			} else if(torik==UOU) {
				shogi->uou_x = s->idou.to_x;
				shogi->uou_y = s->idou.to_y;
				dai_uwate = 1;
			} else if(k!=EMP)dai_uwate = (k&UWATE) ? 1 : 0;
            else dai_uwate = (torik&UWATE) ? 1 : 0;
            assert(komaDai[dai_uwate][torik&KOMATYPE1] > 0);
            komaDai[dai_uwate][torik&KOMATYPE1]--;
        }
		if (k == OU) {
			shogi->ou_x = s->idou.from_x;
			shogi->ou_y = s->idou.from_y;
		}
		if (k == UOU) {
			shogi->uou_x = s->idou.from_x;
			shogi->uou_y = s->idou.from_y;
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

int extractSashie(const Sashite **start, Sashite target, const Sashite *array, int n)
{
	*start = NULL;
	for (int i=0; i<n; ++i) {
		if (target.data.from == array[i].data.from) {
			int tn = 1;
			*start = array+i;
			for (int j=i+1; j<n; ++j) {
				if (target.data.from == array[j].data.from) ++tn;
				else return tn;
			}
			return tn;
		}
	}
	return 0;
}

bool existsSashite(Sashite target, const Sashite *array, int n)
{
	for (int i=0; i<n; ++i) {
		Sashite s = array[i];
		if (target.data.from == s.data.from) {
			for (int j=i+1; j<n; ++j) {
				if (target.data.from == s.data.from) {
					if (target.data.to == s.data.to)
						return true;
				}
				else return false;
			}
		}
	}
	return false;
}
