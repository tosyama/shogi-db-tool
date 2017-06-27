//
//  shogidb.cpp
//  ShogiDBTool
//
//  Created by tosyama on 2014/12/28.
//  Copyright (c) 2016 tosyama. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sqlite3.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <new>
#include "shogiban.h"
#include "kyokumencode.h"
#include "sashite.h"
#include "kifu.h"
#include "shogidb.h"

void createShogiDB(const char* filename)
{
    remove(filename);
    
    sqlite3 *db = NULL;
    int ret = sqlite3_open(filename, &db);
    assert(ret == SQLITE_OK);
    
    // テーブルの作成
    ret = sqlite3_exec(db,
                       "create table KYOKUMEN_ID_MST ("
                       "KY_CODE TEXT PRIMARY KEY, "
                       "KY_ID INTEGER);"
                       , NULL, NULL, NULL);
    assert(ret == SQLITE_OK);
    
    // これは好みに応じてユーザがつくる。
    ret = sqlite3_exec(db,
                       "create table KYOKUMEN_INF ("
                       "KY_ID INTEGER PRIMARY KEY, "
                       "WIN_NUM INTEGER, "
                       "LOSE_NUM INTEGER, "
                       "SENNICHI_NUM INTEGER, "
                       "SCORE INTEGER"
                       ");"
                       , NULL, NULL, NULL);
    assert(ret == SQLITE_OK);
    
    ret = sqlite3_exec(db,
                       "create table KIF_INF ("
                       "KIF_ID INTEGER PRIMARY KEY, "
                       "KIF_DATE TEXT, "
                       "GOTE TEXT, "
                       "SENTE TEXT"
                       ");"
                       , NULL, NULL, NULL);
    assert(ret == SQLITE_OK);
    
    ret = sqlite3_exec(db,
                       "create table KYOKUMEN_KIF_INF ("
                       "KY_ID INTEGER, "
                       "KIF_ID INTEGER, "
                       "TESUU INTEGER"
                       ");"
                       , NULL, NULL, NULL);
    assert(ret == SQLITE_OK);
    
    sqlite3_close(db);
}

