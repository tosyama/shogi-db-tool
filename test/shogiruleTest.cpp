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
	const char *code;
	
	// first case
    createSashiteAll(&shogi, s, &n);
	CHECK(n==30);

	// max case
	code = ".43~~~~00/02~0-~0;~30~~~~00uwy~H000~~~~~~~~~~~~~~~~~~";
	loadKyokumenFromCode(&shogi, code);
    createSashiteAll(&shogi, s, &n);
	CHECK(n==MAX_LEGAL_SASHITE);

	// general case
	code = "#i29~j~308Kc~26~0$E10+fs~20!-rz8020?@BFGNSlJRXZe~~~~~";
	loadKyokumenFromCode(&shogi, code);
    createSashiteAll(&shogi, s, &n);
	CHECK(n==207);

	// uchi FU case
	Sashite uchifu;
	uchifu.type = SASHITE_UCHI;
	uchifu.uchi.uwate = 0;
	uchifu.uchi.koma = FU;
	const Sashite *ss;
	int n_fu;

	// excludes fu oute.
	code = "Dv2&)1w20$*Ux1/p15j20#+sy20!-Wz9000789;<=>?CT_acefgh~";
	loadKyokumenFromCode(&shogi, code);
    createSashiteAll(&shogi, s, &n);
	n_fu = extractSashie(&ss, uchifu, s, n); 
	CHECK(n_fu==4); 

	// includes fu oute.
	code = "Dv2&)1w20$*Vx1/p15j20#+sy20!-rz9000789;<=>?CT_acefgh~";
	loadKyokumenFromCode(&shogi, code);
    createSashiteAll(&shogi, s, &n);
	n_fu = extractSashie(&ss, uchifu, s, n); 
	CHECK(n_fu==4); 

	// includes fu oute.
	code = "Dv2&)1w20$*Ux1/p15j20#+sy20!-Wz9000789;<>?CFT_acefgh~";
	loadKyokumenFromCode(&shogi, code);
    createSashiteAll(&shogi, s, &n);
	n_fu = extractSashie(&ss, uchifu, s, n); 
	CHECK(n_fu==5); 

}
