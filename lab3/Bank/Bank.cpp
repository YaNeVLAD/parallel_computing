#include "Bank.hpp"

void Bank::IncrementOperations() const
{
	m_operationCount.fetch_add(1);
}

Bank::Bank(const Money cash)
{
	if (cash < 0)
	{
		throw BankOperationError("Initial cash cannot be negative");
	}
	m_cashInCirculation.store(cash);
}

std::size_t Bank::GetOperationsCount() const
{
	return m_operationCount.load();
}

Money Bank::GetCash() const
{
	return m_cashInCirculation.load();
}

AccountId Bank::OpenAccount()
{
	std::unique_lock lock(m_bankMutex);
	const AccountId id = m_nextAccountId.fetch_add(1);
	m_accounts[id] = std::make_unique<Account>();
	IncrementOperations();

	return id;
}

Money Bank::CloseAccount(AccountId accountId)
{
	std::unique_lock lock(m_bankMutex);
	const auto it = m_accounts.find(accountId);
	if (it == m_accounts.end())
	{
		throw BankOperationError("Account not found");
	}

	const Money balance = it->second->balance;
	m_accounts.erase(it);
	m_cashInCirculation.fetch_add(balance);
	IncrementOperations();

	return balance;
}

Money Bank::GetAccountBalance(AccountId accountId) const
{
	std::shared_lock lock(m_bankMutex);
	const auto it = m_accounts.find(accountId);
	if (it == m_accounts.end())
	{
		throw BankOperationError("Account not found");
	}

	std::lock_guard accountLock(it->second->mtx);
	IncrementOperations();

	return it->second->balance;
}

void Bank::SendMoney(const AccountId srcAccountId, const AccountId dstAccountId, const Money amount)
{
	if (!TrySendMoney(srcAccountId, dstAccountId, amount))
	{
		throw BankOperationError("Insufficient funds or account not found");
	}
}

bool Bank::TrySendMoney(const AccountId srcAccountId, const AccountId dstAccountId, const Money amount)
{
	try
	{
		if (amount < 0)
		{
			throw std::out_of_range("Negative amount");
		}
		std::shared_lock lock(m_bankMutex);
		const auto itSrc = m_accounts.find(srcAccountId);
		const auto itDst = m_accounts.find(dstAccountId);
		if (itSrc == m_accounts.end() || itDst == m_accounts.end())
		{
			throw BankOperationError("Invalid accounts");
		}

		auto lk = std::scoped_lock(itSrc->second->mtx, itDst->second->mtx);
		if (itSrc->second->balance < amount)
		{
			return false;
		}

		itSrc->second->balance -= amount;
		itDst->second->balance += amount;
		IncrementOperations();

		return true;
	}
	catch (const BankOperationError&)
	{
		throw;
	}
}
void Bank::WithdrawMoney(const AccountId account, const Money amount)
{
	if (!TryWithdrawMoney(account, amount))
	{
		throw BankOperationError("Insufficient funds or account not found");
	}
}

bool Bank::TryWithdrawMoney(const AccountId account, const Money amount)
{
	if (amount < 0)
	{
		throw std::out_of_range("Amount cannot be negative");
	}

	std::shared_lock lock(m_bankMutex);
	const auto it = m_accounts.find(account);
	if (it == m_accounts.end())
	{
		throw BankOperationError("Account not found");
	}

	std::lock_guard accountLock(it->second->mtx);
	if (it->second->balance < amount)
	{
		return false;
	}

	it->second->balance -= amount;
	m_cashInCirculation.fetch_add(amount);
	IncrementOperations();

	return true;
}

void Bank::DepositMoney(const AccountId account, const Money amount)
{
	if (amount < 0)
	{
		throw std::out_of_range("Amount cannot be negative");
	}

	std::shared_lock lock(m_bankMutex);
	const auto it = m_accounts.find(account);
	if (it == m_accounts.end())
	{
		throw BankOperationError("Account not found");
	}

	Money expected = m_cashInCirculation.load();
	do
	{
		if (expected < amount)
		{
			throw BankOperationError("Not enough cash in circulation");
		}
	} while (!m_cashInCirculation.compare_exchange_weak(expected, expected - amount));

	std::lock_guard acc_lock(it->second->mtx);
	it->second->balance += amount;
	IncrementOperations();
}