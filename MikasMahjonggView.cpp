
// MikasMahjonggView.cpp : implementation of the CMikasMahjonggView class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "MikasMahjongg.h"
#endif

#include "MikasMahjonggDoc.h"
#include "MikasMahjonggView.h"
#include "BoardGeometry.h"
#include "HallOfFame.h"
#include "Localization.h"

#include <cmath>
#include <shellapi.h>
#include <vector>
#include <wincodec.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "d2d1.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMikasMahjonggView

IMPLEMENT_DYNCREATE(CMikasMahjonggView, CView)

BEGIN_MESSAGE_MAP(CMikasMahjonggView, CView)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_REGISTERED_MESSAGE(AFX_WM_DRAW2D, &CMikasMahjonggView::OnDraw2D)
	ON_REGISTERED_MESSAGE(AFX_WM_RECREATED2DRESOURCES, &CMikasMahjonggView::OnRecreate2DResources)
	ON_WM_LBUTTONDOWN()
	ON_WM_TIMER()
	ON_COMMAND(ID_FILE_OPEN, &CMikasMahjonggView::OnOpenGame)
	ON_COMMAND(ID_FILE_RESTARTGAME, &CMikasMahjonggView::OnRestartGame)
	ON_COMMAND(ID_EDIT_UNDO, &CMikasMahjonggView::OnUndoMove)
	ON_COMMAND(ID_BACKUP_BACKUP, &CMikasMahjonggView::OnUndoMove)
	ON_COMMAND(ID_HELP_SHOWMOVES, &CMikasMahjonggView::OnHint)
	ON_COMMAND(ID_HELP_RESUFFLE, &CMikasMahjonggView::OnReshuffle)
	ON_COMMAND(ID_HELP_TILESLEFT, &CMikasMahjonggView::OnTilesLeft)
	ON_COMMAND(ID_HELP_HELPCONTENTS, &CMikasMahjonggView::OnHelpContents)
	ON_COMMAND(ID_SPECIAL_VIEWHALLOFFAME, &CMikasMahjonggView::OnShowHallOfFame)
	ON_COMMAND(ID_SPECIAL_CLEARHALLOFFAME, &CMikasMahjonggView::OnClearHallOfFame)
	ON_COMMAND(ID_SPECIAL_PAUSE, &CMikasMahjonggView::OnPauseGame)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, &CMikasMahjonggView::OnUpdateGameCommand)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, &CMikasMahjonggView::OnUpdateGameCommand)
	ON_UPDATE_COMMAND_UI(ID_FILE_RESTARTGAME, &CMikasMahjonggView::OnUpdateGameCommand)
	ON_UPDATE_COMMAND_UI(ID_SPECIAL_PAUSE, &CMikasMahjonggView::OnUpdateGameCommand)
	ON_UPDATE_COMMAND_UI(IDS_STATUS_PANE1, &CMikasMahjonggView::OnUpdateGameStatus)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT, &CMikasMahjonggView::OnUpdateUnavailable)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_PREVIEW, &CMikasMahjonggView::OnUpdateUnavailable)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_SETUP, &CMikasMahjonggView::OnUpdateUnavailable)
	ON_UPDATE_COMMAND_UI(ID_FILE_NEWNUMBEREDGAME, &CMikasMahjonggView::OnUpdateUnavailable)
	ON_UPDATE_COMMAND_UI(ID_FILE_DONE, &CMikasMahjonggView::OnUpdateUnavailable)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PREFERENCES_MOVECHECK, ID_PREFERENCES_FULLSCREEN, &CMikasMahjonggView::OnUpdateUnavailable)
	ON_UPDATE_COMMAND_UI_RANGE(ID_BACKUP_SETMARK, ID_GOTOMARK_4, &CMikasMahjonggView::OnUpdateUnavailable)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CMikasMahjonggView::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

namespace
{
	bool GetExecutableDirectory(CString& directory)
	{
		std::vector<wchar_t> module(32768);
		const DWORD length = GetModuleFileNameW(nullptr, module.data(), static_cast<DWORD>(module.size()));
		if (length == 0 || length >= module.size())
			return false;
		directory = module.data();
		const int separator = directory.ReverseFind(L'\\');
		if (separator < 0)
			return false;
		directory = directory.Left(separator + 1);
		return true;
	}
}

