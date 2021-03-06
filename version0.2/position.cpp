#include "position.h"
#include "search.h"
#include "rollback.h"
#include "evaluate.h"
#include "hash.h"

/* 以下是初始化棋盘部分 */
const int DDSQ1 [] = { 0, 0, 0, +6, +4, +2, 0, -2, -4, -6, -8, -10, -12 };
std::string SQIntToStr ( const int sq ) {
	int r = ROW (sq);
	int c = COL (sq);
	std::string s = "";
	s += (char) (c - 3 + 'a');
	s += (char) (r + DDSQ1[r] + '0');
	return s;
}

const int DDSQ2 [] = {+12, +10, +8, +6, +4, +2, 0, -2, -4, -6};
int SQStrToInt ( const std::string sq ) {
	int c = (int) (sq[0] - 'a' + 3);
	int t = (int) (sq[1] - '0');
	int r = t + DDSQ2[t];
	return POS ( r, c );
}

std::string MoveIntToStr ( const int mv ) {
	if ( mv == 0 ) {
		return "   0";
	}
	std::string s = "";
	s += SQIntToStr ( SRC(mv) );
	s += SQIntToStr ( DST(mv) );
	return s;
}

int MoveStrToInt ( const std::string mv ) {
	std::string s = "";
	s += mv[0]; s += mv[1];
	int src = SQStrToInt ( s );
	s = ""; s += mv[2]; s += mv[3];
	int dst = SQStrToInt ( s );
	return MOVE ( src, dst );
}

int TypeFrom ( const char x ) {
	const char c = islower ( x ) ? x - 'a' + 'A' : x;
	switch ( c ) {
		case 'K':
			return KING_FROM;
		case 'A':
			return ADVISOR_FROM;
		case 'B':
			return BISHOP_FROM;
		case 'N':
			return KNIGHT_FROM;
		case 'R':
			return ROOK_FROM;
		case 'C':
			return CANNON_FROM;
		case 'P':
			return PAWN_FROM;
		default:
			return 0;
	}
}

void PositionStruct::Init ( const char *FenStr, const char *MoveStr, const int MoveNum ) {
	int p;

    // 1. 确定走子方
    char c = ' ';
    int i = 0;
    while ( FenStr[i] != ' ' ) {
    	i ++;
    }
    while ( FenStr[i] != 'w' && FenStr[i] != 'b' ) {
    	i ++;
    }
    c = FenStr[i];
    assert ( c != 'w' || c != 'b' );
    if ( MoveNum & 1 ) {
    	c = ( c == 'w' ) ? 'b' : 'w';
    }
    player = ( c == 'w' ) ? 0 : 1; // w, 0 表示红方；b，1 表示黑方

    // 2. 初始化square数组
    memset ( square, 0, sizeof square );
	// 2.1 通过Fen串得到square数组
	p = STA_POS;
	int status[48];
	memset (status, 0, sizeof status);
    for( int i = 0; FenStr[i] != ' '; i ++ ){
        if( FenStr[i] == '/' ) {
        	continue;
        }
        else if ( isdigit(FenStr[i]) ) {
        	int n = FenStr[i] - '0';
        	for ( int j = 0; j < n; j ++ ) {
        		square[p] = 0;
        		p = NEXTPOS ( p );
        	}
        }
        else if ( isalpha(FenStr[i]) ) {
        	int x = TypeFrom ( FenStr[i] ) + ( islower(FenStr[i]) ? BLACK_TYPE : RED_TYPE );
        	while ( status[x] == 1 ) {
        		x ++;
        	}
        	status[x] = 1;
        	square[p] = x;
        	p = NEXTPOS ( p );
        }
    }
    // 2.2 再通过Move串得到square数组
    int j = 0;
    for ( int T = 0; T < MoveNum; T ++ ) {
    	std::string s = "";
    	for ( int k = j; k < j + 4; k ++ ) {
    		s += MoveStr[k];
    	}
    	int mv = MoveStrToInt ( s );
    	square[ DST(mv) ] = square[ SRC(mv) ];
    	square[ SRC(mv) ] = 0;
    	j += 5;
    }

    // 3. 初始化piece数组
	memset ( piece, 0, sizeof piece );
    p = STA_POS;
    while ( p != 0 ) {
    	if ( square[p] ) {
    		piece[ square[p] ] = p;
    	}
    	p = NEXTPOS (p);
    }

    // 4. 初始化行位压、列位压数组
	memset ( bitRow, 0, sizeof bitRow );
	memset ( bitCol, 0, sizeof bitCol );
    p = STA_POS;
    while ( p != 0 ) {
    	if ( square[p] != 0 ) {
        	bitRow[ ROW(p) ] |= (1<<COL(p));
        	bitCol[ COL(p) ] |= (1<<ROW(p));
    	}
    	p = NEXTPOS (p);
    }

    // 5. 初始化zobrist
    GenZobrist ();

    // 6. 初始化nDistance
    nDistance = 0;

   	// 7. 初始化check，checked，chased
    check = Check ();
    checked = Checked ();
    chased = Chased ();

    // 8. 初始化vlRed及vlBlk
    PreEvaluate ();
}
/* 以上是初始化棋盘部分 */