void insertShogiDB(const char* filename, Kifu* kifu)
{
    sqlite3 *db = NULL;
    int ret = sqlite3_open(filename, &db);
    assert(ret == SQLITE_OK);

    
    {    // sqlの準備
        ret = sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
        assert(ret == SQLITE_OK);
    }

    // 新棋譜IDの取得
    int kif_id = 0;
    {
        sqlite3_stmt *selSql = NULL;
        
        ret = sqlite3_prepare(db, "select count(*) from KIF_INF;", -1, &selSql, NULL);
        assert(ret == SQLITE_OK);
        sqlite3_reset(selSql);
        ret = sqlite3_step(selSql);
        assert(ret == SQLITE_ROW);
        kif_id = sqlite3_column_int(selSql, 0) + 1;
        
        sqlite3_finalize(selSql);
    }
    
    // 棋譜情報の格納
    {
        sqlite3_stmt *insInfSql = NULL;
        ret = sqlite3_prepare(db, "insert into KIF_INF (KIF_ID, KIF_DATE, GOTE, SENTE) values(?,?,?,?)", -1, &insInfSql, NULL);
        assert(ret == SQLITE_OK);
        sqlite3_reset(insInfSql);
        sqlite3_bind_int(insInfSql, 1, kif_id);
        sqlite3_bind_text(insInfSql, 2, kifu->date, 0, SQLITE_TRANSIENT);
        sqlite3_bind_text(insInfSql, 3, kifu->gote, 0, SQLITE_TRANSIENT);
        sqlite3_bind_text(insInfSql, 4, kifu->sente, 0, SQLITE_TRANSIENT);
        
        while ((ret = sqlite3_step(insInfSql)) == SQLITE_BUSY)
            ;
        assert(ret == SQLITE_DONE);
        
        sqlite3_finalize(insInfSql);
    }

    int max_ky_id = 0;
    // 新局面IDの取得
    {
        sqlite3_stmt *selSql = NULL;

        ret = sqlite3_prepare(db, "select count(*) from KYOKUMEN_ID_MST;", -1, &selSql, NULL);
        assert(ret == SQLITE_OK);
    
        sqlite3_reset(selSql);
        ret = sqlite3_step(selSql);
        assert(ret == SQLITE_ROW);
        
        max_ky_id = sqlite3_column_int(selSql, 0);
        sqlite3_finalize(selSql);
        
        max_ky_id++;
    }

    Sashite *sashite =kifu->sashite;
    assert(sashite[kifu->tesuu].type == SASHITE_RESULT);
    
    ShogiKyokumen shogi;
    int ky_id = 0;
    int uwate = 0;
    char ky_code[KyokumenCodeLen];
    resetShogiBan(&shogi);
    
    for (int i=0; i<500; i++) {
        // 指手実施
        if (sasu(&shogi, &sashite[i]) >= 0) break;
        createKyokumenCode(ky_code, &shogi, uwate);

        //同一局面の検索
        {
            sqlite3_stmt *selSql = NULL;
            
            ret = sqlite3_prepare(db, "select KY_ID from KYOKUMEN_ID_MST where KY_CODE=?;", -1, &selSql, NULL);
            assert(ret == SQLITE_OK);
            sqlite3_reset(selSql);
            sqlite3_bind_text(selSql, 1, ky_code, KyokumenCodeLen, SQLITE_TRANSIENT);
            if (SQLITE_ROW == sqlite3_step(selSql)) {
                ky_id = sqlite3_column_int(selSql, 0);
            } else {
                ky_id = -1;
            }
            sqlite3_finalize(selSql);
        }
        
        int doukyokumen = 0;
        if (ky_id > 0) {
            printf("*");
            
            // 千日手等、自分の棋譜の同局面か確認。
            sqlite3_stmt *selSql = NULL;
            
            ret = sqlite3_prepare(db, "select * from KYOKUMEN_KIF_INF where KY_ID=? and KIF_ID=?;", -1, &selSql, NULL);
            assert(ret == SQLITE_OK);
            sqlite3_reset(selSql);
            sqlite3_bind_int(selSql, 1, ky_id);
            sqlite3_bind_int(selSql, 2, kif_id);
            if (SQLITE_ROW == sqlite3_step(selSql)) {
                // 同棋譜に同局面あり;
                printf("-");
                doukyokumen = 1;
            }
            sqlite3_finalize(selSql);
            
        } else {    // 同一局面なし, データ挿入
            // idのみ
            sqlite3_stmt *insMstSql = NULL;
            ret = sqlite3_prepare(db, "insert into KYOKUMEN_ID_MST (KY_CODE, KY_ID) values(?,?);", -1, &insMstSql, NULL);
            assert(ret == SQLITE_OK);
            sqlite3_reset(insMstSql);
            sqlite3_bind_text(insMstSql, 1, ky_code, KyokumenCodeLen, SQLITE_TRANSIENT);
            sqlite3_bind_int(insMstSql, 2, max_ky_id);

            printf("+");

            while ((ret = sqlite3_step(insMstSql)) == SQLITE_BUSY)
                ;
            assert(ret == SQLITE_DONE);
            
            sqlite3_finalize(insMstSql);
            
            ky_id = max_ky_id;
            max_ky_id++;
        }
        
        // 局面と棋譜の紐付け
        if (!doukyokumen) {
            sqlite3_stmt *insMstSql = NULL;
            ret = sqlite3_prepare(db, "insert into KYOKUMEN_KIF_INF (KY_ID,KIF_ID,TESUU) values(?,?,?);", -1, &insMstSql, NULL);
            assert(ret == SQLITE_OK);
            sqlite3_reset(insMstSql);
            sqlite3_bind_int(insMstSql, 1, ky_id);
            sqlite3_bind_int(insMstSql, 2, kif_id);
            sqlite3_bind_int(insMstSql, 3, i);
            
            while ((ret = sqlite3_step(insMstSql)) == SQLITE_BUSY)
                ;
            assert(ret == SQLITE_DONE);
            
            sqlite3_finalize(insMstSql);
        } else {
            // 手数を後のものにアップデート
            sqlite3_stmt *updMstSql = NULL;
            ret = sqlite3_prepare(db, "update KYOKUMEN_KIF_INF set TESUU=? where KY_ID=? and KIF_ID=?;", -1, &updMstSql, NULL);
            assert(ret == SQLITE_OK);
            sqlite3_reset(updMstSql);
            sqlite3_bind_int(updMstSql, 1, i);
            sqlite3_bind_int(updMstSql, 2, ky_id);
            sqlite3_bind_int(updMstSql, 3, kif_id);
            
            while ((ret = sqlite3_step(updMstSql)) == SQLITE_BUSY)
                ;
            assert(ret == SQLITE_DONE);
            
            sqlite3_finalize(updMstSql);
        }

        uwate = (!uwate) ? 1 : 0;
    }
    
    printf("\n");
    printKyokumen(stdout, &shogi);
    loadKyokumenFromCode(&shogi, ky_code);
    printKyokumen(stdout, &shogi);

    // 同一棋譜でないか念のため投了図でチェック
    {
        sqlite3_stmt *selSql = NULL;
        
        ret = sqlite3_prepare(db, "select count(*) from KYOKUMEN_KIF_INF where KY_ID=?;", -1, &selSql, NULL);
        assert(ret == SQLITE_OK);
        sqlite3_reset(selSql);
        sqlite3_bind_int(selSql, 1, ky_id);
        if (SQLITE_ROW == sqlite3_step(selSql)) {
            assert(sqlite3_column_int(selSql, 0) == 1);
            // 同じ棋譜を登録しようとしている可能性あり
        } else {
            assert(0);
        }
        sqlite3_finalize(selSql);
    }

    {
        ret = sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
        assert(ret == SQLITE_OK);
    }
    sqlite3_close(db);
}

