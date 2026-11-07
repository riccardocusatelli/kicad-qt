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
 * PDB files, when the build makes them) before it ends; a test whose event loop does not answer
 * for 90 s prints the stack of the main thread.  Nothing on the other systems.
 */

#ifndef QA_QT_CRASH_H
#define QA_QT_CRASH_H

#ifdef _WIN32

#include <atomic>
#include <cstdio>
#include <memory>
#include <mutex>
#include <thread>

#include <QCoreApplication>

#include <windows.h>
#include <dbghelp.h>

#pragma comment( lib, "dbghelp.lib" )

#undef GetClassName
#undef GetObject
#undef GetCharWidth
#undef FindWindow


namespace QA_QT_CRASH
{

/// Print the stack of @a aThread from @a aContext (the thread stopped or the current one)
inline void printStack( HANDLE aThread, CONTEXT aContext )
{
    HANDLE process = GetCurrentProcess();

    static bool symbols = false;

    if( !symbols )
    {
        SymSetOptions( SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME );
        SymInitialize( process, nullptr, TRUE );
        symbols = true;
    }

    STACKFRAME64 frame = {};
    frame.AddrPC.Offset = aContext.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = aContext.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = aContext.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;

    for( int i = 0; i < 64; ++i )
    {
        if( !StackWalk64( IMAGE_FILE_MACHINE_AMD64, process, aThread, &frame, &aContext, nullptr,
                          SymFunctionTableAccess64, SymGetModuleBase64, nullptr ) )
            break;

        DWORD64 address = frame.AddrPC.Offset;

        if( !address )
            break;

        char         buffer[sizeof( SYMBOL_INFO ) + 512] = {};
        SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>( buffer );
        symbol->SizeOfStruct = sizeof( SYMBOL_INFO );
        symbol->MaxNameLen = 511;

        char              module[MAX_PATH] = "?";
        IMAGEHLP_MODULE64 moduleInfo = {};
        moduleInfo.SizeOfStruct = sizeof( moduleInfo );

        if( SymGetModuleInfo64( process, address, &moduleInfo ) )
            std::snprintf( module, sizeof( module ), "%s", moduleInfo.ModuleName );

        DWORD64     displacement = 0;
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
}


inline LONG WINAPI printCrash( EXCEPTION_POINTERS* aInfo )
{
    std::fprintf( stdout, "CRASH exception 0x%08lX at %p\n",
                  aInfo->ExceptionRecord->ExceptionCode,
                  aInfo->ExceptionRecord->ExceptionAddress );

    printStack( GetCurrentThread(), *aInfo->ContextRecord );
    return EXCEPTION_CONTINUE_SEARCH;
}


/// The application of the hang watch: null once it is deleted (its post routine)
inline std::mutex s_appMutex;
inline bool       s_appDeleted = false;

inline void appDeleted()
{
    std::lock_guard<std::mutex> lock( s_appMutex );
    s_appDeleted = true;
}


/**
 * A thread asking the event loop of the main thread to answer: when it does not answer for
 * @a aSeconds, prints the stack of the main thread (once for each hang).
 */
inline void watchHangs( HANDLE aMainThread, int aSeconds )
{
    std::thread(
            [aMainThread, aSeconds]()
            {
                auto answered = std::make_shared<std::atomic<bool>>( true );

                for( ;; )
                {
                    Sleep( 1000 );

                    {
                        std::lock_guard<std::mutex> lock( s_appMutex );

                        if( s_appDeleted )
                            return;

                        if( !QCoreApplication::instance() )
                            continue;

                        answered->store( false );
                        QMetaObject::invokeMethod( QCoreApplication::instance(),
                                                   [answered]() { answered->store( true ); },
                                                   Qt::QueuedConnection );
                    }

                    int waited = 0;

                    while( !answered->load() && waited < aSeconds * 10 )
                    {
                        Sleep( 100 );
                        ++waited;
                    }

                    if( answered->load() )
                        continue;

                    std::fprintf( stdout, "HANG the event loop does not answer for %d s\n",
                                  aSeconds );

                    if( SuspendThread( aMainThread ) != (DWORD) -1 )
                    {
                        CONTEXT context = {};
                        context.ContextFlags = CONTEXT_FULL;

                        if( GetThreadContext( aMainThread, &context ) )
                            printStack( aMainThread, context );

                        ResumeThread( aMainThread );
                    }

                    // Once for each hang
                    while( !answered->load() )
                        Sleep( 1000 );
                }
            } )
            .detach();
}


inline void install()
{
    SetUnhandledExceptionFilter( printCrash );
    qAddPostRoutine( appDeleted );

    HANDLE mainThread = nullptr;
    DuplicateHandle( GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &mainThread,
                     0, FALSE, DUPLICATE_SAME_ACCESS );
    watchHangs( mainThread, 90 );
}

} // namespace QA_QT_CRASH

#else

namespace QA_QT_CRASH
{
inline void install() {}
}

#endif

#endif // QA_QT_CRASH_H
