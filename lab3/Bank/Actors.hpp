#pragma once

#include "Bank.hpp"
#include "Settings.hpp"

#include <chrono>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <syncstream>
#include <thread>

inline class ThreadSafeLogger
{
	bool m_enabled{ true };

public:
	void SetEnabled(const bool enabled)
	{
		m_enabled = enabled;
	}

	void Log(const std::string& message)
	{
		if (!m_enabled)
		{
			return;
		}
		std::osyncstream{ std::cout } << message << "\n";
	}
} logger;

struct ActorCash
{
	std::string name;
	Money cash{ 0 };
	std::mutex mtx;

	ActorCash(std::string n, const Money initial)
		: name(std::move(n))
		, cash(initial)
	{
	}
};

inline bool TransferCash(ActorCash& from, ActorCash& to, const Money amount)
{
	std::scoped_lock lock(from.mtx, to.mtx);

	if (from.cash < amount)
	{
		return false;
	}

	from.cash -= amount;
	to.cash += amount;

	return true;
}

inline void HomerLogic(
	const std::stop_token& token,
	Bank& bank,
	const AccountId accHomer,
	const AccountId accMarge,
	const AccountId accBurns,
	ActorCash& homerCash,
	ActorCash& bartCash,
	ActorCash& lisaCash)
{
	while (!token.stop_requested())
	{
		if (bank.TrySendMoney(accHomer, accMarge, settings::GROCERY_COST))
		{
			logger.Log("Homer transferred 10 to Marge.");
		}
		if (bank.TrySendMoney(accHomer, accBurns, settings::HOMER_ELECTRICITY_BILL))
		{
			logger.Log("Homer paid 5 to Mr. Burns for electricity.");
		}
		if (bank.TryWithdrawMoney(accHomer, settings::HOMER_WITHDRAW))
		{
			{
				std::lock_guard lock(homerCash.mtx);
				homerCash.cash += settings::HOMER_WITHDRAW;
			}
			logger.Log("Homer withdrew 20 cash.");
		}
		if (TransferCash(homerCash, bartCash, settings::POCKET_MONEY))
		{
			logger.Log("Homer gave 5 cash to Bart.");
		}
		if (TransferCash(homerCash, lisaCash, settings::POCKET_MONEY))
		{
			logger.Log("Homer gave 5 cash to Lisa.");
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(settings::TICK_RATE_HOMER));
	}
}

inline void MargeLogic(
	const std::stop_token& token,
	Bank& bank,
	const AccountId accMarge,
	const AccountId accApu)
{
	while (!token.stop_requested())
	{
		if (bank.TrySendMoney(accMarge, accApu, settings::GROCERY_COST))
		{
			logger.Log("Marge bought groceries from Apu (15).");
		}
		else
		{
			std::this_thread::yield();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(settings::TICK_RATE_MARGE));
	}
}

inline void KidsLogic(const std::stop_token& token, ActorCash& kidCash, ActorCash& apuCash)
{
	while (!token.stop_requested())
	{
		if (TransferCash(kidCash, apuCash, settings::CANDIES_COST))
		{
			logger.Log(kidCash.name + " bought sweets from Apu (2 cash).");
		}
		else
		{
			std::this_thread::yield();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(settings::TICK_RATE_KIDS));
	}
}

inline void ApuLogic(
	const std::stop_token& token,
	Bank& bank,
	const AccountId accApu,
	const AccountId accBurns,
	ActorCash& apuCash)
{
	while (!token.stop_requested())
	{
		if (bank.TrySendMoney(accApu, accBurns, settings::APU_ELECTRICITY_BILL))
		{
			logger.Log("Apu paid 10 to Mr. Burns for electricity.");
		}

		Money cashToDeposit = 0;
		{
			std::lock_guard lock(apuCash.mtx);
			if (apuCash.cash >= settings::APU_DEPOSIT)
			{
				cashToDeposit = settings::APU_DEPOSIT;
				apuCash.cash -= settings::APU_DEPOSIT;
			}
		}

		if (cashToDeposit > 0)
		{
			try
			{
				bank.DepositMoney(accApu, cashToDeposit);
				logger.Log("Apu deposited 10 cash to bank.");
			}
			catch (const std::exception&)
			{
				std::lock_guard lock(apuCash.mtx);
				apuCash.cash += cashToDeposit;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(settings::TICK_RATE_APU));
	}
}

inline void BurnsLogic(
	const std::stop_token& token,
	Bank& bank,
	const AccountId accBurns,
	const AccountId accHomer)
{
	while (!token.stop_requested())
	{
		if (bank.TrySendMoney(accBurns, accHomer, settings::HOMER_SALARY))
		{
			logger.Log("Mr. Burns paid 50 salary to Homer.");
		}
		else
		{
			std::this_thread::yield();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(settings::TICK_RATE_BURNS));
	}
}