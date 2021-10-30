#ifndef POSITION_H
#define POSITION_H

#include "core.h"


namespace agpred {
	class Position {
		const id_t id;
		
		const Symbol symbol;

		const PositionType type;  // TODO type and orderType??
		const OrderType orderType;  // TODO type and orderType??

		const EntryData entryData;
		
	};
}

#endif // POSITION_H
