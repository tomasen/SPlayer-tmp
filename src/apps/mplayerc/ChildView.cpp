﻿/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

// ChildView.cpp : implementation of the CChildView class
//

#include "stdafx.h"
#include "mplayerc.h"
#include "ChildView.h"
#include "MainFrm.h"
#include "libpng.h"
#include "..\..\svplib\SVPToolBox.h"
#include "Controller\PlayerPreference.h"
#include "Controller\SPlayerDefs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChildView

#define TIMER_OFFSET 11
#define  TIMER_SLOWOFFSET 12
#define TIMER_TIPS 15

CChildView::CChildView() :
m_vrect(0,0,0,0)
,m_cover(NULL)
,m_bMouseDown(FALSE)
, m_lastLyricColor(0x00dfe7ff)
, m_blocklistview(0)
, m_blockunit(0)
, m_offsetspeed(0)
, m_preoffsetspeed(0)
{
	m_lastlmdowntime = 0;
	m_lastlmdownpoint.SetPoint(0, 0);

	//m_watermark.LoadFromResource(IDF_LOGO2);
	LoadLogo();
	AppSettings& s = AfxGetAppSettings();
	if(!s.bDisableCenterBigOpenBmp){
		CSUIButton * btnFileOpen = new CSUIButton(L"BTN_BIGOPEN.BMP" , ALIGN_TOPLEFT, CRect(-50 , -62, 0,0)  , FALSE, ID_FILE_OPENQUICK, FALSE  ) ;
		m_btnList.AddTail( btnFileOpen);

	}
	m_mediacenter = MediaCenterController::GetInstance();
  m_blocklistview = &(m_mediacenter->GetBlockListView());
	//m_btnList.AddTail( new CSUIButton(L"BTN_OPENADV.BMP" ,ALIGN_TOPLEFT, CRect(-50 , -62, 0,0)  , FALSE, ID_FILE_OPENMEDIA, FALSE, ALIGN_LEFT,btnFileOpen,  CRect(3,3,3,3) ) ) ;
	
	m_btnList.AddTail( new CSUIButton(L"WATERMARK2.BMP" , ALIGN_BOTTOMRIGHT, CRect(6 , 6, 0,6)  , TRUE, 0, FALSE  ) );

	m_cover = new CSUIButton(L"AUDIO_BG.BMP" , ALIGN_TOPLEFT, CRect(-50 , -44, 0,0)  , TRUE, 0, FALSE  ) ;
}

CChildView::~CChildView()
{
	if (m_cover)
		delete m_cover;
}

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if(!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.style &= ~WS_BORDER;
	
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_HAND), HBRUSH(COLOR_WINDOW+1), NULL);


	return TRUE;
}

BOOL CChildView::PreTranslateMessage(MSG* pMsg)
{
	  if(pMsg->message >= WM_MOUSEFIRST && pMsg->message <= WM_MYMOUSELAST)
	  {
		  CWnd* pParent = GetParent();
		  CPoint p(pMsg->lParam);
		  ::MapWindowPoints(pMsg->hwnd, pParent->m_hWnd, &p, 1);

		  bool fDblClick = false;

		  bool fInteractiveVideo = ((CMainFrame*)AfxGetMainWnd())->IsInteractiveVideo();
/*
		  if(fInteractiveVideo)
		  {
			  if(pMsg->message == WM_LBUTTONDOWN)
			  {
				  if((pMsg->time - m_lastlmdowntime) <= GetDoubleClickTime()
				  && abs(pMsg->pt.x - m_lastlmdownpoint.x) <= GetSystemMetrics(SM_CXDOUBLECLK)
				  && abs(pMsg->pt.y - m_lastlmdownpoint.y) <= GetSystemMetrics(SM_CYDOUBLECLK))
				  {
					  fDblClick = true;
					  m_lastlmdowntime = 0;
					  m_lastlmdownpoint.SetPoint(0, 0);
				  }
				  else
				  {
					  m_lastlmdowntime = pMsg->time;
					  m_lastlmdownpoint = pMsg->pt;
				  }
			  }
			  else if(pMsg->message == WM_LBUTTONDBLCLK)
			  {
				  m_lastlmdowntime = pMsg->time;
				  m_lastlmdownpoint = pMsg->pt;
			  }
		  }
*/
      if((pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_LBUTTONUP || pMsg->message == WM_MOUSEMOVE)
		  && fInteractiveVideo)
		  {
			  if(pMsg->message == WM_MOUSEMOVE)
			  {
				  pParent->PostMessage(pMsg->message, pMsg->wParam, MAKELPARAM(p.x, p.y));
			  }

			  if(fDblClick)
			  {
				  pParent->PostMessage(WM_LBUTTONDOWN, pMsg->wParam, MAKELPARAM(p.x, p.y));
				  pParent->PostMessage(WM_LBUTTONDBLCLK, pMsg->wParam, MAKELPARAM(p.x, p.y));
			  }
		  }
		  else
		  {
        pParent->PostMessage(pMsg->message, pMsg->wParam, MAKELPARAM(p.x, p.y));
			  return TRUE;
		  }
	  }
	  else{
		  //CMainFrame* pFrame = (CMainFrame*)GetParentFrame();
		  //if(pFrame->m_wndToolTopBar.IsWindowVisible())
		  //	return TRUE;
	  }
	return CWnd::PreTranslateMessage(pMsg);
}

