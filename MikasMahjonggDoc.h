
// MikasMahjonggDoc.h : interface of the CMikasMahjonggDoc class
//


#pragma once

#include "MahjongGame.h"
#include <chrono>

class CMikasMahjonggDoc : public CDocument
{
protected: // create from serialization only
	CMikasMahjonggDoc() noexcept;
	DECLARE_DYNCREATE(CMikasMahjonggDoc)

// Attributes
public:
	MahjongGame& Game() { return m_game; }
	const MahjongGame& Game() const { return m_game; }
	GameStatistics Statistics() const;
	void PauseClock();
	void ResumeClock();
	void RestartGame();
	void RecordHint();
	void RecordUndo();
	void RecordReshuffle();
	bool IsGameReady() const { return m_ready; }
	bool IsScoreRecorded() const { return m_statistics.scoreRecorded; }
	void SetScoreRecorded() { m_statistics.scoreRecorded = true; }

// Operations
public:

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR path);
	virtual BOOL OnSaveDocument(LPCTSTR path);
	virtual BOOL DoSave(LPCTSTR path, BOOL replace = TRUE);
	virtual BOOL SaveModified();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// Implementation
public:
	virtual ~CMikasMahjonggDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	MahjongGame m_game;
	GameStatistics m_statistics;
	std::chrono::steady_clock::time_point m_clockStart;
	bool m_clockRunning = false;
	bool m_ready = false;

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
};
