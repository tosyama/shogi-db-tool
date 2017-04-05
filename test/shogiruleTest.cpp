#include "catch.hpp"
#include <stdio.h>
#include "../shogiban.h"
#include "../kyokumencode.h"
#include "../sashite.h"
#include "../shogirule.h"

TEST_CASE("Create all sashite.", "[rule]")
{
    ShogiKykumen shogi;
    resetShogiBan(&shogi);
    Sashite s[600];
    int n;
	
	// first case
    createSashiteAll(&shogi, s, &n);
	CHECK(n==30);

	// max case
	char *code = ".43~~~~00/02~0-~0;~30~~~~00uwy~H000~~~~~~~~~~~~~~~~~~";
	loadKyokumenFromCode(&shogi, code);
    createSashiteAll(&shogi, s, &n);
	CHECK(n==593);

}
