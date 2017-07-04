//
//  shogidb.h
//  ShogiDBTool
//
//  Created by tosyama on 2014/12/28.
//  Copyright (c) 2016 tosyama. All rights reserved.
//

void createShogiDB(const char* filename);
// void insertShogiDB(const char* filename, Kifu* kifu);

typedef int (*ConvResult)(int teban, int shitate_result);

class ShogiDB
{
	class ShogiDBImpl;
	ShogiDBImpl *sdb;

public:
	ShogiDB(const char* filename, ConvResult conv_result = NULL);
	/// @param[in] sente 0:shitate, 1:uwate.
	/// @param[in] result Result of game from shitate.
	/// @return Kifu ID
	/// @retval -1 exists same info.
	int registerKifu(const char* date, const char* uwate_name, const char* shitate_name, 
			int sente, int result, const char* comment);

	void registerKyokumen(const char* kyokumencode, int kif_id, int index);

	/// @return Total number of results.
	int getKyokumenResults(const char* kyokumencode, int *results, int *score);

	void beginTransaction();
	void commit();
	void rollback();

	~ShogiDB();
};