// CMikasMahjonggView construction/destruction

CMikasMahjonggView::CMikasMahjonggView() noexcept
{
	// TODO: add construction code here

}

CMikasMahjonggView::~CMikasMahjonggView()
{
}

int CMikasMahjonggView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	EnableD2DSupport();  // Käynnistää Direct2D-tuen, jotta voimme käyttää Direct2D:tä piirtämiseen

	if (!SetTimer(1, 1000, nullptr))
	{
		AfxMessageBox(Tr(L"Could not start the game status timer.", L"Pelin tilan ajastinta ei voitu käynnistää."), MB_ICONERROR);
		return -1;
	}
	return 0;
}

BOOL CMikasMahjonggView::OnEraseBkgnd(CDC* pDC)
{
	// Estää taustan tyhjentämisen, jotta vältetään välkkymistä
	return TRUE;
}

LRESULT CMikasMahjonggView::OnDraw2D(WPARAM wParam, LPARAM lParam)
{
	auto* pRT = reinterpret_cast<CHwndRenderTarget*>(lParam);
	ASSERT_VALID(pRT);
	auto* target = pRT->GetRenderTarget();
	target->Clear(D2D1::ColorF(0.0f, 0.4f, 0.2f));
	if (m_paused)
	{
		DrawPause(target);
		return TRUE;
	}
	if (m_celebrating)
	{
		DrawCelebration(target);
		return TRUE;
	}
	if (!GetDocument()->IsGameReady() || m_loadFailed)
		return TRUE;
	if (!m_bitmaps[0] && !LoadTileImages(target))
	{
		m_loadFailed = true;
		AfxMessageBox(Tr(L"Tile images could not be loaded. Check that res\\tiles is next to the executable.",
			L"Laattakuvia ei voitu ladata. Tarkista, että res\\tiles on ohjelman vieressä."), MB_ICONERROR);
		return TRUE;
	}
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> outline, shadow, edge;
	if (FAILED(target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Gold), &outline)) ||
		FAILED(target->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.32f), &shadow)) ||
		FAILED(target->CreateSolidColorBrush(D2D1::ColorF(0.30f, 0.20f, 0.16f, 0.55f), &edge)))
	{
		m_loadFailed = true;
		AfxMessageBox(Tr(L"Could not create the Direct2D brushes.", L"Direct2D-piirtosiveltimiä ei voitu luoda."), MB_ICONERROR);
		return TRUE;
	}
	const auto size = target->GetSize();
	const auto& tiles = GetDocument()->Game().Tiles();
	const BoardGeometry geometry(size.width, size.height);
	for (int z = 0; z <= 4; ++z)
	{
		if (z > 0)
		{
			const float offset = 4.0f * geometry.Scale();
			for (const Tile& tile : tiles)
			{
				if (!tile.present || tile.slot.z != z)
					continue;
				auto rect = TileRect(tile.slot, size);
				rect.left -= offset;
				rect.right -= offset;
				rect.top += offset;
				rect.bottom += offset;
				target->FillRectangle(rect, shadow.Get());
			}
		}
		for (int i = 0; i < MahjongGame::TileCount; ++i)
		{
			const Tile& tile = tiles[i];
			if (!tile.present || tile.slot.z != z)
				continue;
			const BoardRect image = geometry.TileImageRect(tile.slot);
			target->DrawBitmap(m_bitmaps[tile.face].Get(),
				D2D1::RectF(image.left, image.top, image.right, image.bottom));
		}
		for (int i = 0; i < MahjongGame::TileCount; ++i)
		{
			const Tile& tile = tiles[i];
			if (!tile.present || tile.slot.z != z)
				continue;
			const auto rect = TileRect(tile.slot, size);
			const D2D1_SIZE_F bitmapSize = m_bitmaps[tile.face]->GetSize();
			const auto face = D2D1::RectF(bitmapSize.width * BoardGeometry::FaceLeft,
				bitmapSize.height * BoardGeometry::FaceTop,
				bitmapSize.width * BoardGeometry::FaceRight,
				bitmapSize.height * BoardGeometry::FaceBottom);
			target->DrawBitmap(m_bitmaps[tile.face].Get(), rect, 1.0f,
				D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, face);
			target->DrawRectangle(rect, edge.Get(), 0.7f * geometry.Scale());
		}
		for (int i = 0; i < MahjongGame::TileCount; ++i)
		{
			const Tile& tile = tiles[i];
			if (!tile.present || tile.slot.z != z)
				continue;
			if (i == m_selected || i == m_hint.first || i == m_hint.second)
				target->DrawRectangle(TileRect(tile.slot, size), outline.Get(), 3.0f);
		}
	}
	return TRUE;
}