void CChildView::SetVideoRect(CRect r)
{
	m_vrect = r;

	Invalidate();
}
static void SVPPreMultiplyBitmap( CBitmap& bmp ){
	BITMAP bm;
	bmp.GetBitmap(&bm);

	if(bm.bmBitsPixel != 32){
		return;
	}
	for (int y=0; y<bm.bmHeight; y++)
	{
		BYTE * pPixel = (BYTE *) bm.bmBits + bm.bmWidth * 4 * y;
		for (int x=0; x<bm.bmWidth; x++)
		{
			pPixel[0] = pPixel[0] * pPixel[3] / 255; 
			pPixel[1] = pPixel[1] * pPixel[3] / 255; 
			pPixel[2] = pPixel[2] * pPixel[3] / 255; 
			pPixel += 4;
		}
	}
}
void CChildView::LoadLogoFromFile(CString fnLogo)
{
	AppSettings& s = AfxGetAppSettings();
	if(s.fXpOrBetter)
		m_logo.Load(fnLogo);
	else if(HANDLE h = LoadImage(NULL, fnLogo, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE))
		m_logo.Attach((HBITMAP)h); // win9x bug: Inside Attach GetObject() will return all zeros in DIBSECTION and silly CImage uses that to init width, height, bpp, ... so we can't use CImage::Draw later

}
void CChildView::LoadLogo()
{
  AppSettings& s = AfxGetAppSettings();

  CAutoLock cAutoLock(&m_csLogo);

  isUsingSkinBG = false;
  m_logo.Destroy();

  if(s.logoext)
  {
    LoadLogoFromFile(s.logofn);
  }

  //Try Skin BG Logo
  if (m_logo.IsNull())
  {
    CSVPToolBox svpTool;
    CPath skinsBGPath(svpTool.GetPlayerPath(_T("skins")));
    skinsBGPath.AddBackslash();
    skinsBGPath.Append(_T("background"));
    CString realSkinsBGPath;
    if (svpTool.ifFileExist(skinsBGPath + _T(".jpg")))
      realSkinsBGPath = skinsBGPath + _T(".jpg");
    else if (svpTool.ifFileExist(skinsBGPath + _T(".png")))
      realSkinsBGPath = skinsBGPath + _T(".png");

    if (!realSkinsBGPath.IsEmpty())
    {
      LoadLogoFromFile(realSkinsBGPath);
      if (!m_logo.IsNull())
        isUsingSkinBG = TRUE;
    }
  }

  if (m_logo.IsNull())
  {
    //Try OEM Logo
    CString OEMBGPath;
    CSVPToolBox svpTool;
    OEMBGPath = svpTool.GetPlayerPath(_T("skins\\oembg.png"));
    if (svpTool.ifFileExist(OEMBGPath))
      LoadLogoFromFile(OEMBGPath);
  }

  if (m_logo.IsNull())
  {
    m_logo.LoadFromResource(IDF_LOGO7);
    if (!m_logo.IsNull())
      isUsingSkinBG = TRUE;
  }
  if (!m_logo.IsNull())
  {
    if (m_logo.IsDIBSection())
    {
      m_logo_bitmap.Detach();
      m_logo_bitmap.Attach((HBITMAP)m_logo);
      SVPPreMultiplyBitmap(m_logo_bitmap);
      m_logo.Detach();
      m_logo.Attach((HBITMAP)m_logo_bitmap);
    }
  }
  if (m_hWnd)
    Invalidate();
}

