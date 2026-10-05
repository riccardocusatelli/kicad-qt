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

#pragma once

// exported from the kiwx library, as the wxWidgets API (WXDLLIMPEXP_BASE)
#pragma GCC visibility push( default )

#include <wx/longlong.h>
#include <wx/string.h>

#include <ctime>


class wxTimeSpan
{
public:
    wxTimeSpan() = default;
    wxTimeSpan( long aHours, long aMinutes = 0, wxLongLong aSeconds = 0, wxLongLong aMs = 0 ) :
            m_ms( ( ( aHours * 60LL + aMinutes ) * 60LL + aSeconds.GetValue() ) * 1000LL + aMs.GetValue() )
    {
    }

    static wxTimeSpan Milliseconds( wxLongLong aMs ) { wxTimeSpan s; s.m_ms = aMs.GetValue(); return s; }
    static wxTimeSpan Seconds( wxLongLong aSec ) { return Milliseconds( aSec.GetValue() * 1000LL ); }
    static wxTimeSpan Minutes( long aMin ) { return Seconds( aMin * 60LL ); }
    static wxTimeSpan Hours( long aHours ) { return Minutes( aHours * 60L ); }
    static wxTimeSpan Days( long aDays ) { return Hours( aDays * 24L ); }
    static wxTimeSpan Weeks( long aWeeks ) { return Days( aWeeks * 7L ); }

    wxLongLong GetValue() const { return m_ms; }
    wxLongLong GetMilliseconds() const { return m_ms; }
    wxLongLong GetSeconds() const { return m_ms / 1000; }
    int        GetMinutes() const { return static_cast<int>( m_ms / 60000 ); }
    int        GetHours() const { return static_cast<int>( m_ms / 3600000 ); }
    int        GetDays() const { return static_cast<int>( m_ms / 86400000 ); }
    int        GetWeeks() const { return GetDays() / 7; }

    bool IsNull() const { return m_ms == 0; }
    bool IsPositive() const { return m_ms > 0; }
    bool IsNegative() const { return m_ms < 0; }
    wxTimeSpan Abs() const { return Milliseconds( m_ms < 0 ? -m_ms : m_ms ); }
    wxTimeSpan Negate() const { return Milliseconds( -m_ms ); }

    bool IsShorterThan( const wxTimeSpan& aOther ) const { return Abs().m_ms < aOther.Abs().m_ms; }
    bool IsLongerThan( const wxTimeSpan& aOther ) const { return Abs().m_ms > aOther.Abs().m_ms; }
    bool IsEqualTo( const wxTimeSpan& aOther ) const { return m_ms == aOther.m_ms; }

    wxTimeSpan operator+( const wxTimeSpan& o ) const { return Milliseconds( m_ms + o.m_ms ); }
    wxTimeSpan operator-( const wxTimeSpan& o ) const { return Milliseconds( m_ms - o.m_ms ); }
    wxTimeSpan operator*( int n ) const { return Milliseconds( m_ms * n ); }
    wxTimeSpan& operator+=( const wxTimeSpan& o ) { m_ms += o.m_ms; return *this; }
    wxTimeSpan& operator-=( const wxTimeSpan& o ) { m_ms -= o.m_ms; return *this; }

    bool operator<( const wxTimeSpan& o ) const { return m_ms < o.m_ms; }
    bool operator>( const wxTimeSpan& o ) const { return m_ms > o.m_ms; }
    bool operator<=( const wxTimeSpan& o ) const { return m_ms <= o.m_ms; }
    bool operator>=( const wxTimeSpan& o ) const { return m_ms >= o.m_ms; }
    bool operator==( const wxTimeSpan& o ) const { return m_ms == o.m_ms; }
    bool operator!=( const wxTimeSpan& o ) const { return m_ms != o.m_ms; }

    wxString Format( const wxString& aFormat = wxT( "%H:%M:%S" ) ) const;

private:
    long long m_ms = 0;
};


class wxDateSpan
{
public:
    wxDateSpan( int aYears = 0, int aMonths = 0, int aWeeks = 0, int aDays = 0 ) :
            m_years( aYears ), m_months( aMonths ), m_weeks( aWeeks ), m_days( aDays )
    {
    }

