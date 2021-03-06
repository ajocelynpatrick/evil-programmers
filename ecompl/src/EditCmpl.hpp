/*
    EditCmpl.hpp
    Copyright (C) 2000-2001 Igor Lyubimov
    Copyright (C) 2002-2008 zg

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

#ifndef __EDITCMPL_HPP__
#define __EDITCMPL_HPP__

#include <tchar.h>

#define FAR165_INFO_SIZE 336

extern PluginStartupInfo Info;
extern FARSTANDARDFUNCTIONS FSF;

#include "avl_windows.hpp"
extern avl_window_tree *windows;

extern const TCHAR *GetMsg(int MsgId);

#endif
