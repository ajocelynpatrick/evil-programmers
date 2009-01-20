/*
    ab_main.h
    Copyright (C) 2000-2008 zg

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __AB_MAIN_HPP__
#define __AB_MAIN_HPP__

// strings in lng file
enum {
  mName,
  mUnknown,
  mDefault,
  mLoading,
  mConfigMenu1,
  mConfigMenu2,
  mConfigMenu3,
  mConfigMenu4,
  mConfigDialogActive,
  mConfigDialogColorizeAll,
  mConfigDialogMaxLines,
  mConfigSave,
  mConfigCancel,
  mColorMain,
  mColorForeground,
  mColorBackground,
  mColorTest1,
  mColorTest2,
  mColorTest3,
  mColorSet,
  mColorCancel,
  mStopQuestion,
  mFatalLine1,
  mFatalLine2,
  mButtonOk,
  mButtonYes,
  mButtonNo,
  mError,
};

const TCHAR* GetMsg(int MsgId);
TCHAR* GetCommaWord(TCHAR* Src,TCHAR* Word);

typedef unsigned long (WINAPI *PLUGINLOADSYNTAXMODULE)(TCHAR* ModuleName,void *Info);
typedef int (WINAPI *PLUGINSETCOLORIZEINFO)(struct ColorizeInfo *AInfo);
typedef int (WINAPI *PLUGINGETPARAMS)(int index,int command,const char **param);
typedef void (WINAPI *PLUGINCOLORIZE)(int index,struct ColorizeParams *params);
typedef int (WINAPI *PLUGININPUT)(const INPUT_RECORD *rec);
typedef unsigned long (WINAPI *PLUGINGETSYNTAXCOUNT)(void);
typedef void (WINAPI *PLUGINEXIT)(void);

struct PluginItem
{
  HMODULE hModule;
  int Index;
  int Type;
  const TCHAR* Name;
  TCHAR* Mask;
  TCHAR* Start;
  int Params;
  PLUGINLOADSYNTAXMODULE pLoadSyntaxModule;
  PLUGINSETCOLORIZEINFO pSetColorizeInfo;
  PLUGINGETPARAMS pGetParams;
  PLUGINCOLORIZE pColorize;
  PLUGININPUT pInput;
  PLUGINGETSYNTAXCOUNT pGetSyntaxCount;
  PLUGINEXIT pExit;
};

extern struct PluginItem *PluginsData;
extern int PluginsCount;

struct StateCache
{
  unsigned long data_size;
  unsigned char *data;
  StateCache *next;
};

typedef struct CEditFile *PEditFile;

struct CEditFile
{
  int id;
  bool main;
  PEditFile next, prev;
//
  StateCache *cache;
  int cachesize;
  int type;
};

//struct CEditFile methods
extern PEditFile ef_create(bool m = false);
extern void ef_free(PEditFile value);
extern PEditFile ef_getfile(int fid);
extern PEditFile ef_addfile(int fid);
extern bool ef_deletefile(int fid);

extern PEditFile editfiles;

extern PEditFile loadfile(int eid,int type);

extern void OnLoad(void);
extern void OnExit(void);
extern void OnConfigure(void);
extern int OnEditorEvent(int event, void *param);
extern int OnEditorInput(const INPUT_RECORD *Rec);

#define PARSER_CACHESTR 50
#define FAR165_INFO_SIZE 336

extern PluginStartupInfo Info;
extern FARSTANDARDFUNCTIONS FSF;
extern HANDLE Mutex;
extern int cursor_row,cursor_col;
extern TCHAR PluginRootKey[];
extern TCHAR PluginMaskKey[];
extern TCHAR PluginColorKey[];
extern TCHAR PluginStartKey[];

extern void LoadPlugs(const TCHAR* ModuleName);
extern void UnloadPlugs(void);

struct InitDialogItem
{
  int Type;
  int X1,Y1,X2,Y2;
  int Focus;
  DWORD_PTR Selected;
  unsigned int Flags;
  int DefaultButton;
  const TCHAR* Data;
};

extern void InitDialogItems(InitDialogItem *Init,FarDialogItem *Item,int ItemsNumber);
extern bool SelectColor(int *fg,int *bg);

struct Options
{
  bool Active;
  int MaxLines;
  bool ColorizeAll;
};

extern struct Options Opt;
extern bool fatal;

#ifndef UNICODE
#define EXP_NAME(p) _export p
#else
#define EXP_NAME(p) _export p ## W
#endif

#endif