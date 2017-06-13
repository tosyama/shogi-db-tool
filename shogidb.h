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
	int registerKifu(char* date, char* uwate_name, char* shitate_name, char* comment);
	~ShogiDB();
};