// 走一步棋
void PositionStruct::MakeMove ( const int mv ) {
	const int src = SRC ( mv );
	const int dst = DST ( mv );
	const int sqSrc = square[ src ];
	const int sqDst = square[ dst ];
	const int thisSide = player;
	const int OppSide = 1 - thisSide;

	// 1. 记录于回滚着法表
	int & nRollNum = roll.nRollNum;
	roll.move[ nRollNum ] = mv;
	roll.dstPiece [ nRollNum ] = square[ DST(mv) ];
	roll.check[ nRollNum ] = check;
	roll.checked[ nRollNum ] = checked;
	roll.chased[ nRollNum ] = chased;
	roll.zobrist[ nRollNum ] = zobrist;
	nRollNum ++;
	const int t = zobrist.first & rbHashMask;
	RollBackHash[t] ++;

	// 2. 修改走子方
	player = 1 - player;

	// 3. 修改square数组
	square[ dst ] = square[ src ];
	square[ src ] = 0;

	// 4. 修改piece数组
	if ( sqDst != 0 ) {
		piece[ sqDst ] = 0;
	}
	piece[ sqSrc ] = dst;

	// 5. 修改位行、位列
	if ( sqDst == 0 ) {
		bitRow[ ROW(dst) ] |= (1<<COL(dst));
		bitCol[ COL(dst) ] |= (1<<ROW(dst));
	}
	bitRow[ ROW(src) ] -= (1<<COL(src));
	bitCol[ COL(src) ] -= (1<<ROW(src));

	// 6. 修改zobrist值
	ModifyZobrist ( mv, sqSrc, sqDst );

	// 7. 修改搜索深度
	nDistance ++;

	// 8. 修改check，checked及chased
	check = Check ();
	checked = Checked ();
	chased = Chased ();

	// 9. 修改vlRed及vlBlk
	if ( sqDst != 0 ) {
		if ( thisSide == 0 ) {
			vlBlk -= vlPiece[OppSide][PIECE_TYPE[sqDst]][dst];
		}
		else {
			vlRed -= vlPiece[OppSide][PIECE_TYPE[sqDst]][dst];
		}
	}
	if ( thisSide == 0 ) {
		vlRed += vlPiece[thisSide][PIECE_TYPE[sqSrc]][dst] - vlPiece[thisSide][PIECE_TYPE[sqSrc]][src];;
	}
	else {
		vlBlk += vlPiece[thisSide][PIECE_TYPE[sqSrc]][dst] - vlPiece[thisSide][PIECE_TYPE[sqSrc]][src];;
	}
}

