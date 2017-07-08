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

static int defaultConvResult(int teban, int shitate_result)
{
	if (teban) {
		if (shitate_result < 6) {
			if (shitate_result & 1) {
				return shitate_result-1;
			} else {
				return shitate_result+1;
			}
		}
	}
	return shitate_result;
}

class ShogiDB::ShogiDBImpl
{
    sqlite3 *db;
	ConvResult convResult;

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
				"SHITATE_NM TEXT, "
				"SENTE INTEGER, " // 0:shitate 1:uwate
				"RESULT INTEGER, " // result of each branch
				"COMMENT TEXT"
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
	
	inline int getTeban(int sente, int index) {
		return sente ? index&1 : (index ^ 1)&1;
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

	// Kif_Inf cache
	int cache_kif_id;
	int cache_kif_sente;
	int cache_kif_result;

	int insertKifInf(int kif_id, const char* date, const char* uwate_name, const char* shitate_name,
			int sente, int result, const char* comment) {
        sqlite3_stmt *insInfSql = NULL;
        int ret = sqlite3_prepare(db,
			"insert into KIF_INF (KIF_ID, KIF_DATE, UWATE_NM, SHITATE_NM, SENTE, RESULT, COMMENT) values(?,?,?,?,?,?,?)", -1, &insInfSql, NULL);
        assert(ret == SQLITE_OK);
        sqlite3_bind_int(insInfSql, 1, kif_id);
        sqlite3_bind_text(insInfSql, 2, date, strlen(date), SQLITE_TRANSIENT);
        sqlite3_bind_text(insInfSql, 3, uwate_name, strlen(uwate_name), SQLITE_TRANSIENT);
        sqlite3_bind_text(insInfSql, 4, shitate_name, strlen(shitate_name), SQLITE_TRANSIENT);
        sqlite3_bind_int(insInfSql, 5, sente);
        sqlite3_bind_int(insInfSql, 6, result);
        sqlite3_bind_text(insInfSql, 7, comment, strlen(comment), SQLITE_TRANSIENT);
        
        ret = sqlite3_step(insInfSql);
        assert(ret == SQLITE_DONE);
        
        sqlite3_finalize(insInfSql);
		
		cache_kif_id = kif_id;
		cache_kif_sente = sente;
		cache_kif_result = result;
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

	int getKifResult(int kif_id, int* sente, int* result) {
		if (kif_id == cache_kif_id) {
			*sente = cache_kif_sente;
			*result = cache_kif_result;
			return 0;
		}

        sqlite3_stmt *selSql = NULL;
        
        int ret = sqlite3_prepare(db,
				"select SENTE, RESULT from KIF_INF"
				" where KIF_ID=?;"
				, -1, &selSql, NULL);

        assert(ret == SQLITE_OK);
        sqlite3_bind_int(selSql, 1, kif_id);
        ret = sqlite3_step(selSql);

		assert(ret == SQLITE_ROW);
		cache_kif_id = kif_id;
		cache_kif_sente = *sente = sqlite3_column_int(selSql, 0);
		cache_kif_result = *result = sqlite3_column_int(selSql, 1);

        sqlite3_finalize(selSql);
		return 0;
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

	bool existsSameResult(int ky_id, int kif_id, int result)
	{
		sqlite3_stmt *selSql = NULL;

		int ret = sqlite3_prepare(db, "select TESUU from KYOKUMEN_KIF_INF where KY_ID=? and KIF_ID=?;", -1, &selSql, NULL);
		assert(ret == SQLITE_OK);
		sqlite3_bind_int(selSql, 1, ky_id);
		sqlite3_bind_int(selSql, 2, kif_id);
		ret = sqlite3_step(selSql);
	
		int exists = false;
		while (sqlite3_step(selSql)== SQLITE_ROW) {
			int sente, result;
			getKifResult(kif_id, &sente, &result);
			int index = sqlite3_column_int(selSql, 0);
			if (result == convResult(getTeban(sente, index), result))
				exists = true;
		}

		sqlite3_finalize(selSql);
		return exists;
	}

public:
	ShogiDBImpl(const char* filename, ConvResult conv_result):
		maxKyID(0),
		cache_kif_id(0),
		convResult(conv_result)
	{
		struct stat st;
		int not_exists = stat(filename, &st);
		int ret = sqlite3_open(filename, &db);

		assert(ret == SQLITE_OK);
		sqlite3_busy_timeout(db, 100);
		if (not_exists) createTables();

		if(!convResult) convResult = defaultConvResult;
	}

	int registerKifu(const char* date, const char* uwate_name, const char* shitate_name, int sente, int result, const char* comment)
	{
		assert(result >= 0 && result < MAX_RESULT_CODE);
		int kif_id = searchKifInf(date, uwate_name, shitate_name);
		if (kif_id!=0) return -1;

		kif_id = getNewKifID();
		insertKifInf(kif_id, date, uwate_name, shitate_name, sente, result, comment);
		return kif_id;
	}

	void registerKyokumen(const char* kyokumencode, int kif_id, int index)
	{
		assert(kif_id > 0);
		int ky_id = getKyokumenID(kyokumencode, true);
		
		// TODO: stop result count if exis same kyokumen.
		int sente, result;
		getKifResult(kif_id, &sente, &result);
		result = convResult(getTeban(sente, index), result);
		if (!existsSameResult(ky_id, kif_id, result)) {
			updateKyokumenInf(ky_id, result);
		}
		insertKyokumenKifInf(ky_id, kif_id, index);
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

	void beginTransaction()
	{
		int ret = sqlite3_exec(db, "begin transaction;" , NULL, NULL, NULL);
		assert(ret == SQLITE_OK);
	}

	void commit()
	{
		int ret = sqlite3_exec(db, "commit;" , NULL, NULL, NULL);
		assert(ret == SQLITE_OK);
	}

	void rollback()
	{
		int ret = sqlite3_exec(db, "rollback;" , NULL, NULL, NULL);
		assert(ret == SQLITE_OK);
		// reset cache.
		maxKyID = 0;
		cache_kif_id = 0;
	}
	
	~ShogiDBImpl()
	{
		sqlite3_close(db);
	}
};

ShogiDB::ShogiDB(const char *filename, ConvResult conv_result)
{
	try {
		sdb = new ShogiDBImpl(filename, conv_result);
	} catch (std::bad_alloc &e) {
		throw;
	} catch (std::exception &e) {
		throw;
	}
}

int ShogiDB::registerKifu(const char* date, const char* uwate_name, const char* shitate_name,
		int sente, int result, const char* comment)
{
	return sdb->registerKifu(date, uwate_name, shitate_name, sente, result, comment);
}

void ShogiDB::registerKyokumen(const char* kyokumencode, int kif_id, int index)
{
	sdb->registerKyokumen(kyokumencode, kif_id, index);
}

int ShogiDB::getKyokumenResults(const char* kyokumencode, int *results, int *score)
{
	return sdb->getKyokumenResults(kyokumencode, results, score);
}

void ShogiDB::beginTransaction()
{
	sdb->beginTransaction();
}

void ShogiDB::commit()
{
	sdb->commit();
}

void ShogiDB::rollback()
{
	sdb->rollback();
}

ShogiDB::~ShogiDB()
{
	delete sdb;
}
