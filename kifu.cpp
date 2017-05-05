//
//  kifu.cpp
//  ShogiDBTool
//
//  Created by tosyama on 2014/12/28.
//  Copyright (c) 2016 tosyama. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <iconv.h>
#include "shogiban.h"
#include "sashite.h"
#include "kifu.h"

static char* chomp(char *str)
{
    for (int i=0; i<1024; i++) {
        if (str[i] == '\0' || str[i]=='\n' || str[i]=='\r') {
            str[i] = '\0';
            break;
        }
    }
    return str;
}

static void sjis2utf8(char *utf8_str, const char *sjis_str, size_t len)
{
    iconv_t ic;
    char *pi = (char*)sjis_str;
    char *po = utf8_str;
    size_t ilen = strlen(sjis_str);
    ic = iconv_open("UTF-8", "SJIS");
    assert(ic);

    iconv(ic,&pi,&ilen,&po,&len);
    *po = '\0';
    iconv_close(ic);
}

#define KIF_HEAD    0
#define KIF_SASHITE 1
#define KIF_RESULT  2

int readKIF(const char *filename, Kifu *kifu)
{
    Sashite* sashite = kifu->sashite;
    char buf[256];
    FILE *f = fopen(filename, "r");
    if (!f) return 0;
    
    int i = 0;
    int to_x = -1;
    int to_y = -1;
    int fm_x = -1;
    int fm_y = -1;
    int nari = 0;
    int status = KIF_HEAD;
    int uwate = 0;
    
    sashite[0].type = SASHITE_EMP;
    kifu->date[0]='\0';
    kifu->kisen = KISEN_SONOTA;
    kifu->gote[0]='\0';
    kifu->sente[0]='\0';
    kifu->tesuu = 0;
    kifu->result = 9;
    
    while (fgets(buf, 256, f)) {
        if(buf[0]=='*') continue; // コメント
        
        if (status == KIF_HEAD) { // ヘッダ情報
            if (strncmp("   1 ", buf, 4) == 0) {
                status = KIF_SASHITE;
            } else if (buf[0]=='\x8A' && buf[8]=='\x81'){ // 開(8A4A)始日時:(8146)
                kifu->date[0] = buf[10];kifu->date[1] = buf[11];kifu->date[2] = buf[12];kifu->date[3] = buf[13];
                assert(buf[14]=='/');
                kifu->date[4] = buf[15];kifu->date[5] = buf[16];
                assert(buf[17]=='/');
                kifu->date[6] = buf[18];kifu->date[7] = buf[19];
                kifu->date[8] = '\0';
                // printf("%s ",kifu->date);
                
            } else if (buf[0]=='\x8A' && buf[4]=='\x81') { // 棋(8AFA)戦:
                char *kisen_str = &buf[6];
                chomp(kisen_str);
                if (strcmp(kisen_str, "\x96\xbc\x90\x6c\x90\xed") == 0) { // 名人戦
                    kifu->kisen = KISEN_MEIJIN;
                    // printf("名人戦 ");
                } else if(strstr(kisen_str, "\x97\xb3\x89\xa4\x90\xed")) { //竜王戦
                    kifu->kisen = KISEN_RYUOU;
                    // printf("竜王戦 ");
                } else if(strstr(kisen_str,"\x8a\xfb\x90\xb9\x90\xed")) { // 棋聖戦
                    kifu->kisen = KISEN_KISEI;
                    // printf("棋聖戦 ");
                } else if(strstr(kisen_str,"\x89\xa4\x88\xca\x90\xed")) { // 王位戦
                    kifu->kisen = KISEN_OUI;
                    // printf("王位戦 ");
                } else if(strstr(kisen_str,"\x89\xa4\x8d\xc0\x90\xed")) { // 王座戦
                    kifu->kisen = KISEN_OUZA;
                    // printf("王座戦 ");
                } else if(strstr(kisen_str,"\x8a\xfb\x89\xa4\x90\xed")) { // 棋王戦
                    kifu->kisen = KISEN_KIOU;
                    // printf("棋王戦 ");
                } else if(strstr(kisen_str,"\x89\xa4\x8f\xab\x90\xed")) { // 王将戦
                    kifu->kisen = KISEN_OUSHOU;
                    // printf("王将戦 ");
                } else if(strstr(kisen_str,"\x8f\x87\x88\xca\x90\xed")) { // 順位戦
                    kifu->kisen = KISEN_JUNI;
                    // printf("順位戦 ");
                } else if(strstr(kisen_str,"\x82\x6d\x82\x67\x82\x6a\x94\x74")) { // NHK杯
                    kifu->kisen = KISEN_NHK;
                    // printf("NHK杯 ");
                } else if(strcmp(kisen_str,"\x8b\xe2\x89\xcd\x90\xed")) { // 銀河戦
                    kifu->kisen = KISEN_NHK;
                    // printf("銀河戦 ");
                } else if(strcmp(kisen_str,"\x8f\x5c\x92\x69\x90\xed")) { // 十段戦
                    kifu->kisen = KISEN_JUUDAN;
                    // printf("十段戦 ");
                } else if(strcmp(kisen_str,"\x8b\xe3\x92\x69\x90\xed")) { // 九段戦
                    kifu->kisen = KISEN_JUUDAN;
                    // printf("九段戦 ");
                } else {
                    kifu->kisen = KISEN_SONOTA;
                    // printf("その他の棋戦 ");
                }
            } else if (buf[0]=='\x8E' && buf[6]=='\x81') { // 手(8EE8)合割:
                // 基本　平(95BD)手
                assert(buf[8]=='\x95' && buf[9]=='\xBD');
            } else if (buf[0]=='\x8C' && buf[4]=='\x81') { // 後(8CE3)手:
                sjis2utf8(kifu->gote, chomp(&buf[6]), KishiNameLen);
                // printf("%s ", kifu->gote);
            } else if (buf[0]=='\x90' && buf[1]=='\xE6' && buf[4]=='\x81') { // 先(90E6)手:
                sjis2utf8(kifu->sente, chomp(&buf[6]), KishiNameLen);
                // printf("%s ", kifu->sente);
            }
        }
        
        if (status == KIF_SASHITE) {
            if (buf[5]=='\x82' || (buf[5]=='\x93' && buf[6]=='\xAF')) {
                if (buf[6]=='\xAF') { //同
                } else {
                    to_x = buf[6] - '\x4F';
                    
                    switch (buf[8]) {
                        case '\xEA': to_y = 1; break;
                        case '\xF1': to_y = 2; break;
                        case '\x4F': to_y = 3; break;
                        case '\x6C': to_y = 4; break;
                        case '\xDC': to_y = 5; break;
                        case '\x5A': to_y = 6; break;
                        case '\xB5': to_y = 7; break;
                        case '\xAA': to_y = 8; break;
                        case '\xE3': to_y = 9; break;
                        default:
                            assert(0);
                            break;
                    }
                }
                
                // 駒種
                // 歩 95E0 香 8D81 桂 8C6A 銀 8BE2 金 8BF0 角 8A70 飛 94F2 玉 8BCA 王 89A4
                // と 82C6 杏 88C7 圭 8C5C 全 9153 竜 97B3 龍 97B4 馬 946E
                // 成 90AC 打 91C5
                Koma km = EMP;
                int from_ind = 11;

                switch (buf[10]) {
                    case '\xE0': {
                        km = (buf[9]=='\x95')? FU : KI;
                        break;
                    }
                    case '\x81': km = KY; break;
                    case '\x6A': km = KE; break;
                    case '\xE2': km = GI; break;
                    case '\x70': km = KA; break;
                    case '\xF2': km = HI; break;
                    case '\xCA': case '\xA4': km = OU; break;
                    case '\xC6': km = NFU; break;
                    case '\xC7': km = NKY; break;
                    case '\x5C': km = NKE; break;
                    case '\x53': km = NGI; break;
                    case '\xB3': case '\xB4': km = RYU; break;
                    case '\x6E': km = UMA; break;
                    case '\xAC':    // 成
                        {
                            switch (buf[12]) {
                                case '\xE2': km=NGI; break;
                                case '\x81': km = NKY; break;
                                case '\x6A': km = NKE; break;
                                default:
                                    assert(0);
                                    break;
                            }
                            from_ind+=2;
                        }
                        break;
                    default:
                        assert(0);
                        break;
                }

                if (buf[from_ind] == '\x91') { // 打ち
                    sashite[i].uchi.type = SASHITE_UCHI;
                    sashite[i].uchi.to_x = INNER_X(to_x);
                    sashite[i].uchi.to_y = INNER_Y(to_y);
                    sashite[i].uchi.koma = km;
                    sashite[i].uchi.uwate = uwate;
                    
                } else {
                    nari = 0;
                    if (buf[from_ind] == '\x90') { // 成り
                        nari = 1;
                        from_ind += 2;
                    }
                    
                    assert(buf[from_ind]=='(');
                    fm_x = buf[from_ind+1] - '0';
                    fm_y = buf[from_ind+2] - '0';
                    
                    sashite[i].idou.type = SASHITE_IDOU;
                    sashite[i].idou.from_x = INNER_X(fm_x);
                    sashite[i].idou.from_y = INNER_Y(fm_y);
                    sashite[i].idou.to_x = INNER_X(to_x);
                    sashite[i].idou.to_y = INNER_Y(to_y);
                    sashite[i].idou.nari = nari;
                }
                i++;
                uwate = !uwate;
                
            } else {
                status = KIF_RESULT;
                kifu->tesuu = i;
            }
        }
        
        if (status == KIF_RESULT) {
            if (strstr(buf, "\x90\xe6\x8e\xe8")) { //先手
                sashite[i].type = SASHITE_RESULT;
                kifu->result = sashite[i].result.winner = 0;
                i++;
                break;
                
            } else if (strstr(buf, "\x8c\xe3\x8e\xe8")) { //後手
                sashite[i].type = SASHITE_RESULT;
                kifu->result = sashite[i].result.winner = 1;
                i++;
                break;
            } else if (strstr(buf, "\x90\xe7\x93\xfa\x8e\xe8")) { //千日手
                sashite[i].type = SASHITE_RESULT;
                kifu->result = sashite[i].result.winner = 2;
                i++;
                break;
            }

        }
    }
    sashite[i].type = SASHITE_EMP;
//	printf("\n");
    fclose(f);
    
    return kifu->tesuu;
}

