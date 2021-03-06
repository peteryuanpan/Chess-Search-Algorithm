#include "move.h"
#include "position.h"
#include "search.h"
#include "debug.h"

int KING_HIT [256][10];	// 将在每个位置的 可达位置，遇0终止
int ADVISOR_HIT [256][10];
int BISHOP_HIT [256][10];
int BISHOP_PIN [256][10]; // 马在每个位置的 马脚，遇0终止
int KNIGHT_HIT [256][10];
int KNIGHT_PIN [256][10];
int PAWN_HIT [256][2][10];

int LOWER_P [1<<16][16][3]; 	// 2进制数 第i个1 低位方向的 第j+1个1 所在的相对位置
int HIGHER_P [1<<16][16][3]; 	// 2进制数 第i个1 高位方向的 第j+1个1 所在的相对位置

void InitMove ( void ) {
	int p = STA_POS;
	int t;

	// 1. HIT & PIN
	// 1.1 初始化
	for ( int i = 0; i < 256; i ++ ) {
		for ( int j = 0; j < 10; j ++ ) {
			 KING_HIT [i][j] = 0;
			 ADVISOR_HIT [i][j] = 0;
			 BISHOP_HIT [i][j] = 0;
			 BISHOP_PIN [i][j] = 0;
			 KNIGHT_HIT [i][j] = 0;
			 KNIGHT_PIN [i][j] = 0;
			 PAWN_HIT [i][0][j] = 0;
			 PAWN_HIT [i][1][j] = 0;
		}
	}
	// 1.2 求结果
	while ( p != 0 ) {
		// 生成将的HIT
		t = 0;
		for ( int i = 0; i < 4 ; i ++ ) {
			int x = p + KING_DIR[i];
			if ( LEGAL_POSITION[x] & POSITION_MASK[ KING_TYPE ] ) {
				KING_HIT[p][t] = x;
				t ++;
			}
		}

		// 生成士的HIT
		t = 0;
		for ( int i = 0; i < 4 ; i ++ ) {
			int x = p + ADVISOR_DIR[i];
			if ( LEGAL_POSITION[x] & POSITION_MASK[ ADVISOR_TYPE ] ) {
				ADVISOR_HIT[p][t] = x;
				t ++;
			}
		}

		// 生成象的HIT及PIN
		t = 0;
		for ( int i = 0; i < 4 ; i ++ ) {
			int x = p + BISHOP_DIR[i];
			if ( LEGAL_POSITION[x] & POSITION_MASK[ BISHOP_TYPE ] ) {
				BISHOP_HIT[p][t] = x;
				BISHOP_PIN[p][t] = p + BISHOP_PIN_DIR[i];
				t ++;
			}
		}

		// 生成马的HIT及PIN
		t = 0;
		for ( int i = 0; i < 8 ; i ++ ) {
			int x = p + KNIGHT_DIR[i];
			if ( LEGAL_POSITION[x] & POSITION_MASK[ KNIGHT_TYPE ] ) {
				KNIGHT_HIT[p][t] = x;
				KNIGHT_PIN[p][t] = p + KNIGHT_PIN_DIR[i];
				t ++;
			}
		}

		// 生成红兵的HIT
		t = 0;
		for ( int i = 0; i < 3; i ++ ) {
			int x = p + RED_PAWN_DIR[i];
			if ( LEGAL_POSITION[x] & POSITION_MASK[ RED_PAWN_TYPE ] ) {
				PAWN_HIT[p][0][t] = x;
				t ++;
			}
		}

		// 生成黑兵的HIT
		t = 0;
		for ( int i = 0; i < 3; i ++ ) {
			int x = p + BLACK_PAWN_DIR[i];
			if ( LEGAL_POSITION[x] & POSITION_MASK[ BLACK_PAWN_TYPE ] ) {
				PAWN_HIT[p][1][t] = x;
				t ++;
			}
		}

		p = NEXTPOS (p);
	}

	// 2. LOWER_P & HIGHER_P
	// 2.1 初始化
	for ( int i = 0; i < (1<<16); i ++ ) {
		for ( int j = 0; j < 16; j ++ ) {
			for ( int k = 0; k < 3; k ++ ) {
				LOWER_P [i][j][k] = 0;
				HIGHER_P [i][j][k] = 0;
			}
		}
	}
	// 2.2 求结果
	for ( int i = 0; i < (1<<16); i ++ ) {
		for ( int j = 0; j < 16; j ++ ) {
			if ( i & (1<<j) ) {
				int t;
				t = 0;
				for ( int k = j - 1; k >= 0; k -- ) {
					if ( i & (1<<k) ) {
						LOWER_P [i][j][t] = j - k;
						t ++;
					}
					if ( t > 2 ) {
						break;
					}
				}

				t = 0;
				for ( int k = j + 1; k < 16; k ++ ) {
					if ( i & (1<<k) ) {
						HIGHER_P [i][j][t] = k - j;
						t ++;
					}
					if ( t > 2 ) {
						break;
					}
				}
			}
		}
	}

}

