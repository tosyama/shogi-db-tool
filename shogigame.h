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
	int board(int x, int y) const;
	int tegoma(int teban, int koma) const;
	int move(int from_x, int from_y, int to_x, int to_y, bool promote);
	int drop(int teban, int koma, int to_x, int to_y);
	void print(bool reverse=false) const;
	char* currentKyCode() const;
};
