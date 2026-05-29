#include "DBManager.hpp"

#include "Network/Common/Logger.hpp"
#include "Network/Common/SQLite/sqlite3.h"

DBManager::DBManager() {
	int result = sqlite3_open(":memory:", &db_);
	if (result != SQLITE_OK) {
		LOG_FATAL("Failed to open database: {}", sqlite3_errmsg(db_));
		db_ = nullptr;
	}
}

DBManager::~DBManager() {
	if (db_ != nullptr) {
		sqlite3_close(db_);
	}
}
