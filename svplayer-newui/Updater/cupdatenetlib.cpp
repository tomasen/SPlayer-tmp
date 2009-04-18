#include "StdAfx.h"
#include "cupdatenetlib.h"
#include <atlpath.h>


static size_t handleWebQuery( void *ptr, size_t size, size_t nmemb, void *stream){


	size_t realsize = size * nmemb;

	
	fwrite(ptr, size ,nmemb,(FILE*)stream);
	return realsize;
}

cupdatenetlib::cupdatenetlib(void)
{
	resetCounter();
	CString path;
	GetModuleFileName(AfxGetInstanceHandle(), path.GetBuffer(MAX_PATH), MAX_PATH);
	path.ReleaseBuffer();
	
	CPath cpath(path);
	cpath.RemoveFileSpec();
	cpath.AddBackslash();
	
	szBasePath = CString(cpath);

	cpath.Append(_T("UPD"));
	cpath.AddBackslash();

	szUpdfilesPath = CString(cpath);

	_wmkdir(szUpdfilesPath);

}

cupdatenetlib::~cupdatenetlib(void)
{
}


void cupdatenetlib::SetCURLopt(CURL *curl )
{
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "SVPlayer Updater");
	return ;
}

void  cupdatenetlib::resetCounter(){
	bSVPCU_DONE = 0;
	iSVPCU_TOTAL_FILE = 0;
	iSVPCU_CURRETN_FILE = 0;
	iSVPCU_TOTAL_FILEBYTE = 0;
	iSVPCU_TOTAL_FILEBYTE_DONE = 0;
	iSVPCU_CURRENT_FILEBYTE = 0;
	iSVPCU_CURRENT_FILEBYTE_DONE = 0;
	bWaiting = FALSE;

}
void cupdatenetlib::procUpdate(){
	
	if (_wmkdir(szUpdfilesPath) < 0 && errno == EEXIST){
		if(szaLists.GetCount() <= 0){
			downloadList() ;
		}
		if ( szaLists.GetCount() >0 ){
			SVP_LogMsg( _T("GOT UPDATE LIST: "));
			SVP_LogMsg( svpToolBox.Implode(_T("\t") , &szaLists) );

			int i = 0;
			

			while(  downloadFiles() != 0 ){
				i++;
				SVP_LogMsg( _T("DOWNLOAD UPDATE "));
				if(i > 10) break;
			}


			SVP_LogMsg( _T("REAL UPDATE") );
			tryRealUpdate();
		}
	}else{
		SVP_LogMsg( _T("UPD dir not exist and write able: "));
	}
	bSVPCU_DONE = 1;
}



