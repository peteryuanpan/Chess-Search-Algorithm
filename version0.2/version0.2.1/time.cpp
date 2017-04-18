#include "time.h"
#include "base.h"
#ifdef _WIN32
#include <windows.h>
#endif

double TimeLimit = 10.0; // 默认10秒

#ifdef _WIN32
	SYSTEMTIME thisTime;
	double startT;
#else
	clock_t startT;
#endif

// 设置时间限制
void SetTimeLimit ( const double limit ) {
	TimeLimit = limit;
}

// 初始化开始时间
void InitBeginTime ( void ) {
#ifdef _WIN32
	GetSystemTime ( &thisTime );
	startT = (double)thisTime.wSecond * 1000 + thisTime.wMilliseconds;
#else
	startT = clock();
#endif
}

// 已超时
bool TimeOut ( void ) {
	double timeCost;
#ifdef _WIN32
	GetSystemTime ( &thisTime );
	timeCost = (double) ( (double)thisTime.wSecond * 1000 + thisTime.wMilliseconds - startT ) / 1000.0;
#else
	timeCost = (double) ( clock() - startT ) / CLOCKS_PER_SEC;
#endif
	return timeCost >= TimeLimit;
}

// 计算用时
double TimeCost ( void ) {
	double timeCost;
#ifdef _WIN32
	GetSystemTime ( &thisTime );
	timeCost = (double) ( (double)thisTime.wSecond * 1000 + thisTime.wMilliseconds - startT ) / 1000.0;
#else
	timeCost = (double) ( clock() - startT ) / CLOCKS_PER_SEC;
#endif
	return timeCost;
}