// 执棋方将军
bool PositionStruct::Check ( void ) const {
	const int ST = SIDE_TAG (player);
	const int OppSideKingPos = piece [ KING_FROM + OPP_SIDE_TAG(player) ];
	int k, r, c, p;

	// 1. 判断马将军
	for ( int i = KNIGHT_FROM; i <= KNIGHT_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( KNIGHT_HIT[ piece[i+ST] ][k] != 0 ) {
				if ( KNIGHT_HIT[ piece[i+ST] ][k] == OppSideKingPos ) {
					int pin = KNIGHT_PIN[ piece[i+ST] ][k]; // 马脚
					if ( square[pin] == 0 ) {
						return true;
					}
				}
				k ++;
			}
		}
	}

	// 2. 判断车将军
	for ( int i = ROOK_FROM; i <= ROOK_TO; i ++ ) {
		if ( piece[i+ST] ) {
			r = ROW ( piece[i+ST] );
			c = COL ( piece[i+ST] );

			p = LOWER_P[ bitCol[c] ][r][0];
			p = piece[i+ST] - ( p << 4 );
			if ( p == OppSideKingPos ) {
				return true;
			}

			p = HIGHER_P[ bitCol[c] ][r][0];
			p = piece[i+ST] + ( p << 4 );
			if ( p == OppSideKingPos ) {
				return true;
			}

			p = LOWER_P [ bitRow[r] ][c][0];
			p = piece[i+ST] - p;
			if ( p == OppSideKingPos ) {
				return true;
			}

			p = HIGHER_P [ bitRow[r] ][c][0];
			p = piece[i+ST] + p;
			if ( p == OppSideKingPos ) {
				return true;
			}
		}
	}

	// 3. 判断炮将军
	for ( int i = CANNON_FROM; i <= CANNON_TO; i ++ ) {
		if ( piece[i+ST] ) {
			r = ROW ( piece[i+ST] );
			c = COL ( piece[i+ST] );

			p = LOWER_P[ bitCol[c] ][r][1];
			p = piece[i+ST] - ( p << 4 );
			if ( p == OppSideKingPos ) {
				return true;
			}

			p = HIGHER_P[ bitCol[c] ][r][1];
			p = piece[i+ST] + ( p << 4 );
			if ( p == OppSideKingPos ) {
				return true;
			}

			p = LOWER_P [ bitRow[r] ][c][1];
			p = piece[i+ST] - p;
			if ( p == OppSideKingPos ) {
				return true;
			}

			p = HIGHER_P [ bitRow[r] ][c][1];
			p = piece[i+ST] + p;
			if ( p == OppSideKingPos ) {
				return true;
			}
		}
	}

	// 4. 判断兵将军
	for ( int i = PAWN_FROM; i <= PAWN_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( PAWN_HIT[piece[i+ST]][player][k] != 0 ) {
				if ( PAWN_HIT[piece[i+ST]][player][k] == OppSideKingPos ) {
					return true;
				}
				k ++;
			}
		}
	}

	return false;
}

