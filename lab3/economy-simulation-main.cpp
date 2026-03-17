#include "Bank/Actors.hpp"
#include "Bank/Bank.hpp"
#include "Bank/Settings.hpp"

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

void ValidateEconomy(
	const Bank& bank,
	const AccountId homerAccount,
	const AccountId margeAccount,
	const AccountId burnsAccount,
	const AccountId apuAccount,
	const ActorCash& homerCash,
	const ActorCash& bartCash,
	const ActorCash& lisaCash,
	const ActorCash& apuCash)
{
	std::cout << "\n--- Итоги симуляции ---\n";
	std::cout << "Количество банковских операций: " << bank.GetOperationsCount() << "\n";

	const Money totalActorsCash = homerCash.cash + bartCash.cash + lisaCash.cash + apuCash.cash;
	const Money totalBankCash = bank.GetCash();
	const Money totalAccountsBalance = bank.GetAccountBalance(homerAccount)
		+ bank.GetAccountBalance(margeAccount)
		+ bank.GetAccountBalance(burnsAccount)
		+ bank.GetAccountBalance(apuAccount);

	std::cout << "Наличные у персонажей: " << totalActorsCash << "\n";
	std::cout << "Наличные в обороте (по данным банка): " << totalBankCash << "\n";
	assert(totalActorsCash == totalBankCash);

	const Money total = totalBankCash + totalAccountsBalance;
	std::cout << "Деньги на счетах: " << totalAccountsBalance << "\n";
	std::cout << "Всего в экономике: " << total << " (Ожидалось: " << settings::INITIAL_MONEY << ")\n";
	assert(total == settings::INITIAL_MONEY);

	std::cout << "Симуляция завершена корректно. Состояние согласовано.\n";
}

int main(int argc, char* argv[])
{
	SetConsoleOutputCP(CP_UTF8);

	bool enableLogging = true;
	for (int i = 1; i < argc; ++i)
	{
		if (std::string arg = argv[i]; arg == "--no-log")
		{
			enableLogging = false;
		}
	}

	logger.SetEnabled(enableLogging);

	std::cout << "Запуск симуляции. Нажмите Ctrl+C для остановки или подождите 2 секунды...\n";

	Bank bank(settings::INITIAL_MONEY);

	auto homerAccount = bank.OpenAccount();
	auto margeAccount = bank.OpenAccount();
	auto burnsAccount = bank.OpenAccount();
	auto apuAccount = bank.OpenAccount();

	bank.DepositMoney(burnsAccount, settings::INITIAL_MONEY / 2);

	ActorCash homerCash("Homer", settings::INITIAL_MONEY / 2);
	ActorCash bartCash("Bart", 0);
	ActorCash lisaCash("Lisa", 0);
	ActorCash apuCash("Apu", 0);

	std::vector<std::jthread> actors;

	actors.emplace_back(HomerLogic, std::ref(bank), homerAccount, margeAccount, burnsAccount, std::ref(homerCash), std::ref(bartCash), std::ref(lisaCash));
	actors.emplace_back(MargeLogic, std::ref(bank), margeAccount, apuAccount);
	actors.emplace_back(KidsLogic, std::ref(bartCash), std::ref(apuCash));
	actors.emplace_back(KidsLogic, std::ref(lisaCash), std::ref(apuCash));
	actors.emplace_back(ApuLogic, std::ref(bank), apuAccount, burnsAccount, std::ref(apuCash));
	actors.emplace_back(BurnsLogic, std::ref(bank), burnsAccount, homerAccount);

	std::this_thread::sleep_for(std::chrono::seconds(settings::SIMULATION_DURATION));

	ValidateEconomy(bank, homerAccount, margeAccount, burnsAccount, apuAccount, homerCash, bartCash, lisaCash, apuCash);

	return 0;
}