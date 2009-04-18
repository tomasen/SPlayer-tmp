/* 
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

// mplayerc.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <imagehlp.h>
#include "mplayerc.h"
#include <atlsync.h>
#include <Tlhelp32.h>
#include "MainFrm.h"
#include "..\..\DSUtil\DSUtil.h"
#include "revision.h"
#include "ChkDefPlayer.h"
#include <locale.h> 
#include <d3d9.h>
#include <d3dx9.h>


/////////
typedef BOOL (WINAPI* MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
										 CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
										 CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
										 CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
static LONG WINAPI  DebugMiniDumpFilter( struct _EXCEPTION_POINTERS *pExceptionInfo )
{
	LONG retval = EXCEPTION_CONTINUE_SEARCH;
	HWND hParent = NULL;                        // find a better value for your app

	// firstly see if dbghelp.dll is around and has the function we need
	// look next to the EXE first, as the one in System32 might be old
	// (e.g. Windows 2000)
	HMODULE hDll = NULL;
	TCHAR szDbgHelpPath[_MAX_PATH];

	if (GetModuleFileName( NULL, szDbgHelpPath, _MAX_PATH ))
	{
		TCHAR *pSlash = _tcsrchr( szDbgHelpPath, _T('\\') );
		if (pSlash)
		{
			_tcscpy(pSlash+1, _T("DBGHELP.DLL"));
			hDll = ::LoadLibrary( szDbgHelpPath );
		}
	}

	if (hDll==NULL)
	{
		// load any version we can
		hDll = ::LoadLibrary(_T("DBGHELP.DLL"));
	}

	LPCTSTR szResult = NULL;

	if (hDll)
	{
		MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress(hDll,"MiniDumpWriteDump");
		if (pDump)
		{
			TCHAR szDumpPath[_MAX_PATH];
			TCHAR szScratch [_MAX_PATH];

			// work out a good place for the dump file
			_tgetcwd(szDumpPath,_MAX_PATH);
			_tcscat( szDumpPath, _T("\\"));

			_tcscat( szDumpPath, _T("splayer_") );
			_tcscat( szDumpPath, SVP_REV_STR );
			_tcscat( szDumpPath, _T(".dmp"));

			// ask the user if they want to save a dump file
			//if (::MessageBox(NULL,_T("����������,�Ƿ񱣴�һ���ļ��������?"), ResStr(IDR_MAINFRAME) ,MB_YESNO)==IDYES)
			{
				// create the file
				HANDLE hFile = ::CreateFile( szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL, NULL );

				if (hFile!=INVALID_HANDLE_VALUE)
				{
					_MINIDUMP_EXCEPTION_INFORMATION ExInfo;
					ExInfo.ThreadId = ::GetCurrentThreadId();
					ExInfo.ExceptionPointers = pExceptionInfo;
					ExInfo.ClientPointers = NULL;

					// write the dump
					BOOL bOK = pDump( GetCurrentProcess(),GetCurrentProcessId(),hFile,MiniDumpNormal,&ExInfo,NULL,NULL);
					if (bOK)
					{
						_stprintf( szScratch, _T("����������,����ļ��Ѿ����浽:'%s'\n�뽫���ļ�������tomasen@gmail.com��https://bbs.shooter.cn\n�Ա����ǲ�������"), szDumpPath );
						szResult = szScratch;
						retval = EXCEPTION_EXECUTE_HANDLER;
						{
							TCHAR sUpdaterPath[_MAX_PATH];
							TCHAR sUpPerm[_MAX_PATH];
							if (GetModuleFileName( NULL, sUpdaterPath, _MAX_PATH ))
							{
								_tcscat( sUpdaterPath, _T("\\Updater.exe"));
								_stprintf( sUpPerm, _T(" /dmp splayer_%s.dmp "), SVP_REV_STR );
								(int)::ShellExecute(NULL, _T("open"), sUpdaterPath, sUpPerm, NULL, SW_HIDE);

							}
						}
					}
					else
					{
						_stprintf( szScratch, _T("�����ļ��� '%s'ʧ��,(�����: %d)"), szDumpPath, GetLastError() );
						szResult = szScratch;
					}
					::CloseHandle(hFile);
				}
				else
				{
					_stprintf( szScratch, _T("��'%s'���� dump �ļ�ʧ��,(����� %d)"), szDumpPath, GetLastError() );
					szResult = szScratch;
				}
			}
		}
		else
		{
			szResult = _T("dbghelp.dll �ļ�̫��,����֧��MiniDumpWriteDump����");
		}
	}
	else
	{
		szResult = _T("dbghelp.dll �ļ�������");
	}

	if (szResult)
	{
		//::MessageBox( NULL, szResult, ResStr(IDR_MAINFRAME), MB_OK );
	}

	
	return retval;
}

//��� VMWare�Ĵ���
bool IsInsideVMWare()
{
	bool rc = true;

	__try
	{
		__asm
		{
			push  edx
				push  ecx
				push  ebx

				mov  eax, 'VMXh';
			mov  ebx, 0 // any value but not the MAGIC VALUE
				mov  ecx, 10 // get VMWare version
				mov  edx, 'VX'; // port number

			in   eax, dx // read port
				// on return EAX returns the VERSION
				cmp  ebx, 'VMXh' // is it a reply from VMWare?
				setz  [rc] // set return value

			pop  ebx
				pop  ecx
				pop  edx
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		rc = false;
	}

	return rc;
}

void CorrectComboListWidth(CComboBox& box, CFont* pWndFont)
{
	int cnt = box.GetCount();
	if(cnt <= 0) return;

	CDC* pDC = box.GetDC();
	pDC->SelectObject(pWndFont);

	int maxw = box.GetDroppedWidth();

	for(int i = 0; i < cnt; i++)
	{
		CString str;
		box.GetLBText(i, str);
		int w = pDC->GetTextExtent(str).cx + 22;
		if(maxw < w) maxw = w;
	}

	box.ReleaseDC(pDC);

	box.SetDroppedWidth(maxw);
}

HICON LoadIcon(CString fn, bool fSmall)
{
	if(fn.IsEmpty()) return(NULL);

	CString ext = fn.Left(fn.Find(_T("://"))+1).TrimRight(':');
	if(ext.IsEmpty() || !ext.CompareNoCase(_T("file")))
		ext = _T(".") + fn.Mid(fn.ReverseFind('.')+1);

	CSize size(fSmall?16:32,fSmall?16:32);

	if(!ext.CompareNoCase(_T(".ifo")))
	{
		if(HICON hIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_DVD), IMAGE_ICON, size.cx, size.cy, 0))
			return(hIcon);
	}

	if(!ext.CompareNoCase(_T(".cda")))
	{
		if(HICON hIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_AUDIOCD), IMAGE_ICON, size.cx, size.cy, 0))
			return(hIcon);
	}

	do
	{
		CRegKey key;

		TCHAR buff[256];
		ULONG len;

		if(ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, ext + _T("\\DefaultIcon"), KEY_READ))
		{
			if(ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, ext, KEY_READ))
				break;

			len = sizeof(buff);
			memset(buff, 0, len);
			if(ERROR_SUCCESS != key.QueryStringValue(NULL, buff, &len) || (ext = buff).Trim().IsEmpty())
				break;

			if(ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, ext + _T("\\DefaultIcon"), KEY_READ))
				break;
		}

		CString icon;

		len = sizeof(buff);
		memset(buff, 0, len);
		if(ERROR_SUCCESS != key.QueryStringValue(NULL, buff, &len) || (icon = buff).Trim().IsEmpty())
			break;

		int i = icon.ReverseFind(',');
		if(i < 0) break;
		
		int id = 0;
		if(_stscanf(icon.Mid(i+1), _T("%d"), &id) != 1)
			break;

		icon = icon.Left(i);

		HICON hIcon = NULL;
		UINT cnt = fSmall 
			? ExtractIconEx(icon, id, NULL, &hIcon, 1)
			: ExtractIconEx(icon, id, &hIcon, NULL, 1);
		if(hIcon) return hIcon;
	}
	while(0);

	return((HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_UNKNOWN), IMAGE_ICON, size.cx, size.cy, 0));
}

bool LoadType(CString fn, CString& type)
{
	CRegKey key;

	TCHAR buff[256];
	ULONG len;

	if(fn.IsEmpty()) return(NULL);

	CString ext = fn.Left(fn.Find(_T("://"))+1).TrimRight(':');
	if(ext.IsEmpty() || !ext.CompareNoCase(_T("file")))
		ext = _T(".") + fn.Mid(fn.ReverseFind('.')+1);

	if(ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, ext))
		return(false);

	CString tmp = ext;

    while(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, tmp))
	{
		len = sizeof(buff);
		memset(buff, 0, len);
		if(ERROR_SUCCESS != key.QueryStringValue(NULL, buff, &len))
			break;

		CString str(buff);
		str.Trim();

		if(str.IsEmpty() || str == tmp)
			break;

		tmp = str;
	}

	type = tmp;

	return(true);
}

bool LoadResource(UINT resid, CStringA& str, LPCTSTR restype)
{
	str.Empty();
	HRSRC hrsrc = FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(resid), restype);
	if(!hrsrc) return(false);
	HGLOBAL hGlobal = LoadResource(AfxGetResourceHandle(), hrsrc);
	if(!hGlobal) return(false);
	DWORD size = SizeofResource(AfxGetResourceHandle(), hrsrc);
	if(!size) return(false);
	memcpy(str.GetBufferSetLength(size), LockResource(hGlobal), size);
	return(true);
}

bool RegSvr32(CString szDllPath){
	//LoadLibrary(path))
	if(HMODULE h = LoadLibraryEx(  szDllPath , 0, LOAD_WITH_ALTERED_SEARCH_PATH))
	{
		typedef HRESULT (__stdcall * PDllRegisterServer)();
		if(PDllRegisterServer p = (PDllRegisterServer)GetProcAddress(h, "DllRegisterServer"))
		{
			p();
		}

		FreeLibrary(h);
	}
	return true;
}
/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	CString m_appname;
	CString m_strRevision;
	virtual BOOL OnInitDialog()
	{
		UpdateData();
#ifdef UNICODE
		m_appname += _T(" (unicode build)");
#endif
		cs_version.GetWindowText(m_strRevision);
		m_strRevision += CString(_T("(Build ")) + SVP_REV_STR + _T(")");
		UpdateData(FALSE);
		return TRUE;
	}
	CStatic cs_version;
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD), m_appname(_T("")), m_strRevision(_T(""))
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_STATIC1, m_appname);
	DDX_Text(pDX, IDC_VERSION, m_strRevision);
	DDX_Control(pDX, IDC_VERSION, cs_version);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMPlayerCApp

BEGIN_MESSAGE_MAP(CMPlayerCApp, CWinApp)
	//{{AFX_MSG_MAP(CMPlayerCApp)
	ON_COMMAND(ID_HELP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_EXIT, OnFileExit)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP_SHOWCOMMANDLINESWITCHES, OnHelpShowcommandlineswitches)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMPlayerCApp construction

CMPlayerCApp::CMPlayerCApp()
//	: m_hMutexOneInstance(NULL)
{
	::SetUnhandledExceptionFilter(DebugMiniDumpFilter);
}

void CMPlayerCApp::ShowCmdlnSwitches()
{
	CString s;

	if(m_s.nCLSwitches&CLSW_UNRECOGNIZEDSWITCH)
	{
		CAtlList<CString> sl;
		for(int i = 0; i < __argc; i++) sl.AddTail(__targv[i]);
		s += "Unrecognized switch(es) found in command line string: \n\n" + Implode(sl, ' ') + "\n\n";
	}

	s +=
		_T("Usage: mplayerc.exe \"pathname\" [switches]\n\n")
		_T("\"pathname\"\tThe main file or directory to be loaded. (wildcards allowed)\n")
		_T("/dub \"dubname\"\tLoad an additional audio file.\n")
		_T("/sub \"subname\"\tLoad an additional subtitle file.\n")
		_T("/filter \"filtername\"\tLoad DirectShow filters from a dynamic link library. (wildcards allowed)\n")
		_T("/dvd\t\tRun in dvd mode, \"pathname\" means the dvd folder (optional).\n")
		_T("/cd\t\tLoad all the tracks of an audio cd or (s)vcd, \"pathname\" means the drive path (optional).\n")
		_T("/open\t\tOpen the file, don't automatically start playing.\n")
		_T("/play\t\tStart playing the file as soon the player is launched.\n")
		_T("/close\t\tClose the player after playback (only works when used with /play).\n")
		_T("/shutdown\tShutdown the operating system after playback\n")
		_T("/fullscreen\tStart in full-screen mode.\n")
		_T("/minimized\tStart in minimized mode.\n")
		_T("/new\t\tUse a new instance of the player.\n")
		_T("/add\t\tAdd \"pathname\" to playlist, can be combined with /open and /play.\n")
		_T("/regvid\t\tCreate file associations for video files.\n")
		_T("/regaud\t\tCreate file associations for audio files.\n")
		_T("/unregall\t\tRemove all file associations.\n")
		_T("/start ms\t\tStart playing at \"ms\" (= milliseconds)\n")
		_T("/fixedsize w,h\tSet fixed window size.\n")
		_T("/monitor N\tStart on monitor N, where N starts from 1.\n")
		_T("/help /h /?\tShow help about command line switches. (this message box)\n");

	AfxMessageBox(s);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMPlayerCApp object

CMPlayerCApp theApp;

HWND g_hWnd = NULL;

bool CMPlayerCApp::StoreSettingsToIni()
{
	CString ini = GetIniPath();
/*
	FILE* f;
	if(!(f = _tfopen(ini, _T("r+"))) && !(f = _tfopen(ini, _T("w"))))
		return StoreSettingsToRegistry();
	fclose(f);
*/
	if(m_pszRegistryKey) free((void*)m_pszRegistryKey);
	m_pszRegistryKey = NULL;
	if(m_pszProfileName) free((void*)m_pszProfileName);
	m_pszProfileName = _tcsdup(ini);

	return(true);
}

bool CMPlayerCApp::StoreSettingsToRegistry()
{
	_tremove(GetIniPath());

	if(m_pszRegistryKey) free((void*)m_pszRegistryKey);
	m_pszRegistryKey = NULL;

	SetRegistryKey(_T("SVPlayer"));

	return(true);
}

CString CMPlayerCApp::GetIniPath()
{
	CString path;
	GetModuleFileName(AfxGetInstanceHandle(), path.GetBuffer(MAX_PATH), MAX_PATH);
	path.ReleaseBuffer();
	path = path.Left(path.ReverseFind('.')+1) + _T("ini");
	return(path);
}

bool CMPlayerCApp::IsIniValid()
{
	CFileStatus fs;
	return CFileGetStatus(GetIniPath(), fs) && fs.m_size > 0;
}