// 执棋方被将军
bool PositionStruct::Checked ( void ) const {
	const int ST = OPP_SIDE_TAG ( player );
	const int ThisSideKingPos = piece [ KING_FROM + SIDE_TAG(player) ];
	int k, r, c, p;

	// 1. 判断被马将军
	for ( int i = KNIGHT_FROM; i <= KNIGHT_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( KNIGHT_HIT[ piece[i+ST] ][k] != 0 ) {
				if ( KNIGHT_HIT[ piece[i+ST] ][k] == ThisSideKingPos ) {
					int pin = KNIGHT_PIN[ piece[i+ST] ][k];
					if ( square[pin] == 0 ) {
						return true;
					}
				}
				k ++;
			}
		}
	}

	// 2. 判断被车将军
	for ( int i = ROOK_FROM; i <= ROOK_TO; i ++ ) {
		if ( piece[i+ST] ) {
			r = ROW ( piece[i+ST] );
			c = COL ( piece[i+ST] );

			p = LOWER_P[ bitCol[c] ][r][0];
			p = piece[i+ST] - ( p << 4 );
			if ( p == ThisSideKingPos ) {
				return true;
			}

			p = HIGHER_P[ bitCol[c] ][r][0];
			p = piece[i+ST] + ( p << 4 );
			if ( p == ThisSideKingPos ) {
				return true;
			}

			p = LOWER_P [ bitRow[r] ][c][0];
			p = piece[i+ST] - p;
			if ( p == ThisSideKingPos ) {
				return true;
			}

			p = HIGHER_P [ bitRow[r] ][c][0];
			p = piece[i+ST] + p;
			if ( p == ThisSideKingPos ) {
				return true;
			}
		}
	}

	// 3. 判断被炮将军
	for ( int i = CANNON_FROM; i <= CANNON_TO; i ++ ) {
		if ( piece[i+ST] ) {
			r = ROW ( piece[i+ST] );
			c = COL ( piece[i+ST] );

			p = LOWER_P[ bitCol[c] ][r][1];
			p = piece[i+ST] - ( p << 4 );
			if ( p == ThisSideKingPos ) {
				return true;
			}

			p = HIGHER_P[ bitCol[c] ][r][1];
			p = piece[i+ST] + ( p << 4 );
			if ( p == ThisSideKingPos ) {
				return true;
			}

			p = LOWER_P [ bitRow[r] ][c][1];
			p = piece[i+ST] - p;
			if ( p == ThisSideKingPos ) {
				return true;
			}

			p = HIGHER_P [ bitRow[r] ][c][1];
			p = piece[i+ST] + p;
			if ( p == ThisSideKingPos ) {
				return true;
			}
		}
	}

	// 4. 判断被兵将军
	for ( int i = PAWN_FROM; i <= PAWN_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( PAWN_HIT[piece[i+ST]][1-player][k] != 0 ) {
				if ( PAWN_HIT[piece[i+ST]][1-player][k] == ThisSideKingPos ) {
					return true;
				}
				k ++;
			}
		}
	}

	return false;
}

// 将对将局面
bool PositionStruct::KingFaceKing ( void ) const {
	int RED_KING_POS = piece [ RED_TYPE + KING_FROM ];
	int BLACK_KING_POS = piece [ BLACK_TYPE + KING_FROM ];

	if ( RED_KING_POS == 0 || BLACK_KING_POS == 0 ) { // 如果将万一不存在，则无解
		return false;
	}
	if ( COL(RED_KING_POS) != COL(BLACK_KING_POS) ) { // 两将必须同列
		return false;
	}

	// 判断两将之间是否有棋子挡住
	int c = COL ( RED_KING_POS );
	int r = ROW ( RED_KING_POS );
	int p;

	p = LOWER_P[ bitCol[c] ][r][0];
	p = RED_KING_POS - ( p << 4 );
	if ( p == BLACK_KING_POS ) {
		return true;
	}

	p = HIGHER_P[ bitCol[c] ][r][0];
	p = RED_KING_POS + ( p << 4 );
	if ( p == BLACK_KING_POS ) {
		return true;
	}

	return false;
}

