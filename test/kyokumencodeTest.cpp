#include "catch.hpp"
#include <stdio.h>
#include <string.h>
#include "../shogiban.h"
#include "../kyokumencode.h"

using namespace Catch::Matchers;

void setInvalidShogiban(ShogiKykumen *s)
{
	for (int y=0; y<BanY; ++y)
		for (int x=0; x<BanX; ++x) {
			s->shogiBan[y][x] = (Koma)0xf;
		}
	for (int i=0; i<DaiN; ++i) {
		s->komaDai[0][i] = 0xf;
		s->komaDai[1][i] = 0xf;
	}
	s->ou_x = s->ou_y = s->uou_x = s->uou_y = 0xa;
}

void equalShogiban(char* result, ShogiKykumen *a, ShogiKykumen *b)
{
	result[0] = 0;
	for (int y=0; y<BanY; ++y)
		for (int x=0; x<BanX; ++x) {
			if (a->shogiBan[y][x] != b->shogiBan[y][x]) {
				sprintf(result, "not match(%d,%d)",x,y);
				return;
			}
		}
	for (int i=0; i<DaiN; ++i) {
		if(a->komaDai[0][i] != b->komaDai[0][i]) {
			sprintf(result, "not match dai:%d",i);
			return;
		}
		if(a->komaDai[1][i] != b->komaDai[1][i]) {
			sprintf(result, "not match udai:%d",i);
			return;
		}
	}

	if (a->ou_x != b->ou_x || a->ou_y != b->ou_y) {
		sprintf(result, "not match ou pos.");
		return;
	}
	if (a->uou_x != b->uou_x || a->uou_y != b->uou_y) {
		sprintf(result, "not match uou pos.");
		return;
	}

	sprintf(result, "match");
}

TEST_CASE("Kyoumen save/load with code", "[kycode]")
{
    ShogiKykumen save_shogi;
    ShogiKykumen load_shogi;

	char code[KykumenCodeLen];
	int len;
	char result[32];

	// first case.
    resetShogiBan(&save_shogi);
	createKyokumenCode(code, &save_shogi);
	len = strlen(code);
	REQUIRE(len == KykumenCodeLen-1);

	setInvalidShogiban(&load_shogi);
	loadKyokumenFromCode(&load_shogi, code);

	equalShogiban(result, &save_shogi, &load_shogi);
	CHECK_THAT(result, Equals("match"));

	// noKoma
    resetShogiBan(&save_shogi);
	for (int y=0; y<BanY; ++y)
		for (int x=0; x<BanX; ++x) {
			save_shogi.shogiBan[y][x] = EMP;
		}
	save_shogi.ou_x = save_shogi.ou_y = NonPos;
	save_shogi.uou_x = save_shogi.uou_y = NonPos;

	createKyokumenCode(code, &save_shogi);
	len = strlen(code);
	REQUIRE(len == KykumenCodeLen-1);

	setInvalidShogiban(&load_shogi);
	loadKyokumenFromCode(&load_shogi, code);

	equalShogiban(result, &save_shogi, &load_shogi);
	CHECK_THAT(result, Equals("match"));

	// tegoma only
	save_shogi.komaDai[0][FU]=18;
	save_shogi.komaDai[0][KY]=4;
	save_shogi.komaDai[0][KE]=4;
	save_shogi.komaDai[0][GI]=4;
	save_shogi.komaDai[0][KI]=4;
	save_shogi.komaDai[0][KA]=2;
	save_shogi.komaDai[0][HI]=2;

	createKyokumenCode(code, &save_shogi);
	len = strlen(code);
	REQUIRE(len == KykumenCodeLen-1);

	printf("%s\n",code);
	setInvalidShogiban(&load_shogi);
	loadKyokumenFromCode(&load_shogi, code);

	equalShogiban(result, &save_shogi, &load_shogi);
	CHECK_THAT(result, Equals("match"));

	save_shogi.komaDai[0][FU]=0;
	save_shogi.komaDai[0][KY]=0;
	save_shogi.komaDai[0][KE]=0;
	save_shogi.komaDai[0][GI]=0;
	save_shogi.komaDai[0][KI]=0;
	save_shogi.komaDai[0][KA]=0;
	save_shogi.komaDai[0][HI]=0;

	save_shogi.komaDai[1][FU]=18;
	save_shogi.komaDai[1][KY]=4;
	save_shogi.komaDai[1][KE]=4;
	save_shogi.komaDai[1][GI]=4;
	save_shogi.komaDai[1][KI]=4;
	save_shogi.komaDai[1][KA]=2;
	save_shogi.komaDai[1][HI]=2;

	createKyokumenCode(code, &save_shogi);
	len = strlen(code);
	REQUIRE(len == KykumenCodeLen-1);

	printf("%s\n",code);
	setInvalidShogiban(&load_shogi);
	loadKyokumenFromCode(&load_shogi, code);

	equalShogiban(result, &save_shogi, &load_shogi);
	CHECK_THAT(result, Equals("match"));
}
