#pragma once

#include <string>
#include <unordered_map>

struct Account {
	std::string userId_;
	std::string password_;
};

class AccountManager {
   public:
	bool RegisterAccount(const Account& account);

   private:
	std::unordered_map<std::string, std::string> accounts_;
};