    static wxDateSpan Days( int n ) { return wxDateSpan( 0, 0, 0, n ); }
    static wxDateSpan Weeks( int n ) { return wxDateSpan( 0, 0, n, 0 ); }
    static wxDateSpan Months( int n ) { return wxDateSpan( 0, n, 0, 0 ); }
    static wxDateSpan Years( int n ) { return wxDateSpan( n, 0, 0, 0 ); }
    static wxDateSpan Day() { return Days( 1 ); }
    static wxDateSpan Week() { return Weeks( 1 ); }
    static wxDateSpan Month() { return Months( 1 ); }
    static wxDateSpan Year() { return Years( 1 ); }

    wxDateSpan& SetYears( int n ) { m_years = n; return *this; }
    wxDateSpan& SetMonths( int n ) { m_months = n; return *this; }
    wxDateSpan& SetWeeks( int n ) { m_weeks = n; return *this; }
    wxDateSpan& SetDays( int n ) { m_days = n; return *this; }

    wxDateSpan Add( const wxDateSpan& o ) const
    {
        return wxDateSpan( m_years + o.m_years, m_months + o.m_months, m_weeks + o.m_weeks, m_days + o.m_days );
    }
    wxDateSpan Subtract( const wxDateSpan& o ) const
    {
        return wxDateSpan( m_years - o.m_years, m_months - o.m_months, m_weeks - o.m_weeks, m_days - o.m_days );
    }
    wxDateSpan Multiply( int n ) const { return wxDateSpan( m_years * n, m_months * n, m_weeks * n, m_days * n ); }
    wxDateSpan Negate() const { return Multiply( -1 ); }
    wxDateSpan operator-() const { return Negate(); }
    wxDateSpan operator+( const wxDateSpan& o ) const { return Add( o ); }
    wxDateSpan operator-( const wxDateSpan& o ) const { return Subtract( o ); }
    wxDateSpan operator*( int n ) const { return Multiply( n ); }
    bool       operator==( const wxDateSpan& o ) const
    {
        return m_years == o.m_years && m_months == o.m_months && GetTotalDays() == o.GetTotalDays();
    }
    bool operator!=( const wxDateSpan& o ) const { return !( *this == o ); }

    int GetYears() const { return m_years; }
    int GetMonths() const { return m_months; }
    int GetWeeks() const { return m_weeks; }
    int GetDays() const { return m_days; }
    int GetTotalDays() const { return m_weeks * 7 + m_days; }

private:
    int m_years;
    int m_months;
    int m_weeks;
    int m_days;
};


/**
 * A point in time with millisecond resolution (UTC internally; broken down and formatted in
 * local time unless a time zone is given).
 */
class wxDateTime
{
public:
    typedef unsigned short wxDateTime_t;

    enum Month
    {
        Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec, Inv_Month
    };

    enum WeekDay
    {
        Sun, Mon, Tue, Wed, Thu, Fri, Sat, Inv_WeekDay
    };

    enum TZ
    {
        Local,
        UTC = 1000,
        GMT0 = UTC
    };

    enum
    {
        Inv_Year = SHRT_MIN
    };

    class TimeZone
    {
    public:
        TimeZone( TZ aTz ) : m_utc( aTz == UTC ) {}
        static TimeZone Make( long aOffset ) { TimeZone tz( Local ); tz.m_offset = aOffset; tz.m_explicit = true; return tz; }
        bool IsLocal() const { return !m_utc && !m_explicit; }
        long GetOffset() const;

    private:
        bool m_utc = false;
        bool m_explicit = false;
        long m_offset = 0;
    };

    wxDateTime() = default;
    wxDateTime( time_t aTime ) { Set( aTime ); }
    wxDateTime( const struct tm& aTm ) { Set( aTm ); }
    explicit wxDateTime( double aJdn );
    wxDateTime( wxDateTime_t aDay, Month aMonth, int aYear = Inv_Year, wxDateTime_t aHour = 0,
                wxDateTime_t aMinute = 0, wxDateTime_t aSecond = 0, wxDateTime_t aMs = 0 )
    {
        Set( aDay, aMonth, aYear, aHour, aMinute, aSecond, aMs );
    }