// 生成吃子着法
void PositionStruct::GenCapMove ( int *move, int &nMoveNum ) const {
	int ST = SIDE_TAG ( player );
	int k, r, c, p;

	// 1. 生成将的吃子着法
	for ( int i = KING_FROM; i <= KING_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( KING_HIT[ piece[i+ST] ][k] != 0 ) {
				int hit = KING_HIT[piece[i+ST]][k];
				if ( square[hit] != 0 && COLOR_TYPE(i+ST) != COLOR_TYPE(square[hit]) ) {
					move[nMoveNum++] = MOVE ( piece[i+ST], hit );
				}
				k ++;
			}
		}
	}

	// 2. 生成士的吃子着法
	for ( int i = ADVISOR_FROM; i <= ADVISOR_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( ADVISOR_HIT[ piece[i+ST] ][k] != 0 ) {
				int hit = ADVISOR_HIT[piece[i+ST]][k];
				if ( square[hit] != 0 && COLOR_TYPE(i+ST) != COLOR_TYPE(square[hit]) ) {
					move[nMoveNum++] = MOVE ( piece[i+ST], hit );
				}
				k ++;
			}
		}
	}

	// 3. 生成象的吃子着法
	for ( int i = BISHOP_FROM; i <= BISHOP_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( BISHOP_HIT[ piece[i+ST] ][k] != 0 ) {
				int hit = BISHOP_HIT[piece[i+ST]][k];
				if ( square[hit] != 0 && COLOR_TYPE(i+ST) != COLOR_TYPE(square[hit]) ) {
					int pin = BISHOP_PIN[piece[i+ST]][k];
					if ( square[pin] == 0 ) { // 象脚
						move[nMoveNum++] = MOVE ( piece[i+ST], hit );
					}
				}
				k ++;
			}
		}
	}

	// 4. 生成马的吃子着法
	for ( int i = KNIGHT_FROM; i <= KNIGHT_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( KNIGHT_HIT[ piece[i+ST] ][k] != 0 ) {
				int hit = KNIGHT_HIT[piece[i+ST]][k];
				if ( square[hit] != 0 && COLOR_TYPE(i+ST) != COLOR_TYPE(square[hit]) ) {
					int pin = KNIGHT_PIN[piece[i+ST]][k];
					if ( square[pin] == 0 ) { // 马脚
						move[nMoveNum++] = MOVE ( piece[i+ST], hit );
					}
				}
				k ++;
			}
		}
	}

	// 5. 生成车的吃子着法
	for ( int i = ROOK_FROM; i <= ROOK_TO; i ++ ) {
		if ( piece[i+ST] ) {
			r = ROW ( piece[i+ST] );
			c = COL ( piece[i+ST] );

			p = LOWER_P[ bitCol[c] ][r][0];
			p = piece[i+ST] - ( p << 4 );
			if ( COLOR_TYPE(i+ST) != COLOR_TYPE(square[p]) ) {
				move[nMoveNum++] = MOVE ( piece[i+ST], p );
			}

			p = HIGHER_P[ bitCol[c] ][r][0];
			p = piece[i+ST] + ( p << 4 );
			if ( COLOR_TYPE(i+ST) != COLOR_TYPE(square[p]) ) {
				move[nMoveNum++] = MOVE ( piece[i+ST], p );
			}

			p = LOWER_P [ bitRow[r] ][c][0];
			p = piece[i+ST] - p;
			if ( COLOR_TYPE(i+ST) != COLOR_TYPE(square[p]) ) {
				move[nMoveNum++] = MOVE ( piece[i+ST], p );
			}

			p = HIGHER_P [ bitRow[r] ][c][0];
			p = piece[i+ST] + p;
			if ( COLOR_TYPE(i+ST) != COLOR_TYPE(square[p]) ) {
				move[nMoveNum++] = MOVE ( piece[i+ST], p );
			}
		}
	}

	// 6. 生成炮的吃子着法
	for ( int i = CANNON_FROM; i <= CANNON_TO; i ++ ) {
		if ( piece[i+ST] ) {
			r = ROW ( piece[i+ST] );
			c = COL ( piece[i+ST] );

			p = LOWER_P[ bitCol[c] ][r][1];
			p = piece[i+ST] - ( p << 4 );
			if ( COLOR_TYPE(i+ST) != COLOR_TYPE(square[p]) ) {
				move[nMoveNum++] = MOVE ( piece[i+ST], p );
			}

			p = HIGHER_P[ bitCol[c] ][r][1];
			p = piece[i+ST] + ( p << 4 );
			if ( COLOR_TYPE(i+ST) != COLOR_TYPE(square[p]) ) {
				move[nMoveNum++] = MOVE ( piece[i+ST], p );
			}

			p = LOWER_P [ bitRow[r] ][c][1];
			p = piece[i+ST] - p;
			if ( COLOR_TYPE(i+ST) != COLOR_TYPE(square[p]) ) {
				move[nMoveNum++] = MOVE ( piece[i+ST], p );
			}

			p = HIGHER_P [ bitRow[r] ][c][1];
			p = piece[i+ST] + p;
			if ( COLOR_TYPE(i+ST) != COLOR_TYPE(square[p]) ) {
				move[nMoveNum++] = MOVE ( piece[i+ST], p );
			}
		}
	}

	// 7. 生成兵的吃子着法
	for ( int i = PAWN_FROM; i <= PAWN_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( PAWN_HIT[piece[i+ST]][player][k] != 0 ) {
				int hit = PAWN_HIT[piece[i+ST]][player][k];
				if ( square[hit] != 0 && COLOR_TYPE(i+ST) != COLOR_TYPE(square[hit]) ) {
					move[nMoveNum++] = MOVE ( piece[i+ST], hit );
				}
				k ++;
			}
		}
	}
}