static int splitInt(int* intarr, int maxsize, const unsigned char* srcstr, int separator)
{
	int i = 0;
	int c;
	intarr[0]=0;
	while (c=*srcstr) {
		if (c==separator) {
			if ((i+1)>=maxsize) return i;
			i++;
			intarr[i] = 0;
		} else { 
			intarr[i] = intarr[i]*10 + (c-'0');
		}
		++srcstr;
	}
	return i;
}

static int joinInt(char* outstr, int maxsize, int* inarr, int insize, int separator)
{
	int n=0;
	for (int i=0; i<insize; i++) {
		if (inarr[i]) {
			n+=snprintf(&outstr[n], maxsize-n, "%d", inarr[i]);
		}
		if ((i<insize-1) && (n<maxsize)) {
			outstr[n]=separator;
			n++;
		}
		if (n==maxsize) return n;
	}
	outstr[n] = 0;
	return n;
}

class ShogiDB::ShogiDBImpl
{
    sqlite3 *db;
	void createTables()
	{
		int ret = sqlite3_exec(db,
				"create table KYOKUMEN_ID_MST ("
				"KY_CODE TEXT PRIMARY KEY, "
				"KY_ID INTEGER);"
				, NULL, NULL, NULL);
		assert(ret == SQLITE_OK);
		ret = sqlite3_exec(db,
				"create table KIF_INF ("
				"KIF_ID INTEGER PRIMARY KEY, "
				"KIF_DATE TEXT, "
				"UWATE_NM TEXT, "
				"SHITATE_NM TEXT"
				");"
				, NULL, NULL, NULL);
		assert(ret == SQLITE_OK);
		ret = sqlite3_exec(db,
				"create table KYOKUMEN_KIF_INF ("
				"KY_ID INTEGER, "
				"KIF_ID INTEGER, "
				"TESUU INTEGER"
				");"
				, NULL, NULL, NULL);
		assert(ret == SQLITE_OK);

		ret = sqlite3_exec(db,
				"create table KYOKUMEN_INF ("
				"KY_ID INTEGER PRIMARY KEY, "
				"SCORE INTEGER, "
				"RESULTS STRING"
				");"
				, NULL, NULL, NULL);
		assert(ret == SQLITE_OK);
	}
	
	int getNewKifID(){
		int kif_id = 0;
        sqlite3_stmt *selSql = NULL;
        
        int ret = sqlite3_prepare(db, "select ifnull(max(KIF_ID),0) from KIF_INF;", -1, &selSql, NULL);
        assert(ret == SQLITE_OK);
        ret = sqlite3_step(selSql);
        assert(ret == SQLITE_ROW);
        kif_id = sqlite3_column_int(selSql, 0) + 1;
        
        sqlite3_finalize(selSql);
		return kif_id;
	}

	int insertKifInf(int kif_id, const char* date, const char* uwate_name, const char* shitate_name, const char* comment) {
        sqlite3_stmt *insInfSql = NULL;
        int ret = sqlite3_prepare(db, "insert into KIF_INF (KIF_ID, KIF_DATE, UWATE_NM, SHITATE_NM) values(?,?,?,?)", -1, &insInfSql, NULL);
        assert(ret == SQLITE_OK);
        sqlite3_bind_int(insInfSql, 1, kif_id);
        sqlite3_bind_text(insInfSql, 2, date, strlen(date), SQLITE_TRANSIENT);
        sqlite3_bind_text(insInfSql, 3, uwate_name, strlen(uwate_name), SQLITE_TRANSIENT);
        sqlite3_bind_text(insInfSql, 4, shitate_name, strlen(shitate_name), SQLITE_TRANSIENT);
        
        ret = sqlite3_step(insInfSql);
        assert(ret == SQLITE_DONE);
        
        sqlite3_finalize(insInfSql);
		return 0;
	}

