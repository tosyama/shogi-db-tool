//
//  kyokumencode.h
//  ShogiDBTool
//
//  Created by tosyama on 2016/2/1.
//  Copyright (c) 2016 tosyama. All rights reserved.
//

#define KykumenCodeLen  54

void createKyokumenCode(char code[], const ShogiKykumen *shogi, int rev=0);
void loadKyokumenFromCode(ShogiKykumen *shogi, const char code[]);