bool CMPlayerCApp::GetAppDataPath(CString& path)
{
	path.Empty();

	CRegKey key;
	if(ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"), KEY_READ))
	{
		ULONG len = MAX_PATH;
		if(ERROR_SUCCESS == key.QueryStringValue(_T("AppData"), path.GetBuffer(MAX_PATH), &len))
			path.ReleaseBufferSetLength(len);
	}

	if(path.IsEmpty())
		return(false);

	CPath p;
	p.Combine(path, _T("Media Player Classic"));
	path = (LPCTSTR)p;

	return(true);
}

void CMPlayerCApp::PreProcessCommandLine()
{
	m_cmdln.RemoveAll();
	for(int i = 1; i < __argc; i++)
	{
		CString str = CString(__targv[i]).Trim(_T(" \""));

		if(str[0] != '/' && str[0] != '-' && str.Find(_T(":")) < 0)
		{
			LPTSTR p = NULL;
			CString str2;
			str2.ReleaseBuffer(GetFullPathName(str, MAX_PATH, str2.GetBuffer(MAX_PATH), &p));
			CFileStatus fs;
			if(!str2.IsEmpty() && CFileGetStatus(str2, fs)) str = str2;
		}

		m_cmdln.AddTail(str);
	}
}

void CMPlayerCApp::SendCommandLine(HWND hWnd)
{
	if(m_cmdln.IsEmpty())
		return;

	int bufflen = sizeof(DWORD);

	POSITION pos = m_cmdln.GetHeadPosition();
	while(pos) bufflen += (m_cmdln.GetNext(pos).GetLength()+1)*sizeof(TCHAR);

	CAutoVectorPtr<BYTE> buff;
	if(!buff.Allocate(bufflen))
		return;

	BYTE* p = buff;

	*(DWORD*)p = m_cmdln.GetCount(); 
	p += sizeof(DWORD);

	pos = m_cmdln.GetHeadPosition();
	while(pos)
	{
		CString s = m_cmdln.GetNext(pos); 
		int len = (s.GetLength()+1)*sizeof(TCHAR);
		memcpy(p, s, len);
		p += len;
	}

	COPYDATASTRUCT cds;
	cds.dwData = 0x6ABE51;
	cds.cbData = bufflen;
	cds.lpData = (void*)(BYTE*)buff;
	SendMessage(hWnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
}

/////////////////////////////////////////////////////////////////////////////
// CMPlayerCApp initialization

#include "..\..\..\include\detours\detours.h"
#include "..\..\..\include\winddk\ntddcdvd.h"

BOOL (__stdcall * Real_IsDebuggerPresent)(void)
= IsDebuggerPresent;

LONG (__stdcall * Real_ChangeDisplaySettingsExA)(LPCSTR a0,
												 LPDEVMODEA a1,
												 HWND a2,
												 DWORD a3,
												 LPVOID a4)
												 = ChangeDisplaySettingsExA;

LONG (__stdcall * Real_ChangeDisplaySettingsExW)(LPCWSTR a0,
												 LPDEVMODEW a1,
												 HWND a2,
												 DWORD a3,
												 LPVOID a4)
												 = ChangeDisplaySettingsExW;

HANDLE (__stdcall * Real_CreateFileA)(LPCSTR a0,
									  DWORD a1,
									  DWORD a2,
									  LPSECURITY_ATTRIBUTES a3,
									  DWORD a4,
									  DWORD a5,
									  HANDLE a6)
									  = CreateFileA;

HANDLE (__stdcall * Real_CreateFileW)(LPCWSTR a0,
									  DWORD a1,
									  DWORD a2,
									  LPSECURITY_ATTRIBUTES a3,
									  DWORD a4,
									  DWORD a5,
									  HANDLE a6)
									  = CreateFileW;

BOOL (__stdcall * Real_DeviceIoControl)(HANDLE a0,
										DWORD a1,
										LPVOID a2,
										DWORD a3,
										LPVOID a4,
										DWORD a5,
										LPDWORD a6,
										LPOVERLAPPED a7)
										= DeviceIoControl;

MMRESULT  (__stdcall * Real_mixerSetControlDetails)( HMIXEROBJ hmxobj, 
													LPMIXERCONTROLDETAILS pmxcd, 
													DWORD fdwDetails)
													= mixerSetControlDetails;

#include <Winternl.h>
typedef NTSTATUS (WINAPI *FUNC_NTQUERYINFORMATIONPROCESS)(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
static FUNC_NTQUERYINFORMATIONPROCESS		Real_NtQueryInformationProcess = NULL;
/*
NTSTATUS (* Real_NtQueryInformationProcess) (HANDLE				ProcessHandle, 
PROCESSINFOCLASS	ProcessInformationClass, 
PVOID				ProcessInformation, 
ULONG				ProcessInformationLength, 
PULONG				ReturnLength)
= NULL;*/

BOOL WINAPI Mine_IsDebuggerPresent()
{
	TRACE(_T("Oops, somebody was trying to be naughty! (called IsDebuggerPresent)\n")); 
	return FALSE;
}
#include "Struct.h"
NTSTATUS WINAPI Mine_NtQueryInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength)
{
	NTSTATUS		nRet;

	nRet = Real_NtQueryInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);

	if (ProcessInformationClass == ProcessBasicInformation)
	{
		PROCESS_BASIC_INFORMATION*		pbi = (PROCESS_BASIC_INFORMATION*)ProcessInformation;
		PEB_NT*							pPEB;
		PEB_NT							PEB;

		pPEB = (PEB_NT*)pbi->PebBaseAddress;
		ReadProcessMemory(ProcessHandle, pPEB, &PEB, sizeof(PEB), NULL);
		PEB.BeingDebugged = 0;
		WriteProcessMemory(ProcessHandle, pPEB, &PEB, sizeof(PEB), NULL);
	}
	else if (ProcessInformationClass == 7) // ProcessDebugPort
	{
		BOOL*		pDebugPort = (BOOL*)ProcessInformation;
		*pDebugPort = FALSE;
	}

	return nRet;
}

LONG WINAPI Mine_ChangeDisplaySettingsEx(LONG ret, DWORD dwFlags, LPVOID lParam)
{
	if(dwFlags&CDS_VIDEOPARAMETERS)
	{
		VIDEOPARAMETERS* vp = (VIDEOPARAMETERS*)lParam;

		if(vp->Guid == GUIDFromCString(_T("{02C62061-1097-11d1-920F-00A024DF156E}"))
			&& (vp->dwFlags&VP_FLAGS_COPYPROTECT))
		{
			if(vp->dwCommand == VP_COMMAND_GET)
			{
				if((vp->dwTVStandard&VP_TV_STANDARD_WIN_VGA)
					&& vp->dwTVStandard != VP_TV_STANDARD_WIN_VGA)
				{
					TRACE(_T("Ooops, tv-out enabled? macrovision checks suck..."));
					vp->dwTVStandard = VP_TV_STANDARD_WIN_VGA;
				}
			}
			else if(vp->dwCommand == VP_COMMAND_SET)
			{
				TRACE(_T("Ooops, as I already told ya, no need for any macrovision bs here"));
				return 0;
			}
		}
	}

	return ret;
}
LONG WINAPI Mine_ChangeDisplaySettingsExA(LPCSTR lpszDeviceName, LPDEVMODEA lpDevMode, HWND hwnd, DWORD dwFlags, LPVOID lParam)
{
	return Mine_ChangeDisplaySettingsEx(
		Real_ChangeDisplaySettingsExA(lpszDeviceName, lpDevMode, hwnd, dwFlags, lParam), 
		dwFlags, 
		lParam);
}
LONG WINAPI Mine_ChangeDisplaySettingsExW(LPCWSTR lpszDeviceName, LPDEVMODEW lpDevMode, HWND hwnd, DWORD dwFlags, LPVOID lParam)
{
	return Mine_ChangeDisplaySettingsEx(
		Real_ChangeDisplaySettingsExW(lpszDeviceName, lpDevMode, hwnd, dwFlags, lParam), 
		dwFlags,
		lParam);
}

HANDLE WINAPI Mine_CreateFileA(LPCSTR p1, DWORD p2, DWORD p3, LPSECURITY_ATTRIBUTES p4, DWORD p5, DWORD p6, HANDLE p7)
{
	//CStringA fn(p1);
	//fn.MakeLower();
	//int i = fn.Find(".part");
	//if(i > 0 && i == fn.GetLength() - 5)
	p3 |= FILE_SHARE_WRITE;

	return Real_CreateFileA(p1, p2, p3, p4, p5, p6, p7);
}
#include "Ifo.h"
BOOL CreateFakeVideoTS(LPCWSTR strIFOPath, LPWSTR strFakeFile, size_t nFakeFileSize)
{
	BOOL		bRet = FALSE;
	WCHAR		szTempPath[MAX_PATH];
	WCHAR		strFileName[MAX_PATH];
	WCHAR		strExt[10];
	CIfo		Ifo;

	if (!GetTempPathW(MAX_PATH, szTempPath)) return FALSE;

	_wsplitpath_s (strIFOPath, NULL, 0, NULL, 0, strFileName, countof(strFileName), strExt, countof(strExt));
	_snwprintf_s  (strFakeFile, nFakeFileSize, _TRUNCATE, L"%sMPC%s%s", szTempPath, strFileName, strExt);

	if (Ifo.OpenFile (strIFOPath) &&
		Ifo.RemoveUOPs()  &&
		Ifo.SaveFile (strFakeFile))
	{
		bRet = TRUE;
	}

	return bRet;
}

HANDLE WINAPI Mine_CreateFileW(LPCWSTR p1, DWORD p2, DWORD p3, LPSECURITY_ATTRIBUTES p4, DWORD p5, DWORD p6, HANDLE p7)
{
	HANDLE	hFile = INVALID_HANDLE_VALUE;
	WCHAR	strFakeFile[MAX_PATH];
	int		nLen  = wcslen(p1);

	p3 |= FILE_SHARE_WRITE;

	if (nLen>=4 && _wcsicmp (p1 + nLen-4, L".ifo") == 0)
	{
		if (CreateFakeVideoTS(p1, strFakeFile, countof(strFakeFile)))
		{
			hFile = Real_CreateFileW(strFakeFile, p2, p3, p4, p5, p6, p7);
		}
	}

	if (hFile == INVALID_HANDLE_VALUE)
		hFile = Real_CreateFileW(p1, p2, p3, p4, p5, p6, p7);

	return hFile;
}


MMRESULT WINAPI Mine_mixerSetControlDetails(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails)
{
	if(fdwDetails == (MIXER_OBJECTF_HMIXER|MIXER_SETCONTROLDETAILSF_VALUE)) 
		return MMSYSERR_NOERROR; // don't touch the mixer, kthx
	return Real_mixerSetControlDetails(hmxobj, pmxcd, fdwDetails);
}

BOOL WINAPI Mine_DeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
{
	BOOL ret = Real_DeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);

	if(IOCTL_DVD_GET_REGION == dwIoControlCode && lpOutBuffer
		&& lpBytesReturned && *lpBytesReturned == sizeof(DVD_REGION))
	{
		DVD_REGION* pDVDRegion = (DVD_REGION*)lpOutBuffer;
		pDVDRegion->SystemRegion = ~pDVDRegion->RegionData;
	}

	return ret;
}

#include "../../subtitles/SSF.h"
#include "../../subtitles/RTS.h"
#include "../../subpic/MemSubPic.h"

class ssftest
{
public:
	ssftest()
	{
		Sleep(10000);

		MessageBeep(-1);
// 8; //
		SubPicDesc spd;
		spd.w = 640;
		spd.h = 480;
		spd.bpp = 32;
		spd.pitch = spd.w*spd.bpp>>3;
		spd.type = MSP_RGB32;
		spd.vidrect = CRect(0, 0, spd.w, spd.h);
		spd.bits = new BYTE[spd.pitch*spd.h];

		CCritSec csLock;
/*
		CRenderedTextSubtitle s(&csLock);
		s.Open(_T("../../subtitles/libssf/demo/demo.ssa"), 1);

		for(int i = 2*60*1000+2000; i < 2*60*1000+17000; i += 10)
		{
			memsetd(spd.bits, 0xff000000, spd.pitch*spd.h);
			CRect bbox;
			bbox.SetRectEmpty();
			s.Render(spd, 10000i64*i, 25, bbox);
		}
*/
		try
		{
			ssf::CRenderer s(&csLock);
			s.Open(_T("../../subtitles/libssf/demo/demo.ssf"));

			for(int i = 2*60*1000+2000; i < 2*60*1000+17000; i += 40)
			// for(int i = 2*60*1000+2000; i < 2*60*1000+17000; i += 1000)
			//for(int i = 0; i < 5000; i += 40)
			{
				memsetd(spd.bits, 0xff000000, spd.pitch*spd.h);
				CRect bbox;
				bbox.SetRectEmpty();
				s.Render(spd, 10000i64*i, 25, bbox);
			}
		}
		catch(ssf::Exception& e)
		{
			TRACE(_T("%s\n"), e.ToString());
			ASSERT(0);
		}

		delete [] spd.bits;

		::ExitProcess(0);
	}
};
void CMPlayerCApp::InitInstanceThreaded(){
	CSVPToolBox svpTool;
	//����ļ�����
	if ( m_s.fCheckFileAsscOnStartup ){
		CChkDefPlayer dlg_chkdefplayer;
		if( ! dlg_chkdefplayer.b_isDefaultPlayer() ){
			if(m_s.fPopupStartUpExtCheck){
				dlg_chkdefplayer.DoModal();
			}else{
				dlg_chkdefplayer.setDefaultPlayer();
			}
		}else{
			dlg_chkdefplayer.setDefaultPlayer();
		}
	}

	CSVPToolBox svpToolBox;
	CStringArray csaDll;
	//csaDll.Add( _T("codecs\\CoreAVCDecoder.ax")); avoid missing reg key problem
	csaDll.Add( _T("codecs\\powerdvd\\CL264dec.ax"));
	for(int i = 0; i < csaDll.GetCount(); i++){
		CString szDllPath = svpToolBox.GetPlayerPath( csaDll.GetAt(i) );
		if(svpToolBox.ifFileExist(szDllPath))
			RegSvr32( szDllPath );
	}

}
UINT __cdecl Thread_InitInstance( LPVOID lpParam ) 
{ 
	CMPlayerCApp * ma =(CMPlayerCApp*) lpParam;
	CoInitialize(NULL);
	ma->InitInstanceThreaded();
	CoUninitialize();
	return 0; 
}
BOOL CMPlayerCApp::InitInstance()
{
	//ssftest s;

	long		lError;
	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourAttach(&(PVOID&)Real_IsDebuggerPresent, (PVOID)Mine_IsDebuggerPresent);
	DetourAttach(&(PVOID&)Real_ChangeDisplaySettingsExA, (PVOID)Mine_ChangeDisplaySettingsExA);
	DetourAttach(&(PVOID&)Real_ChangeDisplaySettingsExW, (PVOID)Mine_ChangeDisplaySettingsExW);
	DetourAttach(&(PVOID&)Real_CreateFileA, (PVOID)Mine_CreateFileA);
	DetourAttach(&(PVOID&)Real_CreateFileW, (PVOID)Mine_CreateFileW);
	DetourAttach(&(PVOID&)Real_mixerSetControlDetails, (PVOID)Mine_mixerSetControlDetails);
	DetourAttach(&(PVOID&)Real_DeviceIoControl, (PVOID)Mine_DeviceIoControl);
#ifndef _DEBUG
	HMODULE hNTDLL	=	LoadLibrary (_T("ntdll.dll"));
	if (hNTDLL)
	{
		Real_NtQueryInformationProcess = (FUNC_NTQUERYINFORMATIONPROCESS)GetProcAddress (hNTDLL, "NtQueryInformationProcess");

		if (Real_NtQueryInformationProcess)
			DetourAttach(&(PVOID&)Real_NtQueryInformationProcess, (PVOID)Mine_NtQueryInformationProcess);
	}
#endif
	CFilterMapper2::Init();

#if !defined(_DEBUG) || !defined(_WIN64)
	lError = DetourTransactionCommit();
	ASSERT (lError == NOERROR);
#endif
	HRESULT hr;
    if(FAILED(hr = OleInitialize(0)))
	{
        AfxMessageBox(_T("OleInitialize failed!"));
		return FALSE;
	}
	

	
    WNDCLASS wndcls;
    memset(&wndcls, 0, sizeof(WNDCLASS));
    wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wndcls.lpfnWndProc = ::DefWindowProc; 
    wndcls.hInstance = AfxGetInstanceHandle();
    wndcls.hIcon = LoadIcon(IDR_MAINFRAME);
    wndcls.hCursor = LoadCursor(IDC_ARROW);
    wndcls.hbrBackground = 0;//(HBRUSH)(COLOR_WINDOW + 1); // no bkg brush, the view and the bars should always fill the whole client area
    wndcls.lpszMenuName = NULL;
    wndcls.lpszClassName = MPC_WND_CLASS_NAME;

	if(!AfxRegisterClass(&wndcls))
    {
		AfxMessageBox(_T("MainFrm class registration failed!"));
		return FALSE;
    }

	if(!AfxSocketInit(NULL))
	{
        AfxMessageBox(_T("AfxSocketInit failed!"));
		return FALSE;
	}

	PreProcessCommandLine();

	if(IsIniValid()) StoreSettingsToIni();
	else StoreSettingsToRegistry();

	CString AppDataPath;
	if(GetAppDataPath(AppDataPath))
		CreateDirectory(AppDataPath, NULL);

	m_s.ParseCommandLine(m_cmdln);

	if(m_s.nCLSwitches&(CLSW_HELP|CLSW_UNRECOGNIZEDSWITCH))
	{
		//if(m_s.nCLSwitches&CLSW_HELP)
			//ShowCmdlnSwitches();

		return FALSE;
	}

	if((m_s.nCLSwitches&CLSW_CLOSE) && m_s.slFiles.IsEmpty())
	{
		return FALSE;
	}

	m_s.UpdateData(false);
	
	if((m_s.nCLSwitches&CLSW_REGEXTVID) || (m_s.nCLSwitches&CLSW_REGEXTAUD))
	{
		CMediaFormats& mf = m_s.Formats;

		for(size_t i = 0; i < mf.GetCount(); i++)
		{
			if(!mf[i].GetLabel().CompareNoCase(_T("Image file"))) continue;
			if(!mf[i].GetLabel().CompareNoCase(_T("Playlist file"))) continue;
				
			bool fAudioOnly = mf[i].IsAudioOnly();

			int j = 0;
			CString str = mf[i].GetExtsWithPeriod();
			for(CString ext = str.Tokenize(_T(" "), j); !ext.IsEmpty(); ext = str.Tokenize(_T(" "), j))
			{
				if(((m_s.nCLSwitches&CLSW_REGEXTVID) && !fAudioOnly) || ((m_s.nCLSwitches&CLSW_REGEXTAUD) && fAudioOnly)) {
					CPPageFormats::RegisterExt(ext, true);
				}
			}
		}

		return FALSE;
	}
	
	if((m_s.nCLSwitches&CLSW_UNREGEXT))
	{
		CMediaFormats& mf = m_s.Formats;

		for(size_t i = 0; i < mf.GetCount(); i++)
		{
			int j = 0;
			CString str = mf[i].GetExtsWithPeriod();
			for(CString ext = str.Tokenize(_T(" "), j); !ext.IsEmpty(); ext = str.Tokenize(_T(" "), j))
			{
				CPPageFormats::RegisterExt(ext, false);
			}
		}

		return FALSE;
	}

	m_mutexOneInstance.Create(NULL, TRUE, MPC_WND_CLASS_NAME);

	if(GetLastError() == ERROR_ALREADY_EXISTS
	&& (!(m_s.fAllowMultipleInst || (m_s.nCLSwitches&CLSW_NEW) || m_cmdln.IsEmpty())
		|| (m_s.nCLSwitches&CLSW_ADD)))
	{
		if(HWND hWnd = ::FindWindow(MPC_WND_CLASS_NAME, NULL))
		{
			SetForegroundWindow(hWnd);

			if(!(m_s.nCLSwitches&CLSW_MINIMIZED) && IsIconic(hWnd))
				ShowWindow(hWnd, SW_RESTORE);

			SendCommandLine(hWnd);

			return FALSE;
		}
	}


	
	if(!__super::InitInstance())
	{
		AfxMessageBox(_T("InitInstance failed!"));
		return FALSE;
	}

	AfxBeginThread(Thread_InitInstance , this,  THREAD_PRIORITY_LOWEST);

	CRegKey key;
	if(ERROR_SUCCESS == key.Create(HKEY_LOCAL_MACHINE, _T("Software\\SVPlayer\\����Ӱ��������")))
	{
		CString path;
		GetModuleFileName(AfxGetInstanceHandle(), path.GetBuffer(MAX_PATH), MAX_PATH);
		path.ReleaseBuffer();
		key.SetStringValue(_T("ExePath"), path);
	}

	AfxEnableControlContainer();

	CMainFrame* pFrame = new CMainFrame;
	m_pMainWnd = pFrame;
	pFrame->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW|FWS_ADDTOTITLE, NULL, NULL);
	pFrame->SetDefaultWindowRect((m_s.nCLSwitches&CLSW_MONITOR)?m_s.iMonitor:0);
	pFrame->RestoreFloatingControlBars();
	pFrame->SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), TRUE);
	pFrame->DragAcceptFiles();
	pFrame->ShowWindow((m_s.nCLSwitches&CLSW_MINIMIZED)?SW_SHOWMINIMIZED:SW_SHOW);
	pFrame->UpdateWindow();
	pFrame->m_hAccelTable = m_s.hAccel;

	m_s.WinLircClient.SetHWND(m_pMainWnd->m_hWnd);
	if(m_s.fWinLirc) m_s.WinLircClient.Connect(m_s.WinLircAddr);
	m_s.UIceClient.SetHWND(m_pMainWnd->m_hWnd);
	if(m_s.fUIce) m_s.UIceClient.Connect(m_s.UIceAddr);

	SendCommandLine(m_pMainWnd->m_hWnd);

	pFrame->SetFocus();

	return TRUE;
}

