#include "catch.hpp"
#include "../shogiban.h"

TEST_CASE("Create all sashite.", "[rule]")
{
    ShogiKykumen shogi;
    resetShogiBan(&shogi);
	REQUIRE(1);
}
