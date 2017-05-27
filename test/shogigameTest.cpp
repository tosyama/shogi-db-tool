#include "catch.hpp"
#include <stdio.h>
#include "../shogigame.h"

using namespace Catch::Matchers;

TEST_CASE("shogigame creation tests", "[game]")
{
    ShogiGame shogi;
	shogi.load("test.kif");
	CHECK_THAT(shogi.date(), Equals("20011113"));
	CHECK_THAT(shogi.uwate(), Equals("\u6240\u53f8\u548c\u6674"));
	CHECK_THAT(shogi.shitate(), Equals("\u6728\u6751\u4e00\u57fa"));
	CHECK(shogi.turn()==0);
	shogi.next();
	REQUIRE(shogi.turn()==1);
	CHECK(shogi.board(2,6)==Fu);
	shogi.previous();
	CHECK(shogi.turn()==0);
 	REQUIRE(shogi.board(2,6)==0);
	shogi.go(11);
	REQUIRE(shogi.tegoma(0,Fu)==1);
	REQUIRE(shogi.tegoma(1,Fu)==1);
	shogi.previous();
	REQUIRE(shogi.tegoma(0,Fu)==0);
	shogi.previous();
	REQUIRE(shogi.tegoma(1,Fu)==0);

	CHECK(shogi.last() == 144);
	shogi.go(shogi.last());
	CHECK(shogi.turn() == -1);
	shogi.previous();
	CHECK(shogi.turn() == 1);
}

TEST_CASE("shogigame move manual tests", "[game]")
{
    ShogiGame shogi;

	// Legal move.
	CHECK(shogi.move(7,7,7,6,false)==SG_SUCCESS);
	REQUIRE(shogi.board(7,6)==Fu);

	CHECK(shogi.move(8,3,8,4,false)==SG_SUCCESS);
	REQUIRE(shogi.board(8,4)==(Fu|Uwate));

	CHECK(shogi.move(8,8,3,3,true)==SG_SUCCESS);
	REQUIRE(shogi.board(3,3)==(Kaku|Promoted));
	REQUIRE(shogi.tegoma(0,Fu)==1);

	// Critical error move.
	CHECK(shogi.move(8,3,8,4,false)==SG_FAILED);
	CHECK(shogi.move(8,4,10,4,false)==SG_FAILED);
	CHECK(shogi.move(8,4,8,10,false)==SG_FAILED);
	CHECK(shogi.current()==3);

	// Foul play
	CHECK(shogi.move(2,2,2,2,false)==SG_FOUL);
	REQUIRE(shogi.tegoma(1,Kaku)==1);
	REQUIRE(shogi.board(2,2)==0);
	CHECK(shogi.current()==4);

	CHECK(shogi.move(2,8,2,8,true)==SG_FOUL);
	REQUIRE(shogi.board(2,8)==(Hisya|Promoted));
	shogi.previous();
	REQUIRE(shogi.board(2,8)==Hisya);

	shogi.previous();
	REQUIRE(shogi.board(2,2)==(Kaku|Uwate));
	REQUIRE(shogi.tegoma(1,Kaku)==0);
}