LRESULT CMikasMahjonggView::OnRecreate2DResources(WPARAM wParam, LPARAM lParam)
{
	for (auto& bitmap : m_bitmaps)
		bitmap.Reset();
	m_loadFailed = false;
	return 0;
}

void CMikasMahjonggView::OnInitialUpdate()
{
	CView::OnInitialUpdate();
	ClearSelection();
	Invalidate(FALSE);
}

bool CMikasMahjonggView::LoadTileImages(ID2D1RenderTarget* target)
{
	static const wchar_t* names[] = {
		L"bamboo1.png", L"bamboo2.png", L"bamboo3.png", L"bamboo4.png", L"bamboo5.png",
		L"bamboo6.png", L"bamboo7.png", L"bamboo8.png", L"bamboo9.png",
		L"circle1.png", L"circle2.png", L"circle3.png", L"circle4.png", L"circle5.png",
		L"circle6.png", L"circle7.png", L"circle8.png", L"circle9.png",
		L"pinyin7.png", L"pinyin8.png", L"pinyin9.png", L"pinyin10.png", L"pinyin11.png",
		L"pinyin12.png", L"pinyin13.png", L"pinyin14.png", L"pinyin15.png",
		L"pinyin1.png", L"pinyin2.png", L"Blank-dragon-pink.png",
		L"pinyin3.png", L"pinyin4.png", L"pinyin5.png", L"pinyin6.png",
		L"chrysanthemum.png", L"lotus.png", L"orchid.png", L"peony.png",
		L"fall.png", L"spring.png", L"summer.png", L"winter.png"
	};
	static_assert(_countof(names) == MahjongGame::FaceCount, "Every tile face needs an image");
	CString directory;
	if (!GetExecutableDirectory(directory))
		return false;
	directory += L"res\\tiles\\";

	Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&factory));
	for (int i = 0; SUCCEEDED(hr) && i < MahjongGame::FaceCount; ++i)
	{
		Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
		Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
		Microsoft::WRL::ComPtr<IWICBitmapScaler> scaler;
		Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
		hr = factory->CreateDecoderFromFilename(directory + names[i], nullptr, GENERIC_READ,
			WICDecodeMetadataCacheOnLoad, &decoder);
		if (SUCCEEDED(hr)) hr = decoder->GetFrame(0, &frame);
		if (SUCCEEDED(hr)) hr = factory->CreateBitmapScaler(&scaler);
		if (SUCCEEDED(hr)) hr = scaler->Initialize(frame.Get(), 124, 172, WICBitmapInterpolationModeFant);
		if (SUCCEEDED(hr)) hr = factory->CreateFormatConverter(&converter);
		if (SUCCEEDED(hr)) hr = converter->Initialize(scaler.Get(), GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom);
		if (SUCCEEDED(hr)) hr = target->CreateBitmapFromWicBitmap(converter.Get(), nullptr, &m_bitmaps[i]);
		if (FAILED(hr))
			TRACE(_T("Failed to load tile image %s (HRESULT 0x%08X)\n"), names[i], hr);
	}
	if (FAILED(hr))
	{
		for (auto& bitmap : m_bitmaps)
			bitmap.Reset();
		return false;
	}
	return true;
}

