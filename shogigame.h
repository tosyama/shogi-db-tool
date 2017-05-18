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

	ShogiGame(const ShogiGame&);
	ShogiGame& operator=(const ShogiGame&);

public:
	ShogiGame(const char* kycode = NULL);
	~ShogiGame();

	// @retval SG_SUCCESS 
	// @retval SG_FAILED TBI 
	int load(const char *kif_fname);

	/// @return Indicated koma ID on the board.
	int board(int x, int y) const;
	/// @return # of indicated koma ID in hand.
	int tegoma(int teban, int koma) const;

	/// @retval 0 Shitate turn.
	/// @retval 1 Uwate turn.
	/// @retval 2 Unknown. (Free mode) 
	/// @retval -1 Game set.
	int turn() const;
	const char* date() const;
	/// @return Player name.
	const char* shitate() const;
	const char* uwate() const;
	
	/// @retval SG_SUCCESS
	/// @retval SG_FAILED Operation was canceled by paramater error.
	int move(int from_x, int from_y, int to_x, int to_y, bool promote);
	int drop(int teban, int koma, int to_x, int to_y);
	
	int next();
	int previous();
	int go(int index);

	int current() const;
	int last() const;
	int result() const; // TBI

	void print(bool reverse=false) const;
	char* kycode() const;
};
