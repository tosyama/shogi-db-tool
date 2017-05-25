#include <stdio.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <string>
#include <new>
#include <assert.h>
#include "shogiban.h"
#include "kyokumencode.h"
#include "sashite.h"
#include "kifu.h"
#include "shogirule.h"
#include "shogigame.h"

const int S_TEBAN=0;
const int U_TEBAN=1;
const int FREE_TEBAN=2;

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
		else if (teban == U_TEBAN)
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
		int n = kif.tesuu + 1;
		kifu.reserve(n);
		for (int i=0; i<n; ++i)
			kifu.push_back(kif.sashite[i]);
		return SG_SUCCESS;
	}

	int turn() {
		if (kifu.size() > 0 && kifu[curIndex-1].type == SASHITE_RESULT)
			return -1;
		else return teban;
	}

	// true:legal
	bool sasu(Sashite &te, bool check_legal = false) {
		Sashite rte = revS(te);
		bool legal = true;
		if (check_legal) {
			if (teban == S_TEBAN) {
				createSashiteAll(&shitate, legalTe, &legalTeNum);
			} else if (teban == U_TEBAN) {
				createSashiteAll(&uwate, legalTe, &legalTeNum);
			}
		}
		::sasu(&shitate,&te);
		::sasu(&uwate,&rte);
		if (teban == S_TEBAN) teban= U_TEBAN;
		else if (teban == U_TEBAN) teban = S_TEBAN;

		return legal;
	}

	int move(int from_x, int from_y, int to_x, int to_y, bool promote)
	{
		// Check critical param err.
		if (from_x < 1 || from_x > 9
			|| from_y < 1 || from_y >9
			|| to_x < 1 || to_x >9
			|| to_y < 1 || to_y >9
			|| shitate.shogiBan[INNER_Y(from_y)][INNER_X(from_x)]==EMP) {
			return SG_FAILED;
		}
		if (promote) {
			switch(shitate.shogiBan[INNER_Y(from_y)][INNER_X(from_x)]&KOMATYPE2) {
				case KI:
				case OU:
					return SG_FAILED;
			}
		}

		Sashite te;
		te.type = SASHITE_IDOU;
		te.idou.from_x = INNER_X(from_x);
		te.idou.from_y = INNER_Y(from_y);
		te.idou.to_x = INNER_X(to_x);
		te.idou.to_y = INNER_Y(to_y);
		te.idou.nari = promote;

		int ret = sasu(te, true) ? SG_SUCCESS : SG_FOUL;

		kifu.resize(curIndex);
		kifu.push_back(te);
		curIndex++;

		return ret; 
	}

	int drop(int teban, int koma, int to_x, int to_y)
	{
		// Check critical param err.
		if (teban < 0 || teban > 1 || koma < 0 || koma >= DaiN)
			return SG_FAILED;
		if ( to_x < 1 || to_x > 9 || to_y < 1 || to_y >9
			|| shitate.shogiBan[INNER_Y(to_y)][INNER_X(to_x)]!=EMP)
			return SG_FAILED;

		Sashite te;
		te.type = SASHITE_UCHI;
		te.uchi.uwate = teban;
		te.uchi.koma = koma;
		te.uchi.to_x = INNER_X(to_x);
		te.uchi.to_y = INNER_Y(to_y);
		sasu(te);

		kifu.resize(curIndex);
		kifu.push_back(te);
		curIndex++;
		return SG_SUCCESS;
	}

	void next()
	{
		if (curIndex < kifu.size()) {
			Sashite &te = kifu[curIndex];
			if (te.type != SASHITE_RESULT)
				sasu(te);
			++curIndex;
		}
	}

	void previous()
	{
		if (curIndex > 0) {
			--curIndex;
			Sashite te = kifu[curIndex];
			if (te.type != SASHITE_RESULT) {
				temodoshi(&shitate, &te);
				Sashite rte=revS(te);
				temodoshi(&uwate, &rte);
				if (teban == S_TEBAN) teban= U_TEBAN;
				else if (teban == U_TEBAN) teban = S_TEBAN;
			}
		}
	}

	void go(int index) {
		if (curIndex > index) {
			if (index < 0) index = 0;
			do previous();
			while (curIndex != index);
		} else if (curIndex < index) {
			if (index > kifu.size()) index = kifu.size();
			do next();
			while (curIndex != index);
		}
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

int ShogiGame::turn() const
{
	return shg->turn();
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

void ShogiGame::next()
{
	shg->next();
}

void ShogiGame::previous()
{
	shg->previous();
}

void ShogiGame::go(int index)
{
	shg->go(index);
}

int ShogiGame::current() const
{
	return shg->curIndex;
}

int ShogiGame::last() const
{
	return shg->kifu.size();
}

void ShogiGame::print(bool reverse) const
{
	if (reverse)
		printKyokumen(stdout, &shg->uwate);
	else
		printKyokumen(stdout, &shg->shitate);
}

char* ShogiGame::kycode() const
{
	return shg->currentKyCode();
}
