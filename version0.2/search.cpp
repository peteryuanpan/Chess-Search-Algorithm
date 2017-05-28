#include "search.h"
#include "base.h"
#include "hash.h"
#include "position.h"
#include "rollback.h"
#include "movesort.h"
#include "evaluate.h"
#include "debug.h"
#include "time.h"

PositionStruct pos;
RollBackListStruct roll;
SearchStruct Search;

MyTreeStruct MyTree[NNODE];
int head[NNODE];
int nNode;
int nEdge;

int DEP_LIMIT;
int BVL_LIMIT;
int NSN_LIMIT;

// 超时
bool TimeOut ( void ) {
	return TimeOut ( THIS_SEARCH_TIME ) || TimeOut ( SEARCH_TOTAL_TIME );
}

// 无害裁剪
int HarmlessPruning ( void ) {
	// 1. 和局局面
	if ( pos.IsDraw() ) {
		return 0; // eleeye上表示，为了安全起见，不用pos.DrawValue()
	}

	// 2. 路径重复
	int vRep = roll.RepStatus ();
	if ( vRep != REP_NONE ) {
		return roll.RepValue ( vRep );
	}

	return - MATE_VALUE;
}

// 零窗口搜索
int SearchCut ( int depth, int beta ) {
	int mv, vl;
	int bmv = 0;
	int bvl = - MATE_VALUE;
	MoveSortStruct mvsort;

	if ( TimeOut() ) { // 超时
		return bvl;
	}

	// 无害裁剪
	vl = HarmlessPruning ();
	if ( vl > - MATE_VALUE ) {
		return vl;
	}

	// 置换裁剪
	vl = QueryValueInHashTable ( depth, beta - 1, beta );
	if ( vl > - BAN_VALUE ) {
		return vl;
	}

	// 达到极限深度
	if ( depth <= 0 ) {
		if ( pos.checked ) {
			return SearchCut ( depth + 1, beta );
		}
		return pos.Evaluate ();
	}

	// 生成着法
	int nMoveNum = mvsort.InitCutMove ();
	Search.nNode ++;

	// 按照着法搜索
	while ( (mv = mvsort.NextMove()) != 0 ) {
		pos.MakeMove ( mv );
		int newDepth = ( pos.checked || nMoveNum == 1 ) ? depth : depth - 1;
		vl = - SearchCut ( newDepth, 1 - beta );
		pos.UndoMakeMove ();

		if ( TimeOut() ) { // 超时
			return bvl;
		}

		// 边界
		if ( vl > bvl ) {
			bvl = vl;
			bmv = mv;
			if ( vl >= beta ) {
				Search.nBeta ++;
				InsertMoveToHashTable ( depth, bmv, bvl, HASH_TYPE_BETA );
				InsertHistoryTable ( bmv, depth );
				return vl;
			}
		}
	}

	// 最后
	InsertMoveToHashTable ( depth, bmv, bvl, HASH_TYPE_ALPHA );
	return bvl;
}

// Alpha-Beta 搜索
int SearchAlphaBeta ( int depth, int alpha, int beta ) {
	int mv, vl;
	int bmv[nBest], bvl[nBest];
	ClearBmvBvl ( bmv, bvl );
	int hash_type = HASH_TYPE_ALPHA;
	MoveSortStruct mvsort;

	if ( TimeOut() ) { // 超时
		return bvl[0];
	}

	// 无害裁剪
	vl = HarmlessPruning ();
	if ( vl > - MATE_VALUE ) {
		return vl;
	}

	// 置换裁剪
	vl = QueryValueInHashTable ( depth, alpha, beta );
	if ( vl > - BAN_VALUE ) {
		Search.bmv[0] = QueryMoveInHashTable ( depth, alpha, beta );
		Search.bvl[0] = vl;
		return vl;
	}

	// 达到极限深度
	if ( depth <= 0 ) {
		if ( pos.checked ) {
			return SearchAlphaBeta ( depth + 1, alpha, beta );
		}
		return pos.Evaluate ();
	}

	// 内部迭代加深启发
	if ( depth > 2 ) {
		mv = QueryMoveInHashTableWithoutLimit ();
		if ( mv == 0 ) {
			SearchAlphaBeta ( depth / 2, alpha, beta );
			if ( TimeOut() ) { // 超时
				return bvl[0];
			}
		}
	}

	// 生成着法
	int nMoveNum = mvsort.InitAlphaBetaMove ();
	Search.nNode ++;

	// 大搜索
	while ( (mv = mvsort.NextMove()) != 0 ) {
		pos.MakeMove ( mv );
		int newDepth = ( pos.checked || nMoveNum == 1 ) ? depth : depth - 1;
		vl = - SearchCut ( newDepth, - alpha );
		if ( vl > alpha && vl < beta ) {
			vl = - SearchAlphaBeta ( newDepth, -beta, -alpha );
		}
		pos.UndoMakeMove ();

		if ( TimeOut() ) {
			return bvl[0];
		}

		UpdateBmvBvl ( bmv, bvl, mv, vl );

		if ( bvl[0] >= beta ) {
			Search.nBeta ++;
			hash_type = HASH_TYPE_BETA;
			break;
		}
		if ( bvl[0] > alpha ) {
			alpha = bvl[0];
			hash_type = HASH_TYPE_PV;
		}
	}

	// 最后
	InsertMoveToHashTable ( depth, bmv[0], bvl[0], hash_type );
	InsertHistoryTable ( bmv[0], depth );
	CopyBmvBvl ( Search.bmv, Search.bvl, bmv, bvl );
	return bvl[0];
}

