#pragma once

#include <cstdint>
#include <string>

class Account {
   public:
	Account(std::string userId, std::string password)
		: userId_(std::move(userId)), password_(std::move(password)) {}

	[[nodiscard]] int64_t GetDbId() const { return dbId_; }

	[[nodiscard]] const std::string& GetUserId() const { return userId_; }
	void SetUserId(const std::string& userId) { userId_ = userId; }

	[[nodiscard]] const std::string& GetPassword() const { return password_; }
	void SetPassword(const std::string& password) { password_ = password; }

   private:
	int64_t dbId_ = 0;
	std::string userId_;
	std::string password_;
};
