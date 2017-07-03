#include "catch.hpp"
#include <stdio.h>
#include <sys/stat.h>
#include "../shogidb.h"

TEST_CASE("shogi database tests", "[db]")
{
	const char* dbname = "testdb.db";
	struct stat st;

	remove(dbname);	
	ShogiDB sdb(dbname);
	// db was created.
	REQUIRE(stat(dbname, &st) == 0);

	// kif info registration.
	int kif_id = sdb.registerKifu("20121212", "Player1", "Player2", 0, 2, "test1");
	REQUIRE(kif_id == 1);
	kif_id = sdb.registerKifu("20131213", "Player1", "Player2", 1, 0, "test2");
	REQUIRE(kif_id == 2);
	kif_id = sdb.registerKifu("20121212", "Player1", "Player2", 1, 0, "test3");
	REQUIRE(kif_id == -1);

	// kyokumen info registration.
	kif_id = 1;
	int score, results[10];
	int num = sdb.getKyokumenResults("code", results, &score);
	REQUIRE(num == 0);
	CHECK(results[0] == 0);
	CHECK(results[9] == 0);

	sdb.registerKyokumen("code", kif_id, 1);
	num = sdb.getKyokumenResults("code", results, &score);
	CHECK(num == 1);
	CHECK(results[0] == 0);
	CHECK(results[2] == 1);
	CHECK(results[9] == 0);
	sdb.registerKyokumen("code1", kif_id, 2);
	num = sdb.getKyokumenResults("code1", results, &score);
	CHECK(num == 1);
	CHECK(results[3] == 1);

	kif_id = 2;
	sdb.registerKyokumen("code", kif_id, 1);
	sdb.registerKyokumen("code", kif_id, 2);
	sdb.registerKyokumen("code", kif_id, 3);
	num = sdb.getKyokumenResults("code", results, &score);
	CHECK(num == 4);
	CHECK(results[0] == 1);
	CHECK(results[1] == 2);
	CHECK(results[2] == 1);
}
