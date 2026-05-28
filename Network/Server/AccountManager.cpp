#include "AccountManager.hpp"

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