int CMPlayerCApp::ExitInstance()
{
	m_s.UpdateData(true);

	OleUninitialize();

	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// CMPlayerCApp message handlers
// App command to run the dialog

void CMPlayerCApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CMPlayerCApp::OnFileExit()
{
	OnAppExit();
}

// CRemoteCtrlClient

CRemoteCtrlClient::CRemoteCtrlClient() 
	: m_pWnd(NULL)
	, m_nStatus(DISCONNECTED)
{
}

void CRemoteCtrlClient::SetHWND(HWND hWnd)
{
	CAutoLock cAutoLock(&m_csLock);

	m_pWnd = CWnd::FromHandle(hWnd);
}

void CRemoteCtrlClient::Connect(CString addr)
{
	CAutoLock cAutoLock(&m_csLock);

	if(m_nStatus == CONNECTING && m_addr == addr)
	{
		TRACE(_T("CRemoteCtrlClient (Connect): already connecting to %s\n"), addr);
		return;
	}

	if(m_nStatus == CONNECTED && m_addr == addr)
	{
		TRACE(_T("CRemoteCtrlClient (Connect): already connected to %s\n"), addr);
		return;
	}

	m_nStatus = CONNECTING;

	TRACE(_T("CRemoteCtrlClient (Connect): connecting to %s\n"), addr);

	Close();

	Create();

	CString ip = addr.Left(addr.Find(':')+1).TrimRight(':');
	int port = _tcstol(addr.Mid(addr.Find(':')+1), NULL, 10);

	__super::Connect(ip, port);

	m_addr = addr;
}

void CRemoteCtrlClient::OnConnect(int nErrorCode)
{
	CAutoLock cAutoLock(&m_csLock);

	m_nStatus = (nErrorCode == 0 ? CONNECTED : DISCONNECTED);

	TRACE(_T("CRemoteCtrlClient (OnConnect): %d\n"), nErrorCode);
}

void CRemoteCtrlClient::OnClose(int nErrorCode)
{
	CAutoLock cAutoLock(&m_csLock);

	if(m_hSocket != INVALID_SOCKET && m_nStatus == CONNECTED)
	{
		TRACE(_T("CRemoteCtrlClient (OnClose): connection lost\n"));
	}

	m_nStatus = DISCONNECTED;

	TRACE(_T("CRemoteCtrlClient (OnClose): %d\n"), nErrorCode);
}

void CRemoteCtrlClient::OnReceive(int nErrorCode)
{
	if(nErrorCode != 0 || !m_pWnd) return;

	CStringA str;
	int ret = Receive(str.GetBuffer(256), 255, 0);
	if(ret <= 0) return;
	str.ReleaseBuffer(ret);

	TRACE(_T("CRemoteCtrlClient (OnReceive): %s\n"), CString(str));

	OnCommand(str);

	__super::OnReceive(nErrorCode);
}

void CRemoteCtrlClient::ExecuteCommand(CStringA cmd, int repcnt)
{
	cmd.Trim();
	if(cmd.IsEmpty()) return;
	cmd.Replace(' ', '_');

	AppSettings& s = AfxGetAppSettings();

	POSITION pos = s.wmcmds.GetHeadPosition();
	while(pos)
	{
		wmcmd wc = s.wmcmds.GetNext(pos);
		CStringA name = CString(wc.name);
		name.Replace(' ', '_');
		if((repcnt == 0 && wc.rmrepcnt == 0 || wc.rmrepcnt > 0 && (repcnt%wc.rmrepcnt) == 0)
		&& (!name.CompareNoCase(cmd) || !wc.rmcmd.CompareNoCase(cmd) || wc.cmd == (WORD)strtol(cmd, NULL, 10)))
		{
			CAutoLock cAutoLock(&m_csLock);
			TRACE(_T("CRemoteCtrlClient (calling command): %s\n"), wc.name);
			m_pWnd->SendMessage(WM_COMMAND, wc.cmd);
			break;
		}
	}
}

// CWinLircClient

CWinLircClient::CWinLircClient()
{
}

void CWinLircClient::OnCommand(CStringA str)
{
	TRACE(_T("CWinLircClient (OnCommand): %s\n"), CString(str));

	int i = 0, j = 0, repcnt = 0;
	for(CStringA token = str.Tokenize(" ", i); 
		!token.IsEmpty();
		token = str.Tokenize(" ", i), j++)
	{
		if(j == 1)
			repcnt = strtol(token, NULL, 16);
		else if(j == 2)
			ExecuteCommand(token, repcnt);
	}
}

// CUIceClient

CUIceClient::CUIceClient()
{
}

void CUIceClient::OnCommand(CStringA str)
{
	TRACE(_T("CUIceClient (OnCommand): %s\n"), CString(str));

	CStringA cmd;
	int i = 0, j = 0;
	for(CStringA token = str.Tokenize("|", i); 
		!token.IsEmpty(); 
		token = str.Tokenize("|", i), j++)
	{
		if(j == 0)
			cmd = token;
		else if(j == 1)
			ExecuteCommand(cmd, strtol(token, NULL, 16));
	}
}

// CMPlayerCApp::Settings

CMPlayerCApp::Settings::Settings() 
	: fInitialized(false)
	, MRU(0, _T("Recent File List"), _T("File%d"), 20)
	, MRUDub(0, _T("Recent Dub List"), _T("Dub%d"), 20)
	, MRUUrl(0, _T("Recent Url List"), _T("Url%d"), 20)
	, hAccel(NULL)
{
#define ADDCMD(cmd) wmcmds.AddTail(wmcmd##cmd)
	ADDCMD((ID_BOSS, VK_OEM_3, FVIRTKEY|FCONTROL|FNOINVERT, _T("�ϰ��")));
	ADDCMD((ID_PLAY_PLAYPAUSE, VK_SPACE, FVIRTKEY|FNOINVERT, _T("����/��ͣ"), APPCOMMAND_MEDIA_PLAY_PAUSE, wmcmd::LDOWN));
	ADDCMD((ID_PLAY_SEEKFORWARDMED, VK_RIGHT, FVIRTKEY|FNOINVERT, _T("���")));//
	ADDCMD((ID_PLAY_SEEKBACKWARDMED, VK_LEFT, FVIRTKEY|FNOINVERT, _T("����")));//

	ADDCMD((ID_FILE_OPENQUICK, 'Q', FVIRTKEY|FCONTROL|FNOINVERT, _T("���ٴ��ļ�")));
	ADDCMD((ID_FILE_OPENURLSTREAM, 'U', FVIRTKEY|FCONTROL|FNOINVERT, _T("����ַ")));
	ADDCMD((ID_FILE_OPENMEDIA, 'O', FVIRTKEY|FCONTROL|FNOINVERT, _T("���ļ�")));
	
	ADDCMD((ID_SUBMOVEUP,  VK_OEM_4 /* [ */, FVIRTKEY|FALT|FNOINVERT, _T("����Ļ����")));
	ADDCMD((ID_SUBMOVEDOWN,  VK_OEM_6  /* ] */, FVIRTKEY|FALT|FNOINVERT, _T("����Ļ����")));
	ADDCMD((ID_SUB2MOVEUP,  VK_OEM_4, FVIRTKEY|FALT|FCONTROL|FNOINVERT, _T("�ڶ���Ļ����")));
	ADDCMD((ID_SUB2MOVEDOWN,  VK_OEM_6, FVIRTKEY|FALT|FCONTROL|FNOINVERT, _T("�ڶ���Ļ����")));
	ADDCMD((ID_SUBFONTDOWNBOTH,  VK_F1, FVIRTKEY|FALT|FNOINVERT, _T("��С��Ļ����")));
	ADDCMD((ID_SUBFONTUPBOTH,  VK_F2, FVIRTKEY|FALT|FNOINVERT, _T("�Ŵ���Ļ����")));
	ADDCMD((ID_SUB1FONTDOWN,  VK_F3, FVIRTKEY|FSHIFT|FNOINVERT, _T("��С����Ļ����")));
	ADDCMD((ID_SUB1FONTUP,  VK_F4, FVIRTKEY|FSHIFT|FNOINVERT, _T("�Ŵ�����Ļ����")));
	ADDCMD((ID_SUB1FONTDOWN,  VK_F5, FVIRTKEY|FSHIFT|FNOINVERT, _T("��С����Ļ����")));
	ADDCMD((ID_SUB1FONTUP,  VK_F6, FVIRTKEY|FSHIFT|FNOINVERT, _T("�Ŵ�����Ļ����")));
	
	ADDCMD((ID_SUB_DELAY_DOWN, VK_F1, FVIRTKEY|FNOINVERT, _T("��������Ļ��ʱ")));
	ADDCMD((ID_SUB_DELAY_UP, VK_F2,   FVIRTKEY|FNOINVERT, _T("��������Ļ��ʱ")));

	ADDCMD((ID_BRIGHTINC, VK_HOME, FVIRTKEY|FCONTROL|FNOINVERT, _T("�������")));
	ADDCMD((ID_BRIGHTDEC, VK_END, FVIRTKEY|FCONTROL|FNOINVERT, _T("��������")));
	
	ADDCMD((ID_FILE_OPENDVD, 'D', FVIRTKEY|FCONTROL|FNOINVERT, _T("��DVD")));
	ADDCMD((ID_FILE_OPENDEVICE, 'V', FVIRTKEY|FCONTROL|FNOINVERT, _T("Open Device")));
	ADDCMD((ID_FILE_SAVE_COPY, 0, FVIRTKEY|FNOINVERT, _T("����Ϊ")));
	ADDCMD((ID_FILE_SAVE_IMAGE, 'I', FVIRTKEY|FALT|FNOINVERT, _T("����ͼƬ")));
	ADDCMD((ID_FILE_SAVE_IMAGE_AUTO, VK_F5, FVIRTKEY|FNOINVERT, _T("�Զ�����ͼƬ")));
	ADDCMD((ID_FILE_LOAD_SUBTITLE, 'L', FVIRTKEY|FCONTROL|FNOINVERT, _T("��ȡ��Ļ")));
	ADDCMD((ID_FILE_SAVE_SUBTITLE, 'S', FVIRTKEY|FCONTROL|FNOINVERT, _T("������Ļ")));
	ADDCMD((ID_FILE_CLOSEPLAYLIST, 'C', FVIRTKEY|FCONTROL|FNOINVERT, _T("�ر�")));
	ADDCMD((ID_FILE_PROPERTIES, VK_F10, FVIRTKEY|FSHIFT|FNOINVERT, _T("����")));
	ADDCMD((ID_FILE_EXIT, 'X', FVIRTKEY|FALT|FNOINVERT, _T("�˳�")));
	ADDCMD((ID_TOGGLE_SUBTITLE, 'H', FVIRTKEY|FNOINVERT, _T("���ػ���ʾ��Ļ")));
	ADDCMD((ID_VIEW_VF_FROMINSIDE, 'C', FVIRTKEY|FNOINVERT, _T("���û���λ��")));
	ADDCMD((ID_PLAY_PLAY, 0, FVIRTKEY|FNOINVERT, _T("��ʼ����")));
	ADDCMD((ID_PLAY_PAUSE, 0, FVIRTKEY|FNOINVERT, _T("��ͣ����")));
	ADDCMD((ID_PLAY_STOP, VK_OEM_PERIOD, FVIRTKEY|FNOINVERT, _T("ֹͣ����"), APPCOMMAND_MEDIA_STOP));
	 ADDCMD((ID_PLAY_FRAMESTEP, VK_RIGHT, FVIRTKEY|FCONTROL|FALT|FNOINVERT, _T("��֡ǰ��")));
	 ADDCMD((ID_PLAY_FRAMESTEPCANCEL, VK_LEFT, FVIRTKEY|FCONTROL|FALT|FNOINVERT, _T("��֡����")));
	ADDCMD((ID_PLAY_INCRATE, VK_UP, FVIRTKEY|FCONTROL|FNOINVERT, _T("���ٲ���")));
	ADDCMD((ID_PLAY_DECRATE, VK_DOWN, FVIRTKEY|FCONTROL|FNOINVERT, _T("���ٲ���")));
	ADDCMD((ID_VIEW_FULLSCREEN, VK_RETURN, FVIRTKEY|FALT|FNOINVERT, _T("�л�ȫ��"), 0, wmcmd::LDBLCLK));
	ADDCMD((ID_VIEW_FULLSCREEN, VK_RETURN, FVIRTKEY|FNOINVERT, _T("�л�ȫ��"), 0, wmcmd::MUP));

	ADDCMD((ID_PLAY_GOTO, 'G', FVIRTKEY|FCONTROL|FNOINVERT, _T("Go To")));
	ADDCMD((ID_PLAY_RESETRATE, 'R', FVIRTKEY|FCONTROL|FNOINVERT, _T("Reset Rate")));
	ADDCMD((ID_PLAY_INCAUDDELAY, VK_ADD, FVIRTKEY|FNOINVERT, _T("Audio Delay +10ms")));
	ADDCMD((ID_PLAY_DECAUDDELAY, VK_SUBTRACT, FVIRTKEY|FNOINVERT, _T("Audio Delay -10ms")));
	ADDCMD((ID_PLAY_SEEKFORWARDSMALL, 0, FVIRTKEY|FNOINVERT, _T("Jump Forward (small)")));
	ADDCMD((ID_PLAY_SEEKBACKWARDSMALL, 0, FVIRTKEY|FNOINVERT, _T("Jump Backward (small)")));
	ADDCMD((ID_PLAY_SEEKFORWARDMED, VK_RIGHT, FVIRTKEY|FCONTROL|FNOINVERT, _T("Jump Forward (medium)")));
	ADDCMD((ID_PLAY_SEEKBACKWARDMED, VK_LEFT, FVIRTKEY|FCONTROL|FNOINVERT, _T("Jump Backward (medium)")));
	ADDCMD((ID_PLAY_SEEKFORWARDLARGE, VK_RIGHT, FVIRTKEY|FALT|FNOINVERT, _T("Jump Forward (large)")));
	ADDCMD((ID_PLAY_SEEKBACKWARDLARGE, VK_LEFT, FVIRTKEY|FALT|FNOINVERT, _T("Jump Backward (large)")));
	ADDCMD((ID_PLAY_SEEKKEYFORWARD, VK_RIGHT, FVIRTKEY|FSHIFT|FNOINVERT, _T("Jump Forward (keyframe)")));
	ADDCMD((ID_PLAY_SEEKKEYBACKWARD, VK_LEFT, FVIRTKEY|FSHIFT|FNOINVERT, _T("Jump Backward (keyframe)")));
	ADDCMD((ID_NAVIGATE_SKIPFORWARD, VK_NEXT, FVIRTKEY|FNOINVERT, _T("Next"), APPCOMMAND_MEDIA_NEXTTRACK, wmcmd::X2DOWN));
	ADDCMD((ID_NAVIGATE_SKIPBACK, VK_PRIOR, FVIRTKEY|FNOINVERT, _T("Previous"), APPCOMMAND_MEDIA_PREVIOUSTRACK, wmcmd::X1DOWN));
	ADDCMD((ID_NAVIGATE_SKIPFORWARDPLITEM, VK_NEXT, FVIRTKEY|FCONTROL|FNOINVERT, _T("Next Playlist Item")));
	ADDCMD((ID_NAVIGATE_SKIPBACKPLITEM, VK_PRIOR, FVIRTKEY|FCONTROL|FNOINVERT, _T("Previous Playlist Item")));
	ADDCMD((ID_VIEW_CAPTIONMENU, '0', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Caption&Menu")));
	ADDCMD((ID_VIEW_SEEKER, '1', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Seeker")));
	ADDCMD((ID_VIEW_CONTROLS, '2', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Controls")));
	ADDCMD((ID_VIEW_INFORMATION, '3', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Information")));
	ADDCMD((ID_VIEW_STATISTICS, '4', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Statistics")));
	ADDCMD((ID_VIEW_STATUS, '5', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Status")));
	ADDCMD((ID_VIEW_SUBRESYNC, '6', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Subresync Bar")));
	ADDCMD((ID_VIEW_PLAYLIST, '7', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Playlist Bar")));
	ADDCMD((ID_VIEW_CAPTURE, '8', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Capture Bar")));
	ADDCMD((ID_VIEW_SHADEREDITOR, '9', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Shader Editor Bar")));
	ADDCMD((ID_VIEW_PRESETS_MINIMAL, '1', FVIRTKEY|FNOINVERT, _T("View Minimal")));
	ADDCMD((ID_VIEW_PRESETS_COMPACT, '2', FVIRTKEY|FNOINVERT, _T("View Compact")));
	ADDCMD((ID_VIEW_PRESETS_NORMAL, '3', FVIRTKEY|FNOINVERT, _T("View Normal")));
	ADDCMD((ID_VIEW_FULLSCREEN_SECONDARY, VK_F11, FVIRTKEY|FNOINVERT, _T("Fullscreen (w/o res.change)")));
	ADDCMD((ID_VIEW_ZOOM_50, '1', FVIRTKEY|FALT|FNOINVERT, _T("Zoom 50%")));
	ADDCMD((ID_VIEW_ZOOM_100, '2', FVIRTKEY|FALT|FNOINVERT, _T("Zoom 100%")));
	ADDCMD((ID_VIEW_ZOOM_200, '3', FVIRTKEY|FALT|FNOINVERT, _T("Zoom 200%")));
	ADDCMD((ID_VIEW_ZOOM_AUTOFIT, '4', FVIRTKEY|FALT|FNOINVERT, _T("Zoom Auto Fit")));	
	ADDCMD((ID_ASPECTRATIO_NEXT, 0, FVIRTKEY|FNOINVERT, _T("Next AR Preset")));
	ADDCMD((ID_VIEW_VF_HALF, 0, FVIRTKEY|FNOINVERT, _T("VidFrm Half")));
	ADDCMD((ID_VIEW_VF_NORMAL, 0, FVIRTKEY|FNOINVERT, _T("VidFrm Normal")));
	ADDCMD((ID_VIEW_VF_DOUBLE, 0, FVIRTKEY|FNOINVERT, _T("VidFrm Double")));
	ADDCMD((ID_VIEW_VF_STRETCH, 0, FVIRTKEY|FNOINVERT, _T("VidFrm Stretch")));
	ADDCMD((ID_VIEW_VF_FROMINSIDE, 0, FVIRTKEY|FNOINVERT, _T("VidFrm Inside")));
	ADDCMD((ID_VIEW_VF_FROMOUTSIDE, 0, FVIRTKEY|FNOINVERT, _T("VidFrm Outside")));
	ADDCMD((ID_ONTOP_ALWAYS, 'T', FVIRTKEY|FCONTROL|FNOINVERT, _T("Always On Top")));
	ADDCMD((ID_VIEW_RESET, VK_NUMPAD5, FVIRTKEY|FNOINVERT, _T("PnS Reset")));
	ADDCMD((ID_VIEW_INCSIZE, VK_NUMPAD9, FVIRTKEY|FNOINVERT, _T("PnS Inc Size")));
	ADDCMD((ID_VIEW_INCWIDTH, VK_NUMPAD6, FVIRTKEY|FNOINVERT, _T("PnS Inc Width")));
	ADDCMD((ID_VIEW_INCHEIGHT, VK_NUMPAD8, FVIRTKEY|FNOINVERT, _T("PnS Inc Height")));
	ADDCMD((ID_VIEW_DECSIZE, VK_NUMPAD1, FVIRTKEY|FNOINVERT, _T("PnS Dec Size")));
	ADDCMD((ID_VIEW_DECWIDTH, VK_NUMPAD4, FVIRTKEY|FNOINVERT, _T("PnS Dec Width")));
	ADDCMD((ID_VIEW_DECHEIGHT, VK_NUMPAD2, FVIRTKEY|FNOINVERT, _T("PnS Dec Height")));
	ADDCMD((ID_PANSCAN_CENTER, VK_NUMPAD5, FVIRTKEY|FCONTROL|FNOINVERT, _T("PnS Center")));
	ADDCMD((ID_PANSCAN_MOVELEFT, VK_NUMPAD4, FVIRTKEY|FCONTROL|FNOINVERT, _T("PnS Left")));
	ADDCMD((ID_PANSCAN_MOVERIGHT, VK_NUMPAD6, FVIRTKEY|FCONTROL|FNOINVERT, _T("PnS Right")));
	ADDCMD((ID_PANSCAN_MOVEUP, VK_NUMPAD8, FVIRTKEY|FCONTROL|FNOINVERT, _T("PnS Up")));
	ADDCMD((ID_PANSCAN_MOVEDOWN, VK_NUMPAD2, FVIRTKEY|FCONTROL|FNOINVERT, _T("PnS Down")));
	ADDCMD((ID_PANSCAN_MOVEUPLEFT, VK_NUMPAD7, FVIRTKEY|FCONTROL|FNOINVERT, _T("PnS Up/Left")));
	ADDCMD((ID_PANSCAN_MOVEUPRIGHT, VK_NUMPAD9, FVIRTKEY|FCONTROL|FNOINVERT, _T("PnS Up/Right")));
	ADDCMD((ID_PANSCAN_MOVEDOWNLEFT, VK_NUMPAD1, FVIRTKEY|FCONTROL|FNOINVERT, _T("PnS Down/Left")));
	ADDCMD((ID_PANSCAN_MOVEDOWNRIGHT, VK_NUMPAD3, FVIRTKEY|FCONTROL|FNOINVERT, _T("PnS Down/Right")));
	ADDCMD((ID_PANSCAN_ROTATEXP, VK_NUMPAD8, FVIRTKEY|FALT|FNOINVERT, _T("PnS Rotate X+")));
	ADDCMD((ID_PANSCAN_ROTATEXM, VK_NUMPAD2, FVIRTKEY|FALT|FNOINVERT, _T("PnS Rotate X-")));
	ADDCMD((ID_PANSCAN_ROTATEYP, VK_NUMPAD4, FVIRTKEY|FALT|FNOINVERT, _T("PnS Rotate Y+")));
	ADDCMD((ID_PANSCAN_ROTATEYM, VK_NUMPAD6, FVIRTKEY|FALT|FNOINVERT, _T("PnS Rotate Y-")));
	ADDCMD((ID_PANSCAN_ROTATEZP, VK_NUMPAD1, FVIRTKEY|FALT|FNOINVERT, _T("PnS Rotate Z+")));
	ADDCMD((ID_PANSCAN_ROTATEZM, VK_NUMPAD3, FVIRTKEY|FALT|FNOINVERT, _T("PnS Rotate Z-")));
	ADDCMD((ID_VOLUME_UP, VK_UP, FVIRTKEY|FNOINVERT, _T("Volume Up"), 0, wmcmd::WUP));//APPCOMMAND_VOLUME_UP
	ADDCMD((ID_VOLUME_DOWN, VK_DOWN, FVIRTKEY|FNOINVERT, _T("Volume Down"), 0, wmcmd::WDOWN));//APPCOMMAND_VOLUME_DOWN
	ADDCMD((ID_VOLUME_MUTE, 'M', FVIRTKEY|FCONTROL|FNOINVERT, _T("Volume Mute")));//, APPCOMMAND_VOLUME_MUTE
	ADDCMD((ID_VOLUME_BOOST_INC, 0, FVIRTKEY|FNOINVERT, _T("Volume Boost Increase")));
	ADDCMD((ID_VOLUME_BOOST_DEC, 0, FVIRTKEY|FNOINVERT, _T("Volume Boost Decrease")));
	ADDCMD((ID_VOLUME_BOOST_MIN, 0, FVIRTKEY|FNOINVERT, _T("Volume Boost Min")));
	ADDCMD((ID_VOLUME_BOOST_MAX, 0, FVIRTKEY|FNOINVERT, _T("Volume Boost Max")));
	ADDCMD((ID_NAVIGATE_TITLEMENU, 0, FVIRTKEY|FNOINVERT, _T("DVD Title Menu")));
	ADDCMD((ID_NAVIGATE_ROOTMENU, 0, FVIRTKEY|FNOINVERT, _T("DVD Root Menu")));
	ADDCMD((ID_NAVIGATE_SUBPICTUREMENU, 0, FVIRTKEY|FNOINVERT, _T("DVD Subtitle Menu")));
	ADDCMD((ID_NAVIGATE_AUDIOMENU, 0, FVIRTKEY|FNOINVERT, _T("DVD Audio Menu")));
	ADDCMD((ID_NAVIGATE_ANGLEMENU, 0, FVIRTKEY|FNOINVERT, _T("DVD Angle Menu")));
	ADDCMD((ID_NAVIGATE_CHAPTERMENU, 0, FVIRTKEY|FNOINVERT, _T("DVD Chapter Menu")));
	ADDCMD((ID_NAVIGATE_MENU_LEFT, 0, FVIRTKEY|FNOINVERT, _T("DVD Menu Left")));
	ADDCMD((ID_NAVIGATE_MENU_RIGHT, 0, FVIRTKEY|FNOINVERT, _T("DVD Menu Right")));
	ADDCMD((ID_NAVIGATE_MENU_UP, 0, FVIRTKEY|FNOINVERT, _T("DVD Menu Up")));
	ADDCMD((ID_NAVIGATE_MENU_DOWN, 0, FVIRTKEY|FNOINVERT, _T("DVD Menu Down")));
	ADDCMD((ID_NAVIGATE_MENU_ACTIVATE, 0, FVIRTKEY|FNOINVERT, _T("DVD Menu Activate")));
	ADDCMD((ID_NAVIGATE_MENU_BACK, 0, FVIRTKEY|FNOINVERT, _T("DVD Menu Back")));
	ADDCMD((ID_NAVIGATE_MENU_LEAVE, 0, FVIRTKEY|FNOINVERT, _T("DVD Menu Leave")));
	
	ADDCMD((ID_MENU_PLAYER_SHORT, 0, FVIRTKEY|FNOINVERT, _T("Player Menu (short)"), 0, wmcmd::RUP));
	ADDCMD((ID_MENU_PLAYER_LONG, 0, FVIRTKEY|FNOINVERT, _T("Player Menu (long)")));
	ADDCMD((ID_MENU_FILTERS, 0, FVIRTKEY|FNOINVERT, _T("Filters Menu")));
	ADDCMD((ID_VIEW_OPTIONS, 'O', FVIRTKEY|FNOINVERT, _T("Options")));
	ADDCMD((ID_STREAM_AUDIO_NEXT, 'A', FVIRTKEY|FNOINVERT, _T("Next Audio")));
	ADDCMD((ID_STREAM_AUDIO_PREV, 'A', FVIRTKEY|FSHIFT|FNOINVERT, _T("Prev Audio")));
	ADDCMD((ID_STREAM_SUB_NEXT, 'S', FVIRTKEY|FNOINVERT, _T("Next Subtitle")));
	ADDCMD((ID_STREAM_SUB_PREV, 'S', FVIRTKEY|FSHIFT|FNOINVERT, _T("Prev Subtitle")));
	ADDCMD((ID_STREAM_SUB_ONOFF, 'W', FVIRTKEY|FNOINVERT, _T("On/Off Subtitle")));
	ADDCMD((ID_SUBTITLES_SUBITEM_START+2, 0, FVIRTKEY|FNOINVERT, _T("Reload Subtitles")));
	ADDCMD((ID_OGM_AUDIO_NEXT, 0, FVIRTKEY|FNOINVERT, _T("Next Audio (OGM)")));
	ADDCMD((ID_OGM_AUDIO_PREV, 0, FVIRTKEY|FNOINVERT, _T("Prev Audio (OGM)")));
	ADDCMD((ID_OGM_SUB_NEXT, 0, FVIRTKEY|FNOINVERT, _T("Next Subtitle (OGM)")));
	ADDCMD((ID_OGM_SUB_PREV, 0, FVIRTKEY|FNOINVERT, _T("Prev Subtitle (OGM)")));
	ADDCMD((ID_DVD_ANGLE_NEXT, 0, FVIRTKEY|FNOINVERT, _T("Next Angle (DVD)")));
	ADDCMD((ID_DVD_ANGLE_PREV, 0, FVIRTKEY|FNOINVERT, _T("Prev Angle (DVD)")));
	ADDCMD((ID_DVD_AUDIO_NEXT, 0, FVIRTKEY|FNOINVERT, _T("Next Audio (DVD)")));
	ADDCMD((ID_DVD_AUDIO_PREV, 0, FVIRTKEY|FNOINVERT, _T("Prev Audio (DVD)")));
	ADDCMD((ID_DVD_SUB_NEXT, 0, FVIRTKEY|FNOINVERT, _T("Next Subtitle (DVD)")));
	ADDCMD((ID_DVD_SUB_PREV, 0, FVIRTKEY|FNOINVERT, _T("Prev Subtitle (DVD)")));
	ADDCMD((ID_DVD_SUB_ONOFF, 0, FVIRTKEY|FNOINVERT, _T("On/Off Subtitle (DVD)")));
#undef ADDCMD
}

CMPlayerCApp::Settings::~Settings()
{
	if(hAccel)
		DestroyAcceleratorTable(hAccel);
}
void CMPlayerCApp::Settings::RegGlobalAccelKey(HWND hWnd){
	if(!hWnd) {
		if(AfxGetMyApp()->m_pMainWnd)
			hWnd = AfxGetMyApp()->m_pMainWnd->m_hWnd;
		if(!hWnd) 
			return;
	}


	POSITION pos = wmcmds.GetHeadPosition();
	while(pos){
		wmcmd& wc = wmcmds.GetNext(pos);
		if(wc.name == _T("�ϰ��")){
			UINT modKey = 0;
			if( wc.fVirt & FCONTROL) {modKey |= MOD_CONTROL;}
			if( wc.fVirt & FALT) {modKey |= MOD_ALT;}
			if( wc.fVirt & FSHIFT) {modKey |= MOD_SHIFT;}
			UnregisterHotKey(hWnd, ID_BOSS);
			RegisterHotKey(hWnd, ID_BOSS, modKey, wc.key); 
		}
	}
}
void CMPlayerCApp::Settings::ThreadedLoading(){
	Sleep(4000);
	CSVPToolBox svptoolbox;
	CWinApp* pApp = AfxGetApp();
	if(!bNotChangeFontToYH){
		BOOL bHadYaheiDownloaded = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS),  _T("HasYaheiDownloaded"), 0); //Ĭ�ϼ���Ƿ�ʹ�þ�����
		if(!svptoolbox.bFontExist(_T("΢���ź�")) && !svptoolbox.bFontExist(_T("Microsoft YaHei")) ){ 
			CString szTTFPath = svptoolbox.GetPlayerPath( _T("msyh.ttf") );
			if( svptoolbox.ifFileExist(szTTFPath)) {
				if( AddFontResourceEx( szTTFPath , FR_PRIVATE, 0) ){

					if(bHadYaheiDownloaded != 1)  //�״γɹ��������ⲿ���壬�´β��ټ���Ƿ�ʹ�þ�����
						pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), _T("HasYaheiDownloaded"), 1 );	
				}else{
					if(bHadYaheiDownloaded != 0)  //û�гɹ������ⲿ���壬�´μ���Ƿ�ʹ�þ�����
						pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), _T("HasYaheiDownloaded"), 0 );	
				}
			}
		}
		if(!bHadYaheiDownloaded ){
			if(svptoolbox.bFontExist(_T("΢���ź�"))){ //
				if(subdefstyle.fontName.CompareNoCase(_T("����") ) == 0 )
					subdefstyle.fontName = _T("΢���ź�");
				if(subdefstyle2.fontName.CompareNoCase(_T("����") ) == 0 )
					subdefstyle2.fontName = _T("΢���ź�");

			}else if( !svptoolbox.bFontExist(_T("����")) ){
				if(subdefstyle.fontName.CompareNoCase(_T("����") ) == 0 )
					subdefstyle.fontName = _T("SimHei");
				if(subdefstyle2.fontName.CompareNoCase(_T("����") ) == 0 )
					subdefstyle2.fontName = _T("SimHei");
			}
			if(svptoolbox.bFontExist(_T("Microsoft YaHei"))){ //Microsoft YaHei
				if(subdefstyle.fontName.CompareNoCase(_T("SimHei") ) == 0 )
					subdefstyle.fontName = _T("Microsoft YaHei");
				if(subdefstyle2.fontName.CompareNoCase(_T("SimHei") ) == 0 )
					subdefstyle2.fontName = _T("Microsoft YaHei");
			}
		}
	}

}
UINT __cdecl Thread_AppSettingLoadding( LPVOID lpParam ) 
{ 
	CMPlayerCApp::Settings * ms =(CMPlayerCApp::Settings*) lpParam;
	ms->ThreadedLoading();
	return 0; 
}
void CMPlayerCApp::Settings::UpdateData(bool fSave)
{
	CWinApp* pApp = AfxGetApp();
	ASSERT(pApp);

	UINT len;
	BYTE* ptr = NULL;

	if(fSave)
	{
		if(!fInitialized) return;

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_CHECKFILEASSCONSTARTUP), fCheckFileAsscOnStartup);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_POPSTARTUPEXTCHECK), fPopupStartUpExtCheck);
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_CHECKFILEEXTSASSCONSTARTUP), szStartUPCheckExts);
		
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_CHECKUPDATERINTERLEAVE), tCheckUpdaterInterleave);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LASTCHECKUPDATER), tLastCheckUpdater);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTORESUMEPLAY), autoResumePlay);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_HIDECAPTIONMENU), fHideCaptionMenu);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_CONTROLSTATE), nCS);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DEFAULTVIDEOFRAME), iDefaultVideoSize);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_KEEPASPECTRATIO), fKeepAspectRatio);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_COMPMONDESKARDIFF), fCompMonDeskARDiff);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_VOLUME), nVolume);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_BALANCE), nBalance);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MUTE), fMute);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOOPNUM), nLoops);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOOP), fLoopForever);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REWIND), fRewind);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ZOOM), iZoomLevel);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MULTIINST), fAllowMultipleInst);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_TITLEBARTEXTSTYLE), iTitleBarTextStyle);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_TITLEBARTEXTTITLE), fTitleBarTextTitle);		
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ONTOP), iOnTop);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_TRAYICON), fTrayIcon);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTOZOOM), fRememberZoomLevel);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_FULLSCREENCTRLS), fShowBarsWhenFullScreen);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_FULLSCREENCTRLSTIMEOUT), nShowBarsWhenFullScreenTimeOut);
		pApp->WriteProfileBinary(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_FULLSCREENRES), (BYTE*)&dmFullscreenRes, sizeof(dmFullscreenRes));
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_EXITFULLSCREENATTHEEND), fExitFullScreenAtTheEnd);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REMEMBERWINDOWPOS), fRememberWindowPos);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REMEMBERWINDOWSIZE), fRememberWindowSize);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SNAPTODESKTOPEDGES), fSnapToDesktopEdges);		
		pApp->WriteProfileBinary(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LASTWINDOWRECT), (BYTE*)&rcLastWindowPos, sizeof(rcLastWindowPos));
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LASTWINDOWTYPE), lastWindowType);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ASPECTRATIO_X), AspectRatio.cx);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ASPECTRATIO_Y), AspectRatio.cy);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_KEEPHISTORY), fKeepHistory);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_USEGPUACEL), useGPUAcel);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_USEGPUCUDA), useGPUCUDA);
		
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTODOWNLAODSVPSUB), autoDownloadSVPSub);
		

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DSVIDEORENDERERTYPE), iDSVideoRendererType);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_RMVIDEORENDERERTYPE), iRMVideoRendererType);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_QTVIDEORENDERERTYPE), iQTVideoRendererType);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_APSURACEFUSAGE), iAPSurfaceUsage);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_VMRSYNCFIX), fVMRSyncFix);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DX9_RESIZER), iDX9Resizer);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_VMR9MIXERMODE), fVMR9MixerMode);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_VMR9MIXERYUV), fVMR9MixerYUV);
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUDIORENDERERTYPE), CString(AudioRendererDisplayName));
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTOLOADAUDIO), fAutoloadAudio);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTOLOADSUBTITLES), fAutoloadSubtitles);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_BLOCKVSFILTER), fBlockVSFilter);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLEWORKERTHREADFOROPENING), fEnableWorkerThreadForOpening);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REPORTFAILEDPINS), fReportFailedPins);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_UPLOADFAILEDPINS), fUploadFailedPinsInfo);
		
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DVDPATH), sDVDPath);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_USEDVDPATH), fUseDVDPath);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MENULANG), idMenuLang);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUDIOLANG), idAudioLang);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SUBTITLESLANG), idSubtitlesLang);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTOSPEAKERCONF), fAutoSpeakerConf);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ONLYUSEINTERNALDEC), onlyUseInternalDec);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_USESMARTDRAG), useSmartDrag);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DECSPEAKERS), iDecSpeakers);
		

		CString style;
		CString style2;
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPLOGFONT), style <<= subdefstyle);
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPLOGFONT2), style2 <<= subdefstyle2);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPOVERRIDEPLACEMENT), fOverridePlacement);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPOVERRIDEPLACEMENT)+_T("2"), fOverridePlacement2);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPHORPOS), nHorPos);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPVERPOS), nVerPos);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPHORPOS2), nHorPos2);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPVERPOS2), nVerPos2);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPCSIZE), nSPCSize);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPCMAXRES), nSPCMaxRes);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SUBDELAYINTERVAL), nSubDelayInterval);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_POW2TEX), fSPCPow2Tex);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLESUBTITLES), fEnableSubtitles);		
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLESUBTITLES2), fEnableSubtitles2);		
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLEAUDIOSWITCHER), fEnableAudioSwitcher);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLEAUDIOTIMESHIFT), fAudioTimeShift);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUDIOTIMESHIFT), tAudioTimeShift);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DOWNSAMPLETO441), fDownSampleTo441);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_CUSTOMCHANNELMAPPING), fCustomChannelMapping);
		pApp->WriteProfileBinary(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPEAKERTOCHANNELMAPPING), (BYTE*)pSpeakerToChannelMap, sizeof(pSpeakerToChannelMap));
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUDIONORMALIZE), fAudioNormalize);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUDIONORMALIZERECOVER), fAudioNormalizeRecover);		
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUDIOBOOST), (int)AudioBoost);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_USEINTERNALTSSPLITER), fUseInternalTSSpliter);
		

		//pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DISABLESMARTDRAG),  disableSmartDrag );
		
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SHADERLIST), strShaderList);

		{
			for(int i = 0; ; i++)
			{
				CString key;
				key.Format(_T("%s\\%04d"), ResStr(IDS_R_FILTERS), i);
				int j = pApp->GetProfileInt(key, _T("Enabled"), -1); 
				pApp->WriteProfileString(key, NULL, NULL);
				if(j < 0) break;
			}
			pApp->WriteProfileString(ResStr(IDS_R_FILTERS), NULL, NULL);

			POSITION pos = filters.GetHeadPosition();
			for(int i = 0; pos; i++)
			{
				FilterOverride* f = filters.GetNext(pos);

				if(f->fTemporary)
					continue;

				CString key;
				key.Format(_T("%s\\%04d"), ResStr(IDS_R_FILTERS), i);

				pApp->WriteProfileInt(key, _T("SourceType"), (int)f->type);
				pApp->WriteProfileInt(key, _T("Enabled"), (int)!f->fDisabled);
				if(f->type == FilterOverride::REGISTERED)
				{
					pApp->WriteProfileString(key, _T("DisplayName"), CString(f->dispname));
					pApp->WriteProfileString(key, _T("Name"), f->name);
				}
				else if(f->type == FilterOverride::EXTERNAL)
				{
					pApp->WriteProfileString(key, _T("Path"), f->path);
					pApp->WriteProfileString(key, _T("Name"), f->name);
					pApp->WriteProfileString(key, _T("CLSID"), CStringFromGUID(f->clsid));
				}
				POSITION pos2 = f->backup.GetHeadPosition();
				for(int i = 0; pos2; i++)
				{
					CString val;
					val.Format(_T("org%04d"), i);
					pApp->WriteProfileString(key, val, CStringFromGUID(f->backup.GetNext(pos2)));
				}
				pos2 = f->guids.GetHeadPosition();
				for(int i = 0; pos2; i++)
				{
					CString val;
					val.Format(_T("mod%04d"), i);
					pApp->WriteProfileString(key, val, CStringFromGUID(f->guids.GetNext(pos2)));
				}
				pApp->WriteProfileInt(key, _T("LoadType"), f->iLoadType);
				pApp->WriteProfileInt(key, _T("Merit"), f->dwMerit);
			}
		}

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_INTREALMEDIA), fIntRealMedia);
		// pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REALMEDIARENDERLESS), fRealMediaRenderless);
		// pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_QUICKTIMERENDERER), iQuickTimeRenderer);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REALMEDIAFPS), *((DWORD*)&RealMediaQuickTimeFPS));

		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS) + _T("\\") + ResStr(IDS_RS_PNSPRESETS), NULL, NULL);
		for(int i = 0, j = m_pnspresets.GetCount(); i < j; i++)
		{
			CString str;
			str.Format(_T("Preset%d"), i);
			pApp->WriteProfileString(ResStr(IDS_R_SETTINGS) + _T("\\") + ResStr(IDS_RS_PNSPRESETS), str, m_pnspresets[i]);
		}

		pApp->WriteProfileString(ResStr(IDS_R_COMMANDS), NULL, NULL);
		POSITION pos = wmcmds.GetHeadPosition();
		for(int i = 0; pos; )
		{
			wmcmd& wc = wmcmds.GetNext(pos);
			if(wc.IsModified())
			{
				CString str;
				str.Format(_T("CommandMod%d"), i);
				CString str2;
				str2.Format(_T("%d %x %x %s %d %d %d"), 
					wc.cmd, wc.fVirt, wc.key, 
					_T("\"") + CString(wc.rmcmd) +  _T("\""), wc.rmrepcnt,
					wc.mouse, wc.appcmd);
				pApp->WriteProfileString(ResStr(IDS_R_COMMANDS), str, str2);
				i++;
			}
		}
		CString		strTemp;

		strTemp.Format (_T("%f"), dBrightness);
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_COLOR_BRIGHTNESS), strTemp);
		strTemp.Format (_T("%f"), dContrast);
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_COLOR_CONTRAST), strTemp);
		strTemp.Format (_T("%f"), dHue);
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_COLOR_HUE), strTemp);
		strTemp.Format (_T("%f"), dSaturation);
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_COLOR_SATURATION), strTemp);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WINLIRC), fWinLirc);
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WINLIRCADDR), WinLircAddr);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_UICE), fUIce);
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_UICEADDR), UIceAddr);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DISABLEXPTOOLBARS), fDisabeXPToolbars);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_USEWMASFREADER), fUseWMASFReader);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_JUMPDISTS), nJumpDistS);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_JUMPDISTM), nJumpDistM);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_JUMPDISTL), nJumpDistL);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_FREEWINDOWRESIZING), fFreeWindowResizing);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_NOTIFYMSN), fNotifyMSN);		
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_NOTIFYGTSDLL), fNotifyGTSdll);

		Formats.UpdateData(true);

		pApp->WriteProfileInt(ResStr(IDS_R_INTERNAL_FILTERS), ResStr(IDS_RS_SRCFILTERS), SrcFilters|~(SRC_LAST-1));
		pApp->WriteProfileInt(ResStr(IDS_R_INTERNAL_FILTERS), ResStr(IDS_RS_TRAFILTERS), TraFilters|~(TRA_LAST-1));

		//pApp->WriteProfileInt(ResStr(IDS_R_INTERNAL_FILTERS), ResStr(IDS_RS_DXVAFILTERS), DXVAFilters|~(DXVA_LAST-1));
		//pApp->WriteProfileInt(ResStr(IDS_R_INTERNAL_FILTERS), ResStr(IDS_RS_FFMPEGFILTERS), FFmpegFilters|~(FFM_LAST-1));

		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOGOFILE), logofn);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOGOID), logoid);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOGOEXT), logoext);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_HIDECDROMSSUBMENU), fHideCDROMsSubMenu);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_PRIORITY), priority);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LAUNCHFULLSCREEN), launchfullscreen);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLEWEBSERVER), fEnableWebServer);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBSERVERPORT), nWebServerPort);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBSERVERPRINTDEBUGINFO), fWebServerPrintDebugInfo);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBSERVERUSECOMPRESSION), fWebServerUseCompression);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBSERVERLOCALHOSTONLY), fWebServerLocalhostOnly);
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBROOT), WebRoot);
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBDEFINDEX), WebDefIndex);
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBSERVERCGI), WebServerCGI);

		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SNAPSHOTPATH), SnapShotPath);
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SNAPSHOTEXT), SnapShotExt);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_THUMBROWS), ThumbRows);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_THUMBCOLS), ThumbCols);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_THUMBWIDTH), ThumbWidth);		

		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ISDB), ISDb);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SHOWCONTROLBAR),bShowControlBar);

		pApp->WriteProfileString(_T("Shaders"), NULL, NULL);
		pApp->WriteProfileInt(_T("Shaders"), _T("Initialized"), 1);
		pApp->WriteProfileString(_T("Shaders"), _T("Combine"), m_shadercombine);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_NOTCHANGEFONTTOYH), bNotChangeFontToYH);

		pos = m_shaders.GetHeadPosition();
		for(int i = 0; pos; i++)
		{
			const Shader& s = m_shaders.GetNext(pos);

			if(!s.label.IsEmpty())
			{
				CString index;
				index.Format(_T("%d"), i);
				CString srcdata = s.srcdata;
				srcdata.Replace(_T("\r"), _T(""));
				srcdata.Replace(_T("\n"), _T("\\n"));
				srcdata.Replace(_T("\t"), _T("\\t"));
				AfxGetApp()->WriteProfileString(_T("Shaders"), index, s.label + _T("|") + s.target + _T("|") + srcdata);
			}
		}

		if(pApp->m_pszRegistryKey)
		{
			// WINBUG: on win2k this would crash WritePrivateProfileString
			pApp->WriteProfileInt(_T(""), _T(""), pApp->GetProfileInt(_T(""), _T(""), 0)?0:1);
		}
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), _T("LastVersion"), 142);		
		
	}
	else
	{
		if(fInitialized) return;

		OSVERSIONINFO vi;
		vi.dwOSVersionInfoSize = sizeof(vi);
		GetVersionEx(&vi);
		fXpOrBetter = (vi.dwMajorVersion >= 5 && vi.dwMinorVersion >= 1 || vi.dwMajorVersion >= 6);
		int iUpgradeReset =  pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), _T("LastVersion"), 1) ;
		/*/	RegQueryStringValue( HKLM, 'SOFTWARE\Microsoft\DirectX', 'Version', sVersion );
		DirectX 8.0 is 4.8.0
		DirectX 8.1 is 4.8.1
		DirectX 9.0 is 4.9.0
		//*/

		iDXVer = 0;
		CRegKey dxver;
		if(ERROR_SUCCESS == dxver.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\DirectX"), KEY_READ))
		{
			CString str;
			ULONG len = 64;
			if(ERROR_SUCCESS == dxver.QueryStringValue(_T("Version"), str.GetBuffer(len), &len))
			{
				str.ReleaseBuffer(len);
				int ver[4];
				_stscanf(str, _T("%d.%d.%d.%d"), ver+0, ver+1, ver+2, ver+3);
				iDXVer = ver[1];
			}
		}

		fCheckFileAsscOnStartup = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_CHECKFILEASSCONSTARTUP), 1);
		szStartUPCheckExts = pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_CHECKFILEEXTSASSCONSTARTUP), _T(".mkv .avi .rmvb .rm .wmv .asf .mov .mp4 .mpeg .mpg .3gp"));
		fPopupStartUpExtCheck = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_POPSTARTUPEXTCHECK), 1);
		
		tCheckUpdaterInterleave = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_CHECKUPDATERINTERLEAVE),  86400);
		tLastCheckUpdater = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LASTCHECKUPDATER),  0);
		

		autoResumePlay = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTORESUMEPLAY), 1);
		fHideCaptionMenu = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_HIDECAPTIONMENU), 0);
		nCS = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_CONTROLSTATE), CS_SEEKBAR|CS_TOOLBAR|CS_STATUSBAR) & ~CS_COLORCONTROLBAR;


		iDefaultVideoSize = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DEFAULTVIDEOFRAME), DVS_FROMINSIDE);
		fKeepAspectRatio = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_KEEPASPECTRATIO), TRUE);
		fCompMonDeskARDiff = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_COMPMONDESKARDIFF), FALSE);
		nVolume = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_VOLUME), 100);
		nBalance = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_BALANCE), 0);
		fMute = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MUTE), 0);
		nLoops = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOOPNUM), 2);
		fLoopForever = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOOP), 0);
		fRewind = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REWIND), FALSE);
		iZoomLevel = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ZOOM), 1);

		fForceRGBrender = 0;
		if(IsInsideVMWare() ){
			fForceRGBrender = 1;
			iDXVer = 7;
		}
		iDSVideoRendererType = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DSVIDEORENDERERTYPE), ( (IsVista() || iDXVer >= 9) ? VIDRNDT_DS_VMR9RENDERLESS : VIDRNDT_DS_VMR7RENDERLESS) );
		iRMVideoRendererType = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_RMVIDEORENDERERTYPE), ( (IsVista() || iDXVer >= 9) ? VIDRNDT_RM_DX9 : VIDRNDT_RM_DX7 ) );
		iQTVideoRendererType = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_QTVIDEORENDERERTYPE),  ( (IsVista() || iDXVer >= 9) ? VIDRNDT_QT_DX9 : VIDRNDT_QT_DX7 ) );
		iAPSurfaceUsage = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_APSURACEFUSAGE), VIDRNDT_AP_TEXTURE2D);
		useGPUAcel = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_USEGPUACEL), 0);
		useGPUCUDA = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_USEGPUCUDA), 0);
		useGPUCUDA = SVP_SetCoreAvcCUDA(useGPUCUDA);

		autoDownloadSVPSub = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTODOWNLAODSVPSUB), 1);
		fVMRSyncFix = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_VMRSYNCFIX), TRUE);
		iDX9Resizer = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DX9_RESIZER), 1);
		fVMR9MixerMode = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_VMR9MIXERMODE), FALSE);
		fVMR9MixerYUV = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_VMR9MIXERYUV), FALSE);
		AudioRendererDisplayName = pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUDIORENDERERTYPE), _T(""));
		fAutoloadAudio = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTOLOADAUDIO), TRUE);
		fAutoloadSubtitles = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTOLOADSUBTITLES), TRUE );//!IsVSFilterInstalled()
		fBlockVSFilter = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_BLOCKVSFILTER), TRUE);
		fEnableWorkerThreadForOpening = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLEWORKERTHREADFOROPENING), TRUE);
		fReportFailedPins = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REPORTFAILEDPINS), TRUE);
		fUploadFailedPinsInfo = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_UPLOADFAILEDPINS), TRUE );
		fAllowMultipleInst = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MULTIINST), 0);
		iTitleBarTextStyle = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_TITLEBARTEXTSTYLE), 1);
		fTitleBarTextTitle = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_TITLEBARTEXTTITLE), FALSE);
		iOnTop = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ONTOP), 0);
		fTrayIcon = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_TRAYICON), 0);
		fRememberZoomLevel = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTOZOOM), 1);
		fShowBarsWhenFullScreen = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_FULLSCREENCTRLS), 1);
		nShowBarsWhenFullScreenTimeOut = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_FULLSCREENCTRLSTIMEOUT), 0);
		if(pApp->GetProfileBinary(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_FULLSCREENRES), &ptr, &len))
		{
			memcpy(&dmFullscreenRes, ptr, sizeof(dmFullscreenRes));
			delete [] ptr;
		}
		else
		{
			dmFullscreenRes.fValid = false;
		}
		fExitFullScreenAtTheEnd = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_EXITFULLSCREENATTHEEND), 0);
		fRememberWindowPos = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REMEMBERWINDOWPOS), 1);
		fRememberWindowSize = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REMEMBERWINDOWSIZE), 1);
		fSnapToDesktopEdges = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SNAPTODESKTOPEDGES), 1);
		if(iUpgradeReset < 51){
			fSnapToDesktopEdges = 1;
		}
		AspectRatio.cx = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ASPECTRATIO_X), 0);
		AspectRatio.cy = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ASPECTRATIO_Y), 0);
		fKeepHistory = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_KEEPHISTORY), 1);
		if(pApp->GetProfileBinary(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LASTWINDOWRECT), &ptr, &len))
		{
			memcpy(&rcLastWindowPos, ptr, sizeof(rcLastWindowPos));
			delete [] ptr;
			
		}
		else
		{
			if ( rcLastWindowPos.Height() < 200){
				rcLastWindowPos.bottom = rcLastWindowPos.top + 480;
			}
			fRememberWindowPos = false;
		}
		lastWindowType = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LASTWINDOWTYPE), SIZE_RESTORED);
		sDVDPath = pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DVDPATH), _T(""));
		fUseDVDPath = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_USEDVDPATH), 0);
		idMenuLang = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MENULANG), ::GetUserDefaultLCID());
		idAudioLang = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUDIOLANG), ::GetUserDefaultLCID());
		idSubtitlesLang = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SUBTITLESLANG), ::GetUserDefaultLCID());
		fAutoSpeakerConf = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTOSPEAKERCONF), 1);
		// TODO: rename subdefstyle -> defStyle, IDS_RS_SPLOGFONT -> IDS_RS_SPSTYLE
		subdefstyle <<= pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPLOGFONT), _T("20,20,20,20,2,0,2.000000,2.000000,3.000000,3.000000,0x00ecec,0x00ffff,0x000000,0x000000,0x00,0x00,0x00,0x80,0,����,20.000000,100.000000,100.000000,0.000000,700,0,0,0,0,0.000000,0.000000,0.000000,0.000000,0,0.700000"));
		subdefstyle2 <<= pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPLOGFONT2), _T("20,20,20,20,8,0,2.000000,2.000000,3.000000,3.000000,0x00ecec,0x00ffff,0x000000,0x000000,0x00,0x00,0x00,0x80,1,����,20.000000,100.000000,100.000000,0.000000,700,0,0,0,0,0.000000,0.000000,0.000000,0.000000,0,0.700000"));

		CheckSVPSubExts = _T(" .ts; .avi; .mkv;");
		bShowControlBar = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SHOWCONTROLBAR), 0);
		dBrightness		= (float)_tstof(pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_COLOR_BRIGHTNESS),	_T("1")));
		dContrast		= (float)_tstof(pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_COLOR_CONTRAST),		_T("1")));
		dHue			= (float)_tstof(pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_COLOR_HUE),			_T("0")));
		dSaturation		= (float)_tstof(pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_COLOR_SATURATION),	_T("1")));

		bNotChangeFontToYH = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_NOTCHANGEFONTTOYH), 0);