CSize CChildView::GetLogoSize()
{
	CSize ret(0,0);
	if(!m_logo.IsNull())
		ret.SetSize(m_logo.GetWidth(), m_logo.GetHeight());
	return ret;
}

IMPLEMENT_DYNAMIC(CChildView, CWnd)

BEGIN_MESSAGE_MAP(CChildView, CWnd)
	//{{AFX_MSG_MAP(CChildView)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGED()
	ON_COMMAND_EX(ID_PLAY_PLAYPAUSE, OnPlayPlayPauseStop)
	ON_COMMAND_EX(ID_PLAY_PLAY, OnPlayPlayPauseStop)
	ON_COMMAND_EX(ID_PLAY_PAUSE, OnPlayPlayPauseStop)
	ON_COMMAND_EX(ID_PLAY_STOP, OnPlayPlayPauseStop)
  ON_COMMAND_EX(IDR_MENU_SETCOVER, OnSetCover)
  ON_WM_SETCURSOR()
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
	ON_WM_CREATE()
	//ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
  ON_WM_RBUTTONUP()
  ON_WM_LBUTTONDBLCLK()
	ON_WM_NCHITTEST()
	ON_WM_KEYUP()
  ON_WM_TIMER()
END_MESSAGE_MAP()

static COLORREF colorNextLyricColor(COLORREF lastColor)
{
        
     return RGB(rand()% 0x50 + 0xa2, rand()% 0x50 + 0xa2,rand()% 0x50 + 0xa2);

}

