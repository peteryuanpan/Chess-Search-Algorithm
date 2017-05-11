#ifndef SEARCH_H_
#define SEARCH_H_

#include "base.h"
#include "position.h"
#include "rollback.h"

extern PositionStruct pos; // 当前搜索局面
extern RollBackListStruct roll; // 回滚着法表

struct SearchStruct {
	int bmv; // 最佳着法
	int bvl; // 最优得分
	int nNode; // 总节点数
	int nBeta; // beta点个数
};
extern SearchStruct Search;

int MainSearch ( void );

#endif /* SEARCH_H_ */