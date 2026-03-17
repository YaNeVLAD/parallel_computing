#pragma once

#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>

using AccountId = unsigned long long;
using Money = long long;

class BankOperationError final : public std::runtime_error
{
public:
	using runtime_error::runtime_error;
};

class Bank
{
	struct Account
	{
		Money balance{ 0 };
		std::mutex mtx;
	};

	std::atomic<Money> m_cashInCirculation{ 0 };
	mutable std::atomic_size_t m_operationCount{ 0 };

	mutable std::shared_mutex m_bankMutex;

	std::unordered_map<AccountId, std::unique_ptr<Account>> m_accounts;
	std::atomic<AccountId> m_nextAccountId{ 1 };

	void IncrementOperations() const;

public:
	explicit Bank(Money cash);

	Bank(const Bank&) = delete;
	Bank& operator=(const Bank&) = delete;

	std::size_t GetOperationsCount() const;

	Money GetCash() const;

	AccountId OpenAccount();

	Money CloseAccount(AccountId accountId);

	Money GetAccountBalance(AccountId accountId) const;

	void SendMoney(AccountId srcAccountId, AccountId dstAccountId, Money amount);

	[[nodiscard]] bool TrySendMoney(AccountId srcAccountId, AccountId dstAccountId, Money amount);

	void WithdrawMoney(AccountId account, Money amount);

	[[nodiscard]] bool TryWithdrawMoney(AccountId account, Money amount);

	void DepositMoney(AccountId account, Money amount);
};