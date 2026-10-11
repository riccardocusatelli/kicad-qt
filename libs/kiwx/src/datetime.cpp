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

#include <wx/datetime.h>
#include <wx/translation.h>

#include "kiwx_compat.h"
#include <chrono>
#include <cstring>
#include <iomanip>
#include <locale>
#include <sstream>


const wxDateTime wxDefaultDateTime;


namespace
{

long long nowMs()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>( system_clock::now().time_since_epoch() ).count();
}


/// Expand the wxWidgets extensions of strftime(): %l (milliseconds).
std::string expandFormat( const wxString& aFormat, int aMs )
{
    std::string fmt = aFormat.utf8_string();
    std::string out;

    for( size_t i = 0; i < fmt.size(); ++i )
    {
        if( fmt[i] == '%' && i + 1 < fmt.size() )
        {
            if( fmt[i + 1] == 'l' )
            {
                char buf[8];
                std::snprintf( buf, sizeof( buf ), "%03d", aMs );
                out += buf;
                ++i;
                continue;
            }

            out += fmt[i];
            out += fmt[++i];
            continue;
        }

        out += fmt[i];
    }

    return out;
}

} // namespace


long wxDateTime::TimeZone::GetOffset() const
{
    if( m_explicit )
        return m_offset;

    if( m_utc )
        return 0;

    return kiwx::GmtOffset( std::time( nullptr ) );
}


struct tm wxDateTime::tm( const TimeZone& aTz ) const
{
    struct tm result;
    std::memset( &result, 0, sizeof( result ) );
    time_t secs = GetTicks();

    if( aTz.IsLocal() )
    {
        kiwx::LocalTime( secs, result );
    }
    else
    {
        time_t shifted = secs + aTz.GetOffset();
        kiwx::GmTime( shifted, result );
    }

    return result;
}


wxDateTime::wxDateTime( double aJdn )
{
    // Julian day number 2440587.5 is the epoch.
    m_ms = static_cast<long long>( ( aJdn - 2440587.5 ) * 86400000.0 );
    m_valid = true;
}


wxDateTime wxDateTime::Now()
{
    return FromMilliseconds( floorDiv( nowMs(), 1000 ) * 1000 );
}


wxDateTime wxDateTime::UNow()
{
    return FromMilliseconds( nowMs() );
}


wxDateTime wxDateTime::Today()
{
    return Now().ResetTime();
}


int wxDateTime::GetCurrentYear()
{
    return Now().GetYear();
}


wxDateTime::Month wxDateTime::GetCurrentMonth()
{
    return Now().GetMonth();
}


bool wxDateTime::IsLeapYear( int aYear )
{
    if( aYear == Inv_Year )
        aYear = GetCurrentYear();

    return ( aYear % 4 == 0 && aYear % 100 != 0 ) || aYear % 400 == 0;
}


wxDateTime::wxDateTime_t wxDateTime::GetNumberOfDays( Month aMonth, int aYear )
{
    static const wxDateTime_t days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if( aMonth == Feb && IsLeapYear( aYear ) )
        return 29;

    return aMonth < Inv_Month ? days[aMonth] : 0;
}


wxString wxDateTime::GetMonthName( Month aMonth, int aFlags )
{
    struct tm t;
    std::memset( &t, 0, sizeof( t ) );
    t.tm_mon = aMonth;
    char buf[64];
    std::strftime( buf, sizeof( buf ), aFlags ? "%b" : "%B", &t );
    return wxString( buf );
}


wxString wxDateTime::GetWeekDayName( WeekDay aDay, int aFlags )
{
    struct tm t;
    std::memset( &t, 0, sizeof( t ) );
    t.tm_wday = aDay;
    char buf[64];
    std::strftime( buf, sizeof( buf ), aFlags ? "%a" : "%A", &t );
    return wxString( buf );
}


wxDateTime& wxDateTime::Set( time_t aTime )
{
    m_ms = static_cast<long long>( aTime ) * 1000;
    m_valid = true;
    return *this;
}


wxDateTime& wxDateTime::Set( const struct tm& aTm )
{
    struct tm copy = aTm;
    copy.tm_isdst = -1;
    time_t t = mktime( &copy );

    if( t == (time_t) -1 )
    {
        m_valid = false;
        return *this;
    }

    return Set( t );
}


