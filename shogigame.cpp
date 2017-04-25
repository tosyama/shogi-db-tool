#include <stdio.h>
#include "shogiban.h"
#include "shogigame.h"

class  ShogiGame::ShogiGameData {
public:
	ShogiKyokumen shitate;
	ShogiKyokumen uwate;
};

ShogiGame::ShogiGame(const char *kycode)
{
	shg = new ShogiGameData();
	if(kycode) {
	} else {
		resetShogiBan(&shg->shitate);
		shg->uwate = shg->shitate;
	}
}
