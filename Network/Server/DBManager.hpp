#pragma once

#include <concepts>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "Network/Common/Logger.hpp"
#include "Network/Common/SQLite/sqlite3.h"

class DBManager {
	using StmtPtr = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

   public:
	DBManager();
	~DBManager();

	template <typename T>
	void BindParameter(sqlite3_stmt* stmt, int index, const T& value);

	template <typename... Args>
	bool ExecuteNonQuery(const std::string& query, Args&&... args);

	template <typename Func, typename... Args>
		requires std::invocable<Func, sqlite3_stmt*>
	bool ExecuteQuery(const std::string& query, Func&& callback,
					  Args&&... args);

	[[nodiscard]] sqlite3* GetDB() const { return db_; }

   private:
	template <typename... Args>
	StmtPtr Prepare(const std::string& query, Args&&... args);

	sqlite3* db_ = nullptr;
};

template <typename T>
void DBManager::BindParameter(sqlite3_stmt* stmt, int index, const T& value) {
	// NOLINTBEGIN (performance-no-int-to-ptr)
	if constexpr (std::is_same_v<std::decay_t<T>, int>) {
		sqlite3_bind_int(stmt, index, value);
	} else if constexpr (std::is_same_v<std::decay_t<T>, int64_t>) {
		sqlite3_bind_int64(stmt, index, value);
	} else if constexpr (std::is_floating_point_v<std::decay_t<T>>) {
		sqlite3_bind_double(stmt, index, value);
	} else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
		sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT);
	} else if constexpr (std::is_convertible_v<std::decay_t<T>, const char*>) {
		sqlite3_bind_text(stmt, index, static_cast<const char*>(value), -1,
						  SQLITE_TRANSIENT);
	} else {
		static_assert(false, "Unsupported parameter type");
	}
	// NOLINTEND
}

template <typename... Args>
bool DBManager::ExecuteNonQuery(const std::string& query, Args&&... args) {
	StmtPtr stmt = Prepare(query, std::forward<Args>(args)...);
	if (!stmt) return false;

	int result = sqlite3_step(stmt.get());
	if (result != SQLITE_DONE && result != SQLITE_ROW) {
		LOG_ERROR("Failed to execute statement: {}", sqlite3_errmsg(db_));
		return false;
	}

	return true;
}

template <typename Func, typename... Args>
	requires std::invocable<Func, sqlite3_stmt*>
bool DBManager::ExecuteQuery(const std::string& query, Func&& callback,
							 Args&&... args) {
	StmtPtr stmt = Prepare(query, std::forward<Args>(args)...);
	if (!stmt) return false;

	int result;
	while ((result = sqlite3_step(stmt.get())) == SQLITE_ROW) {
		callback(stmt.get());
	}
	if (result != SQLITE_DONE) {
		LOG_ERROR("Failed to execute statement: {}", sqlite3_errmsg(db_));
		return false;
	}
	return true;
}

template <typename... Args>
DBManager::StmtPtr DBManager::Prepare(const std::string& query,
									  Args&&... args) {
	sqlite3_stmt* stmt;

	int result = sqlite3_prepare_v2(db_, query.c_str(), -1, &stmt, nullptr);
	if (result != SQLITE_OK) {
		LOG_ERROR("Failed to prepare statement: {}", sqlite3_errmsg(db_));
		return {nullptr, sqlite3_finalize};
	}

	StmtPtr stmtGuard(stmt, sqlite3_finalize);
	int index = 1;
	(BindParameter(stmt, index++, args), ...);

	return stmtGuard;
}