wxDateTime& wxDateTime::Set( wxDateTime_t aHour, wxDateTime_t aMinute, wxDateTime_t aSecond,
                             wxDateTime_t aMs )
{
    time_t    now = time( nullptr );
    struct tm tm;
    kiwx::LocalTime( now, tm );
    tm.tm_hour = aHour;
    tm.tm_min = aMinute;
    tm.tm_sec = aSecond;
    Set( tm );
    return SetMillisecond( aMs );
}


wxDateTime& wxDateTime::Set( wxDateTime_t aDay, Month aMonth, int aYear, wxDateTime_t aHour,
                             wxDateTime_t aMinute, wxDateTime_t aSecond, wxDateTime_t aMs )
{
    if( aYear == Inv_Year )
        aYear = GetCurrentYear();

    if( aMonth == Inv_Month )
        aMonth = GetCurrentMonth();

    struct tm t;
    std::memset( &t, 0, sizeof( t ) );
    t.tm_year = aYear - 1900;
    t.tm_mon = aMonth;
    t.tm_mday = aDay;
    t.tm_hour = aHour;
    t.tm_min = aMinute;
    t.tm_sec = aSecond;
    Set( t );

    if( m_valid )
        m_ms += aMs;

    return *this;
}


wxDateTime& wxDateTime::ResetTime()
{
    struct tm t = tm( Local );
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    return Set( t );
}


wxDateTime& wxDateTime::SetMillisecond( wxDateTime_t aMs )
{
    m_ms = floorDiv( m_ms, 1000 ) * 1000 + aMs;
    return *this;
}


bool wxDateTime::IsSameDate( const wxDateTime& o ) const
{
    struct tm a = tm( Local );
    struct tm b = o.tm( Local );
    return a.tm_year == b.tm_year && a.tm_mon == b.tm_mon && a.tm_mday == b.tm_mday;
}


bool wxDateTime::IsSameTime( const wxDateTime& o ) const
{
    struct tm a = tm( Local );
    struct tm b = o.tm( Local );
    return a.tm_hour == b.tm_hour && a.tm_min == b.tm_min && a.tm_sec == b.tm_sec
           && GetMillisecond() == o.GetMillisecond();
}


wxDateTime& wxDateTime::Add( const wxDateSpan& aSpan )
{
    struct tm t = tm( Local );
    int       ms = GetMillisecond();
    t.tm_year += aSpan.GetYears();
    t.tm_mon += aSpan.GetMonths();
    t.tm_mday += aSpan.GetTotalDays();
    Set( t );
    m_ms += ms;
    return *this;
}


wxDateTime& wxDateTime::Subtract( const wxDateSpan& aSpan )
{
    return Add( wxDateSpan( -aSpan.GetYears(), -aSpan.GetMonths(), -aSpan.GetWeeks(), -aSpan.GetDays() ) );
}


wxString wxDateTime::Format( const wxString& aFormat, const TimeZone& aTz ) const
{
    if( !m_valid )
        return wxString();

    struct tm   t = tm( aTz );
    std::string fmt = expandFormat( aFormat, GetMillisecond() );

    if( fmt.empty() )
        return wxString();

    std::string buf( 256 + fmt.size() * 8, '\0' );
    size_t      len = std::strftime( buf.data(), buf.size(), fmt.c_str(), &t );

    return wxString::FromUTF8( buf.data(), len );
}


bool wxDateTime::ParseFormat( const wxString& aDate, const wxString& aFormat,
                              wxString::const_iterator* aEnd )
{
    struct tm t;
    std::memset( &t, 0, sizeof( t ) );
    t.tm_mday = 1;
    t.tm_isdst = -1;

    std::string date = aDate.utf8_string();
    std::string fmt = aFormat.utf8_string();
#ifdef _WIN32
    // No strptime(): std::get_time() takes the same conversions
    std::istringstream in( date );
    in.imbue( std::locale::classic() );
    in >> std::get_time( &t, fmt.c_str() );

    if( in.fail() )
        return false;

    const char* end = date.c_str() + ( in.eof() ? date.size() : static_cast<size_t>( in.tellg() ) );
#else
    const char* end = strptime( date.c_str(), fmt.c_str(), &t );

    if( !end )
        return false;
#endif

    Set( t );

    if( aEnd )
        *aEnd = aDate.begin() + wxString::FromUTF8( date.c_str(), end - date.c_str() ).length();

    return m_valid;
}


