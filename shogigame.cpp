#include <stdio.h>
#include <string.h>
#include "shogiban.h"
#include "kyokumencode.h"
#include "shogigame.h"

class  ShogiGame::ShogiGameData {
public:
	ShogiKyokumen shitate;
	ShogiKyokumen uwate;
	int teban; // 0: shitate, 1: uwate, -1: free
};

void copyKyokumenInversely(ShogiKyokumen *dst, const ShogiKyokumen *src)
{
	Koma *dk = dst->shogiBan[0];
	const Koma *src_ban = src->shogiBan[0];
	const Koma *sk = src_ban + (BanY*BanX);

	for (; sk>=src_ban; dk++, sk--) {
		Koma k=*sk;
		if (k != EMP) {
			*dk =static_cast<Koma>(k & UWATE ? k & KOMATYPE2 : k | UWATE);
		} else {
			*dk = EMP;
		}
	}

	memcpy(dst->komaDai[0],src->komaDai[1],sizeof(dst->komaDai[0]));
	memcpy(dst->komaDai[1],src->komaDai[0],sizeof(dst->komaDai[0]));

	if (src->ou_x != NonPos) {
		dst->uou_x = BanX - src->ou_x;
		dst->uou_y = BanY - src->ou_y;
	} else {
		dst->uou_x = dst->uou_y = NonPos;
	}

	if (src->uou_x != NonPos) {
		dst->ou_x = BanX - src->uou_x;
		dst->ou_y = BanY - src->uou_y;
	} else {
		dst->ou_x = dst->ou_y = NonPos;
	}
}

ShogiGame::ShogiGame(const char *kycode)
{
	shg = new ShogiGameData();
	if(kycode) {
		loadKyokumenFromCode(&shg->shitate, kycode);
		copyKyokumenInversely(&shg->uwate, &shg->shitate);
		shg->teban = kycode[KyokumenCodeLen] < 'u'? 0:1;
	} else {
		resetShogiBan(&shg->shitate);
		shg->uwate = shg->shitate;
		shg->teban = 0;
	}
}