//		disableSmartDrag = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DISABLESMARTDRAG),  -1 );

		iDecSpeakers = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DECSPEAKERS), 200);
		fUseInternalTSSpliter = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_USEINTERNALTSSPLITER), 0);

		CSVPToolBox svptoolbox;
		AfxBeginThread( Thread_AppSettingLoadding, this, THREAD_PRIORITY_LOWEST );
		
		fOverridePlacement = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPOVERRIDEPLACEMENT), 0);
		fOverridePlacement2 = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPOVERRIDEPLACEMENT)+_T("2"), TRUE);
		nHorPos = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPHORPOS), 50);
		nVerPos = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPVERPOS), 90);
		nHorPos2 = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPHORPOS2), 50);
		nVerPos2 = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPVERPOS2), 10);
		nSPCSize = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPCSIZE), 3);
		nSPCMaxRes = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPCMAXRES), 2);
		nSubDelayInterval = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SUBDELAYINTERVAL), 500);
		fSPCPow2Tex = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_POW2TEX), TRUE);
		fEnableSubtitles = TRUE;
		if(!autoDownloadSVPSub){
		  fEnableSubtitles = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLESUBTITLES), TRUE);
		}
		fEnableSubtitles2 = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLESUBTITLES2), TRUE);
		fEnableAudioSwitcher = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLEAUDIOSWITCHER), TRUE);
		fAudioTimeShift = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLEAUDIOTIMESHIFT), 0);
		tAudioTimeShift = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUDIOTIMESHIFT), 0);
		fDownSampleTo441 = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DOWNSAMPLETO441), 0);
		fCustomChannelMapping = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_CUSTOMCHANNELMAPPING), 0);

		onlyUseInternalDec = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ONLYUSEINTERNALDEC), FALSE);
		useSmartDrag = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_USESMARTDRAG), FALSE);

		if(iUpgradeReset < 49){
			subdefstyle.relativeTo = 0;	
		}
		if(iUpgradeReset < 95){
			nCS |= CS_STATUSBAR;
			fCustomChannelMapping = FALSE;
		}
		if(iUpgradeReset < 122){
			fDownSampleTo441 = 0;
		}
		if(pApp->GetProfileBinary(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPEAKERTOCHANNELMAPPING), &ptr, &len) && fCustomChannelMapping )
		{
			memcpy(pSpeakerToChannelMap, ptr, sizeof(pSpeakerToChannelMap));
			delete [] ptr;
		}
		else
		{
			memset(pSpeakerToChannelMap, 0, sizeof(pSpeakerToChannelMap));
			for(int j = 0; j < 18; j++)
				for(int i = 0; i <= j; i++)
					pSpeakerToChannelMap[j][i] = 1<<i;

			pSpeakerToChannelMap[0][0] = 1<<0;
			pSpeakerToChannelMap[0][1] = 1<<0;

			pSpeakerToChannelMap[3][0] = 1<<0;
			pSpeakerToChannelMap[3][1] = 1<<1;
			pSpeakerToChannelMap[3][2] = 0;
			pSpeakerToChannelMap[3][3] = 0;
			pSpeakerToChannelMap[3][4] = 1<<2;
			pSpeakerToChannelMap[3][5] = 1<<3;

			pSpeakerToChannelMap[4][0] = 1<<0;
			pSpeakerToChannelMap[4][1] = 1<<1;
			pSpeakerToChannelMap[4][2] = 1<<2;
			pSpeakerToChannelMap[4][3] = 0;
			pSpeakerToChannelMap[4][4] = 1<<3;
			pSpeakerToChannelMap[4][5] = 1<<4;
		}
		fAudioNormalize = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUDIONORMALIZE), FALSE);
		fAudioNormalizeRecover = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUDIONORMALIZERECOVER), TRUE);
		AudioBoost = (float)pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUDIOBOOST), 1);


		{
			for(int i = 0; ; i++)
			{
				CString key;
				key.Format(_T("%s\\%04d"), ResStr(IDS_R_FILTERS), i);

				CAutoPtr<FilterOverride> f(new FilterOverride);

				f->fDisabled = !pApp->GetProfileInt(key, _T("Enabled"), 0);

				UINT j = pApp->GetProfileInt(key, _T("SourceType"), -1);
				if(j == 0)
				{
					f->type = FilterOverride::REGISTERED;
					f->dispname = CStringW(pApp->GetProfileString(key, _T("DisplayName"), _T("")));
					f->name = pApp->GetProfileString(key, _T("Name"), _T(""));
				}
				else if(j == 1)
				{
					f->type = FilterOverride::EXTERNAL;
					f->path = pApp->GetProfileString(key, _T("Path"), _T(""));
					f->name = pApp->GetProfileString(key, _T("Name"), _T(""));
					f->clsid = GUIDFromCString(pApp->GetProfileString(key, _T("CLSID"), _T("")));
				}
				else
				{
					pApp->WriteProfileString(key, NULL, 0);
					break;
				}

				f->backup.RemoveAll();
				for(int i = 0; ; i++)
				{
					CString val;
					val.Format(_T("org%04d"), i);
					CString guid = pApp->GetProfileString(key, val, _T(""));
					if(guid.IsEmpty()) break;
					f->backup.AddTail(GUIDFromCString(guid));
				}

				f->guids.RemoveAll();
				for(int i = 0; ; i++)
				{
					CString val;
					val.Format(_T("mod%04d"), i);
					CString guid = pApp->GetProfileString(key, val, _T(""));
					if(guid.IsEmpty()) break;
					f->guids.AddTail(GUIDFromCString(guid));
				}

				f->iLoadType = (int)pApp->GetProfileInt(key, _T("LoadType"), -1);
				if(f->iLoadType < 0) break;

				f->dwMerit = pApp->GetProfileInt(key, _T("Merit"), MERIT_DO_NOT_USE+1);

				filters.AddTail(f);
			}
		}

		fIntRealMedia = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_INTREALMEDIA), 0);
		//fRealMediaRenderless = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REALMEDIARENDERLESS), 0);
		//iQuickTimeRenderer = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_QUICKTIMERENDERER), 2);
		RealMediaQuickTimeFPS = 25.0;
		*((DWORD*)&RealMediaQuickTimeFPS) = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REALMEDIAFPS), *((DWORD*)&RealMediaQuickTimeFPS));

		m_pnspresets.RemoveAll();
		for(int i = 0; i < (ID_PANNSCAN_PRESETS_END - ID_PANNSCAN_PRESETS_START); i++)
		{
			CString str;
			str.Format(_T("Preset%d"), i);
			str = pApp->GetProfileString(ResStr(IDS_R_SETTINGS) + _T("\\") + ResStr(IDS_RS_PNSPRESETS), str, _T(""));
			if(str.IsEmpty()) break;
			m_pnspresets.Add(str);
		}
		if(m_pnspresets.IsEmpty())
		{
			double _4p3 = 4.0/3.0;
			double _16p9 = 16.0/9.0;
			double _185p1 = 1.85/1.0;
			double _235p1 = 2.35/1.0;

			CString str;
			str.Format(_T("16:9,%.3f,%.3f,%.3f,%.3f"), 0.5, 0.5, _4p3/_4p3, _16p9/_4p3);
			m_pnspresets.Add(str);
			str.Format(_T("���ο���Ļ,%.3f,%.3f,%.3f,%.3f"), 0.5, 0.5, _16p9/_4p3, _16p9/_4p3);
			m_pnspresets.Add(str);
			str.Format(_T("2.35:1,%.3f,%.3f,%.3f,%.3f"), 0.5, 0.5, _235p1/_4p3, _235p1/_4p3);
			m_pnspresets.Add(str);
		}

		for(int i = 0; i < wmcmds.GetCount(); i++)
		{
			CString str;
			str.Format(_T("CommandMod%d"), i);
			str = pApp->GetProfileString(ResStr(IDS_R_COMMANDS), str, _T(""));
			if(str.IsEmpty()) break;
			int cmd, fVirt, key, repcnt, mouse, appcmd;
			TCHAR buff[128];
			int n;
			if(5 > (n = _stscanf(str, _T("%d %x %x %s %d %d %d"), &cmd, &fVirt, &key, buff, &repcnt, &mouse, &appcmd)))
				break;
			if(POSITION pos = wmcmds.Find(cmd))
			{
				wmcmd& wc = wmcmds.GetAt(pos);
                wc.cmd = cmd;
				wc.fVirt = fVirt;
				wc.key = key;
				if(n >= 6) wc.mouse = (UINT)mouse;
				if(n >= 7) wc.appcmd = (UINT)appcmd;
				wc.rmcmd = CStringA(buff).Trim('\"');
				wc.rmrepcnt = repcnt;
			}
		}

		CAtlArray<ACCEL> pAccel;
		pAccel.SetCount(wmcmds.GetCount());
		POSITION pos = wmcmds.GetHeadPosition();
		for(int i = 0; pos; i++) pAccel[i] = wmcmds.GetNext(pos);
		hAccel = CreateAcceleratorTable(pAccel.GetData(), pAccel.GetCount());

		WinLircAddr = pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WINLIRCADDR), _T("127.0.0.1:8765"));
		fWinLirc = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WINLIRC), 0);
		UIceAddr = pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_UICEADDR), _T("127.0.0.1:1234"));
		fUIce = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_UICE), 0);

		fDisabeXPToolbars = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DISABLEXPTOOLBARS), 0);
		fUseWMASFReader = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_USEWMASFREADER), FALSE);
		nJumpDistS = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_JUMPDISTS), 5000);
		nJumpDistM = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_JUMPDISTM), 30000);
		nJumpDistL = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_JUMPDISTL), 60000);
		fFreeWindowResizing = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_FREEWINDOWRESIZING), TRUE);
		fNotifyMSN = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_NOTIFYMSN), FALSE);
		fNotifyGTSdll = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_NOTIFYGTSDLL), FALSE);

		Formats.UpdateData(false);
		if(iUpgradeReset < 50){
			for(size_t i = 0; i < Formats.GetCount(); i++){
				if( Formats[i].GetEngineType() == RealMedia || Formats[i].GetEngineType() == QuickTime){
					Formats[i].SetEngineType(DirectShow);
				}
			}
			Formats.UpdateData(true);
		}


		SrcFilters = pApp->GetProfileInt(ResStr(IDS_R_INTERNAL_FILTERS), ResStr(IDS_RS_SRCFILTERS), ~0);//^SRC_MATROSKA^SRC_MP4^SRC_MPEG^SRC_OGG
		TraFilters = pApp->GetProfileInt(ResStr(IDS_R_INTERNAL_FILTERS), ResStr(IDS_RS_TRAFILTERS), ~0);//^TRA_MPEG1^TRA_AAC^TRA_AC3^TRA_DTS^TRA_LPCM^TRA_MPEG2^TRA_VORBIS
		DXVAFilters = pApp->GetProfileInt(ResStr(IDS_R_INTERNAL_FILTERS), ResStr(IDS_RS_DXVAFILTERS), ~0);
		FFmpegFilters = pApp->GetProfileInt(ResStr(IDS_R_INTERNAL_FILTERS), ResStr(IDS_RS_FFMPEGFILTERS), ~0);


		logofn = pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOGOFILE), _T(""));
		logoid = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOGOID), IDF_LOGO7);
		logoext = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOGOEXT), 0);

		fHideCDROMsSubMenu = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_HIDECDROMSSUBMENU), 0);		

		priority = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_PRIORITY), NORMAL_PRIORITY_CLASS);
		::SetPriorityClass(::GetCurrentProcess(), priority);
		launchfullscreen = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LAUNCHFULLSCREEN), FALSE);

		fEnableWebServer = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLEWEBSERVER), FALSE);
		nWebServerPort = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBSERVERPORT), 13579);
		fWebServerPrintDebugInfo = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBSERVERPRINTDEBUGINFO), FALSE);
		fWebServerUseCompression = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBSERVERUSECOMPRESSION), TRUE);
		fWebServerLocalhostOnly = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBSERVERLOCALHOSTONLY), TRUE);
		WebRoot = pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBROOT), _T("*./webroot"));
		WebDefIndex = pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBDEFINDEX), _T("index.html;index.php"));
		WebServerCGI = pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBSERVERCGI), _T(""));

		iEvrBuffers		= pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_EVR_BUFFERS), 5);

		CString MyPictures;

		CRegKey key;
		// grrrrr
		// if(!SHGetSpecialFolderPath(NULL, MyPictures.GetBufferSetLength(MAX_PATH), CSIDL_MYPICTURES, TRUE)) MyPictures.Empty();
		// else MyPictures.ReleaseBuffer();
		if(ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"), KEY_READ))
		{
			ULONG len = MAX_PATH;
			if(ERROR_SUCCESS == key.QueryStringValue(_T("My Pictures"), MyPictures.GetBuffer(MAX_PATH), &len)) MyPictures.ReleaseBufferSetLength(len);
			else MyPictures.Empty();
		}
		SnapShotPath = pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SNAPSHOTPATH), MyPictures);
		SnapShotExt = pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SNAPSHOTEXT), _T(".jpg"));

		ThumbRows = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_THUMBROWS), 4);
		ThumbCols = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_THUMBCOLS), 4);
		ThumbWidth = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_THUMBWIDTH), 1024);

		ISDb = pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ISDB), _T("www.opensubtitles.org/isdb"));

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), _T("LastUsedPage"), 0);

		
		//

		m_shaders.RemoveAll();

		CAtlStringMap<UINT> shaders;

		shaders[_T("16-235 -> 0-255")] = IDF_SHADER_LEVELS;
		shaders[_T("contour")] = IDF_SHADER_CONTOUR;
		shaders[_T("deinterlace (blend)")] = IDF_SHADER_DEINTERLACE;
		shaders[_T("edge sharpen")] = IDF_SHADER_EDGE_SHARPEN;
		shaders[_T("emboss")] = IDF_SHADER_EMBOSS;
		shaders[_T("grayscale")] = IDF_SHADER_GRAYSCALE;
		shaders[_T("invert")] = IDF_SHADER_INVERT;
		shaders[_T("letterbox")] = IDF_SHADER_LETTERBOX;
		shaders[_T("nightvision")] = IDF_SHADER_NIGHTVISION;
		shaders[_T("procamp")] = IDF_SHADER_PROCAMP;
		shaders[_T("sharpen")] = IDF_SHADER_SHARPEN;
		shaders[_T("sharpen complex")] = IDF_SHADER_SHARPEN_COMPLEX;
		shaders[_T("sphere")] = IDF_SHADER_SPHERE;
		shaders[_T("spotlight")] = IDF_SHADER_SPOTLIGHT;
		shaders[_T("wave")] = IDF_SHADER_WAVE;

		int iShader = 0;

		for(; ; iShader++)
		{
			CString str;
			str.Format(_T("%d"), iShader);
			str = pApp->GetProfileString(_T("Shaders"), str);

			CAtlList<CString> sl;
			CString label = Explode(str, sl, '|');
			if(label.IsEmpty()) break;
			if(sl.GetCount() < 3) continue;

			Shader s;
			s.label = sl.RemoveHead();
			s.target = sl.RemoveHead();
			s.srcdata = sl.RemoveHead();
			s.srcdata.Replace(_T("\\n"), _T("\n"));
			s.srcdata.Replace(_T("\\t"), _T("\t"));
			m_shaders.AddTail(s);

			shaders.RemoveKey(s.label);
		}

		pos = shaders.GetStartPosition();
		for(; pos; iShader++)
		{
			CAtlStringMap<UINT>::CPair* pPair = shaders.GetNext(pos);

			CStringA srcdata;
			if(LoadResource(pPair->m_value, srcdata, _T("FILE")))
			{
				Shader s;
				s.label = pPair->m_key;
				s.target = _T("ps_2_0");
				s.srcdata = CString(srcdata);
				m_shaders.AddTail(s);
			}
		}
		
		strShaderList	= pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SHADERLIST), _T(""));

		// TODO: sort shaders by label

		m_shadercombine = pApp->GetProfileString(_T("Shaders"), _T("Combine"), _T(""));

		fInitialized = true;
	}
}

