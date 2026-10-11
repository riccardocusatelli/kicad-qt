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
#ifndef _WIN32
#pragma GCC visibility push( default )
#endif

#include <wx/defs.h>
#include <wx/buffer.h>
#include <wx/fontenc.h>

#include <memory>
#include <string>

class wxString;

#define wxNO_LEN      ( (size_t) -1 )
#define wxCONV_FAILED ( (size_t) -1 )


/**
 * Conversion between wide strings (UTF-32 on Unix) and multibyte strings.
 *
 * The derived classes implement decode() and encode(); the wxWidgets API (ToWChar(),
 * FromWChar(), cMB2WC()...) is built on them, with the same conventions: a source length of
 * wxNO_LEN means a NUL-terminated string and then the result counts the terminator; a null
 * destination returns the size needed; failures return wxCONV_FAILED.
 */
class wxMBConv
{
public:
    virtual ~wxMBConv() = default;

    virtual size_t ToWChar( wchar_t* aDst, size_t aDstLen, const char* aSrc,
                            size_t aSrcLen = wxNO_LEN ) const;
    virtual size_t FromWChar( char* aDst, size_t aDstLen, const wchar_t* aSrc,
                              size_t aSrcLen = wxNO_LEN ) const;

    wxWCharBuffer cMB2WC( const char* aIn ) const;
    wxWCharBuffer cMB2WC( const char* aIn, size_t aInLen, size_t* aOutLen ) const;
    wxCharBuffer  cWC2MB( const wchar_t* aIn ) const;
    wxCharBuffer  cWC2MB( const wchar_t* aIn, size_t aInLen, size_t* aOutLen ) const;

    wxWCharBuffer cMB2WX( const char* aIn ) const { return cMB2WC( aIn ); }
    wxCharBuffer  cWX2MB( const wchar_t* aIn ) const { return cWC2MB( aIn ); }
    const wchar_t* cWC2WX( const wchar_t* aIn ) const { return aIn; }
    const wchar_t* cWX2WC( const wchar_t* aIn ) const { return aIn; }

    size_t MB2WC( wchar_t* aOut, const char* aIn, size_t aOutLen ) const;
    size_t WC2MB( char* aOut, const wchar_t* aIn, size_t aOutLen ) const;

    /// The number of bytes of the NUL character in this encoding.
    virtual size_t GetMBNulLen() const { return 1; }

    virtual wxMBConv* Clone() const = 0;

    /// Decode @a aLen bytes; @return false on invalid input.
    virtual bool Decode( const char* aSrc, size_t aLen, std::wstring& aOut ) const = 0;

    /// Encode @a aLen characters; @return false if a character cannot be encoded.
    virtual bool Encode( const wchar_t* aSrc, size_t aLen, std::string& aOut ) const = 0;
};


class wxMBConvUTF8 : public wxMBConv
{
public:
    enum
    {
        MAP_INVALID_UTF8_NOT = 0,
        MAP_INVALID_UTF8_TO_PUA = 1,
        MAP_INVALID_UTF8_TO_OCTAL = 2
    };

    explicit wxMBConvUTF8( int aOptions = MAP_INVALID_UTF8_NOT ) : m_options( aOptions ) {}

    wxMBConv* Clone() const override { return new wxMBConvUTF8( m_options ); }

    bool Decode( const char* aSrc, size_t aLen, std::wstring& aOut ) const override;
    bool Encode( const wchar_t* aSrc, size_t aLen, std::string& aOut ) const override;

private:
    int m_options;
};


class wxMBConvStrictUTF8 : public wxMBConvUTF8
{
public:
    wxMBConv* Clone() const override { return new wxMBConvStrictUTF8(); }
};


class wxMBConvUTF16Base : public wxMBConv
{
public:
    size_t GetMBNulLen() const override { return 2; }
};


class wxMBConvUTF16LE : public wxMBConvUTF16Base
{
public:
    wxMBConv* Clone() const override { return new wxMBConvUTF16LE(); }

    bool Decode( const char* aSrc, size_t aLen, std::wstring& aOut ) const override;
    bool Encode( const wchar_t* aSrc, size_t aLen, std::string& aOut ) const override;
};


class wxMBConvUTF16BE : public wxMBConvUTF16Base
{
public:
    wxMBConv* Clone() const override { return new wxMBConvUTF16BE(); }

    bool Decode( const char* aSrc, size_t aLen, std::wstring& aOut ) const override;
    bool Encode( const wchar_t* aSrc, size_t aLen, std::string& aOut ) const override;
};

