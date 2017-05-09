#include <stdio.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <string>
#include <new>
#include "shogiban.h"
#include "kyokumencode.h"
#include "sashite.h"
#include "kifu.h"
#include "shogirule.h"
#include "shogigame.h"

const int U_TEBAN=1;
const int S_TEBAN=0;
const int FREE_TEBAN=-1;

class  ShogiGame::ShogiGameImpl {
	int revX(int x) {return BanX-1-x;}
	int revY(int y) {return BanY-1-y;}
	Sashite revS(Sashite te) {
		Sashite rte;
		if (te.type == SASHITE_IDOU) {
			rte.type = SASHITE_IDOU;
			rte.idou.from_x = revX(te.idou.from_x);
			rte.idou.from_y = revY(te.idou.from_y);
			rte.idou.to_x = revX(te.idou.to_x);
			rte.idou.to_y = revY(te.idou.to_y);
			rte.idou.nari = te.idou.nari;
			rte.idou.torigoma = te.idou.torigoma ? te.idou.torigoma ^ UWATE : 0;
		} else if (te.type == SASHITE_UCHI) {
			rte.type = SASHITE_UCHI;
			rte.uchi.uwate = !te.uchi.uwate;
			rte.uchi.koma = te.uchi.koma;
			rte.uchi.to_x = revX(te.uchi.to_x);
			rte.uchi.to_y = revY(te.uchi.to_y);
		}
		return rte;
	}
public:
	std::string gameDate;
	std::string sName;
	std::string uName;
	ShogiKyokumen shitate;
	ShogiKyokumen uwate;
	int teban; // 0: shitate, 1: uwate, -1: free

	int curIndex;
	std::vector<Sashite> kifu;
	
	int legalTeNum;
	Sashite legalTe[MAX_LEGAL_SASHITE];

	char kycode[KyokumenCodeLen+1];
	
	void createLegalTe() {
		if(teban == S_TEBAN)
			createSashiteAll(&shitate, legalTe, &legalTeNum);
		else if(teban == U_TEBAN)
			createSashiteAll(&uwate, legalTe, &legalTeNum);
		else 
			legalTeNum = 0;
	}

	void init(const char *kycode, const char *s_name, const char *u_name) {
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
		time_t now = time(NULL);
		struct tm* today = localtime(&now);
		char dstr[9];
		sprintf(dstr,"%04d%02d%02d",1900+today->tm_year,today->tm_mon+1,today->tm_mday);
		gameDate = dstr;
		sName = s_name;
		uName = u_name;
		kifu.resize(0);
		curIndex = 0;
		createLegalTe();
	}

	int load(const char* kif_fname)
	{
		Kifu kif;
		readKIF(kif_fname, &kif);
		init(NULL, kif.sente, kif.gote);
		gameDate = kif.date;
		kifu.reserve(kif.tesuu);
		for (int i=0; i<kif.tesuu; ++i)
			kifu.push_back(kif.sashite[i]);
		return SG_SUCCESS;
	}

	void sasu(Sashite &te) {
		Sashite rte = revS(te);
		::sasu(&shitate,&te);
		::sasu(&uwate,&rte);
		if (teban == S_TEBAN) teban= U_TEBAN;
		else if (teban == U_TEBAN) teban = S_TEBAN;
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
		sasu(te);
		return SG_SUCCESS;
	}

	int drop(int teban, int koma, int to_x, int to_y)
	{
		Sashite te;
		te.type = SASHITE_UCHI;
		te.uchi.uwate = teban;
		te.uchi.koma = koma;
		te.uchi.to_x = INNER_X(to_x);
		te.uchi.to_y = INNER_Y(to_y);
		sasu(te);
		return SG_SUCCESS;
	}

	int next()
	{
		if (curIndex < kifu.size()) {
			sasu(kifu[curIndex]);
			++curIndex;
		}
		return curIndex;
	}

	int previous()
	{
		if (curIndex > 0) {
			--curIndex;
			Sashite te = kifu[curIndex];
			temodoshi(&shitate, &te);
			Sashite rte=revS(te);
			temodoshi(&uwate, &rte);
		}
		return curIndex;
	}

	int go(int teme) {
		if (curIndex > teme) {
			if (teme < 0) teme = 0;
			do previous();
			while (curIndex != teme);
		} else if (curIndex < teme) {
			if (teme > kifu.size()) teme = kifu.size();
			do next();
			while (curIndex != teme);
		}
		return curIndex;
	}

	char *currentKyCode()
	{
		if (teban==U_TEBAN) {
			createAreaKyokumenCode(kycode, &uwate);
			kycode[KyokumenCodeLen-1]='u';
			kycode[KyokumenCodeLen]='\0';
		} else
			createAreaKyokumenCode(kycode, &shitate);
		return kycode;
	}
};

ShogiGame::ShogiGame(const char *kycode)
{
	try {
		shg = new ShogiGameImpl();
		shg->init(kycode,"Player1","Player2");
	} catch (std::bad_alloc &e) {
		throw;
	} catch (std::exception &e) {
		delete shg;
		throw;
	}
}

ShogiGame::~ShogiGame()
{
	delete shg;
}

int ShogiGame::load(const char* kif_fname)
{
	return shg->load(kif_fname);
}

int ShogiGame::board(int x, int y) const
{
	return shg->shitate.shogiBan[INNER_Y(y)][INNER_X(x)];
}

int ShogiGame::tegoma(int teban, int koma) const
{
	return shg->shitate.komaDai[teban][koma];
}

const char* ShogiGame::date() const
{
	return shg->gameDate.c_str();
}

const char* ShogiGame::shitate() const
{
	return shg->sName.c_str();
}

const char* ShogiGame::uwate() const
{
	return shg->uName.c_str();
}
int ShogiGame::move(int from_x, int from_y, int to_x, int to_y, bool promote)
{
	return shg->move(from_x, from_y, to_x, to_y, promote);
}

int ShogiGame::drop(int teban, int koma, int to_x, int to_y)
{
	return shg->drop(teban, koma, to_x, to_y);
}

int ShogiGame::next()
{
	return shg->next();
}

int ShogiGame::previous()
{
	return shg->previous();
}

int ShogiGame::go(int teme)
{
	return shg->go(teme);
}

void ShogiGame::print(bool reverse) const
{
	if (reverse)
		printKyokumen(stdout, &shg->uwate);
	else
		printKyokumen(stdout, &shg->shitate);
}

char* ShogiGame::currentKyCode() const
{
	return shg->currentKyCode();
}
