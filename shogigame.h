//
//  shogigame.h
//  ShogiDBTool
//
//  Created by tosyama on 2017/4/25.
//  Copyright (c) 2017 tosyama. All rights reserved.
//

enum {
	SG_SUCCESS = 0,
	SG_FAILED = -1
};

class ShogiGame
{
	class ShogiGameImpl;
	ShogiGameImpl *shg;
public:
	ShogiGame(const char* kycode = NULL);
	int load(const char *kif_fname);

	int board(int x, int y) const;
	int tegoma(int teban, int koma) const;
	const char* date() const;
	const char* shitate() const;
	const char* uwate() const;
	
	int move(int from_x, int from_y, int to_x, int to_y, bool promote);
	int drop(int teban, int koma, int to_x, int to_y);
	
	int next();
	int previous();
	int go(int teme);

	void print(bool reverse=false) const;
	char* currentKyCode() const;
};
