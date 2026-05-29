#include "AccountManager.hpp"

#include "Account.hpp"
#include "DBManager.hpp"

AccountManager::AccountManager(DBManager* dbManager) : dbManager_(dbManager) {
	dbManager_->ExecuteNonQuery(
		"CREATE TABLE IF NOT EXISTS accounts ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"username TEXT UNIQUE NOT NULL,"
		"password TEXT NOT NULL);");
}

bool AccountManager::RegisterAccount(const Account& account) {
	return dbManager_->ExecuteNonQuery(
		"INSERT INTO accounts (username, password) VALUES (?, ?);",
		account.GetUserId(), account.GetPassword());
}

int64_t AccountManager::Authenticate(const Account& account) {
	int64_t result = 0;
	dbManager_->ExecuteQuery(
		"SELECT id FROM accounts WHERE username = ? AND password = ?;",
		[&result](sqlite3_stmt* stmt) {
			result = sqlite3_column_int64(stmt, 0);
		},
		account.GetUserId(), account.GetPassword());
	return result;
}