// わたし の 搜索树
int SearchMyTree ( int a, int depth, int alpha, int beta ) {
	int vl = - MATE_VALUE;
	if ( TimeOut() ) { // 超时
		return vl;
	}

	if ( depth <= DEP_LIMIT ) {
		vl = SearchAlphaBeta ( depth, alpha, beta );
		if ( depth == DEP_LIMIT ) {
			AddEdge ( a, depth, Search.bmv, Search.bvl );
		}
		return vl;
	}
	else {
		int bmv[nBest], bvl[nBest];
		ClearBmvBvl ( bmv, bvl );
		int hash_type = HASH_TYPE_ALPHA;

		// 无害裁剪
		vl = HarmlessPruning ();
		if ( vl > - MATE_VALUE ) {
			return vl;
		}

		// 置换裁剪
		vl = QueryValueInHashTableTR ( depth, alpha, beta );
		if ( vl > - BAN_VALUE ) {
			return vl;
		}

		// 大搜索
		Search.nNode ++;
		for ( int i = head[a]; i != -1; i = MyTree[i].next ) {
			pos.MakeMove ( MyTree[i].mv );
			vl = - SearchMyTree ( MyTree[i].to, depth - 1, -beta, -alpha );
			pos.UndoMakeMove ();

			if ( TimeOut() ) {
				return bvl[0];
			}

			UpdateBmvBvl ( bmv, bvl, MyTree[i].mv, vl );

			if ( bvl[0] >= beta ) {
				Search.nBeta ++;
				hash_type = HASH_TYPE_BETA;
				break;
			}
			if ( bvl[0] > alpha ) {
				alpha = bvl[0];
				hash_type = HASH_TYPE_PV;
			}
		}
		InsertMoveToHashTableTR ( depth, bmv[0], bvl[0], hash_type );
		DelEdge ( a, depth, bmv, bvl );
		CopyBmvBvl ( Search.bmv, Search.bvl, bmv, bvl );
		return bvl[0];
	}
}

// 主搜索函数
int SearchMain ( void ) {
	// 特殊情况
	MoveSortStruct mvsort;
	int nMoveNum = mvsort.InitAlphaBetaMove ();
	if ( nMoveNum == 0 ) { // 无着法
		printf ( "bestmove a0a1 resign\n" );
		fflush ( stdout );
		return 0;
	}
	else if ( nMoveNum == 1 ) { // 唯一着法
		printf ( "bestmove %s\n", MoveIntToStr(mvsort.move[0]).c_str() );
		fflush ( stdout );
		return mvsort.move[0];
	}
	if ( pos.IsDraw() ) { // 和局
		printf ( "bestmove a0a1 draw\n" );
		fflush ( stdout );
		// 注意不要return
	}

	// 初始化
	ClearHistoryTable ();
	ClearHashTable ();
	ClearHashTableTR ();
	InitMyTreeStruct ();
	InitBeginTime ( SEARCH_TOTAL_TIME );

	// 迭代加深搜索
	printf("depth   time    nNode  rBeta");
	for ( int i = 0; i < 3; i ++ ) {
		printf("   bvl[%d]  bmv[%d]", i, i);
	}
	printf("\n");
	fflush ( stdout );
	int lastbvl[nBest], lastbmv[nBest];
	ClearBmvBvl ( lastbmv, lastbvl );
	for ( int depth = 1; /*depth <= ?*/; depth ++ ) {
		InitBeginTime ( THIS_SEARCH_TIME );
		InitSearchStruct ();
		ClearHashTableTR ();
		//SearchAlphaBeta ( depth, - MATE_VALUE, MATE_VALUE );
		SearchMyTree ( 1, depth, - MATE_VALUE, MATE_VALUE );

		if ( TimeOut() ) {
			CopyBmvBvl ( Search.bmv, Search.bvl, lastbmv, lastbvl );
			break;
		}
		else {
			CopyBmvBvl ( lastbmv, lastbvl, Search.bmv, Search.bvl );
		}

		printf( "%5d  %.2fs  %7d    %2.0f%%", depth, TimeCost(THIS_SEARCH_TIME), Search.nNode, 100.0*Search.nBeta/Search.nNode);
		for ( int i = 0; i < 3; i ++ ) {
			printf("   %6d    %s", Search.bvl[i], MoveIntToStr(Search.bmv[i]).c_str());
		}
		printf("\n");
		fflush ( stdout );

		if ( Search.bvl[0] <= - BAN_VALUE || Search.bvl[0] >= BAN_VALUE) {
			break;
		}
		if ( Search.bvl[1] <= - BAN_VALUE || Search.bmv[1] == 0 ) {
			break;
		}
	}
	printf ( "totaltime: %.2fs\n", TimeCost(SEARCH_TOTAL_TIME) );
	fflush ( stdout );

	// 输出最优着法
	if ( Search.bmv[0] == 0 || Search.bvl[0] <= - BAN_VALUE ) {
		printf ( "bestmove a0a1 resign\n" ); // 认输
		fflush ( stdout );
		return 0;
	}
	else {
		printf ( "bestmove %s\n", MoveIntToStr(Search.bmv[0]).c_str() );
		fflush ( stdout );
		return Search.bmv[0];
	}
}