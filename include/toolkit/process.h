/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

/**
 * @file process.h
 * The child processes launched by the GUI (the plugins of the API, the Python interpreter), and
 * the functions run later from the event loop of the GUI.
 *
 * Each GUI build has its own implementation (common/toolkit/process_wx.cpp,
 * qt/common/toolkit/process_qt.cpp).
 */

#include <functional>
#include <vector>

#include <wx/string.h>
#include <wx/utils.h>       // wxExecuteEnv

#include <kicommon.h>


namespace TOOLKIT
{

/// Called when a process ends: its exit code, and its standard output and error.
using PROCESS_END_HANDLER = std::function<void( int aExitCode, const wxString& aOutput,
                                                const wxString& aError )>;

/// Given the standard output of a process while it runs (from any thread).
using PROCESS_OUTPUT_HANDLER = std::function<void( const wxString& aOutput )>;

/**
 * Launch a process without waiting for its end.
 *
 * @param aArgv is the program and its arguments.
 * @param aEnv is the environment and the working directory of the process (nullptr for the ones
 *             of KiCad).
 * @param aMaxOutput is the maximum length of the output and of the error given to aOnEnd.
 * @param aHideConsole hides the console of the process (on Windows).
 * @param aOnOutput if set, is given the standard output while the process runs (the output is
 *                  then not given to aOnEnd).
 * @param aOnEnd is called from the event loop of the GUI when the process ends, with its
 *               standard output and error.
 * @return the pid of the process, or 0 if it could not be launched (the handlers are not called).
 */
KICOMMON_API long ExecuteAsync( const std::vector<wxString>& aArgv, const wxExecuteEnv* aEnv,
                                size_t aMaxOutput, bool aHideConsole,
                                PROCESS_OUTPUT_HANDLER aOnOutput, PROCESS_END_HANDLER aOnEnd );

/**
 * Run a function from the event loop of the GUI (on the main thread), after the events being
 * handled.  It can be called from any thread.
 *
 * @param aDelayMs delays the call by this time (in milliseconds).
 */
KICOMMON_API void RunOnMainThread( std::function<void()> aFunction, int aDelayMs = 0 );

} // namespace TOOLKIT