void CMPlayerCApp::Settings::ParseCommandLine(CAtlList<CString>& cmdln)
{
	nCLSwitches = 0;
	slFiles.RemoveAll();
	slDubs.RemoveAll();
	slSubs.RemoveAll();
	slFilters.RemoveAll();
	rtStart = 0;
	fixedWindowSize.SetSize(0, 0);
	iMonitor = 0;

	if(launchfullscreen) nCLSwitches |= CLSW_FULLSCREEN;

	POSITION pos = cmdln.GetHeadPosition();
	while(pos)
	{
		CString param = cmdln.GetNext(pos);
		if(param.IsEmpty()) continue;

		if((param[0] == '-' || param[0] == '/') && param.GetLength() > 1)
		{
			CString sw = param.Mid(1).MakeLower();
			if(sw == _T("open")) nCLSwitches |= CLSW_OPEN;
			else if(sw == _T("play")) nCLSwitches |= CLSW_PLAY;
			else if(sw == _T("fullscreen")) nCLSwitches |= CLSW_FULLSCREEN;
			else if(sw == _T("minimized")) nCLSwitches |= CLSW_MINIMIZED;
			else if(sw == _T("new")) nCLSwitches |= CLSW_NEW;
			else if(sw == _T("help") || sw == _T("h") || sw == _T("?")) nCLSwitches |= CLSW_HELP;
			else if(sw == _T("dub") && pos) slDubs.AddTail(cmdln.GetNext(pos));
			else if(sw == _T("sub") && pos) slSubs.AddTail(cmdln.GetNext(pos));
			else if(sw == _T("filter") && pos) slFilters.AddTail(cmdln.GetNext(pos));
			else if(sw == _T("dvd")) nCLSwitches |= CLSW_DVD;
			else if(sw == _T("cd")) nCLSwitches |= CLSW_CD;
			else if(sw == _T("add")) nCLSwitches |= CLSW_ADD;
			else if(sw == _T("regvid")) nCLSwitches |= CLSW_REGEXTVID;
			else if(sw == _T("regaud")) nCLSwitches |= CLSW_REGEXTAUD;
			else if(sw == _T("unregall")) nCLSwitches |= CLSW_UNREGEXT;
			else if(sw == _T("unregvid")) nCLSwitches |= CLSW_UNREGEXT; /* keep for compatibility with old versions */
			else if(sw == _T("unregaud")) nCLSwitches |= CLSW_UNREGEXT; /* keep for compatibility with old versions */
			else if(sw == _T("start") && pos) {rtStart = 10000i64*_tcstol(cmdln.GetNext(pos), NULL, 10); nCLSwitches |= CLSW_STARTVALID;}
			else if(sw == _T("startpos") && pos) {/* TODO: mm:ss. */;}
			else if(sw == _T("nofocus")) nCLSwitches |= CLSW_NOFOCUS;
			else if(sw == _T("close")) nCLSwitches |= CLSW_CLOSE;
			else if(sw == _T("standby")) nCLSwitches |= CLSW_STANDBY;
			else if(sw == _T("hibernate")) nCLSwitches |= CLSW_HIBERNATE;
			else if(sw == _T("shutdown")) nCLSwitches |= CLSW_SHUTDOWN;
			else if(sw == _T("logoff")) nCLSwitches |= CLSW_LOGOFF;
			else if(sw == _T("fixedsize") && pos)
			{
				CAtlList<CString> sl;
				Explode(cmdln.GetNext(pos), sl, ',', 2);
				if(sl.GetCount() == 2)
				{
					fixedWindowSize.SetSize(_ttol(sl.GetHead()), _ttol(sl.GetTail()));
					if(fixedWindowSize.cx > 0 && fixedWindowSize.cy > 0)
						nCLSwitches |= CLSW_FIXEDSIZE;
				}
			}
			else if(sw == _T("monitor") && pos) {iMonitor = _tcstol(cmdln.GetNext(pos), NULL, 10); nCLSwitches |= CLSW_MONITOR;}
			else nCLSwitches |= CLSW_HELP|CLSW_UNRECOGNIZEDSWITCH;
		}
		else
		{
			slFiles.AddTail(param);
		}
	}
}