BOOL cupdatenetlib::downloadList(){
	if ( !(_wmkdir(szUpdfilesPath) < 0 && errno == EEXIST) ){
		SVP_LogMsg( _T("UPD dir not exist and writeable! "));
		return 0;
	}
	resetCounter();


	CString szBranch = svpToolBox.fileGetContent(szUpdfilesPath + _T("branch") );

	if(szBranch.IsEmpty() )
		szBranch = _T("stable");

	FILE* stream_file_list;
	CString szTmpFilename = this->svpToolBox.getTmpFileName();
	if ( _wfopen_s( &stream_file_list, szTmpFilename, _T("wb") ) != 0){
		return 0; //input file open error
	}
	CURL *curl;
	CURLcode res;
	CString szPostPerm;
	szPostPerm.Format(_T("branch=%s"), szBranch);
	int rret = 0;
	curl = curl_easy_init();
	if(curl) {
		long respcode;

		this->SetCURLopt(curl);

		curl_easy_setopt(curl, CURLOPT_URL, "http://svplayer.shooter.cn/api/updater.php");

		int iDescLen = 0;
		char* szPostFields = svpToolBox.CStringToUTF8(szPostPerm, &iDescLen) ;
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (void *)szPostFields);

		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)stream_file_list);

		res = curl_easy_perform(curl);
		if(res == 0){
			curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE, &respcode);

			if(respcode == 200){
				//good to go
				rret = 1;

			}else{
				//error
				SVP_LogMsg(_T("None Update Required "));
			}
		}else{
			//error
			SVP_LogMsg(_T("HTTP connection error  ")); //TODO handle this
		}
		curl_easy_cleanup(curl);
	}
	fclose(stream_file_list);
	CString szLog;
	if (rret){
		//iSVPCU_TOTAL_FILE = 0;
		iSVPCU_TOTAL_FILEBYTE  = 0;
		CString szData = svpToolBox.fileGetContent( szTmpFilename ) ;
		CStringArray szaLines;
		svpToolBox.Explode( szData, _T("\n") , &szaLines );
		for(int i = 0; i < szaLines.GetCount(); i++){
			if (szaLines.GetAt(i).IsEmpty()){break;}
			this->iSVPCU_TOTAL_FILE++;
			
			szLog.Format(_T("Total Files need to download: %d"), iSVPCU_TOTAL_FILE);
			SVP_LogMsg(szLog);
			CStringArray szaTmp;
			svpToolBox.Explode( szaLines.GetAt(i), _T(";") , &szaTmp );
			if(szaTmp.GetCount() < LFILETOTALPARMS){
				continue;
			}
			if(szaTmp.GetAt(LFILESETUPPATH) == _T("msyh.ttf") ){
				CSVPToolBox svptoolbox;
				if(svptoolbox.bFontExist(_T("΢���ź�")) || svptoolbox.bFontExist(_T("Microsoft YaHei")) ){ 
					continue;
				}
			}
			iSVPCU_TOTAL_FILEBYTE += _wtoi(szaTmp.GetAt(LFILEGZLEN));
			szaTmp.SetSize(LFILETOTALPARMS);
			szaLists.Append( szaTmp );
			
		}
		szLog.Format(_T("Total Files: %d ; Total Len %d"), iSVPCU_TOTAL_FILE, iSVPCU_TOTAL_FILEBYTE);
		SVP_LogMsg(szLog);
	}
	return rret;
}
void cupdatenetlib::tryRealUpdate(){

	struct szaMoveFile
	{
		CString szMoveSrcFile;
		CString szMoveDestFile;
	};
	CAtlList<szaMoveFile> szaMoveFiles;

	for(int i = 0; i < szaLists.GetCount(); i+= LFILETOTALPARMS){
		if(szaLists.GetCount() < (i+LFILETOTALPARMS)){ break; }

		
		CString szSetupPath = szaLists.GetAt(i+LFILESETUPPATH);

		if (szSetupPath.CompareNoCase( _T("svplayer.exe")) == 0){
			if(!svpToolBox.ifFileExist(szBasePath + szSetupPath) && svpToolBox.ifFileExist(szBasePath + _T("mplayerc.exe"))){
				szSetupPath = _T("mplayerc.exe");
			}
		}

		bool bUpdateThis = FALSE;
	
		//check file hash
		CMD5Checksum cmd5;
		CString updTmpHash ;
		CString currentHash ;
		if( svpToolBox.ifFileExist(szUpdfilesPath + szaLists.GetAt(i+LFILETMPATH) ) ){
			updTmpHash = cmd5.GetMD5(szUpdfilesPath + szaLists.GetAt(i+LFILETMPATH) ); //Get Hash for current Temp File
		}
		
		if( svpToolBox.ifFileExist(szUpdfilesPath + szSetupPath ) ){
			currentHash = cmd5.GetMD5(szBasePath + szSetupPath); //Get Hash for bin file
		}


		if (currentHash.CompareNoCase( szaLists.GetAt(i+LFILEHASH) ) != 0 && updTmpHash.CompareNoCase( szaLists.GetAt(i+LFILEHASH) ) == 0 ){
			bUpdateThis = TRUE;
		}

		if(bUpdateThis){
			//if not match download
			szaMoveFile mFiles;
			mFiles.szMoveSrcFile = szUpdfilesPath + szaLists.GetAt(i+LFILETMPATH) ;
			mFiles.szMoveDestFile = szBasePath + szSetupPath;
			szaMoveFiles.AddTail(mFiles);
// 			while( MoveFileEx( szUpdfilesPath + szaLists.GetAt(i+LFILETMPATH)  , szBasePath + szSetupPath , MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH) == 0 ){
// 				Sleep(50000);
// 			}
		}
	}

	while(1){
		POSITION pos = szaMoveFiles.GetHeadPosition();
		if(!pos){break;}
		bWaiting = TRUE;
		while(pos){
			szaMoveFile mFiles;
			POSITION orgPos = pos;
			mFiles = szaMoveFiles.GetNext(pos);
			if(!svpToolBox.ifFileExist(mFiles.szMoveSrcFile)){
				szaMoveFiles.RemoveAt(orgPos);
				continue;
			}
			svpToolBox.CreatDirForFile(mFiles.szMoveDestFile);
			if( MoveFileEx( mFiles.szMoveSrcFile , mFiles.szMoveDestFile , MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH) == 0 ){
				MoveFileEx( mFiles.szMoveSrcFile , mFiles.szMoveDestFile , MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING|MOVEFILE_DELAY_UNTIL_REBOOT) ;
			}
			
		}
		Sleep(15);
	}

	bWaiting = FALSE;
}