    static wxDateTime Now();
    static wxDateTime UNow();
    static wxDateTime Today();
    static time_t     GetTimeNow() { return std::time( nullptr ); }
    static int        GetCurrentYear();
    static Month      GetCurrentMonth();
    static wxDateTime_t GetNumberOfDays( Month aMonth, int aYear = Inv_Year );
    static bool       IsLeapYear( int aYear = Inv_Year );
    static wxString   GetMonthName( Month aMonth, int aFlags = 0 );
    static wxString   GetWeekDayName( WeekDay aDay, int aFlags = 0 );

    wxDateTime& Set( time_t aTime );
    wxDateTime& Set( const struct tm& aTm );
    wxDateTime& Set( wxDateTime_t aDay, Month aMonth, int aYear = Inv_Year, wxDateTime_t aHour = 0,
                     wxDateTime_t aMinute = 0, wxDateTime_t aSecond = 0, wxDateTime_t aMs = 0 );
    /// Today at the given time.
    wxDateTime& Set( wxDateTime_t aHour, wxDateTime_t aMinute = 0, wxDateTime_t aSecond = 0,
                     wxDateTime_t aMs = 0 );
    wxDateTime& SetToCurrent() { return *this = Now(); }
    wxDateTime& ResetTime();
    wxDateTime& SetMillisecond( wxDateTime_t aMs );

    bool IsValid() const { return m_valid; }

    time_t     GetTicks() const { return static_cast<time_t>( floorDiv( m_ms, 1000 ) ); }
    wxLongLong GetValue() const { return m_ms; }

    int          GetYear( const TimeZone& aTz = Local ) const { return tm( aTz ).tm_year + 1900; }
    Month        GetMonth( const TimeZone& aTz = Local ) const { return static_cast<Month>( tm( aTz ).tm_mon ); }
    wxDateTime_t GetDay( const TimeZone& aTz = Local ) const { return tm( aTz ).tm_mday; }
    WeekDay      GetWeekDay( const TimeZone& aTz = Local ) const { return static_cast<WeekDay>( tm( aTz ).tm_wday ); }
    wxDateTime_t GetHour( const TimeZone& aTz = Local ) const { return tm( aTz ).tm_hour; }
    wxDateTime_t GetMinute( const TimeZone& aTz = Local ) const { return tm( aTz ).tm_min; }
    wxDateTime_t GetSecond( const TimeZone& aTz = Local ) const { return tm( aTz ).tm_sec; }
    wxDateTime_t GetMillisecond( const TimeZone& = Local ) const { return static_cast<wxDateTime_t>( floorMod( m_ms, 1000 ) ); }
    wxDateTime_t GetDayOfYear( const TimeZone& aTz = Local ) const { return tm( aTz ).tm_yday + 1; }

    wxDateTime ToUTC( bool = false ) const { return *this; }
    wxDateTime FromUTC( bool = false ) const { return *this; }
    wxDateTime& MakeUTC( bool = false ) { return *this; }
    wxDateTime& MakeFromUTC( bool = false ) { return *this; }

    wxString Format( const wxString& aFormat = wxT( "%c" ), const TimeZone& aTz = Local ) const;
    wxString FormatDate() const { return Format( wxT( "%x" ) ); }
    wxString FormatTime() const { return Format( wxT( "%X" ) ); }
    wxString FormatISODate() const { return Format( wxT( "%Y-%m-%d" ) ); }
    wxString FormatISOTime() const { return Format( wxT( "%H:%M:%S" ) ); }
    wxString FormatISOCombined( char aSep = 'T' ) const
    {
        return FormatISODate() + wxUniChar( aSep ) + FormatISOTime();
    }