// 撤回走法
void PositionStruct::UndoMakeMove ( void ) {
	const int src = SRC ( roll.LastMove() );
	const int dst = DST ( roll.LastMove() );
	const int sqDst = square[ dst ];
	const int lastDstPiece = roll.LastDstPiece ();
	const int thisSide = 1 - player;
	const int OppSide = 1 - thisSide;

	// 1. 修改走子方
	player = 1 - player;

	// 2. 修改square数组
	square[ src ] = square[ dst ];
	square[ dst ] = roll.LastDstPiece ();

	// 3. 修改piece数组
	piece[ sqDst ] = src;
	if ( lastDstPiece != 0 ) {
		piece[ lastDstPiece ] = dst;
	}

	// 4. 修改位行、位列
	bitRow[ ROW(src) ] |= (1<<COL(src));
	bitCol[ COL(src) ] |= (1<<ROW(src));
	if ( lastDstPiece == 0 ) {
		bitRow[ ROW(dst) ] -= (1<<COL(dst));
		bitCol[ COL(dst) ] -= (1<<ROW(dst));
	}

	// 5. 修改zobrist值
	zobrist = roll.LastZobrist ();

	// 6. 修改搜索深度
	nDistance --;

	// 7. 修改check与checked
	check = roll.LastCheck ();
	checked = roll.LastChecked ();
	chased = roll.LastChased ();

	// 8. 修改vlRed及vlBlk
	if ( lastDstPiece != 0 ) {
		if ( thisSide == 0 ) {
			vlBlk += vlPiece[OppSide][PIECE_TYPE[lastDstPiece]][dst];
		}
		else {
			vlRed += vlPiece[OppSide][PIECE_TYPE[lastDstPiece]][dst];
		}
	}
	if ( thisSide == 0 ) {
		vlRed += vlPiece[thisSide][PIECE_TYPE[sqDst]][src] - vlPiece[thisSide][PIECE_TYPE[sqDst]][dst];
	}
	else {
		vlBlk += vlPiece[thisSide][PIECE_TYPE[sqDst]][src] - vlPiece[thisSide][PIECE_TYPE[sqDst]][dst];
	}

	// 9. 修改回滚着法表
	const int t = roll.LastZobrist().first & rbHashMask;
	RollBackHash[t] --;
	roll.nRollNum --;
}

// 走一步空着
void PositionStruct::NullMove ( void ) {
	// 1. 记录回滚着法表
	int & nRollNum = roll.nRollNum;
	roll.move[ nRollNum ] = 0;
	roll.dstPiece[ nRollNum ] = 0;
	roll.check[ nRollNum ] = check;
	roll.checked[ nRollNum ] = checked;
	roll.chased[ nRollNum ] = chased;
	roll.zobrist[ nRollNum ] = zobrist;
	nRollNum ++;
	const int t = zobrist.first & rbHashMask;
	RollBackHash[t] ++;

	// 2. 修改走子方
	player = 1 - player;

	// 3. 修改zobrist
	zobrist.first &= ZobristPlayer_1;
	zobrist.second &= ZobristPlayer_2;

	// 4. 修改nDistance
	nDistance ++;

	// 5. 修改check，checked及chased
	SWAP ( check, checked );
	chased = Chased ();
}

// 撤回空着
void PositionStruct::UndoNullMove ( void ) {
	// 1. 修改走子方
	player = 1 - player;

	// 2. 修改zobrist
	zobrist = roll.LastZobrist ();

	// 3. 修改nDistance
	nDistance --;

	// 4. 修改check，checked及chased
	check = roll.LastCheck ();
	checked = roll.LastChecked ();
	chased = roll.LastChased ();

	// 5. 修改回滚着法表
	const int t = roll.LastZobrist().first & rbHashMask;
	RollBackHash[t] --;
	roll.nRollNum --;
}

// 判断和局
bool PositionStruct::IsDraw ( void ) const {
	for ( int i = KNIGHT_FROM; i <= PAWN_TO; i ++ ) {
		if ( piece[i+RED_TYPE] || piece[i+BLACK_TYPE] ) {
			return false;
		}
	}
	return true;
}

const int NULLOKAY_MARGIN = 200;
const int NULLSAFE_MARGIN = 400;

bool PositionStruct::NullOkay ( void ) const {
	return ( player == 0 ? vlRed : vlBlk ) > NULLOKAY_MARGIN;
}

bool PositionStruct::NullSafe ( void ) const {
	return ( player == 0 ? vlRed : vlBlk ) > NULLSAFE_MARGIN;
}

