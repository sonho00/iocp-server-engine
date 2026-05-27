#include "AccountManager.hpp"

#include <unordered_map>

bool AccountManager::RegisterAccount(const Account& account) {
	if (accounts_.contains(account.userId_)) return false;

	accounts_[account.userId_].userId_ = account.userId_;
	accounts_[account.userId_].password_ = account.password_;
	return true;
}