/////////////////////////////////////////////////////////////////////////////
// CChildView message handlers
#include "../../svplib/svplib.h"
void CChildView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
  
	CMainFrame* pFrame = (CMainFrame*)GetParentFrame();
  if (pFrame->IsSomethingLoading()) //�ļ̲Repaint
    return;
	pFrame->RepaintVideo();

	CRect rcClient;
  GetClientRect(&rcClient);
  
	if(!pFrame->IsSomethingLoaded() || (pFrame->IsSomethingLoaded() && (pFrame->m_fAudioOnly || pFrame->IsSomethingLoading())) )
  {
		AppSettings& s = AfxGetAppSettings();

		CRect rcWnd;
		GetWindowRect(rcWnd);
		CRect rcClient;
		GetClientRect(&rcClient);
		CMemoryDC hdc(&dc, rcClient);

    // only response of media center messages(WM_PAINT)
    if (m_mediacenter->GetPlaneState())
    {
      m_blocklistview->DoPaint(hdc.m_hDC, rcClient);
      return;
    }

		hdc.FillSolidRect( rcClient, s.GetColorFromTheme(_T("MainBackgroundColor"),0));
    CRect rcLoading(rcClient);
    
		/*if(m_cover && !m_cover->IsNull()){
			BITMAP bm;
			GetObject(*m_cover, sizeof(bm), &bm);

			CRect r;
			GetClientRect(r);
			// ��ֿ��
			CRect cover_r;
			if ( bm.bmWidth * 100 / bm.bmHeight > r.Width() * 100 / r.Height() ){
				//�r.Width()Ϊ׼
				int w = r.Width();
				int h = r.Width() * bm.bmHeight/ bm.bmWidth ;
				int x = 0;
				int y = (r.Height() - h) / 2;
				cover_r = CRect(CPoint(x, y), CSize(w, h));
			}else{
				int w = r.Height() * bm.bmWidth / bm.bmHeight;
				int h = r.Height();
				int x = (r.Width() - w) / 2;
				int y = 0;
				cover_r = CRect(CPoint(x, y), CSize(w, h));
			}
			int oldmode = hdc.SetStretchBltMode(STRETCH_HALFTONE);
			m_cover->StretchBlt(hdc, cover_r, CRect(0,0,bm.bmWidth,abs(bm.bmHeight)));
		}else	*/
		if( !m_logo.IsNull() ){ 
			BITMAP bm;
			GetObject(m_logo, sizeof(bm), &bm);

			CRect r;
			GetClientRect(r);
      PlayerPreference* pref = PlayerPreference::GetInstance();
			/*
			if( s.logostretch == 1){ // ֿ�  ��
							int w = min(bm.bmWidth, r.Width());
							int h = min(abs(bm.bmHeight), r.Height());
							int x = (r.Width() - w) / 2;
							int y = (r.Height() - h) / 2;
							m_logo_r = CRect(CPoint(x, y), CSize(w, h));
						}else */
			if(pref->GetIntVar(INTVAR_LOGO_AUTOSTRETCH) == 2 || isUsingSkinBG){ // ��ֿ��
				m_logo_r = r;
			}else if(pref->GetIntVar(INTVAR_LOGO_AUTOSTRETCH) == 3){// ��ֿ��
				if ( bm.bmWidth * 100 / bm.bmHeight > r.Width() * 100 / r.Height() ){
					//�r.Width()Ϊ׼
					int w = r.Width();
					int h = r.Width() * bm.bmHeight/ bm.bmWidth ;
					int x = 0;
					int y = (r.Height() - h) / 2;
					m_logo_r = CRect(CPoint(x, y), CSize(w, h));
				}else{
					int w = r.Height() * bm.bmWidth / bm.bmHeight;
					int h = r.Height();
					int x = (r.Width() - w) / 2;
					int y = 0;
					m_logo_r = CRect(CPoint(x, y), CSize(w, h));
				}
				
			}else{ //��ֿ��
				int w = bm.bmWidth;
				int h = bm.bmHeight ;
				CPoint pos = r.CenterPoint();
				pos.x -= w/2;
				pos.y -= h/2;
				m_logo_r = CRect( pos, CSize(w, h));
			}
			
			int oldmode = hdc.SetStretchBltMode(STRETCH_HALFTONE);
			m_logo.StretchBlt(hdc, m_logo_r, CRect(0,0,bm.bmWidth,abs(bm.bmHeight)));
		}
		if(!pFrame->IsSomethingLoaded() || !pFrame->m_fAudioOnly)
    {
      m_btnList.SetHideStat(ID_FILE_OPENQUICK, pFrame->IsSomethingLoading());
      m_btnList.PaintAll( &hdc, rcWnd );
    }
		else if(pFrame->IsSomethingLoaded() && pFrame->m_fAudioOnly){
			m_cover->OnPaint(&hdc, rcWnd );

			if(!m_strAudioInfo.IsEmpty()){
				HFONT holdft = NULL; 
                hdc.SetBkMode(TRANSPARENT);
                //hdc.SetBkColor( 0x00d6d7ce);
                wchar_t music_note[] = {0x266A, 0x0020, 0};
                if(m_strAudioInfo.Find(music_note) >= 0)
                {
                    (HFONT)hdc.SelectObject(m_font_lyric);
                    CRect rcTextArea( CPoint(21,6)  , CSize(rcWnd.Width()-20, 22));
                    hdc.SetTextColor(0x363636);
                    DrawText(hdc, m_strAudioInfo, m_strAudioInfo.GetLength(), &rcTextArea, DT_LEFT|DT_SINGLELINE | DT_VCENTER );
                    rcTextArea = CRect( CPoint(20,5)  , CSize(rcWnd.Width()-20, 22));
                    hdc.SetTextColor(colorNextLyricColor(m_lastLyricColor));
                    DrawText(hdc, m_strAudioInfo, m_strAudioInfo.GetLength(), &rcTextArea, DT_LEFT|DT_SINGLELINE | DT_VCENTER );
                }else{
                    (HFONT)hdc.SelectObject(m_font);
                    CRect rcTextArea( CPoint(11,6)  , CSize(rcWnd.Width()-20, 18));
                    hdc.SetTextColor(s.GetColorFromTheme(_T("AudioOnlyInfoText"),0xffffff));
                    DrawText(hdc, m_strAudioInfo, m_strAudioInfo.GetLength(), &rcTextArea, DT_LEFT|DT_SINGLELINE | DT_VCENTER );
                }
				
				//SVP_LogMsg5( L"shduf %s" , m_strAudioInfo);
				hdc.SelectObject(holdft);
			}
		}

        if( pFrame->IsSomethingLoading() )
        {
            //rcLoading.top += rcLoading.Height()/2;
            //hdc.FillSolidRect( rcLoading, 0xffffff);
            //TODO: �opening 
        }
	}
	// Do not call CWnd::OnPaint() for painting messages
}
void CChildView::SetLyricLasting(int time_sec)
{

    KillTimer(TIMER_CLEAR_LYRIC_CHILDVIEW);
    SetTimer(TIMER_CLEAR_LYRIC_CHILDVIEW, time_sec*1000, NULL);

}
void CChildView::ReCalcBtn(){
	CRect rcWnd;
	GetWindowRect(rcWnd);
	m_btnList.OnSize(rcWnd);
	m_cover->OnSize(rcWnd);
}
void CChildView::OnSetFocus(CWnd* pOldWnd){
	
}
BOOL CChildView::OnEraseBkgnd(CDC* pDC)
{
	CRect r;

	CAutoLock cAutoLock(&m_csLogo);
	CMainFrame* pFrame = (CMainFrame*)GetParentFrame();
	if(pFrame->IsSomethingLoaded() && ! pFrame->m_fAudioOnly)
	{
		//pDC->FillSolidRect(CRect(30,30,60,60) , RGB(0xff,0,0));
		pDC->ExcludeClipRect(m_vrect);
		GetClientRect(r);
		pDC->FillSolidRect(r, 0);
	}
	else 
	{
		
	
		
		/*
		if(!m_watermark.IsNull()){
					BITMAP bmw;
					GetObject(m_watermark, sizeof(bmw), &bmw);
					CRect rw;
					GetClientRect(rw);
					int ww = min(bmw.bmWidth, rw.Width());
					int hw = min(abs(bmw.bmHeight), rw.Height());
					rw = CRect(CPoint(rw.Width() - ww , rw.Height() - hw), CSize(ww, hw));
					m_watermark.StretchBlt( *pDC ,  rw,  CRect(0,0,bmw.bmWidth,abs(bmw.bmHeight)));
					pDC->ExcludeClipRect(rw);
				}* /
		
//		m_logo.Draw(*pDC, r);
		pDC->SetStretchBltMode(oldmode);
		pDC->ExcludeClipRect(r);
*/

		/*
CRect rcWnd;
		GetWindowRect(rcWnd);
		CRect rcClient;
		GetClientRect(rcClient);
		//pDC->FillSolidRect(rcClient, 0);

		CMemoryDC hdc(pDC, rcClient);
		hdc.FillSolidRect( rcClient, 0);
		m_btnBBList.PaintAll( &hdc, rcWnd );
*/

		
	}

	
	

	
	return TRUE;
}