int my_progress_func(cupdatenetlib *cup,
					 double t, /* dltotal */
					 double d, /* dlnow */
					 double ultotal,
					 double ulnow)
{
	cup->iSVPCU_CURRENT_FILEBYTE_DONE = (int)d;
	//cup->iSVPCU_CURRENT_FILEBYTE = t;

// 	CString szLog;
// 	szLog.Format(_T("Progress %d / %d "), (int)d, (int)cup->iSVPCU_CURRENT_FILEBYTE);
// 	SVP_LogMsg(szLog);
	return 0;
}
int cupdatenetlib::downloadFileByID(CString szID, CString szTmpPath){
	FILE* stream_file_list;
	CString szTmpFilename = this->svpToolBox.getTmpFileName();
	if ( _wfopen_s( &stream_file_list, szTmpFilename, _T("wb") ) != 0){
		return 0; //input file open error
	}
	CURL *curl;
	CURLcode res;
	CString szPostPerm;
	szPostPerm.Format(_T("setupfileid=%s"), szID);
	int rret = 0;
	curl = curl_easy_init();
	if(curl) {
		long respcode;

		this->SetCURLopt(curl);

		curl_easy_setopt(curl, CURLOPT_URL, "http://svplayer.shooter.cn/api/updater.php");

		int iDescLen = 0;
		char* szPostFields = svpToolBox.CStringToUTF8(szPostPerm, &iDescLen) ;
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (void *)szPostFields);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)stream_file_list);

		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, my_progress_func);
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);

		res = curl_easy_perform(curl);
		if(res == 0){
			curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE, &respcode);

			if(respcode == 200){
				//good to go
				rret = 1;

			}else{
				//error
				SVP_LogMsg(_T("None File As That Id "));
			}
		}else{
			//error
			SVP_LogMsg(_T("HTTP connection error  ")); //TODO handle this
		}
		free(szPostFields);

		curl_easy_cleanup(curl);
	}
	fclose(stream_file_list);
	if (rret){
		svpToolBox.unpackGZfile( szTmpFilename, szTmpPath );
	}
	return rret;
}
int cupdatenetlib::downloadFiles(){
	int iTotalDown = 0;
	
	for(int i = 0; i < szaLists.GetCount(); i+= LFILETOTALPARMS){
		if(szaLists.GetCount() < (i+LFILETOTALPARMS)){ break; }
		CString szSetupPath = szaLists.GetAt(i+LFILESETUPPATH);
		szCurFilePath = szSetupPath;

		if (szSetupPath.CompareNoCase( _T("svplayer.exe")) == 0){
			if(!svpToolBox.ifFileExist(szBasePath + szSetupPath) && svpToolBox.ifFileExist(szBasePath + _T("mplayerc.exe"))){
				szSetupPath = _T("mplayerc.exe");
			}
		}

		bool bDownloadThis = FALSE, bSkipThis = FALSE;
		if ( szaLists.GetAt(i+LFILEACTION) == "codec" ){
			if(!svpToolBox.ifFileExist(szBasePath + szaLists.GetAt(i + LFILESETUPPATH))){
				//skip this file
				bSkipThis = TRUE;
			}
		}
		if(!bSkipThis){
			//check file hash
			CMD5Checksum cmd5;
			int iLen;
			BYTE* szByteBuf = (BYTE*) svpToolBox.CStringToUTF8(szSetupPath, &iLen) ;
			szaLists.SetAt(i+LFILETMPATH, cmd5.GetMD5(szByteBuf ,iLen ) ); //Set Temp File Path
			free(szByteBuf);
			cmd5.Clean();
			CString myHash ;
			CString myHashFilePath = _T("none");
			if( svpToolBox.ifFileExist(szUpdfilesPath + szaLists.GetAt(i+LFILETMPATH) ) ){
				myHashFilePath = szUpdfilesPath + szaLists.GetAt(i+LFILETMPATH);
				myHash = cmd5.GetMD5(myHashFilePath); //Get Hash for current Temp File
				
			}else if( svpToolBox.ifFileExist(szBasePath + szSetupPath ) ){
				myHashFilePath = szBasePath + szSetupPath ;
				myHash = cmd5.GetMD5(myHashFilePath); //Get Hash for bin file
			}else{
				myHash = _T("");
			}

			CString szLog;
			szLog.Format(_T("hash %s as %s vs. %s | %s %s "), myHashFilePath , myHash , szaLists.GetAt(i+LFILEHASH) ,
				szUpdfilesPath + szaLists.GetAt(i+LFILETMPATH) , szUpdfilesPath + szSetupPath );
			SVP_LogMsg(szLog);

			if (myHash.CompareNoCase( szaLists.GetAt(i+LFILEHASH) ) != 0){
				bDownloadThis = TRUE;
			}

			CString szGZlen = szaLists.GetAt(i+LFILEGZLEN);
			iSVPCU_CURRENT_FILEBYTE = _wtoi(szGZlen);
		}
		if(bDownloadThis){
			//if not match download
			//iSVPCU_TOTAL_FILEBYTE += _wtoi(szGZlen);
			iSVPCU_CURRENT_FILEBYTE_DONE = 0;
			
			downloadFileByID( szaLists.GetAt(i+LFILEID) ,  szUpdfilesPath + szaLists.GetAt(i+LFILETMPATH));
			iTotalDown++;
		}
		iSVPCU_CURRETN_FILE++;
		iSVPCU_CURRENT_FILEBYTE_DONE = iSVPCU_CURRENT_FILEBYTE;
		iSVPCU_TOTAL_FILEBYTE_DONE += iSVPCU_CURRENT_FILEBYTE;

	}

	return iTotalDown;
}