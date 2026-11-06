
// MikasMahjonggDoc.cpp : implementation of the CMikasMahjonggDoc class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "MikasMahjongg.h"
#endif

#include "MikasMahjonggDoc.h"
#include "Localization.h"

#include <propkey.h>
#include <sstream>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMikasMahjonggDoc

IMPLEMENT_DYNCREATE(CMikasMahjonggDoc, CDocument)

BEGIN_MESSAGE_MAP(CMikasMahjonggDoc, CDocument)
END_MESSAGE_MAP()


// CMikasMahjonggDoc construction/destruction

CMikasMahjonggDoc::CMikasMahjonggDoc() noexcept
{
	// TODO: add one-time construction code here

}

CMikasMahjonggDoc::~CMikasMahjonggDoc()
{
}

BOOL CMikasMahjonggDoc::OnNewDocument()
{
	MahjongGame game;
	if (!game.NewGame())
	{
		AfxMessageBox(Tr(L"Could not generate a solvable game. Please try again.", L"Ratkaistavaa peliä ei voitu luoda. Yritä uudelleen."), MB_ICONERROR);
		return FALSE;
	}
	if (!CDocument::OnNewDocument())
		return FALSE;
	SetTitle(Tr(L"Untitled", L"Nimetön"));
	m_game = std::move(game);
	m_statistics = {};
	m_ready = true;
	m_clockRunning = false;
	ResumeClock();
	SetModifiedFlag(TRUE);
	return TRUE;
}

GameStatistics CMikasMahjonggDoc::Statistics() const
{
	GameStatistics result = m_statistics;
	if (m_clockRunning)
		result.elapsedMilliseconds += static_cast<std::uint64_t>(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - m_clockStart).count());
	return result;
}

void CMikasMahjonggDoc::PauseClock()
{
	if (m_clockRunning)
	{
		m_statistics = Statistics();
		m_clockRunning = false;
	}
}

void CMikasMahjonggDoc::ResumeClock()
{
	if (m_ready && !m_clockRunning && m_game.Remaining() > 0)
	{
		m_clockStart = std::chrono::steady_clock::now();
		m_clockRunning = true;
	}
}

void CMikasMahjonggDoc::RestartGame()
{
	m_game.Restart();
	m_statistics = {};
	m_clockRunning = false;
	ResumeClock();
	SetModifiedFlag(TRUE);
	UpdateAllViews(nullptr);
}

void CMikasMahjonggDoc::RecordHint()
{
	++m_statistics.hints;
	SetModifiedFlag(TRUE);
}

void CMikasMahjonggDoc::RecordUndo()
{
	++m_statistics.undos;
	SetModifiedFlag(TRUE);
	ResumeClock();
}

void CMikasMahjonggDoc::RecordReshuffle()
{
	++m_statistics.reshuffles;
	SetModifiedFlag(TRUE);
}

BOOL CMikasMahjonggDoc::SaveModified()
{
	if (!m_ready || m_game.Remaining() == 0)
		return TRUE;
	const int answer = AfxMessageBox(
		Tr(L"Save the unfinished game before continuing?", L"Tallennetaanko keskeneräinen peli ennen jatkamista?"),
		MB_YESNOCANCEL | MB_ICONQUESTION);
	if (answer == IDCANCEL)
		return FALSE;
	if (answer == IDNO)
		return TRUE;
	return answer == IDYES && DoFileSave();
}

BOOL CMikasMahjonggDoc::DoSave(LPCTSTR path, BOOL replace)
{
	CString name = path;
	if (name.IsEmpty())
	{
		CFileDialog dialog(FALSE, _T("mahjongg"), nullptr,
			OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,
			Tr(L"Mahjongg games (*.mahjongg)|*.mahjongg|All files (*.*)|*.*||", L"Mahjongg-pelit (*.mahjongg)|*.mahjongg|Kaikki tiedostot (*.*)|*.*||"));
		if (dialog.DoModal() != IDOK)
			return FALSE;
		name = dialog.GetPathName();
	}
	if (!OnSaveDocument(name))
		return FALSE;
	if (replace)
		SetPathName(name);
	return TRUE;
}

