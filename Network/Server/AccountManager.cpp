#include "AccountManager.hpp"

#include <unordered_map>

bool AccountManager::RegisterAccount(const Account& account) {
	if (accounts_.contains(account.GetUserId())) return false;

	accounts_.emplace(account.GetUserId(), account);
	return true;
}