bool wxDateTime::ParseFormat( const wxString& aDate, const wxString& aFormat, const wxDateTime&,
                              wxString::const_iterator* aEnd )
{
    return ParseFormat( aDate, aFormat, aEnd );
}


bool wxDateTime::ParseISODate( const wxString& aDate )
{
    wxString::const_iterator end;
    return ParseFormat( aDate, wxT( "%Y-%m-%d" ), &end ) && end == aDate.end();
}


bool wxDateTime::ParseISOTime( const wxString& aTime )
{
    wxString::const_iterator end;
    return ParseFormat( aTime, wxT( "%H:%M:%S" ), &end ) && end == aTime.end();
}


bool wxDateTime::ParseISOCombined( const wxString& aDate, char aSep )
{
    wxString fmt = wxString( wxT( "%Y-%m-%d" ) ) + wxUniChar( aSep ) + wxT( "%H:%M:%S" );
    wxString::const_iterator end;
    return ParseFormat( aDate, fmt, &end ) && end == aDate.end();
}


bool wxDateTime::ParseDateTime( const wxString& aDateTime, wxString::const_iterator* aEnd )
{
    for( const wchar_t* fmt : { L"%Y-%m-%dT%H:%M:%S", L"%Y-%m-%d %H:%M:%S", L"%Y-%m-%d %H:%M",
                                L"%c", L"%x %X" } )
    {
        if( ParseFormat( aDateTime, fmt, aEnd ) )
            return true;
    }

    return false;
}


bool wxDateTime::ParseDate( const wxString& aDate, wxString::const_iterator* aEnd )
{
    for( const wchar_t* fmt : { L"%Y-%m-%d", L"%x", L"%d %b %Y", L"%b %d %Y" } )
    {
        if( ParseFormat( aDate, fmt, aEnd ) )
            return true;
    }

    return false;
}


bool wxDateTime::ParseTime( const wxString& aTime, wxString::const_iterator* aEnd )
{
    wxCHECK_MSG( aEnd, false, "end iterator pointer must be specified" );

    static const struct
    {
        const char*  name;
        wxDateTime_t hour;
    } stdTimes[] = { { wxTRANSLATE( "noon" ), 12 }, { wxTRANSLATE( "midnight" ), 0 } };

    for( const auto& stdTime : stdTimes )
    {
        const wxString timeString = wxGetTranslation( stdTime.name );

        if( timeString.CmpNoCase( wxString( aTime, timeString.length() ) ) == 0 )
        {
            Set( stdTime.hour, 0, 0 );
            *aEnd = aTime.begin() + timeString.length();
            return true;
        }
    }

    // from the longest to the shortest, as wxWidgets
    for( const wchar_t* fmt : { L"%I:%M:%S %p", L"%H:%M:%S", L"%I:%M %p", L"%H:%M", L"%I %p", L"%H",
                                L"%X" } )
    {
        if( ParseFormat( aTime, fmt, aEnd ) )
            return true;
    }

    return false;
}


wxString wxTimeSpan::Format( const wxString& aFormat ) const
{
    long long ms = m_ms < 0 ? -m_ms : m_ms;
    wxString  out = m_ms < 0 ? wxT( "-" ) : wxT( "" );
    const std::wstring& fmt = aFormat.impl();

    for( size_t i = 0; i < fmt.size(); ++i )
    {
        if( fmt[i] != L'%' || i + 1 >= fmt.size() )
        {
            out += fmt[i];
            continue;
        }

        switch( fmt[++i] )
        {
        case L'D': out += wxString::Format( wxT( "%lld" ), ms / 86400000 ); break;
        case L'H': out += wxString::Format( wxT( "%02lld" ), ms / 3600000 ); break;
        case L'M': out += wxString::Format( wxT( "%02lld" ), ( ms / 60000 ) % 60 ); break;
        case L'S': out += wxString::Format( wxT( "%02lld" ), ( ms / 1000 ) % 60 ); break;
        case L'l': out += wxString::Format( wxT( "%03lld" ), ms % 1000 ); break;
        case L'%': out += L'%'; break;
        default:   out += L'%'; out += fmt[i]; break;
        }
    }

    return out;
}


wxLongLong wxGetLocalTimeMillis()
{
    return nowMs();
}


wxLongLong wxGetUTCTimeMillis()
{
    return nowMs();
}


long wxGetLocalTime()
{
    return static_cast<long>( std::time( nullptr ) );
}


long wxGetUTCTime()
{
    return static_cast<long>( std::time( nullptr ) );
}
