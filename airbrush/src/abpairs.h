/*
    abpairs.h
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

#ifndef __ABPAIRS_HPP__
#define __ABPAIRS_HPP__

//pair manager

struct PairStack
{
  int index;
  int row;
  int col;
  int len;
  int flag; //cursor here
  struct PairStack *next;
};

inline void PairStackPush(HANDLE Heap,struct PairStack **ptr,int index,int row,int col,int len,int flag)
{
  struct PairStack *a=(struct PairStack *)HeapAlloc(Heap,HEAP_ZERO_MEMORY,sizeof(struct PairStack));
  a->index=index;
  a->row=row;
  a->col=col;
  a->len=len;
  a->flag=flag;
  a->next=*ptr;
  *ptr=a;
}

inline void PairStackPop(HANDLE Heap,struct PairStack **ptr)
{
  struct PairStack *a=*ptr;
  if(a)
  {
    *ptr=(*ptr)->next;
    HeapFree(Heap,0,a);
  }
}

inline void PairStackClear(HANDLE Heap,struct PairStack **ptr)
{
  while(*ptr)
    PairStackPop(Heap,ptr);
}

inline int PairStackCursor(int row,int col,int len,int cursor_row,int cursor_col)
{
  int res=0,diff=cursor_col-col;
  if(row==cursor_row)
    if((diff>=0)&&(diff<len))
      res=1;
  return res;
}

//pair highlite stuff

#define PUSH_PAIR_0(LEVEL,COLOR,SIMPLE) \
{ \
  int flag=PairStackCursor(lno,yytok-line,yycur-yytok,hl_row,hl_col); \
  PairStackPush(params->LocalHeap,&hl_state,LEVEL,lno,yytok-line,yycur-yytok,flag); \
  if(flag) \
    Info.pAddColor(params,lno,yytok-line,yycur-yytok,colors+HC_HIGHLITE,EPriorityBrackets); \
  else if(SIMPLE)\
    Info.pAddColor(params,lno,yytok-line,yycur-yytok,colors+COLOR,EPriorityNormal); \
}

#define POP_PAIR_0(LEVEL1,LEVEL2,COLOR,SIMPLE) \
{ \
  int flag=PairStackCursor(lno,yytok-line,yycur-yytok,hl_row,hl_col); \
  if(flag) \
    Info.pAddColor(params,lno,yytok-line,yycur-yytok,colors+HC_HIGHLITE,EPriorityBrackets); \
  else if(SIMPLE) \
    Info.pAddColor(params,lno,yytok-line,yycur-yytok,colors+COLOR,EPriorityNormal); \
  if(hl_state) \
  { \
    bool err=!((hl_state->index>=LEVEL1)&&(hl_state->index<=LEVEL2)); \
    if(hl_state->flag) \
    { \
      Info.pAddColor(params,lno,yytok-line,yycur-yytok,colors+HC_HIGHLITE+err,EPriorityBrackets); \
      Info.pSetBracket(params->eid,lno,yytok-line); \
    } \
    if(flag) \
    { \
      Info.pAddColor(params,hl_state->row,hl_state->col,hl_state->len,colors+HC_HIGHLITE+err,EPriorityBrackets); \
      Info.pSetBracket(params->eid,hl_state->row,hl_state->col); \
    } \
  } \
  PairStackPop(params->LocalHeap,&hl_state); \
}

#define PUSH_PAIR_1(LEVEL,COLOR) PUSH_PAIR_0(LEVEL,COLOR,1)
#define POP_PAIR_1(LEVEL1,COLOR) POP_PAIR_0(LEVEL1,LEVEL1,COLOR,1)

#define PUSH_PAIR_S(LEVEL) PUSH_PAIR_0(LEVEL,HC_STRING1,0)
#define POP_PAIR_S(LEVEL1) POP_PAIR_0(LEVEL1,LEVEL1,HC_STRING1,0)

#define PUSH_PAIR(LEVEL) PUSH_PAIR_0(LEVEL,HC_KEYWORD1,1) goto colorize_clear;
#define POP_PAIR(LEVEL1) POP_PAIR_0(LEVEL1,LEVEL1,HC_KEYWORD1,1) goto colorize_clear;
#define POP_PAIR_EXT(LEVEL1,LEVEL2) POP_PAIR_0(LEVEL1,LEVEL2,HC_KEYWORD1,1) goto colorize_clear;

#endif
