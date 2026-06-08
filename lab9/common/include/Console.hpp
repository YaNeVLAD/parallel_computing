#pragma once

namespace pc
{

enum class Locale
{
	ANSI,
	UTF8,
};

namespace Console
{

void SetLocale(Locale locale);

}

} // namespace pp