// 生成非吃子着法
void PositionStruct::GenNonCapMove ( int *move, int &nMoveNum ) const {
	int ST = SIDE_TAG ( player );
	int k;

	// 1. 生成将的非吃子着法
	for ( int i = KING_FROM; i <= KING_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( KING_HIT[ piece[i+ST] ][k] != 0 ) {
				int hit = KING_HIT[piece[i+ST]][k];
				if ( square[hit] == 0 ) {
					move[nMoveNum++] = MOVE ( piece[i+ST], hit );
				}
				k ++;
			}
		}
	}

	// 2. 生成士的非吃子着法
	for ( int i = ADVISOR_FROM; i <= ADVISOR_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( ADVISOR_HIT[ piece[i+ST] ][k] != 0 ) {
				int hit = ADVISOR_HIT[piece[i+ST]][k];
				if ( square[hit] == 0 ) {
					move[nMoveNum++] = MOVE ( piece[i+ST], hit );
				}
				k ++;
			}
		}
	}

	// 3. 生成象的非吃子着法
	for ( int i = BISHOP_FROM; i <= BISHOP_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( BISHOP_HIT[ piece[i+ST] ][k] != 0 ) {
				int hit = BISHOP_HIT[piece[i+ST]][k];
				if ( square[hit] == 0 ) {
					int pin = BISHOP_PIN[piece[i+ST]][k];
					if ( square[pin] == 0 ) { // 象脚
						move[nMoveNum++] = MOVE ( piece[i+ST], hit );
					}
				}
				k ++;
			}
		}
	}

	// 4. 生成马的非吃子着法
	for ( int i = KNIGHT_FROM; i <= KNIGHT_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( KNIGHT_HIT[ piece[i+ST] ][k] != 0 ) {
				int hit = KNIGHT_HIT[piece[i+ST]][k];
				if ( square[hit] == 0 ) {
					int pin = KNIGHT_PIN[piece[i+ST]][k];
					if ( square[pin] == 0 ) { // 马脚
						move[nMoveNum++] = MOVE ( piece[i+ST], hit );
					}
				}
				k ++;
			}
		}
	}

	// 5. 生成车的非吃子着法
	for ( int i = ROOK_FROM; i <= ROOK_TO; i ++ ) {
		if ( piece[i+ST] ) {
			for ( int j = 0; j < 4; j ++ ) {
				int hit = piece[i+ST];
				int meetTime = 0;
				while ( true ) {
					hit = hit + DIR[j];
					if ( ! IN_BOARD(hit) ) {
						break;
					}
					if ( square[hit] != 0 ) {
						meetTime ++;
					}
					if ( meetTime > 0 ) {
						break;
					}
					move[nMoveNum++] = MOVE ( piece[i+ST], hit );
				}
			}
		}
	}

	// 6. 生成炮的非吃子着法
	for ( int i = CANNON_FROM; i <= CANNON_TO; i ++ ) {
		if ( piece[i+ST] ) {
			for ( int j = 0; j < 4; j ++ ) {
				int hit = piece[i+ST];
				int meetTime = 0;
				while ( true ) {
					hit = hit + DIR[j];
					if ( ! IN_BOARD(hit) ) {
						break;
					}
					if ( square[hit] != 0 ) {
						meetTime ++;
					}
					if ( meetTime > 0 ) {
						break;
					}
					move[nMoveNum++] = MOVE ( piece[i+ST], hit );
				}
			}
		}
	}

	// 7. 生成兵的非吃子着法
	for ( int i = PAWN_FROM; i <= PAWN_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( PAWN_HIT[piece[i+ST]][player][k] != 0 ) {
				int hit = PAWN_HIT[piece[i+ST]][player][k];
				if ( square[hit] == 0 ) {
					move[nMoveNum++] = MOVE ( piece[i+ST], hit );
				}
				k ++;
			}
		}
	}
}

// 生成所有着法
void PositionStruct::GenAllMove ( int *move, int &nMoveNum ) const {
	GenCapMove ( move, nMoveNum );
	GenNonCapMove ( move, nMoveNum );
}

// 去除无意义着法，无意义着法包括：1.被将军的着法  2.将对将的着法
void PositionStruct::DelMeaningLessMove ( int *move, int &nMoveNum ) {
	for ( int i = 0; i < nMoveNum; i ++ ) {
		MakeMove ( move[i] );
		if ( check || KingFaceKing() ) {
			SWAP ( move[i], move[nMoveNum-1] );
			nMoveNum --;
			i --;
		}
		UndoMakeMove ();
	}
}

