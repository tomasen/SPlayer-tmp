﻿#include "stdafx.h"
#include "MediaSpiderFolderTree.h"
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include "../Model/MediaDB.h"
#include "../Utils/SPlayerGUID.h"
#include "MediaCenterController.h"

////////////////////////////////////////////////////////////////////////////////
// Normal part
MediaSpiderFolderTree::MediaSpiderFolderTree()
: m_tSleep(1)
, m_spiderinterval(120*60*1000)
{
  // Init the media type and the exclude folders & files from the database
  // Note: the case is not sensitive, will change it to lowercase before compare
  SetSupportExtension(L".avi");
  SetSupportExtension(L".wmv");
  SetSupportExtension(L".mkv");
  SetSupportExtension(L".rmvb");
  SetSupportExtension(L".rm");
  SetSupportExtension(L".asf");
  SetSupportExtension(L".mov");
  SetSupportExtension(L".mp4");
  SetSupportExtension(L".mpeg");
  SetSupportExtension(L".3gp");
  SetSupportExtension(L".divx");

  SetExcludePath(L"c:\\Windows\\");
  SetExcludePath(L"C:\\Program Files\\");
  SetExcludePath(L"C:\\Program Files (x86)\\");

  SetFilteredItem(L"private");
}

MediaSpiderFolderTree::~MediaSpiderFolderTree()
{
}

BOOL MediaSpiderFolderTree::SearchPath(const std::wstring &folder)
{
  std::stack<std::wstring> tmppack;
  std::vector<std::wstring> pack;
  std::vector<std::wstring> detectpaths;

  m_treeModel.splitPath(folder, tmppack);

  while (!tmppack.empty())
  {
    pack.push_back(tmppack.top());
    tmppack.pop();
  }

  if (pack.size() < 2) // don't search root
    return FALSE;
  else if (pack.size() == 2) // driver:\folderA\file.mkv
  {
    // the path like: driver:\folderA
    std::wstring p;
    p = pack[0] + L"\\" + pack[1] + L"\\";

    // search sub dir
    boost::filesystem::directory_iterator it(p);
    boost::filesystem::directory_iterator itEnd;
    while (it != itEnd)
    {
      if (boost::filesystem::is_directory(it->path())
        && !isHiddenPath(it->path().wstring()))
        detectpaths.push_back(it->path().wstring());
    }
  }
  else if (pack.size() > 2) // driver:\folderA\folderB\[...]\file.mkv
  {
    std::wstring p;
    // we can get path like this: driver:\folderA
    for (size_t i=0; i < pack.size()-1; ++i)
      p += pack[i] + L"\\";

    detectpaths.push_back(p);

    // search sibling dir
    boost::filesystem::directory_iterator it(p);
    boost::filesystem::directory_iterator itEnd;
    while (it != itEnd)
    {
      if (boost::filesystem::is_directory(it->path())
        && !isHiddenPath(it->path().wstring()))
      {
        detectpaths.push_back(it->path().wstring());
        // sub dir
        boost::filesystem::directory_iterator sit(it->path());
        boost::filesystem::directory_iterator sitend;
        while (sit != sitend)
        {
          if (boost::filesystem::is_directory(sit->path())
            && !isHiddenPath(sit->path().wstring()))
            detectpaths.push_back(sit->path().wstring());
          ++sit;
        }
      } // if
      ++it;
    } // while
  }

  for (size_t i=0; i<detectpaths.size(); ++i)
  {
    if (!Search(detectpaths[i]))
      return FALSE;
  }
  return TRUE;
}

void MediaSpiderFolderTree::_Thread()
{
  UINT uniqueid;
  std::wstring sPath;
  wchar_t execsql[500];
  time_t lasttime, oldlasttime;

  wchar_t* updatesql = L"UPDATE detect_path SET lasttime=%I64d WHERE uniqueid=%u";
  wchar_t* selectsql = L"SELECT uniqueid, path, lasttime FROM detect_path WHERE lasttime<=%I64d ORDER BY merit limit 0, 1";

  while (true)
  {
    sPath = L"";
    uniqueid = 0;
    lasttime = ::time(0) - m_spiderinterval;
    oldlasttime = 0;

    memset(execsql, 0, 500);
    wsprintf(execsql, selectsql, lasttime);
    MediaDB<UINT, std::wstring, time_t>::exec(execsql, &uniqueid, &sPath, &oldlasttime);

    if (uniqueid)
    {
      memset(execsql, 0, 500);
      wsprintf(execsql, updatesql, ::time(0), uniqueid);
      MediaDB<>::exec(execsql);

      if (!SearchPath(sPath))
      {
        memset(execsql, 0, 500);
        if (0 == oldlasttime)
          oldlasttime = ::time(0);
        wsprintf(execsql, updatesql, oldlasttime, uniqueid);
        MediaDB<>::exec(execsql);
      }
    }

    if (_Exit_state(m_tSleep * 1000))
      return;
  }
}

BOOL MediaSpiderFolderTree::Search(const std::wstring &sFolder)
{
  using std::wstring;
  using std::vector;
  using boost::wregex;
  using boost::regex_replace;
  using namespace boost::filesystem;

  // check if we should spider this folder
  if (!IsValidPath(sFolder))
    return FALSE;

  try
  {
    // First, fetch all db's data to tree
    vector<wstring> vtPath;
    vector<wstring> vtFilename;
    vector<wstring> vtFilmname;
    vector<wstring> vtThumbnailPath;
    vector<bool> vtHide;

    std::wstringstream ssSQL;
    ssSQL.str(L"");
    ssSQL << L"SELECT path, filename, filmname, thumbnailpath, hide FROM media_data WHERE path='" << sFolder << L"'";
    MediaDB<wstring, wstring, wstring, wstring, bool>::exec(ssSQL.str(), &vtPath, &vtFilename, &vtFilmname, &vtThumbnailPath, &vtHide);
    for (size_t i = 0; i < vtPath.size(); ++i)
    {
      MediaData md;
      md.path = vtPath[i];
      md.filename = vtFilename[i];
      md.filmname = vtFilmname[i];
      md.bHide = vtHide[i];

      m_treeModel.addFile(md);

      if (_Exit_state(100))
        return FALSE;
    }

    // Second, search media in the path
    directory_iterator it(sFolder);
    directory_iterator itEnd;
    while (it != itEnd)
    {
      if (_Exit_state(300))
        return FALSE;

      if (IsSupportExtension(it->path().wstring())
       && !is_directory(it->path())
       && !isHiddenPath(it->path().wstring())
       && !IsFilteredItem(it->path().wstring()))
      {
        MediaData md;
        md.path = sFolder;
        md.filename = it->path().filename().wstring();
        m_treeModel.addFile(md);
      }
      ++it;
    }

    // store info to db
    m_treeModel.save2DB();

    // delete tree
    m_treeModel.delTree();
  }
  catch (const filesystem_error &err)
  {
    Logging(err.what());
    return FALSE;
  }
  return TRUE;
}