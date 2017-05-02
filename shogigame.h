//
//  shogigame.h
//  ShogiDBTool
//
//  Created by tosyama on 2017/4/25.
//  Copyright (c) 2017 tosyama. All rights reserved.
//

class ShogiGame
{
	class ShogiGameImpl;
	ShogiGameImpl *shg;
public:
	ShogiGame(const char* kycode = NULL);
	int move(int from_x, int from_y, int to_x, int to_y, bool promote);
	void print(bool reverse=false);
};
