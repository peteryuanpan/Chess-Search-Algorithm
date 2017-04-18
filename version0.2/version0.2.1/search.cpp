#include "search.h"
#include "base.h"
#include "hash.h"
#include "position.h"
#include "rollback.h"
#include "movesort.h"
#include "evaluate.h"
#include "debug.h"
#include "time.h"

PositionStruct pos; // 当前搜索局面
RollBackListStruct roll; // 回滚着法表

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

	return -MATE_VALUE;
}

// 主要遍历搜索
int SearchPV ( int depth, int alpha, int beta ) {
	int val;
	int bestval = - MATE_VALUE;
	int bestmv = 0;
	MoveSortStruct mvsort;

	if ( TimeOut() ) { // 超时
		return bestval;
	}

	// 1. 打分
	if ( depth <= 0 ) {
		return pos.Evaluate ();
	}

	// 2. 无害裁剪
	val = HarmlessPruning ();
	if ( val > - MATE_VALUE ) {
		return val;
	}

	// 3. 置换裁剪
	val = QueryValueInHashTable ( depth );
	if ( val != - MATE_VALUE ) {
		return val;
	}

	// 4. 生成着法
	mvsort.InitPV ();

	// 5. 递归搜索
	int mv;
	while ( (mv = mvsort.NextPV()) != 0 ) {
		pos.MakeMove ( mv ); // 走一步
		int val = -SearchPV ( depth - 1, -beta, -alpha ); // 搜下一层
		pos.UndoMakeMove (); // 回一步

		if ( TimeOut() ) { // 超时
			return bestval;
		}

		if ( val > bestval ) { // 更新
			bestval = val;
			bestmv = mv;
			if ( bestval >= beta ) {
				InsertHashTable ( depth, bestval, bestmv );
				return bestval;
			}
			if ( bestval > alpha ) {
				alpha = bestval;
			}
		}
	}
	InsertHashTable ( depth, bestval, bestmv );
	return bestval;
}

// 主搜索函数
void MainSearch ( void ) {
	// 1. 初始化时间器
	InitBeginTime ();

	// 2. 清空置换表
	ClearHashTable ();

	// 3. 迭代加深搜索，并计算时间
	int bestmove[100];
	int nb = 0;
	for ( int depth = 1; depth <= 32; depth ++ ) {
		// 搜索
		int value = SearchPV ( depth, - MATE_VALUE, MATE_VALUE );
		if ( TimeOut() ) { // 超时
			break;
		}

		// 记录着法，输出重要信息
		bestmove[++nb] = QueryMoveInHashTable ();
		printf("depth: %2d, time = %.2f, value: %5d, bestmove = %s\n", depth, TimeCost(), value, MoveIntToStr(bestmove[nb]).c_str());

		// 搜到杀棋 或 无解
		if ( value >= MATE_VALUE || value <= - MATE_VALUE) {
			break;
		}
	}
	printf("TotalTime = %.2fs\n", TimeCost());

	// 4. 输出最优着法
	// 分情况确定最优着法
	int bestmv = 0;
	if ( QueryValueInHashTable (0) == MATE_VALUE ) {
		bestmv = QueryMoveInHashTable ();
	}
	else if ( QueryValueInHashTable (0) == - MATE_VALUE ) {
		bestmv = 0;
	}
	else {
		// 在所有着法中找出权值最高的 ( 这个方法有待研究改进 )
		int maxval = 0;
		for ( int i = 1; i <= nb; i ++ ) {
			int val = 0;
			for ( int j = 1; j <= nb; j ++ ) {
				if ( bestmove[i] == bestmove[j] ) {
					val += j / 10 + 1;
				}
			}
			if ( val > maxval ) {
				maxval = val;
				bestmv = bestmove[i];
			}
		}
	}
	// 输出
	if ( bestmv == 0 ) {
		printf("nobestmove\n");
		fflush(stdout);
	}
	else {
		printf("bestmove %s\n", MoveIntToStr(bestmv).c_str());
		fflush(stdout);
	}
}