D2D1_RECT_F CMikasMahjonggView::TileRect(const Slot& slot, D2D1_SIZE_F size) const
{
	const auto rect = BoardGeometry(size.width, size.height).TileRect(slot);
	return D2D1::RectF(rect.left, rect.top, rect.right, rect.bottom);
}

void CMikasMahjonggView::ClearSelection()
{
	m_selected = -1;
	m_hint = { -1, -1 };
}

void CMikasMahjonggView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CView::OnLButtonDown(nFlags, point);
	if (!GetDocument()->IsGameReady() || m_loadFailed || m_paused)
		return;
	auto* renderTarget = GetRenderTarget();
	if (!renderTarget || !renderTarget->IsValid())
		return;
	const auto size = renderTarget->GetRenderTarget()->GetSize();
	const float dpiScale = 96.0f / static_cast<float>(GetDpiForWindow(GetSafeHwnd()));
	const float x = point.x * dpiScale, y = point.y * dpiScale;
	auto& game = GetDocument()->Game();
	const int i = BoardGeometry(size.width, size.height).HitTest(game.Tiles(), x, y);
	if (i >= 0)
	{
		m_hint = { -1, -1 };
		if (game.IsFree(i))
		{
			if (i == m_selected)
				m_selected = -1;
			else if (m_selected >= 0 && game.Remove(m_selected, i))
			{
				GetDocument()->SetModifiedFlag(TRUE);
				ClearSelection();
				Invalidate(FALSE);
				ReportPosition();
				return;
			}
			else
				m_selected = i;
		}
		Invalidate(FALSE);
	}
}

void CMikasMahjonggView::OnOpenGame()
{
	CFileDialog dialog(TRUE, _T("mahjongg"), nullptr, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		Tr(L"Mahjongg games (*.mahjongg)|*.mahjongg|All files (*.*)|*.*||", L"Mahjongg-pelit (*.mahjongg)|*.mahjongg|Kaikki tiedostot (*.*)|*.*||"));
	if (dialog.DoModal() == IDOK)
		AfxGetApp()->OpenDocumentFile(dialog.GetPathName());
}

void CMikasMahjonggView::OnRestartGame()
{
	if (!GetDocument()->IsGameReady() || !GetDocument()->SaveModified())
		return;
	GetDocument()->RestartGame();
	ClearSelection();
	Invalidate(FALSE);
}

void CMikasMahjonggView::OnUndoMove()
{
	if (GetDocument()->IsGameReady() && GetDocument()->Game().Undo())
	{
		GetDocument()->RecordUndo();
		ClearSelection();
		Invalidate(FALSE);
	}
}

void CMikasMahjonggView::OnHint()
{
	if (!GetDocument()->IsGameReady()) return;
	ClearSelection();
	if (!GetDocument()->Game().Hint(m_hint))
		AfxMessageBox(Tr(L"No matching free tiles remain.", L"Yhtään vapaata laattaparia ei ole jäljellä."), MB_ICONINFORMATION);
	else
		GetDocument()->RecordHint();
	Invalidate(FALSE);
}

void CMikasMahjonggView::OnReshuffle()
{
	if (!GetDocument()->IsGameReady() || GetDocument()->Game().Remaining() == 0) return;
	if (!GetDocument()->Game().Reshuffle())
	{
		AfxMessageBox(Tr(L"Could not find a solvable shuffle for this position. Undo moves and try again.",
			L"Ratkaistavaa sekoitusta ei löytynyt. Kumoa siirtoja ja yritä uudelleen."), MB_ICONWARNING);
		return;
	}
	GetDocument()->RecordReshuffle();
	ClearSelection();
	Invalidate(FALSE);
}

void CMikasMahjonggView::OnTilesLeft()
{
	CString message;
	message.Format(Tr(L"Tiles left: %d", L"Laattoja jäljellä: %d"), GetDocument()->Game().Remaining());
	AfxMessageBox(message, MB_ICONINFORMATION);
}

