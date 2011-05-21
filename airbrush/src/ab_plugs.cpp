/*
    ab_plugs.cpp
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

#include <stdio.h>
#include <tchar.h>
#include "plugin.hpp"
#include "farcolor.hpp"
#include "memory.h"
#include "ab_main.h"
#include "abplugin.h"
#include "abversion.h"
#include "guid.h"
#include "far_settings.h"

struct PluginItem *PluginsData=NULL;
int PluginsCount=0;

static TCHAR Unknown[20];

static void ConvertColor(int Color,FarColor& NewColor)
{
  NewColor.Flags=FMSG_FG_4BIT|FMSG_BG_4BIT;
  NewColor.ForegroundColor=Color&0xf;
  NewColor.BackgroundColor=(Color>>4)&0xf;
  NewColor.Reserved=NULL;
}

static void WINAPI addcolor(int lno,int start,int len,int fg,int bg,enum ColorizePriority priority)
{
  if((fg==-1)&&(bg==-1)) return;
  if(len==0) return;
  WaitForSingleObject(Mutex,INFINITE);
  if(bg==-1) bg=Info.AdvControl(&MainGuid,ACTL_GETCOLOR,COL_EDITORTEXT,NULL)&0xF0;
  else bg=bg<<4;
  if(fg==-1) fg=Info.AdvControl(&MainGuid,ACTL_GETCOLOR,COL_EDITORTEXT,NULL)&0x0F;
  EditorColor ec;
  ec.StructSize=sizeof(ec);
  ec.StringNumber=lno;
  ec.StartPos=start;
  ec.EndPos=start+len-1;
  ConvertColor(fg|bg,ec.Color);
  ec.Owner=MainGuid;
  switch(priority)
  {
    case EPriorityNormal:
      ec.Priority=0;
      break;
    case EPriorityBrackets:
      ec.Priority=100;
      break;
  }
  Info.EditorControl(-1,ECTL_ADDCOLOR,0,&ec);
  ReleaseMutex(Mutex);
}

static void WINAPI delcolor(int lno)
{
  WaitForSingleObject(Mutex,INFINITE);
  EditorDeleteColor edc;
  edc.StructSize=sizeof(edc);
  edc.Owner=MainGuid;
  edc.StringNumber=lno;
  edc.StartPos=-1;
  Info.EditorControl(-1,ECTL_DELCOLOR,0,&edc);
  ReleaseMutex(Mutex);
}

static const TCHAR WINAPI *getline(int lno,int *len)
{
  EditorGetString egs;
  EditorSetPosition esp;
  esp.CurPos=-1; esp.CurTabPos=-1; esp.TopScreenLine=-1; esp.LeftPos=-1; esp.Overtype=-1;
  egs.StringNumber=-1;
  esp.CurLine=lno;
  WaitForSingleObject(Mutex,INFINITE);
  Info.EditorControl(-1,ECTL_SETPOSITION,0,&esp);
  Info.EditorControl(-1,ECTL_GETSTRING,0,&egs);
  ReleaseMutex(Mutex);
  if(len)
    *len=egs.StringLength;
  return egs.StringText;
}

static bool WINAPI addstate(int eid,int pos,unsigned long size,unsigned char *data)
{
  bool res=true;
  WaitForSingleObject(Mutex,INFINITE);
  PEditFile fl=ef_getfile(eid);
  if(fl)
  {
    if(pos==fl->cachesize)
    {
      StateCache **prev_cache=&(fl->cache);
      while(*prev_cache)
        prev_cache=&((*prev_cache)->next);
      *prev_cache=(StateCache *)malloc(sizeof(StateCache));
      if(*prev_cache)
      {
        if(size) (*prev_cache)->data=(unsigned char *)malloc(size);
        if((*prev_cache)->data)
        {
          (*prev_cache)->data_size=size;
          memcpy((*prev_cache)->data,data,size);
        }
        (fl->cachesize)++;
      }
    }
    else if((pos+1)!=fl->cachesize)
    {
      res=false;
      TCHAR buff[256];
      FSF.sprintf(buff,GetMsg(mFatalLine2),fl->cachesize,pos);
      const TCHAR* MsgItems[]={GetMsg(mError),GetMsg(mFatalLine1),buff,GetMsg(mButtonOk)};
      Info.Message(&MainGuid,FMSG_WARNING,NULL,MsgItems,sizeof(MsgItems)/sizeof(MsgItems[0]),1);
    }
  }
  if(!res) fatal=true;
  ReleaseMutex(Mutex);
  return res;
}

static void WINAPI getcursor(int *row,int *col)
{
  if((cursor_row>=0)&&(cursor_col>=0))
  {
    *row=cursor_row;
    *col=cursor_col;
  }
  else
  {
    EditorInfo ei;
    WaitForSingleObject(Mutex,INFINITE);
    Info.EditorControl(-1,ECTL_GETINFO,0,&ei);
    ReleaseMutex(Mutex);
    *row=ei.CurLine;
    *col=ei.CurPos;
  }
}

static void WINAPI callparser(const TCHAR *parser,struct ColorizeParams *params)
{
  for(int i=0;i<PluginsCount;i++)
    if(!lstrcmpi(parser,PluginsData[i].Name))
      if(PluginsData[i].pColorize)
        PluginsData[i].pColorize(PluginsData[i].Index,params);
}

void LoadPlugs(const TCHAR* ModuleName)
{
  HANDLE hSScr=Info.SaveScreen(0,0,-1,-1);
  {
    const TCHAR* MsgItems[]={GetMsg(mName),GetMsg(mLoading)};
    Info.Message(&MainGuid,0,NULL,MsgItems,sizeof(MsgItems)/sizeof(MsgItems[0]),0);
  }
  TCHAR PluginsFolder[MAX_PATH],PluginsMask[MAX_PATH],*NamePtr;
  lstrcpy(Unknown,GetMsg(mUnknown));
  lstrcpy(PluginsFolder,ModuleName);
  NamePtr=(TCHAR*)FSF.PointToName(PluginsFolder);
  lstrcpy(NamePtr,_T("\\Formats\\"));
  FSF.sprintf(PluginsMask,_T("%s*.fmt"),PluginsFolder);

  HANDLE FindHandle;
  WIN32_FIND_DATA fdata;
  int Done=((FindHandle=FindFirstFile(PluginsMask,&fdata))==INVALID_HANDLE_VALUE);
  while(!Done)
  {
    TCHAR PluginName[MAX_PATH];
    FSF.sprintf(PluginName,_T("%s%s"),PluginsFolder,fdata.cFileName);
    HMODULE hModule=LoadLibrary(PluginName);
    if (hModule!=NULL)
    {
      struct PluginItem CurPlugin;
      CurPlugin.hModule=hModule;
      CurPlugin.Type=0;
      CurPlugin.pLoadSyntaxModule=(PLUGINLOADSYNTAXMODULE)GetProcAddress(hModule,"LoadSyntaxModule");
      CurPlugin.pSetColorizeInfo=(PLUGINSETCOLORIZEINFO)GetProcAddress(hModule,"SetColorizeInfo");
      CurPlugin.pGetParams=(PLUGINGETPARAMS)GetProcAddress(hModule,"GetParams");
      CurPlugin.pColorize=(PLUGINCOLORIZE)GetProcAddress(hModule,"Colorize");
      CurPlugin.pInput=(PLUGININPUT)GetProcAddress(hModule,"Input");
      CurPlugin.pGetSyntaxCount=(PLUGINGETSYNTAXCOUNT)GetProcAddress(hModule,"GetSyntaxCount");
      CurPlugin.pExit=(PLUGINEXIT)GetProcAddress(hModule,"Exit");

      if(CurPlugin.pLoadSyntaxModule)
        CurPlugin.Type=CurPlugin.pLoadSyntaxModule(PluginName,&Info);
      int accept_plug=true;
      if(CurPlugin.pSetColorizeInfo)
      {
        ColorizeInfo lInfo;
        lInfo.size=sizeof(lInfo);
        lInfo.version=VER_ALL;
        lInfo.api=VER_API;
        lInfo.cachestr=PARSER_CACHESTR;
        lstrcpy(lInfo.folder,PluginsFolder);
        lInfo.pAddColor=addcolor;
        lInfo.pDelColor=delcolor;
        lInfo.pGetLine=getline;
        lInfo.pAddState=addstate;
        lInfo.pGetCursor=getcursor;
        lInfo.pCallParser=callparser;
        accept_plug=CurPlugin.pSetColorizeInfo(&lInfo);
      }

      if(accept_plug)
      {
        unsigned long subcount=1;
        if(CurPlugin.pGetSyntaxCount)
          subcount=CurPlugin.pGetSyntaxCount();

        if(subcount)
        {
          struct PluginItem *NewPluginsData=(struct PluginItem *)realloc(PluginsData,sizeof(*PluginsData)*(PluginsCount+subcount));
          if (NewPluginsData==NULL)
            break;
          PluginsData=NewPluginsData;
          for(unsigned long i=0;i<subcount;i++)
          {
            CurPlugin.Index=i;
            CurPlugin.Mask=NULL;
            CurPlugin.Start=NULL;
            CurPlugin.Params=0;
            const TCHAR *mask,*start,*name=NULL;
            TCHAR buff_mask[2048],buff_start[2048];
            lstrcpy(buff_mask,_T("")); lstrcpy(buff_start,_T(""));
            if(CurPlugin.pGetParams)
            {
              CurPlugin.Params=CurPlugin.pGetParams(CurPlugin.Index,PAR_GET_PARAMS,NULL);
              CurPlugin.pGetParams(CurPlugin.Index,PAR_GET_NAME,(const char**)&name);
              //load mask
              if(name&&(CurPlugin.Params&PAR_MASK_CACHE))
              {
                if(!CurPlugin.pGetParams(CurPlugin.Index,PAR_GET_MASK,(const char**)&mask))
                  mask=_T("");
                lstrcpy(buff_mask,mask);
                if(CurPlugin.Params&PAR_MASK_STORE)
                {
                  CFarSettings settings(MainGuid);
                  settings.Change(PLUGIN_MASK_KEY);
                  settings.Get(name,buff_mask,ArraySize(buff_mask));
                }
              }
              //load starts
              if(name&&(CurPlugin.Params&PAR_FILESTART_CACHE))
              {
                if(!CurPlugin.pGetParams(CurPlugin.Index,PAR_GET_FILESTART,(const char**)&start))
                  start=_T("");
                lstrcpy(buff_start,start);
                if(CurPlugin.Params&PAR_FILESTART_STORE)
                {
                  CFarSettings settings(MainGuid);
                  settings.Change(PLUGIN_START_KEY);
                  settings.Get(name,buff_start,ArraySize(buff_start));
                }
              }
            }
            if(name) CurPlugin.Name=name;
            else CurPlugin.Name=Unknown;
            if(lstrlen(buff_mask))
              CurPlugin.Mask=(TCHAR*)malloc((lstrlen(buff_mask)+1)*sizeof(TCHAR));
            if(CurPlugin.Mask)
              lstrcpy(CurPlugin.Mask,buff_mask);
            if(lstrlen(buff_start))
              CurPlugin.Start=(TCHAR*)malloc((lstrlen(buff_start)+1)*sizeof(TCHAR));
            if(CurPlugin.Start)
              lstrcpy(CurPlugin.Start,buff_start);
            if(CurPlugin.pGetParams)
            { //load colors
              int ColorCount; int *Colors;
              if((CurPlugin.Params&PAR_COLORS_STORE)&&CurPlugin.pGetParams(CurPlugin.Index,PAR_GET_COLOR_COUNT,(const char **)&ColorCount)&&CurPlugin.pGetParams(CurPlugin.Index,PAR_GET_COLOR,(const char **)&Colors))
              {
                CFarSettings settings(MainGuid);
                settings.Change(PLUGIN_COLOR_KEY);
                settings.Get(name,(void*)Colors,ColorCount*2*sizeof(int));
              }
            }
            PluginsData[PluginsCount]=CurPlugin;
            PluginsCount++;
          }
        }
      }
      else
        FreeLibrary(hModule);
    }
    Done=!FindNextFile(FindHandle,&fdata);
  }
  FindClose(FindHandle);
  Info.RestoreScreen(hSScr);
}

void UnloadPlugs(void)
{
  for(int i=0;i<PluginsCount;i++)
  {
    if(!PluginsData[i].Index)
    {
      if(PluginsData[i].pExit) PluginsData[i].pExit();
      FreeLibrary(PluginsData[i].hModule);
    }
    free(PluginsData[i].Mask);
    free(PluginsData[i].Start);
  }
  free(PluginsData);
}
