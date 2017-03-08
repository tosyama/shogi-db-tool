# shogi-db-tool

将棋局面コード  

位置 9x9 + 手駒台  
!#$&()*+-  
./0123456  
789:;<=>?  
@ABCDEFGH  
IJKLMNOPQ  
RSTUVWXYZ  
_abcdefgh  
ijklmnopq  
rstuvwxyz  
持ち駒: ~  
なし: ' ' (スペース)  
  
状態4個  
0 0個後手  
1 1個後手  
...  
A 10個後手  
...  
H 18個  

状態4個' 成りパターン →が一番目  
0 0000  
1 0001  
2 0010  
3 0011  
....  
A 001010  
...  
F 001111  
...  
Z 100011  
a 100100  
...  
z 111101  
{ 111110  
| 111111  

状態2個 [なりパターン][後手の持ち数]  
0 0000　(0個)  
1 0001 (1個)  
2 0010 (2個)  
4 0100  
5 0101  
6 0110  
8 1000  
9 1001  
A 1010  
C 1100  
D 1101  
E 1110  

王　位置(後手)  
玉　位置(先手)  
金4　成りなし  
銀4  
飛2  
角2  
桂4  
香4  
歩18  

Sample  
(v2&)uw20$*tx1/p15j20#+sy20!-rz9000789:;<=>?_abcdefgh  

表示  
上手の持駒：なし  
  ９ ８ ７ ６ ５ ４ ３ ２ １  
+---------------------------+  
|v香v桂v銀v金v玉v金v銀v桂v香|一  
| ・v飛 ・ ・ ・ ・ ・v角 ・|二  
|v歩v歩v歩v歩v歩v歩v歩v歩v歩|三  
| ・ ・ ・ ・ ・ ・ ・ ・ ・|四  
| ・ ・ ・ ・ ・ ・ ・ ・ ・|五  
| ・ ・ ・ ・ ・ ・ ・ ・ ・|六  
| 歩 歩 歩 歩 歩 歩 歩 歩 歩|七  
| ・ 角 ・ ・ ・ ・ ・ 飛 ・|八  
| 香 桂 銀 金 玉 金 銀 桂 香|九  
+---------------------------+  
下手の持駒：なし  

==  
テーブル設計  
KYOKUMEN_ID_MST  
局面コードKY_CODE, KY_ID  

KIF_INFO  
KIF_ID,DATE,PLAYER1,PLAYER2,RESULT,KISEN,SENKEI,KIF  

KYOKUMEN_INF  
KY_ID,勝数,負数,千日手数,持合,評価値,コメント  

KYOKUMEN_KIFU  
KY_ID,KIF_ID  

