#include "../pch.h"
#include "../HallOfFame.h"

#include <iostream>

int main()
{
	if (!AfxWinInit(GetModuleHandle(nullptr), nullptr, GetCommandLine(), 0))
		return 1;
	CString path;
	path.Format(_T("Software\\MikasMahjonggTest_%lu"), GetCurrentProcessId());
	HKEY existing = nullptr;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, path, 0, KEY_READ, &existing) != ERROR_FILE_NOT_FOUND)
	{
		if (existing) RegCloseKey(existing);
		std::cerr << "Test registry key already exists or is inaccessible\n";
		return 1;
	}
	bool ok = true;
	CString error;
	HallOfFame hall(std::wstring(path.GetString()));
	ok = hall.Load(error) && hall.Entries().empty();
	for (int i = 20; i >= 1; --i)
	{
		HallEntry entry;
		entry.name = L"Test \u00c4";
		entry.statistics.elapsedMilliseconds = i * 1000;
		entry.statistics.hints = i;
		entry.statistics.undos = i + 1;
		entry.statistics.reshuffles = i + 2;
		hall.Add(entry);
	}
	ok = ok && hall.Entries().size() == 10 && hall.Qualifies(9999) && !hall.Qualifies(10000);
	ok = ok && hall.Save(error);
	HallOfFame reloaded(std::wstring(path.GetString()));
	ok = ok && reloaded.Load(error) && reloaded.Entries().size() == 10;
	for (size_t i = 0; ok && i < reloaded.Entries().size(); ++i)
	{
		const auto& entry = reloaded.Entries()[i];
		ok = entry.name == L"Test \u00c4" &&
			entry.statistics.elapsedMilliseconds == (i + 1) * 1000 &&
			entry.statistics.hints == i + 1 &&
			entry.statistics.undos == i + 2 &&
			entry.statistics.reshuffles == i + 3;
	}
	reloaded.Clear();
	ok = ok && reloaded.Save(error);
	HallOfFame empty(std::wstring(path.GetString()));
	ok = ok && empty.Load(error) && empty.Entries().empty();
	HKEY key = nullptr;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, path, 0, KEY_SET_VALUE, &key) == ERROR_SUCCESS)
	{
		const DWORD invalid = 1;
		ok = ok && RegSetValueExW(key, L"Results", 0, REG_DWORD,
			reinterpret_cast<const BYTE*>(&invalid), sizeof(invalid)) == ERROR_SUCCESS;
		RegCloseKey(key);
		ok = ok && !empty.Load(error) && !error.IsEmpty();
	}
	else
		ok = false;
	if (RegDeleteKeyW(HKEY_CURRENT_USER, path) != ERROR_SUCCESS)
	{
		std::cerr << "Could not remove test registry key\n";
		return 1;
	}
	if (!ok)
	{
		std::cerr << "Hall of Fame test failed\n";
		return 1;
	}
	std::cout << "Hall of Fame ranking, persistence, clearing and corruption tests passed\n";
	return 0;
}