	int searchKifInf(const char* date, const char* uwate_name, const char* shitate_name) {
		int kif_id = 0;
        sqlite3_stmt *selSql = NULL;
        
        int ret = sqlite3_prepare(db,
				"select KIF_ID from KIF_INF"
				" where KIF_DATE=?"
				" and UWATE_NM=?"
				" and SHITATE_NM=?;"
				, -1, &selSql, NULL);
        assert(ret == SQLITE_OK);
        sqlite3_bind_text(selSql, 1, date, strlen(date), SQLITE_TRANSIENT);
        sqlite3_bind_text(selSql, 2, uwate_name, strlen(uwate_name), SQLITE_TRANSIENT);
        sqlite3_bind_text(selSql, 3, shitate_name, strlen(shitate_name), SQLITE_TRANSIENT);
        ret = sqlite3_step(selSql);
        if (ret == SQLITE_ROW)
			kif_id = sqlite3_column_int(selSql, 0);
        else if (ret == SQLITE_DONE) kif_id = 0;
		else kif_id = -1;

        sqlite3_finalize(selSql);
		return kif_id;
	}

	int getMaxKyID()
	{
        sqlite3_stmt *selSql = NULL;

        int ret = sqlite3_prepare(db, "select ifnull(max(KY_ID),0) from KYOKUMEN_ID_MST;", -1, &selSql, NULL);
        assert(ret == SQLITE_OK);
    
        ret = sqlite3_step(selSql);
        assert(ret == SQLITE_ROW);
        
        int max_ky_id = sqlite3_column_int(selSql, 0);
        sqlite3_finalize(selSql);
		return max_ky_id;
	}

	int maxKyID;
	int insertKyokumenMst(const char* ky_code)
	{
		if (!maxKyID) maxKyID = getMaxKyID();
		
		maxKyID++;

		sqlite3_stmt *insMstSql = NULL;
		int ret = sqlite3_prepare(db, "insert into KYOKUMEN_ID_MST (KY_CODE, KY_ID) values(?,?);", -1, &insMstSql, NULL);
		assert(ret == SQLITE_OK);

		sqlite3_bind_text(insMstSql, 1, ky_code, strlen(ky_code), SQLITE_TRANSIENT);
		sqlite3_bind_int(insMstSql, 2, maxKyID);
		ret = sqlite3_step(insMstSql);
	
		assert(ret == SQLITE_DONE);
		sqlite3_finalize(insMstSql);
	
		ret = sqlite3_prepare(db, "insert into KYOKUMEN_INF "
				"(KY_ID,SCORE,RESULTS) "
				"values(?,0,'');", -1, &insMstSql, NULL);
		assert(ret == SQLITE_OK);
		
		sqlite3_bind_int(insMstSql, 1, maxKyID);
		ret = sqlite3_step(insMstSql);
		
		assert(ret == SQLITE_DONE);
		sqlite3_finalize(insMstSql);

		return  maxKyID;
	}

	static const int MAX_RESULT_CODE=10;
	static const int RESULT_SEP=',';

	int getKyokumenInf(int ky_id, int* results, int maxsize, int* score)
	{
        sqlite3_stmt *selSql = NULL;
        
        int ret = sqlite3_prepare(db,
				"select SCORE, RESULTS from KYOKUMEN_INF"
				" where KY_ID=?;" , -1, &selSql, NULL);
        assert(ret == SQLITE_OK);
		sqlite3_bind_int(selSql, 1, ky_id);
        ret = sqlite3_step(selSql);
		assert(ret==SQLITE_ROW);

		const unsigned char* results_str;
        if (ret == SQLITE_ROW) {
        	*score = sqlite3_column_int(selSql, 0);
			results_str = sqlite3_column_text(selSql, 1);
		}
		int maxind = splitInt(results, maxsize, results_str, RESULT_SEP);
		sqlite3_finalize(selSql);

		return maxind;
	}

	void updateKyokumenInf(int ky_id, int result)
	{
		assert(result >= 0 && result < MAX_RESULT_CODE);
			
		int results[MAX_RESULT_CODE];
		int score, maxind;
		maxind = getKyokumenInf(ky_id, results, MAX_RESULT_CODE, &score);
		while (maxind<result) {
			++maxind;
			results[maxind] = 0;
		}
		results[result]++;
		
		char data_str[128];
		int len = joinInt(data_str, 128, results, maxind+1, RESULT_SEP);
        sqlite3_stmt *updSql = NULL;
        int ret = sqlite3_prepare(db,
				"update KYOKUMEN_INF set RESULTS=?"
				" where KY_ID=?;" , -1, &updSql, NULL);
        assert(ret == SQLITE_OK);
		sqlite3_bind_text(updSql, 1, data_str, len, SQLITE_TRANSIENT);
		sqlite3_bind_int(updSql, 2, ky_id);
        ret = sqlite3_step(updSql);
        assert(ret == SQLITE_DONE);

		sqlite3_finalize(updSql);
	}

