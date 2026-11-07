/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file qa_qt_crash.h
 * On Windows, a test that crashes prints the exception and the stack (with the symbols of the
 * PDB files, when the build makes them) before it ends.  Nothing on the other systems.
 */

#ifndef QA_QT_CRASH_H
#define QA_QT_CRASH_H

#ifdef _WIN32

#include <cstdio>

#include <windows.h>
#include <dbghelp.h>

#pragma comment( lib, "dbghelp.lib" )

#undef GetClassName
#undef GetObject
#undef GetCharWidth
#undef FindWindow


namespace QA_QT_CRASH
{

inline LONG WINAPI printCrash( EXCEPTION_POINTERS* aInfo )
{
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    std::fprintf( stdout, "CRASH exception 0x%08lX at %p\n",
                  aInfo->ExceptionRecord->ExceptionCode,
                  aInfo->ExceptionRecord->ExceptionAddress );

    SymSetOptions( SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME );
    SymInitialize( process, nullptr, TRUE );

    CONTEXT      context = *aInfo->ContextRecord;
    STACKFRAME64 frame = {};
    frame.AddrPC.Offset = context.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;

    for( int i = 0; i < 64; ++i )
    {
        if( !StackWalk64( IMAGE_FILE_MACHINE_AMD64, process, thread, &frame, &context, nullptr,
                          SymFunctionTableAccess64, SymGetModuleBase64, nullptr ) )
            break;

        DWORD64 address = frame.AddrPC.Offset;

        if( !address )
            break;

        char         buffer[sizeof( SYMBOL_INFO ) + 512] = {};
        SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>( buffer );
        symbol->SizeOfStruct = sizeof( SYMBOL_INFO );
        symbol->MaxNameLen = 511;

        char             module[MAX_PATH] = "?";
        IMAGEHLP_MODULE64 moduleInfo = {};
        moduleInfo.SizeOfStruct = sizeof( moduleInfo );

        if( SymGetModuleInfo64( process, address, &moduleInfo ) )
            std::snprintf( module, sizeof( module ), "%s", moduleInfo.ModuleName );

        DWORD64 displacement = 0;
        const char* name = SymFromAddr( process, address, &displacement, symbol ) ? symbol->Name
                                                                                   : "?";

        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof( line );
        DWORD lineDisplacement = 0;

        if( SymGetLineFromAddr64( process, address, &lineDisplacement, &line ) )
            std::fprintf( stdout, "  #%d %s!%s  %s:%lu\n", i, module, name, line.FileName,
                          line.LineNumber );
        else
            std::fprintf( stdout, "  #%d %s!%s +0x%llx\n", i, module, name,
                          (unsigned long long) displacement );
    }

    std::fflush( stdout );
    return EXCEPTION_CONTINUE_SEARCH;
}


inline void install()
{
    SetUnhandledExceptionFilter( printCrash );
}

} // namespace QA_QT_CRASH

#else

namespace QA_QT_CRASH
{
inline void install() {}
}

#endif

#endif // QA_QT_CRASH_H