void CMPlayerCApp::Settings::GetFav(favtype ft, CAtlList<CString>& sl, BOOL bRecent)
{
	sl.RemoveAll();

	CString root;

	switch(ft)
	{
	case FAV_FILE: root = ResStr(IDS_R_FAVFILES); break;
	case FAV_DVD: root = ResStr(IDS_R_FAVDVDS); break;
	case FAV_DEVICE: root = ResStr(IDS_R_FAVDEVICES); break;
	default: return;
	}

	if (bRecent){ root += _T("_Recent"); }

	for(int i = 0; ; i++)
	{
		CString s;
		s.Format(_T("Name%d"), i);
		s = AfxGetApp()->GetProfileString(root, s, NULL);
		if(s.IsEmpty()) break;
		sl.AddTail(s);
	}
}

void CMPlayerCApp::Settings::SetFav(favtype ft, CAtlList<CString>& sl, BOOL bRecent)
{
	CString root;

	switch(ft)
	{
	case FAV_FILE: root = ResStr(IDS_R_FAVFILES); break;
	case FAV_DVD: root = ResStr(IDS_R_FAVDVDS); break;
	case FAV_DEVICE: root = ResStr(IDS_R_FAVDEVICES); break;
	default: return;
	}

	if (bRecent){ root += _T("_Recent"); }
	AfxGetApp()->WriteProfileString(root, NULL, NULL);

	int i = 0;
	POSITION pos = sl.GetHeadPosition();
	while(pos)
	{
		CString s;
		s.Format(_T("Name%d"), i++);
		AfxGetApp()->WriteProfileString(root, s, sl.GetNext(pos));
	}
}
void CMPlayerCApp::Settings::DelFavByFn(favtype ft, BOOL bRecent, CString szMatch){
	CAtlList<CString> sl;
	GetFav(ft, sl, bRecent);
	if(bRecent){
		POSITION pos = sl.GetHeadPosition();
		while(pos){
			if( sl.GetAt(pos).Find(szMatch) >= 0 ){
				sl.RemoveAt(pos);
				break;
			}
			sl.GetNext(pos);
		}
		if(sl.GetCount() > 10){
			sl.RemoveHead();
		}
	}else{
		//if(sl.Find(s)) return;
	}
	
	SetFav(ft, sl , bRecent);
}
void CMPlayerCApp::Settings::AddFav(favtype ft, CString s, BOOL bRecent, CString szMatch)
{
	CAtlList<CString> sl;
	GetFav(ft, sl, bRecent);
	if(bRecent){
		POSITION pos = sl.GetHeadPosition();
		while(pos){
			if( sl.GetAt(pos).Find(szMatch) >= 0 ){
				sl.RemoveAt(pos);
				break;
			}
			sl.GetNext(pos);
		}
		if(sl.GetCount() > 10){
			sl.RemoveHead();
		}
	}else{
		if(sl.Find(s)) return;
	}
	sl.AddTail(s);
	SetFav(ft, sl , bRecent);
}

