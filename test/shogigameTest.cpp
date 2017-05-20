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
	CHECK(shogi.board(2,6)==1);
	shogi.previous();
	CHECK(shogi.turn()==0);
 	REQUIRE(shogi.board(2,6)==0);
	shogi.go(11);
	REQUIRE(shogi.tegoma(0,1)==1);
	REQUIRE(shogi.tegoma(1,1)==1);
	shogi.previous();
	REQUIRE(shogi.tegoma(0,1)==0);
	shogi.previous();
	REQUIRE(shogi.tegoma(1,1)==0);

	CHECK(shogi.last() == 144);
	shogi.go(shogi.last());
	CHECK(shogi.turn() == -1);
	shogi.previous();
	CHECK(shogi.turn() == 1);
}


