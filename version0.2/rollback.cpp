#include "rollback.h"
#include "evaluate.h"
#include "search.h"
#include "debug.h"

int RollBackHash [ rbHashNum ];

// 初始化回滚结构体
void RollBackListStruct::Init ( void ) {
	nRollNum = 0;
	for ( int i = 0; i < RBL_MAXN; i ++ ) {
		move[i] = 0;
		dstPiece[i] = 0;
		check[i] = 0;
		checked[i] = 0;
		chased[i] = 0;
		zobrist[i] = std::make_pair ( 0, 0 );
	}
	for ( int i = 0; i < rbHashNum; i ++ ) {
		RollBackHash[i] = 0;
	}
}

// 返回着法类型
int GetMoveStatus ( const bool checked, const bool chased ) {
	return checked ? 2 : ( chased ? 1 : 0 );
}

// 判断重复类型
int RollBackListStruct::RepStatus ( void ) const {
	if ( nRollNum == 0 ) {
		return REP_NONE;
	}
	const int t = pos.zobrist.first & rbHashMask;
	if ( RollBackHash[t] == 0 ) {
		return REP_NONE;
	}

	// 判断连将或者连捉
	int ThisSideConCC = GetMoveStatus ( checked[nRollNum-1], chased[nRollNum-1] );
	int OppSideConCC = GetMoveStatus ( pos.checked, pos.chased );
	int TurnThisSide = 1;
	for ( int i = nRollNum - 1; i >= 0; i -- ) {
		if ( TurnThisSide ) {
			TurnThisSide = 0;
			ThisSideConCC &= GetMoveStatus ( checked[i], chased[i] );
		}
		else {
			TurnThisSide = 1;
			OppSideConCC &= GetMoveStatus ( checked[i], chased[i] );
		}
		if ( zobrist[i] == pos.zobrist ) {
			return ThisSideConCC == OppSideConCC ? REP_DRAW : ( ThisSideConCC > OppSideConCC ? REP_LOSE : REP_WIN );
		}
	}

	return REP_NONE;
}

// 返回重复打分值
int RollBackListStruct::RepValue ( const int vRep ) const {
	assert( vRep != REP_NONE );
	switch ( vRep ) {
		case REP_WIN:
			return BAN_VALUE;
		case REP_LOSE:
			return - BAN_VALUE;
		default: // REP_DRAW
			return 0;
	}
	return 0;
}