// 判断位置是否被保护
bool PositionStruct::Protected ( const int sd, const int dst, const int ban ) const {
	int k, p, r, c, t;
	const int ST = SIDE_TAG ( sd );

	// 1. 被将保护
	for ( int i = KING_FROM; i <= KING_TO; i ++ ) {
		if ( piece[i+ST] && piece[i+ST] != ban ) {
			k = 0;
			while ( KING_HIT[ piece[i+ST] ][k] != 0 ) {
				if ( KING_HIT[ piece[i+ST] ][k] == dst ) {
					return true;
				}
				k ++;
			}
		}
	}

	// 2. 被士保护
	for ( int i = ADVISOR_FROM; i <= ADVISOR_TO; i ++ ) {
		if ( piece[i+ST] && piece[i+ST] != ban ) {
			k = 0;
			while ( ADVISOR_HIT[ piece[i+ST] ][k] != 0 ) {
				if ( ADVISOR_HIT[ piece[i+ST] ][k] == dst ) {
					return true;
				}
				k ++;
			}
		}
	}

	// 3. 被象保护
	for ( int i = BISHOP_FROM; i <= BISHOP_TO; i ++ ) {
		if ( piece[i+ST] && piece[i+ST] != ban ) {
			k = 0;
			while ( BISHOP_HIT[ piece[i+ST] ][k] != 0 ) {
				if ( BISHOP_HIT[ piece[i+ST] ][k] == dst ) {
					int pin = BISHOP_PIN[ piece[i+ST] ][k];
					if ( square[pin] == 0 ) {
						return true;
					}
				}
				k ++;
			}
		}
	}

	// 4. 被马保护
	for ( int i = KNIGHT_FROM; i <= KNIGHT_TO; i ++ ) {
		if ( piece[i+ST] && piece[i+ST] != ban ) {
			k = 0;
			while ( KNIGHT_HIT[ piece[i+ST] ][k] != 0 ) {
				if ( KNIGHT_HIT[ piece[i+ST] ][k] == dst ) {
					int pin = KNIGHT_PIN[ piece[i+ST] ][k];
					if ( square[pin] == 0 ) {
						return true;
					}
				}
				k ++;
			}
		}
	}

	// 5. 被车保护
	for ( int i = ROOK_FROM; i <= ROOK_TO; i ++ ) {
		p = piece[i+ST];
		if ( p != ban ) {
			r = ROW (p);
			c = COL (p);

			t = LOWER_P[bitCol[c]][r][0];
			if ( t != 0 && p - (t<<4) == dst ) {
				return true;
			}

			t = HIGHER_P[bitCol[c]][r][0];
			if ( t != 0 && p + (t<<4) == dst ) {
				return true;
			}

			t = LOWER_P[bitRow[r]][c][0];
			if ( t != 0 && p - t == dst ) {
				return true;
			}

			t = HIGHER_P[bitRow[r]][c][0];
			if ( t != 0 && p + t == dst ) {
				return true;
			}
		}
	}

	// 6. 被炮保护
	for ( int i = CANNON_FROM; i <= CANNON_TO; i ++ ) {
		p = piece[i+ST];
		if ( p != ban ) {
			r = ROW (p);
			c = COL (p);

			t = LOWER_P[bitCol[c]][r][1];
			if ( t != 0 && p - (t<<4) == dst ) {
				return true;
			}

			t = HIGHER_P[bitCol[c]][r][1];
			if ( t != 0 && p + (t<<4) == dst ) {
				return true;
			}

			t = LOWER_P[bitRow[r]][c][1];
			if ( t != 0 && p - t == dst ) {
				return true;
			}

			t = HIGHER_P[bitRow[r]][c][1];
			if ( t != 0 && p + t == dst ) {
				return true;
			}
		}
	}

	// 7. 被兵保护
	for ( int i = PAWN_FROM; i <= PAWN_TO; i ++ ) {
		if ( piece[i+ST] && piece[i+ST] != ban ) {
			k = 0;
			while ( PAWN_HIT[ piece[i+ST] ][sd][k] != 0 ) {
				if ( PAWN_HIT[ piece[i+ST] ][sd][k] == dst ) {
					return true;
				}
				k ++;
			}
		}
	}

	return false;
}