    bool ParseFormat( const wxString& aDate, const wxString& aFormat = wxT( "%c" ),
                      wxString::const_iterator* aEnd = nullptr );
    bool ParseFormat( const wxString& aDate, const wxString& aFormat, const wxDateTime& aDefault,
                      wxString::const_iterator* aEnd = nullptr );
    bool ParseISODate( const wxString& aDate );
    bool ParseISOTime( const wxString& aTime );
    bool ParseISOCombined( const wxString& aDate, char aSep = 'T' );
    bool ParseDateTime( const wxString& aDateTime, wxString::const_iterator* aEnd = nullptr );
    bool ParseDate( const wxString& aDate, wxString::const_iterator* aEnd = nullptr );
    bool ParseTime( const wxString& aTime, wxString::const_iterator* aEnd );

    bool IsEarlierThan( const wxDateTime& o ) const { return m_ms < o.m_ms; }
    bool IsLaterThan( const wxDateTime& o ) const { return m_ms > o.m_ms; }
    bool IsEqualTo( const wxDateTime& o ) const { return m_ms == o.m_ms; }
    bool IsSameDate( const wxDateTime& o ) const;
    bool IsSameTime( const wxDateTime& o ) const;
    bool IsBetween( const wxDateTime& a, const wxDateTime& b ) const { return m_ms >= a.m_ms && m_ms <= b.m_ms; }

    wxDateTime& Add( const wxTimeSpan& aSpan ) { m_ms += aSpan.GetValue().GetValue(); return *this; }
    wxDateTime& Subtract( const wxTimeSpan& aSpan ) { m_ms -= aSpan.GetValue().GetValue(); return *this; }
    wxDateTime& Add( const wxDateSpan& aSpan );
    wxDateTime& Subtract( const wxDateSpan& aSpan );
    wxTimeSpan  Subtract( const wxDateTime& aOther ) const { return wxTimeSpan::Milliseconds( m_ms - aOther.m_ms ); }

    wxDateTime  operator+( const wxTimeSpan& s ) const { return wxDateTime( *this ).Add( s ); }
    wxDateTime  operator-( const wxTimeSpan& s ) const { return wxDateTime( *this ).Subtract( s ); }
    wxDateTime  operator+( const wxDateSpan& s ) const { return wxDateTime( *this ).Add( s ); }
    wxDateTime  operator-( const wxDateSpan& s ) const { return wxDateTime( *this ).Subtract( s ); }
    wxTimeSpan  operator-( const wxDateTime& o ) const { return Subtract( o ); }
    wxDateTime& operator+=( const wxTimeSpan& s ) { return Add( s ); }
    wxDateTime& operator-=( const wxTimeSpan& s ) { return Subtract( s ); }
    wxDateTime& operator+=( const wxDateSpan& s ) { return Add( s ); }
    wxDateTime& operator-=( const wxDateSpan& s ) { return Subtract( s ); }

    bool operator<( const wxDateTime& o ) const { return m_ms < o.m_ms; }
    bool operator>( const wxDateTime& o ) const { return m_ms > o.m_ms; }
    bool operator<=( const wxDateTime& o ) const { return m_ms <= o.m_ms; }
    bool operator>=( const wxDateTime& o ) const { return m_ms >= o.m_ms; }
    bool operator==( const wxDateTime& o ) const { return m_valid == o.m_valid && m_ms == o.m_ms; }
    bool operator!=( const wxDateTime& o ) const { return !( *this == o ); }

    /// Internal: from milliseconds since the epoch.
    static wxDateTime FromMilliseconds( long long aMs )
    {
        wxDateTime dt;
        dt.m_ms = aMs;
        dt.m_valid = true;
        return dt;
    }

private:
    static long long floorDiv( long long a, long long b ) { return a / b - ( ( a % b ) < 0 ); }
    static long long floorMod( long long a, long long b ) { return a - floorDiv( a, b ) * b; }

    struct tm tm( const TimeZone& aTz ) const;

    long long m_ms = 0;
    bool      m_valid = false;
};


extern const wxDateTime wxDefaultDateTime;
#define wxInvalidDateTime wxDefaultDateTime

/// Milliseconds since the epoch.
wxLongLong wxGetLocalTimeMillis();
wxLongLong wxGetUTCTimeMillis();
long       wxGetLocalTime();
long       wxGetUTCTime();

#pragma GCC visibility pop
