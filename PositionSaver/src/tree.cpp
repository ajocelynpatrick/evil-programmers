/*
    Position Saver plugin for FAR Manager
    Copyright (C) 2002-2005 Alex Yaroslavsky

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "tree.hpp"
#include "memory.hpp"
#include "crt.hpp"

TreeLink::TreeLink(void)
{
  data = 0;
  size = 0;
  Parent = 0;
  ChildLeft = 0;
  ChildRight = 0;
}

TreeLink::~TreeLink(void)
{
  if (data)
    free(data);
}

size_t TreeLink::GetDataSize(void)
{
  return size;
}

void TreeLink::GetData(void *user_data)
{
  memcpy(user_data,data,size);
}

PTreeLink TreeLink::GetParent(void)
{
  return Parent;
}

PTreeLink TreeLink::GetChildLeft(void)
{
  return ChildLeft;
}

PTreeLink TreeLink::GetChildRight(void)
{
  return ChildRight;
}

bool TreeLink::SetData(void *user_data, size_t user_size)
{
  if (!user_data || !user_size)
    return false;
  void *temp = (void *)malloc(user_size);
  if (!temp)
    return false;
  if (data)
    free(data);
  memcpy(temp,user_data,user_size);
  data = temp;
  size = user_size;
  return true;
}

void TreeLink::SetParent(PTreeLink new_parent)
{
  Parent = new_parent;
}

void TreeLink::SetChildLeft(PTreeLink new_childleft)
{
  ChildLeft = new_childleft;
}

void TreeLink::SetChildRight(PTreeLink new_childright)
{
  ChildRight = new_childright;
}

//===============================================================================================

int CmpID(PSTrackInfo *info1, PSTrackInfo *info2)
{
  #ifdef ALLOW_VIEWER_API
  int i1 = (info1->Type == TI_EDITOR ? info1->EditorID : info1->ViewerID);
  int i2 = (info2->Type == TI_EDITOR ? info2->EditorID : info2->ViewerID);
  #else
  int i1 = info1->EditorID;
  int i2 = info2->EditorID;
  #endif
  if (i1 == i2)
  {
    if (info1->Type == info2->Type)
      return 0;
    else
      return -1;
  }
  if (i1 > i2)
    return 1;
  return -1;
}

int CmpFN(PSTrackInfo *info1, PSTrackInfo *info2)
{
  if (info1->Type != info2->Type)
    return 1;
  return FSF.LStricmp(info1->FileName,info2->FileName);
}

void PSTree::RecDelete(PTreeLink link)
{
  if (link->GetChildLeft())
    RecDelete(link->GetChildLeft());
  if (link->GetChildRight())
    RecDelete(link->GetChildRight());
  delete link;
}

bool PSTree::RecInsert(PTreeLink link, PSTrackInfo *info, CMPFNC Cmp)
{
  PSTrackInfo temp;
  link->GetData(&temp);
  int i = Cmp(&temp,info);
  if (!i)
  {
    link->SetData(info,sizeof(PSTrackInfo));
    return true;
  }
  if (i > 0)
  {
    if (!link->GetChildRight())
    {
      PTreeLink newlink = new TreeLink;
      if (!newlink)
        return false;
      newlink->SetData(info,sizeof(PSTrackInfo));
      newlink->SetParent(link);
      link->SetChildRight(newlink);
      return true;
    }
    else
      return RecInsert(link->GetChildRight(),info, Cmp);
  }
  else
  {
    if (!link->GetChildLeft())
    {
      PTreeLink newlink = new TreeLink;
      if (!newlink)
        return false;
      newlink->SetData(info,sizeof(PSTrackInfo));
      newlink->SetParent(link);
      link->SetChildLeft(newlink);
      return true;
    }
    else
      return RecInsert(link->GetChildLeft(),info,Cmp);
  }
}

PTreeLink PSTree::RecFind(PTreeLink link, PSTrackInfo *info, CMPFNC Cmp)
{
  if (!link)
    return NULL;
  PSTrackInfo temp;
  link->GetData(&temp);
  int i = Cmp(&temp,info);
  if (!i)
  {
    return link;
  }
  if (i > 0)
  {
    return RecFind(link->GetChildRight(),info,Cmp);
  }
  else
  {
    return RecFind(link->GetChildLeft(),info,Cmp);
  }
}

PTreeLink PSTree::RecSearch(PTreeLink link, PSTrackInfo *info, CMPFNC Cmp)
{
  if (!link)
    return NULL;
  PSTrackInfo temp;
  link->GetData(&temp);
  if (!Cmp(&temp,info))
  {
    return link;
  }
  TreeLink *search = RecSearch(link->GetChildRight(),info,Cmp);
  if (search)
    return search;
  return RecSearch(link->GetChildLeft(),info,Cmp);
}

PSTree::PSTree(void)
{
  base = 0;
}

PSTree::~PSTree(void)
{
  if (base)
    RecDelete(base);
}

bool PSTree::Insert(PSTrackInfo *info)
{
  if (!base)
  {
    base = new TreeLink;
    if (!base)
      return false;
    base->SetData(info,sizeof(PSTrackInfo));
    return true;
  }
  return RecInsert(base, info, CmpID);
}

bool PSTree::FindID(PSTrackInfo *info)
{
  if (!base)
    return false;
  return (RecFind(base, info, CmpID) == NULL ? false : true);
}

bool PSTree::FindFN(PSTrackInfo *info)
{
  if (!base)
    return false;
  return (RecSearch(base, info, CmpFN) == NULL ? false : true);
}

bool PSTree::GetID(PSTrackInfo *info)
{
  if (!base)
    return false;
  PTreeLink link = RecFind(base, info, CmpID);
  if (!link)
    return false;
  link->GetData(info);
  return true;
}

bool PSTree::GetFN(PSTrackInfo *info)
{
  if (!base)
    return false;
  PTreeLink link = RecSearch(base, info, CmpFN);
  if (!link)
    return false;
  link->GetData(info);
  return true;
}

void PSTree::Delete(PSTrackInfo *info)
{
  if (!base)
    return;
  PTreeLink link = RecFind(base, info, CmpID);
  if (!link)
    return;
  PTreeLink left = link->GetChildLeft();
  PTreeLink right = link->GetChildRight();
  PTreeLink parent = link->GetParent();
  PTreeLink final;
  if (right && left)
  {
    PTreeLink temp = right;
    while (temp->GetChildLeft())
      temp = temp->GetChildLeft();
    temp->SetChildLeft(left);
    left->SetParent(temp);
    final = right;
  }
  else
  {
    final = (right ? right : left);
  }
  if (parent)
  {
    PTreeLink temp = parent->GetChildRight();
    if (temp == link)
    {
      parent->SetChildRight(final);
    }
    else
    {
      parent->SetChildLeft(final);
    }
  }
  else
  {
    base = final;
  }
  if (final)
    final->SetParent(parent);
  delete link;
}
