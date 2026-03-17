#pragma once

#include "Bank.hpp"

#include <chrono>

namespace settings
{

constexpr Money INITIAL_MONEY = 10000;

constexpr Money HOMER_SALARY = 50;
constexpr Money HOMER_WITHDRAW = 20;
constexpr Money HOMER_ELECTRICITY_BILL = 5;

constexpr Money GROCERY_COST = 10;
constexpr Money POCKET_MONEY = 5;
constexpr Money CANDIES_COST = 2;

constexpr Money APU_ELECTRICITY_BILL = 10;
constexpr Money APU_DEPOSIT = 10;

constexpr auto TICK_RATE_HOMER = std::chrono::milliseconds(10);
constexpr auto TICK_RATE_KIDS = std::chrono::milliseconds(12);
constexpr auto TICK_RATE_MARGE = std::chrono::milliseconds(15);
constexpr auto TICK_RATE_APU = std::chrono::milliseconds(20);
constexpr auto TICK_RATE_BURNS = std::chrono::milliseconds(50);

constexpr auto SIMULATION_DURATION = std::chrono::seconds(2);

} // namespace settings