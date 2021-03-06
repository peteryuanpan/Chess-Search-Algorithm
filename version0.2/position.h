#ifndef POSITION_H_
#define POSITION_H_

#include "base.h"

//两种棋盘坐标表示法下的字符转换
/*
 a9 b9 c9 d9 e9 f9 g9 h9 i9     51  52  53  54  55  56  57  58  59  // LINE 3
 a8 b8 c8 d8 e8 f8 g8 h8 i8     67  68  69  70  71  72  73  74  75  // LINE 4
 a7 b7 c7 d7 e7 f7 g7 h7 i7     83  84  85  86  87  88  89  90  91  // LINE 5
 a6 b6 c6 d6 e6 f6 g6 h6 i6     99 100 101 102 103 104 105 106 107  // LINE 6
 a5 b5 c5 d5 e5 f5 g5 h5 i5    115 116 117 118 119 120 121 122 123  // LINE 7
 a4 b4 c4 d4 e4 f4 g4 h4 i4    131 132 133 134 135 136 137 138 139  // LINE 8
 a3 b3 c3 d3 e3 f3 g3 h3 i3    147 148 149 150 151 152 153 154 155  // LINE 9
 a2 b2 c2 d2 e2 f2 g2 h2 i2    163 164 165 166 167 168 169 170 171  // LINE 10
 a1 b1 c1 d1 e1 f1 g1 h1 i1    179 180 181 182 183 184 185 186 187  // LINE 11
 a0 b0 c0 d0 e0 f0 g0 h0 i0    195 196 197 198 199 200 201 202 203  // LINE 12

 							// C3  C4  C5  C6  C7  C8  C9  C10 C11
 */

const int PIECE_TYPE[48] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
							0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 6, 6, 6,
							0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 6, 6, 6};

const int RED_TYPE = 16;
const int BLACK_TYPE = 32;

const int KING_TYPE = 0;
const int ADVISOR_TYPE = 1;
const int BISHOP_TYPE = 2;
const int KNIGHT_TYPE = 3;
const int ROOK_TYPE = 4;
const int CANNON_TYPE = 5;
const int PAWN_TYPE = 6;

const int KING_FROM = 0;
const int KING_TO = 0;
const int ADVISOR_FROM = 1;
const int ADVISOR_TO = 2;
const int BISHOP_FROM = 3;
const int BISHOP_TO = 4;
const int KNIGHT_FROM = 5;
const int KNIGHT_TO = 6;
const int ROOK_FROM = 7;
const int ROOK_TO = 8;
const int CANNON_FROM = 9;
const int CANNON_TO = 10;
const int PAWN_FROM = 11;
const int PAWN_TO = 15;

const int STA_POS = 51;

inline int ROW ( const int p ) {
	return p >> 4;
}

inline int COL ( const int p ) {
	return p & 15;
}

inline int POS ( const int row, const int col ) {
	return (row << 4) | col;
}

inline int NEXTPOS ( const int x ) {
	return COL( x ) == 11 ? ( x == 203 ? 0 : x + 8 ) : x + 1;
}

inline int SRC ( const int mv ) {
	return mv >> 8;
}

inline int DST ( const int mv ) {
	return mv & 255;
}

inline int MOVE ( const int src, const int dst ) {
	return (src << 8) | dst;
}

inline bool IN_BOARD ( const int p ) {
	return ROW(p) >= 3 && ROW(p) <= 12 && COL(p) >= 3 && COL(p) <= 11;
}

inline bool IN_THIS_SIDE_BOARD ( const int x, const int p ) {
	return ((x&BLACK_TYPE) && ROW(p) <= 7) || ((x&RED_TYPE) && ROW(p) > 7);
}

inline bool IN_OPP_SIDE_BOARD ( const int x, const int p ) {
	return ((x&RED_TYPE) && ROW(p) <= 7) || ((x&BLACK_TYPE) && ROW(p) > 7);
}

inline int REVERSE_POS ( const int p ) {
	return POS ( 15 - ROW(p), 14 - COL(p) );
}

inline int COLOR_TYPE ( const int x ) {
	return x == 0 ? 0 : ( (x&RED_TYPE) ? RED_TYPE : BLACK_TYPE ) ;
}

inline int SIDE_VALUE ( const int player, const int val ) {
	return player == 0 ? val : - val;
}

inline int SIDE_TAG ( const int player ) {
	return 16 + ( player << 4 );
}

inline int OPP_SIDE_TAG ( const int player ) {
	return 32 - ( player << 4 );
}

struct PositionStruct {
	// 基本成员
	int player;	// 轮到哪方走，0表示红方，1表示黑方
	int square[256];	// 每个格子放的棋子，0表示没有棋子
	int piece[48];	// 每个棋子放的位置，0表示被吃
	int bitRow[16];	// 每行的位压
	int bitCol[16]; // 每列的位压
	std::pair<ULL, ULL> zobrist; // zobrist值，双哈希
	int nDistance; // 搜索深度，初始值为0
	bool check; // 将军态
	bool checked; // 被将军态
	bool chased; // 被捉态
	int vlRed; // 红方子力值
	int vlBlk; // 黑方子力值

	void Init ( const char *FenStr, const char *MoveStr, const int MoveNum ); // 初始化棋盘

	// 以下函数见hash.cpp
	void GenZobrist ( void ); // 生成局面的zobrist值
	void ModifyZobrist ( const int mv, const int sqSrc, const int sqDst ); // 修改局面的zobrist值

	// 以下函数见position.cpp
	bool IsDraw ( void ) const; // 判断和局
	void MakeMove ( const int mv ); // 走一步棋
	void UndoMakeMove ( void ); // 撤回走法
	void NullMove ( void ); // 走一步空着
	void UndoNullMove ( void ); // 撤回空着
	bool NullOkay ( void ) const; // 空着条件
	bool NullSafe ( void ) const; // 空着条件2

	// 以下函数见move.cpp
	bool Check ( void ) const; // 执棋方将军
	bool Checked ( void ) const; // 执棋方被将军
	bool KingFaceKing ( void ) const; // 将对将局面
	void GenCapMove ( int *move, int &nMoveNum ) const; // 生成吃子着法
	void GenNonCapMove ( int *move, int &nMoveNum ) const; // 生成非吃子着法
	void GenAllMove ( int *move, int &nMoveNum ) const; // 生成所有着法
	void DelMeaningLessMove ( int *move, int &nMoveNum ); // 去除无意义着法
	bool Protected ( const int sd, const int p, const int ban = 0 ) const; // 判断位置是否被保护
	bool Protected2 ( const int sd, const int src, const int dst ) const; // 判断位置是否被保护
	bool Chased ( void ) const; // 被捉

	// 以下函数见movesort.cpp
	int MvvLva ( const int src, const int dst ) const; // 吃子着法估分
	bool GoodCapMove ( const int mv ) const; // 判断是否是好的吃子着法

	// 以下函数见evaluate.cpp
	void PreEvaluate ( void ); // 预评估，给 vlRed 及 vlBlk 赋值
	int Material ( void ) const; // 子力平衡
	int AdvisorShape ( void ) const; // 特殊棋形
	int StringHold ( void ) const; // 牵制
	int RookMobility ( void ) const; // 车的灵活性
	int KnightTrap ( void ) const; // 马的阻碍
	int Evaluate ( void ) const; // 给局面打分
};

std::string MoveIntToStr ( const int mv );

#endif /* POSITION_H_ */