void CMikasMahjonggView::OnHelpContents()
{
	CString directory;
	if (!GetExecutableDirectory(directory))
	{
		AfxMessageBox(Tr(L"Could not locate the help file directory.", L"Ohjetiedoston kansiota ei löytynyt."), MB_ICONERROR);
		return;
	}
	const CString path = directory + (IsFinnish() ? L"Ohje-fi.chm" : L"Help-en.chm");
	const DWORD attributes = GetFileAttributes(path);
	if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		CString message;
		message.Format(Tr(L"Help file not found: %s", L"Ohjetiedostoa ei löytynyt: %s"), path.GetString());
		AfxMessageBox(message, MB_ICONERROR);
		return;
	}
	const INT_PTR result = reinterpret_cast<INT_PTR>(
		ShellExecuteW(GetSafeHwnd(), L"open", path, nullptr, nullptr, SW_SHOWNORMAL));
	if (result <= 32)
	{
		CString message;
		message.Format(Tr(L"Could not open help file: %s (error %Id).", L"Ohjetiedostoa ei voitu avata: %s (virhe %Id)."), path.GetString(), result);
		AfxMessageBox(message, MB_ICONERROR);
	}
}

void CMikasMahjonggView::ReportPosition()
{
	if (GetDocument()->Game().Remaining() == 0)
	{
		GetDocument()->PauseClock();
		m_celebrating = true;
		m_celebrationRenderError = false;
		m_celebrationStart = std::chrono::steady_clock::now();
		if (!SetTimer(3, 33, nullptr))
		{
			m_celebrating = false;
			AfxMessageBox(Tr(L"Could not start the celebration animation.", L"Voittoanimaatiota ei voitu käynnistää."), MB_ICONERROR);
			FinishVictory();
			return;
		}
		GetParentFrame()->EnableWindow(FALSE);
		Invalidate(FALSE);
	}
	else
	{
		MahjongGame::Pair pair;
		if (!GetDocument()->Game().Hint(pair) &&
			AfxMessageBox(Tr(L"No moves left. Restart this game?", L"Siirtoja ei ole jäljellä. Aloitetaanko sama peli alusta?"), MB_YESNO | MB_ICONQUESTION) == IDYES)
			OnRestartGame();
	}
}

void CMikasMahjonggView::FinishVictory()
{
	if (!GetDocument()->IsGameReady() || GetDocument()->Game().Remaining() != 0)
		return;
	RecordWinningScore();
	if (AfxMessageBox(Tr(L"Congratulations! Play a new game?", L"Onneksi olkoon! Pelataanko uusi peli?"),
		MB_YESNO | MB_ICONQUESTION) == IDYES)
		GetDocument()->GetDocTemplate()->OpenDocumentFile(nullptr);
	else if (AfxMessageBox(Tr(L"Exit the game?", L"Lopetetaanko peli?"), MB_YESNO | MB_ICONQUESTION) == IDYES)
		AfxGetMainWnd()->PostMessage(WM_CLOSE);
}

void CMikasMahjonggView::RecordWinningScore()
{
	if (GetDocument()->IsScoreRecorded()) return;
	HallOfFame hall;
	CString error;
	if (!hall.Load(error))
	{
		AfxMessageBox(error, MB_ICONERROR);
		return;
	}
	const GameStatistics stats = GetDocument()->Statistics();
	if (!hall.Qualifies(stats.elapsedMilliseconds))
		return;
	CString name;
	if (!AskHallName(name, this)) return;
	hall.Add({ std::wstring(name.GetString()), stats });
	if (!hall.Save(error))
	{
		AfxMessageBox(error, MB_ICONERROR);
		return;
	}
	GetDocument()->SetScoreRecorded();
	ShowHallOfFame(hall, this);
}

void CMikasMahjonggView::OnShowHallOfFame()
{
	HallOfFame hall;
	CString error;
	if (!hall.Load(error))
		AfxMessageBox(error, MB_ICONERROR);
	else
		ShowHallOfFame(hall, this);
}

void CMikasMahjonggView::OnClearHallOfFame()
{
	if (AfxMessageBox(Tr(L"Clear all Hall of Fame results?", L"Tyhjennetäänkö kaikki kunniataulun tulokset?"),
		MB_YESNO | MB_ICONQUESTION) != IDYES) return;
	HallOfFame hall;
	CString error;
	hall.Clear();
	if (!hall.Save(error))
		AfxMessageBox(error, MB_ICONERROR);
}