void CChildView::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	((CMainFrame*)GetParentFrame())->MoveVideoWindow();
	ReCalcBtn();

  if (m_mediacenter->GetPlaneState())
  {
    RECT rc;
    GetClientRect(&rc);
    m_blocklistview->Update(rc.right - rc.left, rc.bottom - rc.top);
    Invalidate();
  }
}


void CChildView::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CWnd::OnWindowPosChanged(lpwndpos);
  
	((CMainFrame*)GetParentFrame())->MoveVideoWindow();
}

BOOL CChildView::OnPlayPlayPauseStop(UINT nID)
{
	if(nID == ID_PLAY_STOP) SetVideoRect();
  return FALSE;
}

BOOL CChildView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if(((CMainFrame*)GetParentFrame())->m_fHideCursor)
	{
		SetCursor(NULL);
		return TRUE;
	}

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

void CChildView::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	if(!((CMainFrame*)GetParentFrame())->IsFrameLessWindow()) 
	{
		//InflateRect(&lpncsp->rgrc[0], -1, -1);
	}

	CWnd::OnNcCalcSize(bCalcValidRects, lpncsp);
}

void CChildView::OnNcPaint()
{
	if(!((CMainFrame*)GetParentFrame())->IsFrameLessWindow()) 
	{
// 		CRect r;
// 		GetWindowRect(r);
// 		r.OffsetRect(-r.left, -r.top);
// 
// 		CWindowDC(this).Draw3dRect(&r, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DHILIGHT)); 
	}
}


