#pragma once

#include "Account.hpp"

class DBManager;

class AccountManager {
   public:
	AccountManager(DBManager* dbManager);

	bool RegisterAccount(const Account& account);

   private:
	DBManager* dbManager_;
};
