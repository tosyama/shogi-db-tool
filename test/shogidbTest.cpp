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
	int kif_id = sdb.registerKifu("20121212", "Player1", "Player2", 0, 0, "test1");
	REQUIRE(kif_id == 1);
	kif_id = sdb.registerKifu("20131213", "Player1", "Player2", 0, 1, "test2");
	REQUIRE(kif_id == 2);
	kif_id = sdb.registerKifu("20121212", "Player1", "Player2", 0, 1, "test3");
	REQUIRE(kif_id == -1);

	// kyokumen info registration.
	kif_id = 1;
	int score, results[10];
	int num = sdb.getKyokumenResults("code", results, &score);
	REQUIRE(num == 0);
	CHECK(results[0] == 0);
	CHECK(results[9] == 0);

	int ret = sdb.registerKyokumen("code", kif_id, 1, 2);
	REQUIRE(ret == 0);
	num = sdb.getKyokumenResults("code", results, &score);
	CHECK(num == 1);
	CHECK(results[0] == 0);
	CHECK(results[2] == 1);
	CHECK(results[9] == 0);

	kif_id = 2;
	ret = sdb.registerKyokumen("code", kif_id, 1, 0);
	REQUIRE(ret == 0);
	ret = sdb.registerKyokumen("code", kif_id, 2, 2);
	REQUIRE(ret == 0);
	ret = sdb.registerKyokumen("code", kif_id, 3, 9);
	REQUIRE(ret == 0);
	num = sdb.getKyokumenResults("code", results, &score);
	CHECK(num == 4);
	CHECK(results[0] == 1);
	CHECK(results[2] == 2);
	CHECK(results[9] == 1);

}
