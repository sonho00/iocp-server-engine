#pragma once

#include <string>

class Account {
   public:
	Account(std::string userId, std::string password)
		: userId_(std::move(userId)), password_(std::move(password)) {}

	[[nodiscard]] const std::string& GetUserId() const { return userId_; }
	void SetUserId(const std::string& userId) { userId_ = userId; }

	[[nodiscard]] const std::string& GetPassword() const { return password_; }
	void SetPassword(const std::string& password) { password_ = password; }

   private:
	std::string userId_;
	std::string password_;
};
