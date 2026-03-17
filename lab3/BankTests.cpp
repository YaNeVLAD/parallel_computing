#include "Bank/Bank.hpp"

#include <gtest/gtest.h>

// ==============================================================================
// BASIC TESTS
// ==============================================================================

TEST(BankBasicTests, Initialization)
{
	const Bank bank(1000);
	EXPECT_EQ(bank.GetCash(), 1000);
	EXPECT_EQ(bank.GetOperationsCount(), 0);

	EXPECT_THROW(Bank(-100), BankOperationError);
}

TEST(BankBasicTests, OpenAndCloseAccount)
{
	Bank bank(1000);
	AccountId acc1 = bank.OpenAccount();
	AccountId acc2 = bank.OpenAccount();

	EXPECT_NE(acc1, acc2); // ID должны быть уникальными
	EXPECT_EQ(bank.GetAccountBalance(acc1), 0);

	// Закрытие пустого счета
	Money closedBalance = bank.CloseAccount(acc1);
	EXPECT_EQ(closedBalance, 0);
	EXPECT_THROW(bank.GetAccountBalance(acc1), BankOperationError);

	// Закрытие несуществующего счета
	EXPECT_THROW(bank.CloseAccount(999), BankOperationError);
}

TEST(BankBasicTests, DepositAndWithdraw)
{
	Bank bank(1000);
	AccountId acc = bank.OpenAccount();

	// Внесение
	bank.DepositMoney(acc, 400);
	EXPECT_EQ(bank.GetCash(), 600);
	EXPECT_EQ(bank.GetAccountBalance(acc), 400);

	// Нельзя внести больше, чем есть в обороте
	EXPECT_THROW(bank.DepositMoney(acc, 700), BankOperationError);
	// Нельзя внести отрицательную сумму
	EXPECT_THROW(bank.DepositMoney(acc, -50), std::out_of_range);

	// Снятие
	bank.WithdrawMoney(acc, 150);
	EXPECT_EQ(bank.GetCash(), 750);
	EXPECT_EQ(bank.GetAccountBalance(acc), 250);

	// Снятие через TryWithdrawMoney
	EXPECT_TRUE(bank.TryWithdrawMoney(acc, 50));
	EXPECT_EQ(bank.GetAccountBalance(acc), 200);

	// Недостаточно средств
	EXPECT_FALSE(bank.TryWithdrawMoney(acc, 500));
	EXPECT_THROW(bank.WithdrawMoney(acc, 500), BankOperationError);

	// Отрицательная сумма
	EXPECT_THROW(bank.WithdrawMoney(acc, -10), std::out_of_range);
}

TEST(BankBasicTests, SendMoney)
{
	Bank bank(1000);
	AccountId acc1 = bank.OpenAccount();
	AccountId acc2 = bank.OpenAccount();

	bank.DepositMoney(acc1, 500);

	// Успешный перевод
	bank.SendMoney(acc1, acc2, 200);
	EXPECT_EQ(bank.GetAccountBalance(acc1), 300);
	EXPECT_EQ(bank.GetAccountBalance(acc2), 200);

	// Перевод через TrySendMoney
	EXPECT_TRUE(bank.TrySendMoney(acc1, acc2, 100));
	EXPECT_EQ(bank.GetAccountBalance(acc1), 200);
	EXPECT_EQ(bank.GetAccountBalance(acc2), 300);

	// Недостаточно средств
	EXPECT_FALSE(bank.TrySendMoney(acc1, acc2, 1000));
	EXPECT_THROW(bank.SendMoney(acc1, acc2, 1000), BankOperationError);

	// Отрицательная сумма
	EXPECT_THROW(bank.SendMoney(acc1, acc2, -10), std::out_of_range);

	// Несуществующие счета
	EXPECT_THROW(bank.SendMoney(acc1, 999, 50), BankOperationError);
}

// ==============================================================================
// CONCURRENCY TESTS
// ==============================================================================