void CMikasMahjonggView::OnPauseGame()
{
	if (!GetDocument()->IsGameReady() || GetDocument()->Game().Remaining() == 0)
		return;
	GetDocument()->PauseClock();
	m_paused = true;
	m_pauseRenderError = false;
	m_pauseFrame = 0;
	if (!SetTimer(2, 40, nullptr))
	{
		m_paused = false;
		GetDocument()->ResumeClock();
		AfxMessageBox(Tr(L"Could not start the pause animation.", L"Taukoanimaatiota ei voitu käynnistää."), MB_ICONERROR);
		return;
	}
	Invalidate(FALSE);
	UpdateWindow();
	AfxMessageBox(Tr(L"Press OK to continue.", L"Paina OK, kun haluat jatkaa."), MB_OK | MB_ICONINFORMATION);
	KillTimer(2);
	m_paused = false;
	GetDocument()->ResumeClock();
	Invalidate(FALSE);
}

void CMikasMahjonggView::OnTimer(UINT_PTR timer)
{
	if (timer == 3 && m_celebrating)
	{
		if (std::chrono::steady_clock::now() - m_celebrationStart >= std::chrono::seconds(6))
		{
			KillTimer(3);
			m_celebrating = false;
			GetParentFrame()->EnableWindow(TRUE);
			Invalidate(FALSE);
			FinishVictory();
		}
		else
			Invalidate(FALSE);
	}
	else if (timer == 2 && m_paused)
	{
		++m_pauseFrame;
		Invalidate(FALSE);
	}
	else if (timer == 1 && GetParentFrame())
		GetParentFrame()->SendMessage(WM_IDLEUPDATECMDUI, TRUE, 0);
	CView::OnTimer(timer);
}

void CMikasMahjonggView::DrawPause(ID2D1RenderTarget* target)
{
	target->Clear(D2D1::ColorF(D2D1::ColorF::Black));
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
	if (FAILED(target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush)))
	{
		if (!m_pauseRenderError)
		{
			m_pauseRenderError = true;
			AfxMessageBox(Tr(L"Could not draw the pause animation.", L"Taukoanimaatiota ei voitu piirtää."), MB_ICONERROR);
		}
		return;
	}
	const auto size = target->GetSize();
	for (int trail = 7; trail >= 0; --trail)
	{
		const float phase = static_cast<float>(m_pauseFrame) * 0.035f - trail * 0.12f;
		brush->SetColor(D2D1::ColorF(0.2f + 0.1f * trail, 0.9f - 0.08f * trail, 1.0f, 0.3f + 0.07f * (7 - trail)));
		D2D1_POINT_2F points[4];
		for (int i = 0; i < 4; ++i)
		{
			points[i] = D2D1::Point2F(size.width * (0.5f + 0.43f * std::sin(phase * (0.7f + i * 0.13f) + i * 1.7f)),
				size.height * (0.5f + 0.43f * std::sin(phase * (0.9f + i * 0.11f) + i * 2.2f)));
		}
		for (int i = 0; i < 4; ++i)
			target->DrawLine(points[i], points[(i + 1) % 4], brush.Get(), 2.0f);
	}
}