// CMPlayerCApp::Settings::CRecentFileAndURLList

CMPlayerCApp::Settings::CRecentFileAndURLList::CRecentFileAndURLList(UINT nStart, LPCTSTR lpszSection,
															LPCTSTR lpszEntryFormat, int nSize,	
															int nMaxDispLen) 
	: CRecentFileList(nStart, lpszSection, lpszEntryFormat, nSize, nMaxDispLen)	
{
}

//#include <afximpl.h>
extern BOOL AFXAPI AfxFullPath(LPTSTR lpszPathOut, LPCTSTR lpszFileIn);
extern BOOL AFXAPI AfxComparePath(LPCTSTR lpszPath1, LPCTSTR lpszPath2);

void CMPlayerCApp::Settings::CRecentFileAndURLList::Add(LPCTSTR lpszPathName)
{
	ASSERT(m_arrNames != NULL);
	ASSERT(lpszPathName != NULL);
	ASSERT(AfxIsValidString(lpszPathName));

	if(CString(lpszPathName).MakeLower().Find(_T("@device:")) >= 0)
		return;

	bool fURL = (CString(lpszPathName).Find(_T("://")) >= 0);

	// fully qualify the path name
	TCHAR szTemp[1024];
	if(fURL) _tcscpy_s(szTemp, countof(szTemp), lpszPathName);
	else AfxFullPath(szTemp, lpszPathName);

	// update the MRU list, if an existing MRU string matches file name
	int iMRU;
	for (iMRU = 0; iMRU < m_nSize-1; iMRU++)
	{
		if((fURL && !_tcscmp(m_arrNames[iMRU], szTemp))
		|| AfxComparePath(m_arrNames[iMRU], szTemp))
			break;      // iMRU will point to matching entry
	}
	// move MRU strings before this one down
	for (; iMRU > 0; iMRU--)
	{
		ASSERT(iMRU > 0);
		ASSERT(iMRU < m_nSize);
		m_arrNames[iMRU] = m_arrNames[iMRU-1];
	}
	// place this one at the beginning
	m_arrNames[0] = szTemp;
}


