//
//  kyokumencode.h
//  ShogiDBTool
//
//  Created by tosyama on 2016/2/1.
//  Copyright (c) 2016 tosyama. All rights reserved.
//

#define KyokumenCodeLen  54

void createAreaKyokumenCode(char code[], const ShogiKyokumen *shogi);
void createKyokumenCode(char code[], const ShogiKyokumen *shogi, int rev=0);
void loadKyokumenFromCode(ShogiKyokumen *shogi, const char code[]);
