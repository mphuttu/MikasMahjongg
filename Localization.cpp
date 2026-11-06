#include "pch.h"
#include "Localization.h"

#include <vector>

namespace
{
	bool finnish = false;
}

bool ConfigureLanguage()
{
	std::vector<wchar_t> module(32768);
	const DWORD length = GetModuleFileNameW(nullptr, module.data(), static_cast<DWORD>(module.size()));
	if (length == 0 || length >= module.size())
	{
		AfxMessageBox(L"Could not locate the language configuration.", MB_ICONERROR);
		return false;
	}
	CString path(module.data());
	const int separator = path.ReverseFind(L'\\');
	if (separator < 0)
	{
		AfxMessageBox(L"Could not locate the language configuration.", MB_ICONERROR);
		return false;
	}
	path = path.Left(separator + 1) + L"language.ini";
	const DWORD attributes = GetFileAttributesW(path);
	if (attributes == INVALID_FILE_ATTRIBUTES)
	{
		const DWORD error = GetLastError();
		if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
		{
			AfxMessageBox(L"Could not read the language configuration.", MB_ICONERROR);
			return false;
		}
	}
	else
	{
		wchar_t language[16] = {};
		GetPrivateProfileStringW(L"Language", L"Code", L"", language, _countof(language), path);
		if (wcscmp(language, L"fi") == 0)
			finnish = true;
		else if (wcscmp(language, L"en") != 0)
		{
			AfxMessageBox(L"Invalid language.ini: Code must be en or fi.", MB_ICONERROR);
			return false;
		}
	}
	const LANGID language = finnish ? MAKELANGID(LANG_FINNISH, SUBLANG_DEFAULT) :
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
	if (!SetThreadUILanguage(language))
	{
		AfxMessageBox(Tr(L"Could not set the application language.",
			L"Sovelluksen kielen asettaminen epäonnistui."), MB_ICONERROR);
		return false;
	}
	return true;
}

bool IsFinnish()
{
	return finnish;
}

const wchar_t* Tr(const wchar_t* english, const wchar_t* finnishText)
{
	return finnish ? finnishText : english;
}
