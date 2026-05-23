#include "AccountManager.hpp"

#include <unordered_map>

bool AccountManager::RegisterAccount(const Account& account) {
	if (accounts_.contains(account.userId_)) return false;

	accounts_[account.userId_] = account.password_;
	return true;
}
