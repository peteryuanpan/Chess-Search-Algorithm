#ifndef DEBUG_H_
#define DEBUG_H_
#include "position.h"
#include "search.h"

void MainDebug ( void );

inline void PrintChessboard ( void ) {
	printf ( "player = %d\n", pos.player );
	int p = STA_POS;
	for ( int i = 0; i < 10; i ++ ) {
		for ( int j = 0; j < 9; j ++ ) {
			printf("%2d ", pos.square[p]);
			p = NEXTPOS ( p );
		}
		printf("\n");
	}
	printf("\n");
}

#endif /* DEBUG_H_ */