int CChildView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

    AppSettings& s = AfxGetAppSettings();

	GetSystemFontWithScale(&m_font, 14.0);
    GetSystemFontWithScale(&m_font_lyric, 20.0, FW_BOLD, s.subdefstyle.fontName); //
	// TODO:  Add your specialized creation code here
    
    DWORD dwTransFlag = 0; 
    if (s.bUserAeroUI())
      dwTransFlag = WS_EX_TRANSPARENT;
    
    WNDCLASSEX layeredClass;
    layeredClass.cbSize        = sizeof(WNDCLASSEX);
    layeredClass.style         = CS_HREDRAW | CS_VREDRAW;
    layeredClass.lpfnWndProc   = 0;
    layeredClass.cbClsExtra    = 0;
    layeredClass.cbWndExtra    = 0;
    layeredClass.hInstance     = AfxGetMyApp()->m_hInstance;
    layeredClass.hIcon         = NULL;
    layeredClass.hCursor       = NULL;
    layeredClass.hbrBackground = NULL;
    layeredClass.lpszMenuName  = NULL;
    layeredClass.lpszClassName = _T("SVPLayered");
    layeredClass.hIconSm       = NULL;
    RegisterClassEx(&layeredClass) ;
    if (!m_tip.CreateEx(WS_EX_NOACTIVATE|WS_EX_TOPMOST|dwTransFlag, _T("SVPLayered"), _T("TIP"), WS_POPUP, CRect( 20,20,21,21 ) , this,  0))
      AfxMessageBox(ResStr(IDS_MSG_CREATE_SEEKTIP_FAIL));
    

    m_blocklistview->SetFrameHwnd(m_hWnd);
    m_blocklistview->SetScrollSpeed(&m_offsetspeed);

    m_menu.LoadMenu(IDR_MEDIACENTERMENU);
    
    SetMenu(&m_menu);
    return 0;
}

BOOL CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

  if (m_mediacenter->GetPlaneState())
  {
    RECT rc;
    GetClientRect(&rc);
    m_blocklistview->HandleMouseMove(point, &m_blockunit);
    return TRUE;
  }

  CSize diff = m_lastMouseMove - point;
	BOOL bMouseMoved =  diff.cx || diff.cy ;
	m_lastMouseMove = point;

	CRect rc;
	CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());
	GetWindowRect(&rc);
	point += rc.TopLeft() ;

  if (bMouseMoved){

    UINT ret = m_btnList.OnHitTest(point,rc,-1);
		m_nItemToTrack = ret;
		
    if ( m_btnList.HTRedrawRequired ){
		  Invalidate();
	  }
  }


  if (::GetKeyState(VK_LBUTTON) & 0x8000)
  {
    // The left button is still pressed
  } 
  else if (m_bMouseDown)
  {
    // The left button is released and the Windows don't send the WM_LBUTTONUP
    // and WM_MOVE
    PostMessage(WM_LBUTTONUP, 0, MAKELPARAM(point.x, point.y));
    PostMessage(WM_MOVE, 0, MAKELPARAM(point.x, point.y));
  }
  
  CWnd::OnMouseMove(nFlags, point);
  return FALSE;
}
void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
  
  CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());
	iBottonClicked = -1;
	m_bMouseDown = TRUE;
	CRect rc;
	GetWindowRect(&rc);

  if (m_mediacenter->GetPlaneState())
  {
    RECT rc;
    GetClientRect(&rc);
    m_blocklistview->HandleLButtonDown(point, &m_blockunit);
    return;
  }

  point += rc.TopLeft() ;
	UINT ret = m_btnList.OnHitTest(point,rc,TRUE);
	if( m_btnList.HTRedrawRequired ){
		if(ret)
			SetCapture();
		Invalidate();
	}
	m_nItemToTrack = ret;

  CWnd::OnLButtonDown(nFlags, point);
}

void CChildView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());
	KillTimer(TIMER_FASTFORWORD);
	ReleaseCapture();

	CRect rc;
	GetWindowRect(&rc);

  if (m_mediacenter->GetPlaneState())
  {
    RECT rc;
    GetClientRect(&rc);
    m_blocklistview->HandleLButtonUp(point, &m_blockunit);
    InvalidateRect(&m_scrollbarrect);
    return;
  }
  
	CPoint xpoint = point + rc.TopLeft() ;
	UINT ret = m_btnList.OnHitTest(xpoint,rc,FALSE);
	if( m_btnList.HTRedrawRequired ){
		if(ret){
			pFrame->PostMessage( WM_COMMAND, ret);
		}
		Invalidate();
	}
	m_nItemToTrack = ret;

  m_bMouseDown = FALSE;
  __super::OnLButtonUp(nFlags, point);
}

BOOL CChildView::OnRButtonUP(UINT nFlags, CPoint point)
{
   BOOL bmenutrack = FALSE;

   if (m_mediacenter->GetPlaneState())
   {
     CRect rc;
     GetClientRect(&rc);
     bmenutrack = m_blocklistview->HandleRButtonUp(point, &m_blockunit, &m_menu);
   }

   return bmenutrack;
}