// 判断位置是否被保护
bool PositionStruct::Protected2 ( const int sd, const int src, const int dst ) const {
	int k;
	const int ST = SIDE_TAG ( sd );

	// 1. 被将保护
	for ( int i = KING_FROM; i <= KING_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( KING_HIT[ piece[i+ST] ][k] != 0 ) {
				if ( KING_HIT[ piece[i+ST] ][k] == dst ) {
					return true;
				}
				k ++;
			}
		}
	}

	// 2. 被士保护
	for ( int i = ADVISOR_FROM; i <= ADVISOR_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( ADVISOR_HIT[ piece[i+ST] ][k] != 0 ) {
				if ( ADVISOR_HIT[ piece[i+ST] ][k] == dst ) {
					return true;
				}
				k ++;
			}
		}
	}

	// 3. 被象保护
	for ( int i = BISHOP_FROM; i <= BISHOP_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( BISHOP_HIT[ piece[i+ST] ][k] != 0 ) {
				if ( BISHOP_HIT[ piece[i+ST] ][k] == dst ) {
					int pin = BISHOP_PIN[ piece[i+ST] ][k];
					if ( pin == src || square[pin] == 0 ) {
						return true;
					}
				}
				k ++;
			}
		}
	}

	// 4. 被马保护
	for ( int i = KNIGHT_FROM; i <= KNIGHT_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( KNIGHT_HIT[ piece[i+ST] ][k] != 0 ) {
				if ( KNIGHT_HIT[ piece[i+ST] ][k] == dst ) {
					int pin = KNIGHT_PIN[ piece[i+ST] ][k];
					if ( pin == src || square[pin] == 0 ) {
						return true;
					}
				}
				k ++;
			}
		}
	}

	// 5. 被车保护
	for ( int i = ROOK_FROM; i <= ROOK_TO; i ++ ) {
		const int nLimit = 0;
		if ( piece[i+ST] ) {
			for ( int j = 0; j < 4; j ++ ) {
				int p = piece[i+ST];
				int n = 0;
				while ( true ) {
					p = p + DIR[j];
					if ( ! IN_BOARD(p) ) {
						break;
					}
					if ( p == dst && n == nLimit ) {
						return true;
					}
					if ( p == dst || (p != src && square[p] != 0) ) {
						n ++;
					}
					if ( n > nLimit ) {
						break;
					}
				}
			}
		}
	}

	// 6. 被炮保护
	for ( int i = CANNON_FROM; i <= CANNON_TO; i ++ ) {
		const int nLimit = 1;
		if ( piece[i+ST] ) {
			for ( int j = 0; j < 4; j ++ ) {
				int p = piece[i+ST];
				int n = 0;
				while ( true ) {
					p = p + DIR[j];
					if ( ! IN_BOARD(p) ) {
						break;
					}
					if ( p == dst && n == nLimit ) {
						return true;
					}
					if ( p == dst || (p != src && square[p] != 0) ) {
						n ++;
					}
					if ( n > nLimit ) {
						break;
					}
				}
			}
		}
	}

	// 7. 被兵保护
	for ( int i = PAWN_FROM; i <= PAWN_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( PAWN_HIT[ piece[i+ST] ][sd][k] != 0 ) {
				if ( PAWN_HIT[ piece[i+ST] ][sd][k] == dst ) {
					return true;
				}
				k ++;
			}
		}
	}

	return false;
}