	void insertKyokumenKifInf(int ky_id, int kif_id, int index)
	{
		sqlite3_stmt *insMstSql = NULL;
		int ret = sqlite3_prepare(db, "insert into KYOKUMEN_KIF_INF (KY_ID,KIF_ID,TESUU) values(?,?,?);", -1, &insMstSql, NULL);
		assert(ret == SQLITE_OK);
		sqlite3_reset(insMstSql);
		sqlite3_bind_int(insMstSql, 1, ky_id);
		sqlite3_bind_int(insMstSql, 2, kif_id);
		sqlite3_bind_int(insMstSql, 3, index);

		ret = sqlite3_step(insMstSql);
		assert(ret == SQLITE_DONE);

        sqlite3_finalize(insMstSql);
	}

	int getKyokumenID(const char* kyokumencode, bool do_create=false)
	{
		sqlite3_stmt *selSql = NULL;

		int ret = sqlite3_prepare(db, "select KY_ID from KYOKUMEN_ID_MST where KY_CODE=?;", -1, &selSql, NULL);
		assert(ret == SQLITE_OK);
		sqlite3_reset(selSql);
		sqlite3_bind_text(selSql, 1, kyokumencode, strlen(kyokumencode), SQLITE_TRANSIENT);
		ret = sqlite3_step(selSql);

		int ky_id;
		if (ret == SQLITE_ROW)
			ky_id = sqlite3_column_int(selSql, 0);
		else if (ret == SQLITE_DONE && do_create) {
			ky_id = insertKyokumenMst(kyokumencode);
		} else ky_id = -1;	

		sqlite3_finalize(selSql);
		return ky_id;
	}

public:
	ShogiDBImpl(const char* filename):maxKyID(0)
	{
		struct stat st;
		int not_exists = stat(filename, &st);
		int ret = sqlite3_open(filename, &db);
		assert(ret == SQLITE_OK);
		sqlite3_busy_timeout(db, 100);
		if (not_exists) createTables();
	}

	int registerKifu(const char* date, const char* uwate_name, const char* shitate_name, const char* comment)
	{
		int kif_id = searchKifInf(date, uwate_name, shitate_name);
		if (kif_id!=0) return -1;

		kif_id = getNewKifID();
		insertKifInf(kif_id, date, uwate_name, shitate_name, comment);
		return kif_id;
	}

	int registerKyokumen(const char* kyokumencode, int kif_id, int index, int result)
	{
		int ky_id = getKyokumenID(kyokumencode, true);
		if (kif_id <= 0) return -1;
		insertKyokumenKifInf(ky_id, kif_id, index);
		updateKyokumenInf(ky_id, result);
	}

	int getKyokumenResults(const char* kyokumencode, int *results, int *score)
	{
		int total, ind;
		int ky_id = getKyokumenID(kyokumencode);
		if (ky_id > 0) {
			ind = getKyokumenInf(ky_id, results, MAX_RESULT_CODE, score);
			total = results[0];
			for (int i=1; i<=ind; i++)
				total += results[i];
			++ind;
		} else {
			total = ind = 0;
			*score =0;
		}
		for (int i=ind; i<MAX_RESULT_CODE; i++)
			results[i]=0;	
		return total;
	}

	~ShogiDBImpl()
	{
		sqlite3_close(db);
	}
};

ShogiDB::ShogiDB(const char *filename)
{
	try {
		sdb = new ShogiDBImpl(filename);
	} catch (std::bad_alloc &e) {
		throw;
	} catch (std::exception &e) {
		throw;
	}
}

int ShogiDB::registerKifu(const char* date, const char* uwate_name, const char* shitate_name, const char* comment)
{
	return sdb->registerKifu(date, uwate_name, shitate_name, comment);
}

int ShogiDB::registerKyokumen(const char* kyokumencode, int kif_id, int index, int result)
{
	return sdb->registerKyokumen(kyokumencode, kif_id, index, result);
}

int ShogiDB::getKyokumenResults(const char* kyokumencode, int *results, int *score)
{
	return sdb->getKyokumenResults(kyokumencode, results, score);
}

ShogiDB::~ShogiDB()
{
	delete sdb;
}
