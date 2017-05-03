#include <stdio.h>
#include <string.h>
#include "shogiban.h"
#include "kyokumencode.h"
#include "sashite.h"
#include "shogirule.h"
#include "shogigame.h"

const int U_TEBAN=1;
const int S_TEBAN=0;
const int FREE_TEBAN=-1;

class  ShogiGame::ShogiGameImpl {
public:
	ShogiKyokumen shitate;
	ShogiKyokumen uwate;
	int teban; // 0: shitate, 1: uwate, -1: free
	int teNum;
	Sashite legalTe[MAX_LEGAL_SASHITE];
	
	void createLegalTe() {
		if(teban == S_TEBAN)
			createSashiteAll(&shitate, legalTe, &teNum);
		else if(teban == U_TEBAN)
			createSashiteAll(&uwate, legalTe, &teNum);
		else 
			teNum = 0;
	}

	void init(const char *kycode) {
		if(kycode) {
			teban = kycode[KyokumenCodeLen] < 'u'? S_TEBAN:U_TEBAN;
			if (teban == S_TEBAN) {
				loadKyokumenFromCode(&shitate, kycode);
				copyKyokumenInversely(&uwate, &shitate);
			} else {
				loadKyokumenFromCode(&uwate, kycode);
				copyKyokumenInversely(&shitate, &uwate);
			}
			// TODO: legal check.
		} else {
			resetShogiBan(&shitate);
			uwate = shitate;
			teban = S_TEBAN;
		}
		createLegalTe();
	}

	int move(int from_x, int from_y, int to_x, int to_y, bool promote)
	{
		Sashite te;
		te.type = SASHITE_IDOU;
		te.idou.from_x = INNER_X(from_x);
		te.idou.from_y = INNER_Y(from_y);
		te.idou.to_x = INNER_X(to_x);
		te.idou.to_y = INNER_Y(to_y);
		te.idou.nari = promote;
		Sashite rte;
		rte.type = SASHITE_IDOU;
		rte.idou.from_x = BanX-1-INNER_X(from_x);
		rte.idou.from_y = BanY-1-INNER_Y(from_y);
		rte.idou.to_x = BanX-1-INNER_X(to_x);
		rte.idou.to_y = BanY-1-INNER_Y(to_y);
		rte.idou.nari = promote;
		sasu(&shitate,&te);
		sasu(&uwate,&rte);
		if (teban == S_TEBAN) teban= U_TEBAN;
		else if (teban == U_TEBAN) teban = S_TEBAN;
		return 0;
	}
	int drop(int teban, int koma, int to_x, int to_y)
	{
		Sashite te;
		te.type = SASHITE_UCHI;
		te.uchi.uwate = teban;
		te.uchi.koma = koma;
		te.uchi.to_x = INNER_X(to_x);
		te.uchi.to_y = INNER_Y(to_y);
		Sashite rte;
		rte.type = SASHITE_UCHI;
		rte.uchi.uwate = teban;
		rte.uchi.koma = koma;
		rte.uchi.to_x = BanX-1-INNER_X(to_x);
		rte.uchi.to_y = BanY-1-INNER_Y(to_y);
		sasu(&shitate,&te);
		sasu(&uwate,&rte);
		if (teban == S_TEBAN) teban= U_TEBAN;
		else if (teban == U_TEBAN) teban = S_TEBAN;
		return 0;
	}
};

ShogiGame::ShogiGame(const char *kycode)
{
	shg = new ShogiGameImpl();
	shg->init(kycode);
}

int ShogiGame::board(int x, int y) const
{
	return shg->shitate.shogiBan[INNER_Y(y)][INNER_X(x)];
}

int ShogiGame::tegoma(int teban, int koma) const
{
	return shg->shitate.komaDai[teban][koma];
}

int ShogiGame::move(int from_x, int from_y, int to_x, int to_y, bool promote)
{
	return shg->move(from_x, from_y, to_x, to_y, promote);
}

int ShogiGame::drop(int teban, int koma, int to_x, int to_y)
{
	return shg->drop(teban, koma, to_x, to_y);
}

void ShogiGame::print(bool reverse) const
{
	if (reverse)
		printKyokumen(stdout, &shg->uwate);
	else
		printKyokumen(stdout, &shg->shitate);
}