BOOL CChildView::OnLButtonDBCLK(UINT nFlags, CPoint point)
{
  BOOL bl = FALSE;

  if (m_mediacenter->GetPlaneState())
    bl = TRUE;

  return bl;
}

LRESULT CChildView::OnNcHitTest(CPoint point)
{
	// TODO: Add your message handler code here and/or call default
  
	return CWnd::OnNcHitTest(point);
}

void CChildView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());
	if(pFrame)
	{
		pFrame->m_lastSeekAction = 0;
		pFrame->KillTimer(TIMER_CLEAR_LAST_SEEK_ACTION);
	}
	CWnd::OnKeyUp(nChar, nRepCnt, nFlags);
}
static BOOL bl = FALSE;
void CChildView::OnTimer(UINT_PTR nIDEvent)
{
    // TODO: Add your message handler code here and/or call default
    if(TIMER_CLEAR_LYRIC_CHILDVIEW == nIDEvent)
    {
        m_strAudioInfo.Empty(); 

    }

    if (nIDEvent == TIMER_OFFSET)
    {
      m_preoffsetspeed = m_offsetspeed;

      if (m_offsetspeed == 0)
        return;

      m_blocklistview->SetScrollBarDragDirection(m_offsetspeed);
      std::list<BlockUnit*>* list = m_blocklistview->GetEmptyList();
      if (list && !m_mediacenter->LoadMediaDataAlive())
        m_mediacenter->LoadMediaData(m_offsetspeed, list, m_blocklistview->GetViewCapacity(),
                                     m_blocklistview->GetListCapacity(),
                                     m_blocklistview->GetListRemainItem());
      else
      {
        if (m_blocklistview->BeDirectionChange())
        {
          list = m_blocklistview->GetIdleList();
          m_mediacenter->LoadMediaData(m_offsetspeed, list, m_blocklistview->GetViewCapacity(),
                                       m_blocklistview->GetListCapacity(),
                                       m_blocklistview->GetListRemainItem(), 2);
        }
      }
      
      m_blocklistview->SetOffset(m_offsetspeed);
    }

//     if (nIDEvent == TIMER_SLOWOFFSET)
//     {
//       m_blocklistview->SetOffset(m_offsetspeed);
//       if (m_preoffsetspeed == m_offsetspeed)
//         KillTimer(TIMER_SLOWOFFSET);
//     }

    if (nIDEvent == TIMER_TIPS)
    {
      KillTimer(TIMER_TIPS);
      std::wstring tips = m_blocklistview->m_tipstring;
      if (tips.empty())
        m_tip.ClearStat();
      else
        m_tip.SetTips(tips.c_str(), TRUE);

      RECT rc;
      if (m_blockunit)
      {
        rc = m_blockunit->GetHittest();
        InvalidateRect(&rc);
      }
      return;
    }

    RECT rc;
    GetClientRect(&rc);
    m_blocklistview->Update(rc.right - rc.left, rc.bottom - rc.top);
    InvalidateRect(&rc);
    CWnd::OnTimer(nIDEvent);
}

void CChildView::ShowMediaCenter(BOOL bl)
{
  m_mediacenter->SetPlaneState(bl);

  if (!bl)
    return;

  RECT rc;
  GetClientRect(&rc);
  m_mediacenter->UpdateBlock(rc);
  std::list<BlockUnit*>* list = m_blocklistview->GetEmptyList();
  if (list)
    m_mediacenter->LoadMediaData(1, list, m_blocklistview->GetViewCapacity(), 
                                 m_blocklistview->GetListCapacity(), 0);
  
  SetCursor(::LoadCursor(NULL, IDC_WAIT));

  // wait until the load data thread is exit and finish its job
  ::WaitForSingleObject(m_mediacenter->GetMediaDataThreadHandle(), INFINITE);
  
  SetCursor(::LoadCursor(NULL, IDC_ARROW));
  
  m_mediacenter->UpdateBlock(rc);
  InvalidateRect(0, FALSE);
}

BOOL CChildView::OnSetCover(UINT nID)
{
  CFileDialog filedlg(TRUE, L"jpg", 0, OFN_READONLY, L"JPEG Files (*.jpg)|*.jpg||", this);
  filedlg.DoModal();

  std::wstring orgpath = filedlg.GetPathName().GetString();
  m_mediacenter->SetCover(m_blockunit, orgpath);

  return TRUE;
}
