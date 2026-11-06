#pragma once

#include "MahjongGame.h"
#include <string>
#include <vector>

struct HallEntry
{
	std::wstring name;
	GameStatistics statistics;
};

class HallOfFame
{
public:
	explicit HallOfFame(const std::wstring& registryPath = L"Software\\MikasMahjongg\\HallOfFame")
		: registryPath_(registryPath) {}
	bool Load(CString& error);
	bool Save(CString& error) const;
	bool Qualifies(std::uint64_t elapsedMilliseconds) const;
	void Add(const HallEntry& entry);
	void Clear() { entries_.clear(); }
	const std::vector<HallEntry>& Entries() const { return entries_; }

private:
	std::vector<HallEntry> entries_;
	std::wstring registryPath_;
};

bool AskHallName(CString& name, CWnd* parent);
void ShowHallOfFame(const HallOfFame& hall, CWnd* parent);
