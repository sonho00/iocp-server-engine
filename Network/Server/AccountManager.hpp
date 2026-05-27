#pragma once

#include <string>
#include <unordered_map>

#include "Account.hpp"

class AccountManager {
   public:
	bool RegisterAccount(const Account& account);

   private:
	std::unordered_map<std::string, Account> accounts_;
};