void CMikasMahjonggView::DrawCelebration(ID2D1RenderTarget* target)
{
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
	if (FAILED(target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush)))
	{
		if (!m_celebrationRenderError)
		{
			m_celebrationRenderError = true;
			AfxMessageBox(Tr(L"Could not draw the celebration animation.", L"Voittoanimaatiota ei voitu piirtää."), MB_ICONERROR);
		}
		return;
	}
	const float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - m_celebrationStart).count();
	const D2D1_SIZE_F size = target->GetSize();
	const float scale = (std::min)(size.width, size.height);
	const float starts[] = { 0.1f, 0.7f, 1.3f, 1.9f, 2.5f, 3.1f, 3.7f, 4.3f, 4.8f };
	const D2D1_COLOR_F colors[] = {
		D2D1::ColorF(1.0f, 0.77f, 0.30f), D2D1::ColorF(1.0f, 0.44f, 0.49f),
		D2D1::ColorF(0.52f, 0.87f, 1.0f), D2D1::ColorF(0.86f, 0.62f, 1.0f)
	};
	constexpr float pi = 3.14159265f;
	for (int burst = 0; burst < _countof(starts); ++burst)
	{
		const float age = elapsed - starts[burst];
		if (age < 0.0f || age > 1.7f) continue;
		const float cx = size.width * (0.18f + 0.32f * (burst % 3));
		const float cy = size.height * (0.18f + 0.15f * ((burst * 7) % 3));
		D2D1_COLOR_F color = colors[burst % 4];
		color.a = (std::min)(1.0f, (1.7f - age) * 1.5f);
		brush->SetColor(color);
		for (int spark = 0; spark < 24; ++spark)
		{
			const float angle = 2.0f * pi * spark / 24.0f + burst * 0.37f;
			const float speed = scale * (0.14f + 0.035f * (spark % 3));
			const float radius = speed * age;
			const float tail = speed * (std::max)(0.0f, age - 0.12f);
			const float gravity = scale * 0.045f * age * age;
			target->DrawLine(D2D1::Point2F(cx + std::cos(angle) * tail, cy + std::sin(angle) * tail + gravity),
				D2D1::Point2F(cx + std::cos(angle) * radius, cy + std::sin(angle) * radius + gravity),
				brush.Get(), (std::max)(1.5f, scale * 0.0035f));
		}
	}
	D2D1_MATRIX_3X2_F original;
	target->GetTransform(&original);
	for (int petal = 0; petal < 28; ++petal)
	{
		const float x = size.width * (0.04f + 0.92f * ((petal * 37 % 97) / 97.0f)) +
			scale * 0.035f * std::sin(elapsed * 1.8f + petal);
		const float y = std::fmod(size.height * (petal * 29 % 100) / 100.0f + elapsed * size.height * 0.12f,
			(std::max)(1.0f, size.height));
		const float radius = (std::max)(4.0f, scale * (0.012f + (petal % 3) * 0.0015f));
		target->SetTransform(D2D1::Matrix3x2F::Rotation(elapsed * 95.0f + petal * 31.0f) *
			D2D1::Matrix3x2F::Translation(x, y) * original);
		brush->SetColor(D2D1::ColorF(1.0f, 0.68f + (petal % 3) * 0.07f, 0.78f, 0.85f));
		target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(0, 0), radius, radius * 0.48f), brush.Get());
	}
	target->SetTransform(original);
}

void CMikasMahjonggView::OnUpdateGameCommand(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetDocument()->IsGameReady() && !m_paused);
}

void CMikasMahjonggView::OnUpdateGameStatus(CCmdUI* pCmdUI)
{
	const auto seconds = GetDocument()->Statistics().elapsedMilliseconds / 1000;
	CString text;
	text.Format(Tr(L"Tiles: %d   Time: %llu:%02llu", L"Laatat: %d   Aika: %llu:%02llu"),
		GetDocument()->Game().Remaining(), seconds / 60, seconds % 60);
	pCmdUI->Enable(TRUE);
	pCmdUI->SetText(text);
}

void CMikasMahjonggView::OnUpdateUnavailable(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(FALSE);
}

BOOL CMikasMahjonggView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CMikasMahjonggView drawing

void CMikasMahjonggView::OnDraw(CDC* /*pDC*/)
{
	CMikasMahjonggDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}


// CMikasMahjonggView printing


void CMikasMahjonggView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

BOOL CMikasMahjonggView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CMikasMahjonggView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CMikasMahjonggView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void CMikasMahjonggView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CMikasMahjonggView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}


// CMikasMahjonggView diagnostics

#ifdef _DEBUG
void CMikasMahjonggView::AssertValid() const
{
	CView::AssertValid();
}

void CMikasMahjonggView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CMikasMahjonggDoc* CMikasMahjonggView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMikasMahjonggDoc)));
	return (CMikasMahjonggDoc*)m_pDocument;
}
#endif //_DEBUG


// CMikasMahjonggView message handlers
