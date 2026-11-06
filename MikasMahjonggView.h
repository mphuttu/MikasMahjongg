
// MikasMahjonggView.h : interface of the CMikasMahjonggView class
//

#pragma once

#include "MahjongGame.h"
#include <wrl/client.h>
#include <d2d1.h>
#include <chrono>

class CMikasMahjonggView : public CView
{
protected: // create from serialization only
	CMikasMahjonggView() noexcept;
	DECLARE_DYNCREATE(CMikasMahjonggView)

// Attributes
public:
	CMikasMahjonggDoc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CMikasMahjonggView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	int m_selected = -1;
	MahjongGame::Pair m_hint = { -1, -1 };
	Microsoft::WRL::ComPtr<ID2D1Bitmap> m_bitmaps[MahjongGame::FaceCount];
	bool m_loadFailed = false;
	bool m_paused = false;
	bool m_pauseRenderError = false;
	unsigned int m_pauseFrame = 0;
	bool m_celebrating = false;
	bool m_celebrationRenderError = false;
	std::chrono::steady_clock::time_point m_celebrationStart;

	bool LoadTileImages(ID2D1RenderTarget* target);
	D2D1_RECT_F TileRect(const Slot& slot, D2D1_SIZE_F size) const;
	void ClearSelection();
	void ReportPosition();
	void RecordWinningScore();
	void DrawPause(ID2D1RenderTarget* target);
	void DrawCelebration(ID2D1RenderTarget* target);
	void FinishVictory();

// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg LRESULT OnDraw2D(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRecreate2DResources(WPARAM wParam, LPARAM lParam);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR timer);
	afx_msg void OnOpenGame();
	afx_msg void OnRestartGame();
	afx_msg void OnUndoMove();
	afx_msg void OnHint();
	afx_msg void OnReshuffle();
	afx_msg void OnTilesLeft();
	afx_msg void OnHelpContents();
	afx_msg void OnShowHallOfFame();
	afx_msg void OnClearHallOfFame();
	afx_msg void OnPauseGame();
	afx_msg void OnUpdateGameCommand(CCmdUI* pCmdUI);
	afx_msg void OnUpdateGameStatus(CCmdUI* pCmdUI);
	afx_msg void OnUpdateUnavailable(CCmdUI* pCmdUI);
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in MikasMahjonggView.cpp
inline CMikasMahjonggDoc* CMikasMahjonggView::GetDocument() const
   { return reinterpret_cast<CMikasMahjonggDoc*>(m_pDocument); }
#endif