BOOL CMikasMahjonggDoc::OnSaveDocument(LPCTSTR path)
{
	std::ostringstream stream;
	if (!m_ready || !m_game.Save(stream, Statistics()))
	{
		AfxMessageBox(Tr(L"Could not prepare the game for saving.", L"Peliä ei voitu valmistella tallennettavaksi."), MB_ICONERROR);
		return FALSE;
	}
	const std::string data = stream.str();
	std::vector<wchar_t> fullPath(32768);
	wchar_t tempPath[MAX_PATH] = {};
	const DWORD length = GetFullPathNameW(path, static_cast<DWORD>(fullPath.size()), fullPath.data(), nullptr);
	if (length == 0 || length >= fullPath.size())
	{
		AfxMessageBox(Tr(L"Could not resolve the save path.", L"Tallennuspolkua ei voitu selvittää."), MB_ICONERROR);
		return FALSE;
	}
	CString directory(fullPath.data());
	const int separator = directory.ReverseFind(L'\\');
	if (separator < 0 || !GetTempFileNameW(directory.Left(separator + 1), L"mjg", 0, tempPath))
	{
		AfxMessageBox(Tr(L"Could not create a temporary save file in the destination folder.", L"Väliaikaista tallennustiedostoa ei voitu luoda kohdekansioon."), MB_ICONERROR);
		return FALSE;
	}
	HANDLE file = CreateFileW(tempPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	DWORD written = 0;
	bool success = file != INVALID_HANDLE_VALUE;
	DWORD error = success ? ERROR_SUCCESS : GetLastError();
	if (success)
	{
		if (!WriteFile(file, data.data(), static_cast<DWORD>(data.size()), &written, nullptr))
		{
			success = false;
			error = GetLastError();
		}
		else if (written != data.size())
		{
			success = false;
			error = ERROR_WRITE_FAULT;
		}
		else if (!FlushFileBuffers(file))
		{
			success = false;
			error = GetLastError();
		}
		if (!CloseHandle(file) && success)
		{
			success = false;
			error = GetLastError();
		}
	}
	if (success && !MoveFileExW(tempPath, fullPath.data(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
	{
		success = false;
		error = GetLastError();
	}
	if (!success)
	{
		if (!DeleteFileW(tempPath))
			TRACE(_T("Could not remove temporary save file %s (error %lu).\n"), tempPath, GetLastError());
		CString message;
		message.Format(Tr(L"Could not save the game (Windows error %lu).", L"Peliä ei voitu tallentaa (Windows-virhe %lu)."), error);
		AfxMessageBox(message, MB_ICONERROR);
		return FALSE;
	}
	SetModifiedFlag(FALSE);
	return TRUE;
}

BOOL CMikasMahjonggDoc::OnOpenDocument(LPCTSTR path)
{
	CFile file;
	CFileException error;
	if (!file.Open(path, CFile::modeRead | CFile::shareDenyWrite, &error))
	{
		CString message;
		message.Format(Tr(L"Could not open game file %s (Windows error %ld).",
			L"Pelitiedostoa %s ei voitu avata (Windows-virhe %ld)."), path, error.m_lOsError);
		AfxMessageBox(message, MB_ICONERROR);
		return FALSE;
	}
	std::string data;
	try
	{
		const ULONGLONG length = file.GetLength();
		if (length == 0 || length > 65536)
		{
			AfxMessageBox(Tr(L"Invalid game file size.", L"Pelitiedoston koko on virheellinen."), MB_ICONERROR);
			return FALSE;
		}
		data.resize(static_cast<size_t>(length));
		if (file.Read(&data[0], static_cast<UINT>(length)) != length)
		{
			AfxMessageBox(Tr(L"The game file could not be read completely.", L"Pelitiedostoa ei voitu lukea kokonaan."), MB_ICONERROR);
			return FALSE;
		}
		file.Close();
	}
	catch (CFileException* exception)
	{
		CString message;
		message.Format(Tr(L"Could not read game file %s (Windows error %ld).",
			L"Pelitiedostoa %s ei voitu lukea (Windows-virhe %ld)."), path, exception->m_lOsError);
		AfxMessageBox(message, MB_ICONERROR);
		exception->Delete();
		return FALSE;
	}
	MahjongGame game;
	GameStatistics statistics;
	std::istringstream input(data);
	if (!game.Load(input, statistics))
	{
		AfxMessageBox(Tr(L"The game file is invalid or unsupported.", L"Pelitiedosto on virheellinen tai sen muotoa ei tueta."), MB_ICONERROR);
		return FALSE;
	}
	m_game = std::move(game);
	m_statistics = statistics;
	m_ready = true;
	m_clockRunning = false;
	ResumeClock();
	SetModifiedFlag(FALSE);
	return TRUE;
}



// CMikasMahjonggDoc serialization

void CMikasMahjonggDoc::Serialize(CArchive& ar)
{
	AfxThrowNotSupportedException();
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CMikasMahjonggDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CMikasMahjonggDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data.
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CMikasMahjonggDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = nullptr;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != nullptr)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CMikasMahjonggDoc diagnostics

#ifdef _DEBUG
void CMikasMahjonggDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CMikasMahjonggDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CMikasMahjonggDoc commands