void CMPlayerCApp::OnHelpShowcommandlineswitches()
{
	ShowCmdlnSwitches();
}

//

void GetCurDispMode(dispmode& dm)
{
	if(HDC hDC = ::GetDC(0))
	{
		dm.fValid = true;
		dm.size = CSize(GetDeviceCaps(hDC, HORZRES), GetDeviceCaps(hDC, VERTRES));
		dm.bpp = GetDeviceCaps(hDC, BITSPIXEL);
		dm.freq = GetDeviceCaps(hDC, VREFRESH);
		::ReleaseDC(0, hDC);
	}
}

bool GetDispMode(int i, dispmode& dm)
{
	DEVMODE devmode;
	devmode.dmSize = sizeof(DEVMODE);
	if(!EnumDisplaySettings(0, i, &devmode))
		return(false);

	dm.fValid = true;
	dm.size = CSize(devmode.dmPelsWidth, devmode.dmPelsHeight);
	dm.bpp = devmode.dmBitsPerPel;
	dm.freq = devmode.dmDisplayFrequency;

	return(true);
}

void SetDispMode(dispmode& dm)
{
	if(!dm.fValid) return;

	DEVMODE dmScreenSettings;
	memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
	dmScreenSettings.dmSize = sizeof(dmScreenSettings);
	dmScreenSettings.dmPelsWidth = dm.size.cx;
	dmScreenSettings.dmPelsHeight = dm.size.cy;
	dmScreenSettings.dmBitsPerPel = dm.bpp;
	dmScreenSettings.dmDisplayFrequency = dm.freq;
	dmScreenSettings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;
	ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);
}

#include <afxsock.h>
#include <atlsync.h>
#include <atlutil.h> // put this before the first detours macro above to see an ICE with vc71 :)
#include <D:\-=SVN=-\ATL Server\include\atlrx.h> // http://www.codeplex.com/AtlServer/
#include "afxwin.h"

typedef CAtlRegExp<CAtlRECharTraits> CAtlRegExpT;
typedef CAtlREMatchContext<CAtlRECharTraits> CAtlREMatchContextT;

bool FindRedir(CUrl& src, CString ct, CString& body, CAtlList<CString>& urls, CAutoPtrList<CAtlRegExpT>& res)
{
	POSITION pos = res.GetHeadPosition();
	while(pos)
	{
		CAtlRegExpT* re = res.GetNext(pos);

		CAtlREMatchContextT mc;
		const CAtlREMatchContextT::RECHAR* s = (LPCTSTR)body;
		const CAtlREMatchContextT::RECHAR* e = NULL;
		for(; s && re->Match(s, &mc, &e); s = e)
		{
			const CAtlREMatchContextT::RECHAR* szStart = 0;
			const CAtlREMatchContextT::RECHAR* szEnd = 0;
			mc.GetMatch(0, &szStart, &szEnd);

			CString url;
			url.Format(_T("%.*s"), szEnd - szStart, szStart);
			url.Trim();

			if(url.CompareNoCase(_T("asf path")) == 0) continue;

			CUrl dst;
			dst.CrackUrl(CString(url));
			if(_tcsicmp(src.GetSchemeName(), dst.GetSchemeName())
			|| _tcsicmp(src.GetHostName(), dst.GetHostName())
			|| _tcsicmp(src.GetUrlPath(), dst.GetUrlPath()))
			{
				urls.AddTail(url);
			}
			else
			{
				// recursive
				urls.RemoveAll();
				break;
			}
		}
	}

	return urls.GetCount() > 0;
}

bool FindRedir(CString& fn, CString ct, CAtlList<CString>& fns, CAutoPtrList<CAtlRegExpT>& res)
{
	CString body;

	CTextFile f(CTextFile::ANSI);
	if(f.Open(fn)) for(CString tmp; f.ReadString(tmp); body += tmp + '\n');

	CString dir = fn.Left(max(fn.ReverseFind('/'), fn.ReverseFind('\\'))+1); // "ReverseFindOneOf"

	POSITION pos = res.GetHeadPosition();
	while(pos)
	{
		CAtlRegExpT* re = res.GetNext(pos);

		CAtlREMatchContextT mc;
		const CAtlREMatchContextT::RECHAR* s = (LPCTSTR)body;
		const CAtlREMatchContextT::RECHAR* e = NULL;
		for(; s && re->Match(s, &mc, &e); s = e)
		{
			const CAtlREMatchContextT::RECHAR* szStart = 0;
			const CAtlREMatchContextT::RECHAR* szEnd = 0;
			mc.GetMatch(0, &szStart, &szEnd);

			CString fn2;
			fn2.Format(_T("%.*s"), szEnd - szStart, szStart);
			fn2.Trim();

			if(!fn2.CompareNoCase(_T("asf path"))) continue;
			if(fn2.Find(_T("EXTM3U")) == 0 || fn2.Find(_T("#EXTINF")) == 0) continue;

			if(fn2.Find(_T(":")) < 0 && fn2.Find(_T("\\\\")) != 0 && fn2.Find(_T("//")) != 0)
			{
				CPath p;
				p.Combine(dir, fn2);
				fn2 = (LPCTSTR)p;
			}

			if(!fn2.CompareNoCase(fn))
				continue;

			fns.AddTail(fn2);
		}
	}

	return fns.GetCount() > 0;
}
void GetSystemFontWithScale(CFont* pFont, double dDefaultSize){
	HDC hdc = ::GetDC(NULL);
	double scale = 1.0*GetDeviceCaps(hdc, LOGPIXELSY) / 96.0;
	::ReleaseDC(0, hdc);

	pFont->m_hObject = NULL;

	if(!(::GetVersion()&0x80000000))
		pFont->CreateFont(int(dDefaultSize * scale), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, 
		_T("Microsoft Sans Serif"));
	if(!pFont->m_hObject)
		pFont->CreateFont(int(dDefaultSize * scale), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, 
		_T("MS Sans Serif"));
}
CString GetContentType(CString fn, CAtlList<CString>* redir)
{
	CUrl url;
	CString ct, body;

	if(fn.Find(_T("://")) >= 0)
	{
		url.CrackUrl(fn);

		if(_tcsicmp(url.GetSchemeName(), _T("pnm")) == 0)
			return "audio/x-pn-realaudio";

		if(_tcsicmp(url.GetSchemeName(), _T("mms")) == 0)
			return "video/x-ms-asf";

		if(_tcsicmp(url.GetSchemeName(), _T("http")) != 0)
			return "";

		DWORD ProxyEnable = 0;
		CString ProxyServer;
		DWORD ProxyPort = 0;

		ULONG len = 256+1;
		CRegKey key;
		if(ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), KEY_READ)
		&& ERROR_SUCCESS == key.QueryDWORDValue(_T("ProxyEnable"), ProxyEnable) && ProxyEnable
		&& ERROR_SUCCESS == key.QueryStringValue(_T("ProxyServer"), ProxyServer.GetBufferSetLength(256), &len))
		{
			ProxyServer.ReleaseBufferSetLength(len);

			CAtlList<CString> sl;
			ProxyServer = Explode(ProxyServer, sl, ';');
			if(sl.GetCount() > 1)
			{
				POSITION pos = sl.GetHeadPosition();
				while(pos)
				{
					CAtlList<CString> sl2;
					if(!Explode(sl.GetNext(pos), sl2, '=', 2).CompareNoCase(_T("http"))
					&& sl2.GetCount() == 2)
					{
						ProxyServer = sl2.GetTail();
						break;
					}
				}
			}

			ProxyServer = Explode(ProxyServer, sl, ':');
			if(sl.GetCount() > 1) ProxyPort = _tcstol(sl.GetTail(), NULL, 10);
		}

		CSocket s;
		s.Create();
		if(s.Connect(
			ProxyEnable ? ProxyServer : url.GetHostName(), 
			ProxyEnable ? ProxyPort : url.GetPortNumber()))
		{
			CStringA host = CStringA(url.GetHostName());
			CStringA path = CStringA(url.GetUrlPath()) + CStringA(url.GetExtraInfo());

			if(ProxyEnable) path = "http://" + host + path;

			CStringA hdr;
			hdr.Format(
				"GET %s HTTP/1.0\r\n"
				"User-Agent: Media Player Classic\r\n"
				"Host: %s\r\n"
				"Accept: */*\r\n"
				"\r\n", path, host);

// MessageBox(NULL, CString(hdr), _T("Sending..."), MB_OK);

			if(s.Send((LPCSTR)hdr, hdr.GetLength()) < hdr.GetLength()) return "";

			hdr.Empty();
			while(1)
			{
				CStringA str;
				str.ReleaseBuffer(s.Receive(str.GetBuffer(256), 256)); // SOCKET_ERROR == -1, also suitable for ReleaseBuffer
				if(str.IsEmpty()) break;
				hdr += str;
				int hdrend = hdr.Find("\r\n\r\n");
				if(hdrend >= 0) {body = hdr.Mid(hdrend+4); hdr = hdr.Left(hdrend); break;}
			}

// MessageBox(NULL, CString(hdr), _T("Received..."), MB_OK);

			CAtlList<CStringA> sl;
			Explode(hdr, sl, '\n');
			POSITION pos = sl.GetHeadPosition();
			while(pos)
			{
				CStringA& hdrline = sl.GetNext(pos);
				CAtlList<CStringA> sl2;
				Explode(hdrline, sl2, ':', 2);
				CStringA field = sl2.RemoveHead().MakeLower();
				if(field == "location" && !sl2.IsEmpty())
					return GetContentType(CString(sl2.GetHead()), redir);
				if(field == "content-type" && !sl2.IsEmpty())
					ct = sl2.GetHead();
			}

			while(body.GetLength() < 256)
			{
				CStringA str;
				str.ReleaseBuffer(s.Receive(str.GetBuffer(256), 256)); // SOCKET_ERROR == -1, also suitable for ReleaseBuffer
				if(str.IsEmpty()) break;
				body += str;
			}

			if(body.GetLength() >= 8)
			{
				CStringA str = TToA(body);
				if(!strncmp((LPCSTR)str, ".ra", 3))
					return "audio/x-pn-realaudio";
				if(!strncmp((LPCSTR)str, ".RMF", 4))
					return "audio/x-pn-realaudio";
				if(*(DWORD*)(LPCSTR)str == 0x75b22630)
					return "video/x-ms-wmv";
				if(!strncmp((LPCSTR)str+4, "moov", 4))
					return "video/quicktime";
			}

			if(redir && (ct == _T("audio/x-scpls") || ct == _T("audio/x-mpegurl")))
			{
				while(body.GetLength() < 4*1024) // should be enough for a playlist...
				{
					CStringA str;
					str.ReleaseBuffer(s.Receive(str.GetBuffer(256), 256)); // SOCKET_ERROR == -1, also suitable for ReleaseBuffer
					if(str.IsEmpty()) break;
					body += str;
				}
			}
		}
	}
	else if(!fn.IsEmpty())
	{
		CPath p(fn);
		CString ext = p.GetExtension().MakeLower();
		if(ext == _T(".asx")) ct = _T("video/x-ms-asf");
		else if(ext == _T(".pls")) ct = _T("audio/x-scpls");
		else if(ext == _T(".m3u")) ct = _T("audio/x-mpegurl");
		else if(ext == _T(".qtl")) ct = _T("application/x-quicktimeplayer");
		else if(ext == _T(".mpcpl")) ct = _T("application/x-mpc-playlist");

		if(FILE* f = _tfopen(fn, _T("rb")))
		{
			CStringA str;
			str.ReleaseBufferSetLength(fread(str.GetBuffer(10240), 1, 10240, f));
			body = AToT(str);
			fclose(f);
		}
	}

	if(body.GetLength() >= 4) // here only those which cannot be opened through dshow
	{
		CStringA str = TToA(body);
		if(!strncmp((LPCSTR)str, ".ra", 3))
			return "audio/x-pn-realaudio";
		if(!strncmp((LPCSTR)str, "FWS", 3))
			return "application/x-shockwave-flash";
	}

	if(redir && !ct.IsEmpty())
	{
		CAutoPtrList<CAtlRegExpT> res;
		CAutoPtr<CAtlRegExpT> re;

		if(ct == _T("video/x-ms-asf"))
		{
			// ...://..."/>
			re.Attach(new CAtlRegExpT());
			if(re && REPARSE_ERROR_OK == re->Parse(_T("{[a-zA-Z]+://[^\n\">]*}"), FALSE))
				res.AddTail(re);
			// Ref#n= ...://...\n
			re.Attach(new CAtlRegExpT());
			if(re && REPARSE_ERROR_OK == re->Parse(_T("Ref\\z\\b*=\\b*[\"]*{([a-zA-Z]+://[^\n\"]+}"), FALSE))
				res.AddTail(re);
		}
		else if(ct == _T("audio/x-scpls"))
		{
			// File1=...\n
			re.Attach(new CAtlRegExp<>());
			if(re && REPARSE_ERROR_OK == re->Parse(_T("file\\z\\b*=\\b*[\"]*{[^\n\"]+}"), FALSE))
				res.AddTail(re);
		}
		else if(ct == _T("audio/x-mpegurl"))
		{
			// #comment
			// ...
			re.Attach(new CAtlRegExp<>());
			if(re && REPARSE_ERROR_OK == re->Parse(_T("{[^#][^\n]+}"), FALSE))
				res.AddTail(re);
		}
		else if(ct == _T("audio/x-pn-realaudio"))
		{
			// rtsp://...
			re.Attach(new CAtlRegExp<>());
			if(re && REPARSE_ERROR_OK == re->Parse(_T("{rtsp://[^\n]+}"), FALSE))
				res.AddTail(re);
		}

		if(!body.IsEmpty())
		{
			if(fn.Find(_T("://")) >= 0) FindRedir(url, ct, body, *redir, res);
			else FindRedir(fn, ct, *redir, res);
		}
	}

	return ct;
}
bool CMPlayerCApp::IsVista()
{
	OSVERSIONINFO osver;

	osver.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	
	if (	::GetVersionEx( &osver ) && 
			osver.dwPlatformId == VER_PLATFORM_WIN32_NT && 
			(osver.dwMajorVersion >= 6 ) )
		return TRUE;

	return FALSE;
}

bool CMPlayerCApp::IsVSFilterInstalled()
{
	bool result = false;
	CRegKey key;
	if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, _T("CLSID\\{083863F1-70DE-11d0-BD40-00A0C911CE86}\\Instance\\{9852A670-F845-491B-9BE6-EBD841B8A613}"), KEY_READ)) {
		result = true;
	}
	
	return result;
}

HINSTANCE CMPlayerCApp::GetD3X9Dll()
{
	if (m_hD3DX9Dll == NULL)
	{
		m_nDXSdkRelease = 0;
		// Try to load latest DX9 available
		for (int i=D3DX_SDK_VERSION; i>23; i--)
		{
			if (i != 33)	// Prevent using DXSDK April 2007 (crash sometimes during shader compilation)
			{
				m_strD3DX9Version.Format(_T("d3dx9_%d.dll"), i);
				m_hD3DX9Dll = LoadLibrary (m_strD3DX9Version);
				if (m_hD3DX9Dll) 
				{
					m_nDXSdkRelease = i;
					break;
				}
			}
		}
	}

	return m_hD3DX9Dll;
}

LONGLONG CMPlayerCApp::GetPerfCounter()
{
	LONGLONG		i64Ticks100ns;
	if (m_PerfFrequency != 0)
	{
		QueryPerformanceCounter ((LARGE_INTEGER*)&i64Ticks100ns);
		i64Ticks100ns	= i64Ticks100ns * 10000000;
		i64Ticks100ns	= i64Ticks100ns / m_PerfFrequency;

		return i64Ticks100ns;
	}
	return 0;
}