#pragma once

#include <cstdint>

class Account;
class DBManager;

class AccountManager {
   public:
	AccountManager(DBManager* dbManager);

	bool RegisterAccount(const Account& account);
	int64_t Authenticate(const Account& account);

   private:
	DBManager* dbManager_;
};
