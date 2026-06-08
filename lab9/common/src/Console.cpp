#include <Console.hpp>

#ifdef _WIN32

#define PP_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#else

#define PP_PLATFORM_POSIX

#endif

namespace pc::Console
{

void SetLocale(const Locale locale)
{
#ifdef PP_PLATFORM_WINDOWS
	static constexpr auto INVALID_CODEPAGE_ID = static_cast<UINT>(-1);

	UINT codePageId = INVALID_CODEPAGE_ID;
	switch (locale)
	{
	case Locale::UTF8:
		codePageId = CP_UTF8;
		break;
	case Locale::ANSI:
		codePageId = CP_ACP;
		break;
	default:
		break;
	}

	if (codePageId == INVALID_CODEPAGE_ID)
	{
		return;
	}

	SetConsoleCP(codePageId);
	SetConsoleOutputCP(codePageId);
#endif
}

} // namespace pc::Console