// 被捉
bool PositionStruct::Chased ( void ) const {
	const int ST = SIDE_TAG ( 1 - player );
	int k, r, c, p;

	// 判断象捉子
	for ( int i = BISHOP_FROM; i <= BISHOP_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( BISHOP_HIT[piece[i+ST]][k] != 0 ) {
				int hit = BISHOP_HIT[piece[i+ST]][k];
				if ( square[hit] != 0 && COLOR_TYPE(square[hit]) != COLOR_TYPE(i+ST) ) {
					int pin = BISHOP_PIN[piece[i+ST]][k];
					if ( square[pin] == 0 ) {
						// 象捉马
						if ( PIECE_TYPE[square[hit]] == KNIGHT_TYPE ) {
							return true;
						}
						// 象捉车
						else if ( PIECE_TYPE[square[hit]] == ROOK_TYPE ) {
							return true;
						}
						// 象捉炮
						else if ( PIECE_TYPE[square[hit]] == CANNON_TYPE ) {
							return true;
						}
						// 象捉兵
						else if ( PIECE_TYPE[square[hit]] == PAWN_TYPE ) {
							if ( !Protected2(player, piece[i+ST], hit) ) {
								if ( IN_OPP_SIDE_BOARD(square[hit], hit) ) {
									return true;
								}
							}
						}
					}
				}
				k ++;
			}
		}
	}

	// 判断马捉子
	for ( int i = KNIGHT_FROM; i <= KNIGHT_TO; i ++ ) {
		if ( piece[i+ST] ) {
			k = 0;
			while ( KNIGHT_HIT[piece[i+ST]][k] != 0 ) {
				int hit = KNIGHT_HIT[piece[i+ST]][k];
				if ( square[hit] != 0 && COLOR_TYPE(square[hit]) != COLOR_TYPE(i+ST) ) {
					int pin = KNIGHT_PIN[piece[i+ST]][k];
					if ( square[pin] == 0 ) {
						// 马捉马
						if ( PIECE_TYPE[square[hit]] == KNIGHT_TYPE ) {
							if ( !Protected2(player, piece[i+ST], hit) ) {
								return true;
							}
						}
						// 马捉车
						else if ( PIECE_TYPE[square[hit]] == ROOK_TYPE ) {
							return true;
						}
						// 马捉炮
						else if ( PIECE_TYPE[square[hit]] == CANNON_TYPE ) {
							if ( !Protected2(player, piece[i+ST], hit) ) {
								return true;
							}
						}
						// 马捉兵
						else if ( PIECE_TYPE[square[hit]] == PAWN_TYPE ) {
							if ( !Protected2(player, piece[i+ST], hit) ) {
								if ( IN_OPP_SIDE_BOARD(square[hit], hit) ) {
									return true;
								}
							}
						}
					}
				}
				k ++;
			}
		}
	}

	// 判断车捉子
	int RookHit[10];
	int nRHit = 0;
	for ( int i = ROOK_FROM; i <= ROOK_TO; i ++ ) {
		if ( piece[i+ST] ) {
			r = ROW ( piece[i+ST] );
			c = COL ( piece[i+ST] );

			p = LOWER_P[ bitCol[c] ][r][0];
			p = piece[i+ST] - ( p << 4 );
			RookHit[nRHit++] = p;

			p = HIGHER_P[ bitCol[c] ][r][0];
			p = piece[i+ST] + ( p << 4 );
			RookHit[nRHit++] = p;

			p = LOWER_P [ bitRow[r] ][c][0];
			p = piece[i+ST] - p;
			RookHit[nRHit++] = p;

			p = HIGHER_P [ bitRow[r] ][c][0];
			p = piece[i+ST] + p;
			RookHit[nRHit++] = p;

			for ( int j = 0; j < nRHit; j ++ ) {
				p = RookHit[j];
				if ( square[p] != 0 && piece[i+ST] != p ) {
					if ( COLOR_TYPE(square[p]) != COLOR_TYPE(i+ST) ) {
						// 车捉马
						if ( PIECE_TYPE[square[p]] == KNIGHT_TYPE ) {
							if ( !Protected2(player, piece[i+ST], p) ) {
								return true;
							}
						}
						// 车捉车
						else if ( PIECE_TYPE[square[p]] == ROOK_TYPE ) {
							if ( !Protected2(player, piece[i+ST], p) ) {
								return true;
							}
						}
						// 车捉炮
						else if ( PIECE_TYPE[square[p]] == CANNON_TYPE ) {
							if ( !Protected2(player, piece[i+ST], p) ) {
								return true;
							}
						}
						// 车捉兵
						else if ( PIECE_TYPE[square[p]] == PAWN_TYPE ) {
							if ( !Protected2(player, piece[i+ST], p) ) {
								if ( IN_OPP_SIDE_BOARD(square[p], p) ) {
									return true;
								}
							}
						}
					}
				}
			}
		}
	}

	// 判断炮捉子
	int CannonHit[10];
	int nCHit = 0;
	for ( int i = CANNON_FROM; i <= CANNON_TO; i ++ ) {
		if ( piece[i+ST] ) {
			r = ROW ( piece[i+ST] );
			c = COL ( piece[i+ST] );

			p = LOWER_P[ bitCol[c] ][r][1];
			p = piece[i+ST] - ( p << 4 );
			CannonHit[nCHit++] = p;

			p = HIGHER_P[ bitCol[c] ][r][1];
			p = piece[i+ST] + ( p << 4 );
			CannonHit[nCHit++] = p;

			p = LOWER_P [ bitRow[r] ][c][1];
			p = piece[i+ST] - p;
			CannonHit[nCHit++] = p;

			p = HIGHER_P [ bitRow[r] ][c][1];
			p = piece[i+ST] + p;
			CannonHit[nCHit++] = p;

			for ( int j = 0; j < nCHit; j ++ ) {
				p = CannonHit[j];
				if ( square[p] != 0 && piece[i+ST] != p ) {
					if ( COLOR_TYPE(square[p]) != COLOR_TYPE(i+ST) ) {
						// 炮捉马
						if ( PIECE_TYPE[square[p]] == KNIGHT_TYPE ) {
							if ( !Protected2(player, piece[i+ST], p) ) {
								return true;
							}
						}
						// 炮捉车
						else if ( PIECE_TYPE[square[p]] == ROOK_TYPE ) {
							return true;
						}
						// 炮捉炮
						else if ( PIECE_TYPE[square[p]] == CANNON_TYPE ) {
							if ( !Protected2(player, piece[i+ST], p) ) {
								return true;
							}
						}
						// 炮捉兵
						else if ( PIECE_TYPE[square[p]] == PAWN_TYPE ) {
							if ( !Protected2(player, piece[i+ST], p) ) {
								if ( IN_OPP_SIDE_BOARD(square[p], p) ) {
									return true;
								}
							}
						}
					}
				}
			}
		}
	}

	return false;
}
