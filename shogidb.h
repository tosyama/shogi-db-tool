//
//  shogidb.h
//  ShogiDBTool
//
//  Created by tosyama on 2014/12/28.
//  Copyright (c) 2016 tosyama. All rights reserved.
//

void createShogiDB(const char* filename);
// void insertShogiDB(const char* filename, Kifu* kifu);

class ShogiDB
{
	class ShogiDBImpl;
	ShogiDBImpl *sdb;

public:
	ShogiDB(const char* filename);
	/// @return Kifu ID
	/// @retval -1 exists same info.
	int registerKifu(const char* date, const char* uwate_name, const char* shitate_name, const char* comment);
	/// @retval 0 Success.
	/// @retval -1 Exists same info.
	int registerKyokumen(const char* kyokumencode, int kif_id, int index, int result);
	/// @return total number of results.
	int getKyokumenResults(const char* kyokumencode, int *results, int *score);

	~ShogiDB();
};
