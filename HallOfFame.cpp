#include "pch.h"
#include "HallOfFame.h"
#include "resource.h"
#include "Localization.h"

#include <algorithm>
#include <sstream>

namespace
{
	const wchar_t* const valueName = L"Results";

	void RegistryError(CString& error, LONG code)
	{
		error.Format(Tr(L"Hall of Fame registry error: %ld.", L"Kunniataulun rekisterivirhe: %ld."), code);
	}

	CString FormatTime(std::uint64_t milliseconds)
	{
		const auto seconds = milliseconds / 1000;
		CString value;
		value.Format(_T("%llu:%02llu"), seconds / 60, seconds % 60);
		return value;
	}

	class HallDialog : public CDialogEx
	{
	public:
		HallDialog(const HallOfFame& hall, CWnd* parent)
			: CDialogEx(IDD_HALL_OF_FAME, parent), hall_(hall) {}
	protected:
		BOOL OnInitDialog() override
		{
			CDialogEx::OnInitDialog();
			if (!list_.SubclassDlgItem(IDC_HALL_LIST, this))
			{
				AfxMessageBox(Tr(L"Could not create the Hall of Fame list.", L"Kunniataulua ei voitu luoda."), MB_ICONERROR);
				EndDialog(IDCANCEL);
				return FALSE;
			}
			list_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
			CRect bounds;
			list_.GetClientRect(&bounds);
			const int width = bounds.Width() - GetSystemMetrics(SM_CXVSCROLL);
			list_.InsertColumn(0, Tr(L"Name", L"Nimi"), LVCFMT_LEFT, width * 38 / 100);
			list_.InsertColumn(1, Tr(L"Help", L"Vihje"), LVCFMT_RIGHT, width * 12 / 100);
			list_.InsertColumn(2, Tr(L"Backup", L"Kumoa"), LVCFMT_RIGHT, width * 15 / 100);
			list_.InsertColumn(3, Tr(L"Shuffle", L"Sekoita"), LVCFMT_RIGHT, width * 15 / 100);
			list_.InsertColumn(4, Tr(L"Time", L"Aika"), LVCFMT_RIGHT, width * 20 / 100);
			int row = 0;
			for (const auto& entry : hall_.Entries())
			{
				list_.InsertItem(row, entry.name.c_str());
				CString text;
				text.Format(_T("%u"), entry.statistics.hints); list_.SetItemText(row, 1, text);
				text.Format(_T("%u"), entry.statistics.undos); list_.SetItemText(row, 2, text);
				text.Format(_T("%u"), entry.statistics.reshuffles); list_.SetItemText(row, 3, text);
				list_.SetItemText(row, 4, FormatTime(entry.statistics.elapsedMilliseconds));
				++row;
			}
			return TRUE;
		}
	private:
		const HallOfFame& hall_;
		CListCtrl list_;
	};

	class NameDialog : public CDialogEx
	{
	public:
		explicit NameDialog(CWnd* parent) : CDialogEx(IDD_HALL_NAME, parent) {}
		CString name;
	protected:
		BOOL OnInitDialog() override
		{
			CDialogEx::OnInitDialog();
			SendDlgItemMessage(IDC_HALL_NAME, EM_SETLIMITTEXT, 40);
			return TRUE;
		}
		void OnOK() override
		{
			GetDlgItemText(IDC_HALL_NAME, name);
			name.Trim();
			if (name.IsEmpty() || name.FindOneOf(_T("\r\n\t")) >= 0)
			{
				AfxMessageBox(Tr(L"Please enter a name.", L"Kirjoita nimi."), MB_ICONINFORMATION);
				return;
			}
			CDialogEx::OnOK();
		}
	};
}

