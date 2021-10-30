#ifndef ACCOUNT_CONTROLLER_H
#define ACCOUNT_CONTROLLER_H

#include <nlohmann/json.hpp>

#include "core.h"


using json = nlohmann::json;


namespace agpred {
	class AccountController {

		void onSnapshot(const Symbol& symbol, const Snapshot& snapshot);

	};
}

#endif // ACCOUNT_CONTROLLER_H