TEST(BankConcurrentTests, ConcurrentDepositsAndWithdrawals)
{
	constexpr Money INITIAL_CASH = 100000;
	constexpr int NUM_THREADS = 10;

	Bank bank(INITIAL_CASH);
	AccountId acc = bank.OpenAccount();

	std::vector<std::thread> threads;
	threads.reserve(NUM_THREADS);
	for (int i = 0; i < NUM_THREADS; ++i)
	{
		constexpr int OPERATIONS_PER_THREAD = 1000;

		threads.emplace_back([&bank, acc, i, OPERATIONS_PER_THREAD] {
			for (int j = 0; j < OPERATIONS_PER_THREAD; ++j)
			{
				if (i % 2 == 0)
				{
					bank.DepositMoney(acc, 1);
				}
				else
				{
					std::ignore = bank.TryWithdrawMoney(acc, 1);
				}
			}
		});
	}

	for (auto& t : threads)
	{
		t.join();
	}

	// Проверяем главный инвариант: общая сумма денег в системе не должна измениться
	const Money totalSystemMoney = bank.GetCash() + bank.GetAccountBalance(acc);
	EXPECT_EQ(totalSystemMoney, INITIAL_CASH);
}

TEST(BankConcurrentTests, DeadlockPreventionOnTransfers)
{
	constexpr Money INITIAL_CASH = 10000;
	constexpr int NUM_THREADS = 4;

	Bank bank(INITIAL_CASH);
	AccountId accA = bank.OpenAccount();
	AccountId accB = bank.OpenAccount();

	bank.DepositMoney(accA, INITIAL_CASH / 2);
	bank.DepositMoney(accB, INITIAL_CASH / 2);

	std::vector<std::thread> threads;

	// Запускаем потоки, делающие встречные переводы.
	// Если std::lock в SendMoney работает неверно, тест зависнет (Deadlock).
	for (int i = 0; i < NUM_THREADS; ++i)
	{
		constexpr int OPERATIONS_PER_THREAD = 2000;

		threads.emplace_back([&bank, accA, accB, i, OPERATIONS_PER_THREAD] {
			for (int j = 0; j < OPERATIONS_PER_THREAD; ++j)
			{
				if (i % 2 == 0)
				{
					std::ignore = bank.TrySendMoney(accA, accB, 2);
				}
				else
				{
					std::ignore = bank.TrySendMoney(accB, accA, 2);
				}
			}
		});
	}

	for (auto& t : threads)
	{
		t.join();
	}

	const Money totalSystemMoney = bank.GetCash() + bank.GetAccountBalance(accA) + bank.GetAccountBalance(accB);
	EXPECT_EQ(totalSystemMoney, INITIAL_CASH);
}

TEST(BankConcurrentTests, ConcurrentAccountCreationAndTransfers)
{
	constexpr Money INITIAL_CASH = 50000;
	Bank bank(INITIAL_CASH);
	AccountId masterAcc = bank.OpenAccount();
	bank.DepositMoney(masterAcc, INITIAL_CASH);

	std::atomic_bool running{ true };
	std::vector<std::thread> threads;

	// Поток 1: Постоянно открывает и закрывает счета
	threads.emplace_back([&bank, &running] {
		for (int i = 0; i < 500; ++i)
		{
			const AccountId newAcc = bank.OpenAccount();
			std::this_thread::yield();
			bank.CloseAccount(newAcc);
		}
		running = false;
	});

	// Поток 2 и 3: Пытаются переводить деньги с masterAcc на несуществующие/существующие счета
	// Это проверяет корректность shared_mutex (остановка системы при добавлении аккаунтов)
	for (int i = 0; i < 2; ++i)
	{
		threads.emplace_back([&bank, masterAcc, &running]() {
			while (running)
			{
				try
				{
					constexpr AccountId DUMMY_ID = 99999;

					bank.SendMoney(masterAcc, DUMMY_ID, 1);
				}
				catch (const BankOperationError&)
				{
					// Ожидаемо, так как счет скорее всего не существует
				}
			}
		});
	}

	for (auto& t : threads)
	{
		t.join();
	}

	const Money finalSystemMoney = bank.GetCash() + bank.GetAccountBalance(masterAcc);
	EXPECT_EQ(finalSystemMoney, INITIAL_CASH);
}