typedef wxMBConvUTF16LE wxMBConvUTF16;


class wxMBConvUTF32LE : public wxMBConv
{
public:
    size_t    GetMBNulLen() const override { return 4; }
    wxMBConv* Clone() const override { return new wxMBConvUTF32LE(); }

    bool Decode( const char* aSrc, size_t aLen, std::wstring& aOut ) const override;
    bool Encode( const wchar_t* aSrc, size_t aLen, std::string& aOut ) const override;
};


class wxMBConvUTF32BE : public wxMBConv
{
public:
    size_t    GetMBNulLen() const override { return 4; }
    wxMBConv* Clone() const override { return new wxMBConvUTF32BE(); }

    bool Decode( const char* aSrc, size_t aLen, std::wstring& aOut ) const override;
    bool Encode( const wchar_t* aSrc, size_t aLen, std::string& aOut ) const override;
};

typedef wxMBConvUTF32LE wxMBConvUTF32;


/**
 * The conversion of the C library.  KiCad works with UTF-8 everywhere, so this is UTF-8.
 */
class wxMBConvLibc : public wxMBConvUTF8
{
public:
    wxMBConv* Clone() const override { return new wxMBConvLibc(); }
};


/**
 * A conversion to and from a named character set (iconv); ISO-8859-1, UTF-8 and the UTF-16/32
 * encodings are handled directly.
 */
class wxCSConv : public wxMBConv
{
public:
    explicit wxCSConv( const wxString& aCharset );
    explicit wxCSConv( wxFontEncoding aEncoding );
    wxCSConv( const wxCSConv& aOther );
    ~wxCSConv() override;

    wxCSConv& operator=( const wxCSConv& aOther );

    bool IsOk() const;

    wxMBConv* Clone() const override { return new wxCSConv( *this ); }

    size_t GetMBNulLen() const override;

    bool Decode( const char* aSrc, size_t aLen, std::wstring& aOut ) const override;
    bool Encode( const wchar_t* aSrc, size_t aLen, std::string& aOut ) const override;

    const std::string& GetCharset() const { return m_charset; }

private:
    void init();

    std::string               m_charset;
    std::unique_ptr<wxMBConv> m_direct;    ///< for the encodings handled without iconv
    bool                      m_ok = false;
};


/**
 * Detects the encoding of the input: a byte order mark, else UTF-8, else the fallback
 * encoding (ISO-8859-1).  Writes UTF-8.
 */
class wxConvAuto : public wxMBConv
{
public:
    explicit wxConvAuto( wxFontEncoding aFallback = wxFONTENCODING_DEFAULT ) :
            m_fallback( aFallback )
    {
    }

    wxMBConv* Clone() const override { return new wxConvAuto( m_fallback ); }

    bool Decode( const char* aSrc, size_t aLen, std::wstring& aOut ) const override;
    bool Encode( const wchar_t* aSrc, size_t aLen, std::string& aOut ) const override;

private:
    wxFontEncoding m_fallback;
};


// The global conversions are function-local statics (usable during static initialisation).
wxMBConvUTF8& wxGet_wxConvUTF8();
wxMBConv&     wxGet_wxConvLibc();
wxCSConv&     wxGet_wxConvLocal();
wxCSConv&     wxGet_wxConvISO8859_1();
wxMBConv*&    wxGet_wxConvCurrent();
wxMBConv*&    wxGet_wxConvFileName();
wxMBConv*&    wxGet_wxConvUI();

#define wxConvUTF8      wxGet_wxConvUTF8()
#define wxConvLibc      wxGet_wxConvLibc()
#define wxConvLocal     wxGet_wxConvLocal()
#define wxConvISO8859_1 wxGet_wxConvISO8859_1()
#define wxConvCurrent   wxGet_wxConvCurrent()
#define wxConvFileName  wxGet_wxConvFileName()
#define wxConvUI        wxGet_wxConvUI()

#define wxConvFile wxConvLocal
#define wxConvWhateverWorks wxConvUTF8


/// UTF-8 encoding of a wide string (no validation needed: every code point is encodable).
std::string kiwxToUTF8( const wchar_t* aStr, size_t aLen );

/// UTF-8 decoding; @return false on invalid input (aOut is then undefined).
bool kiwxFromUTF8( const char* aStr, size_t aLen, std::wstring& aOut );

#ifndef _WIN32
#pragma GCC visibility pop
#endif