bool HallOfFame::Load(CString& error)
{
	HKEY key;
	LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, registryPath_.c_str(), 0, KEY_QUERY_VALUE, &key);
	if (result == ERROR_FILE_NOT_FOUND)
	{
		entries_.clear();
		return true;
	}
	if (result != ERROR_SUCCESS) { RegistryError(error, result); return false; }
	DWORD type = 0, bytes = 0;
	result = RegQueryValueExW(key, valueName, nullptr, &type, nullptr, &bytes);
	if (result == ERROR_FILE_NOT_FOUND)
	{
		RegCloseKey(key);
		entries_.clear();
		return true;
	}
	if (result != ERROR_SUCCESS || type != REG_SZ || bytes < sizeof(wchar_t) ||
		bytes > 16384 || bytes % sizeof(wchar_t) != 0)
	{
		RegCloseKey(key);
		error = Tr(L"The Hall of Fame data is invalid or could not be read.", L"Kunniataulun tiedot ovat virheelliset tai niitä ei voitu lukea.");
		return false;
	}
	std::vector<wchar_t> buffer(bytes / sizeof(wchar_t));
	result = RegQueryValueExW(key, valueName, nullptr, &type,
		reinterpret_cast<BYTE*>(buffer.data()), &bytes);
	RegCloseKey(key);
	if (result != ERROR_SUCCESS || type != REG_SZ || buffer.back() != L'\0' ||
		std::find(buffer.begin(), buffer.end() - 1, L'\0') != buffer.end() - 1)
	{
		error = Tr(L"The Hall of Fame data is invalid or could not be read.", L"Kunniataulun tiedot ovat virheelliset tai niitä ei voitu lukea.");
		return false;
	}
	std::wistringstream stream(std::wstring(buffer.data(), buffer.size() - 1));
	std::vector<HallEntry> loaded;
	std::wstring line;
	while (std::getline(stream, line))
	{
		std::wistringstream row(line);
		HallEntry entry;
		if (!std::getline(row, entry.name, L'\t') || entry.name.empty() || entry.name.size() > 40 ||
			!(row >> entry.statistics.elapsedMilliseconds >> entry.statistics.hints >>
				entry.statistics.undos >> entry.statistics.reshuffles) ||
			entry.statistics.elapsedMilliseconds > 3155760000000ULL ||
			entry.statistics.hints > 1000000 || entry.statistics.undos > 1000000 ||
			entry.statistics.reshuffles > 1000000)
		{
			error = Tr(L"The Hall of Fame data is invalid.", L"Kunniataulun tiedot ovat virheelliset.");
			return false;
		}
		row >> std::ws;
		if (!row.eof() || loaded.size() == 10 ||
			(!loaded.empty() && loaded.back().statistics.elapsedMilliseconds > entry.statistics.elapsedMilliseconds))
		{
			error = Tr(L"The Hall of Fame data is invalid.", L"Kunniataulun tiedot ovat virheelliset.");
			return false;
		}
		loaded.push_back(entry);
	}
	entries_ = std::move(loaded);
	return true;
}

bool HallOfFame::Save(CString& error) const
{
	std::wostringstream stream;
	for (const auto& entry : entries_)
		stream << entry.name << L'\t' << entry.statistics.elapsedMilliseconds << L' '
			<< entry.statistics.hints << L' ' << entry.statistics.undos << L' '
			<< entry.statistics.reshuffles << L'\n';
	const std::wstring data = stream.str();
	HKEY key;
	LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, registryPath_.c_str(), 0, nullptr, 0,
		KEY_SET_VALUE, nullptr, &key, nullptr);
	if (result != ERROR_SUCCESS) { RegistryError(error, result); return false; }
	result = RegSetValueExW(key, valueName, 0, REG_SZ,
		reinterpret_cast<const BYTE*>(data.c_str()),
		static_cast<DWORD>((data.size() + 1) * sizeof(wchar_t)));
	RegCloseKey(key);
	if (result != ERROR_SUCCESS) { RegistryError(error, result); return false; }
	return true;
}

bool HallOfFame::Qualifies(std::uint64_t elapsedMilliseconds) const
{
	return entries_.size() < 10 || elapsedMilliseconds < entries_.back().statistics.elapsedMilliseconds;
}

void HallOfFame::Add(const HallEntry& entry)
{
	entries_.push_back(entry);
	std::stable_sort(entries_.begin(), entries_.end(), [](const HallEntry& first, const HallEntry& second) {
		return first.statistics.elapsedMilliseconds < second.statistics.elapsedMilliseconds;
	});
	if (entries_.size() > 10) entries_.resize(10);
}

bool AskHallName(CString& name, CWnd* parent)
{
	NameDialog dialog(parent);
	const INT_PTR result = dialog.DoModal();
	if (result == -1)
		AfxMessageBox(Tr(L"Could not open the Hall of Fame name dialog.", L"Kunniataulun nimikyselyä ei voitu avata."), MB_ICONERROR);
	if (result != IDOK) return false;
	name = dialog.name;
	return true;
}

void ShowHallOfFame(const HallOfFame& hall, CWnd* parent)
{
	HallDialog dialog(hall, parent);
	if (dialog.DoModal() == -1)
		AfxMessageBox(Tr(L"Could not open the Hall of Fame dialog.", L"Kunniataulua ei voitu avata."), MB_ICONERROR);
}
