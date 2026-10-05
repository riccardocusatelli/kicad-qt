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
 * @file kiwx_diff_misc.cpp
 *
 * Differential test of kiwx: XML, regular expressions, file configs, wxAny and wxVariant,
 * base64 and URIs (see run_diff.sh).
 */

#include "kiwx_diff.h"

#include <wx/any.h>
#include <wx/base64.h>
#include <wx/ffile.h>
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/mstream.h>
#include <wx/regex.h>
#include <wx/sstream.h>
#include <wx/uri.h>
#include <wx/variant.h>
#include <wx/xml/xml.h>

#include <cstring>
#include <optional>


static std::string memText( wxMemoryOutputStream& aOut )
{
    std::string s( aOut.GetLength(), '\0' );
    aOut.CopyTo( s.data(), s.size() );
    return s;
}


static void dumpXml( const wxXmlNode* aNode, int aDepth )
{
    for( ; aNode; aNode = aNode->GetNext() )
    {
        wxString attrs;

        for( wxXmlAttribute* a = aNode->GetAttributes(); a; a = a->GetNext() )
            attrs += a->GetName() + wxT( "=" ) + a->GetValue() + wxT( ";" );

        OUT( wxString::Format( wxT( "%d type=%d name=%s content=%s attrs=%s line=%d" ), aDepth,
                               (int) aNode->GetType(), aNode->GetName(), aNode->GetContent(), attrs,
                               aNode->GetLineNumber() ) );

        if( aNode->GetType() == wxXML_ELEMENT_NODE )
            OUT( aNode->GetNodeContent() );

        dumpXml( aNode->GetChildren(), aDepth + 1 );
    }
}


static void testXml()
{
    SECTION( "xml" );

    const char* text = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                       "<!DOCTYPE eagle SYSTEM \"eagle.dtd\">\n"
                       "<!-- a comment -->\n"
                       "<root a=\"1\" b='x &amp; y'>\n"
                       "  <child>text &lt;here&gt; \xc3\xa8</child>\n"
                       "  <empty/>\n"
                       "  <mixed>one<b>two</b>three<![CDATA[ <raw> ]]></mixed>\n"
                       "  <?pi data?>\n"
                       "  <ws>   </ws>\n"
                       "</root>\n";

    for( int flags : { (int) wxXMLDOC_NONE, (int) wxXMLDOC_KEEP_WHITESPACE_NODES } )
    {
        wxMemoryInputStream in( text, strlen( text ) );
        wxXmlDocument       doc;
        OUT( doc.Load( in, wxT( "UTF-8" ), flags ) );
        OUT( doc.IsOk() );
        OUT( doc.GetVersion() );
        OUT( doc.GetFileEncoding() );
        OUT( doc.GetDoctype().GetFullString() );
        dumpXml( doc.GetDocumentNode(), 0 );

        wxXmlNode* root = doc.GetRoot();
        OUT( root->GetAttribute( wxT( "a" ) ) );
        OUT( root->GetAttribute( wxT( "zz" ), wxT( "def" ) ) );
        OUT( root->HasAttribute( wxT( "b" ) ) );
        OUT( root->GetChildren()->GetDepth() );

        for( int indent : { 2, 0, -1 } )
        {
            wxMemoryOutputStream out;
            OUT( doc.Save( out, indent ) );
            OUT( memText( out ) );
        }
    }

    // a built document, as the exporters (IPC-2581) do
    wxXmlDocument doc;
    wxXmlNode*    root = new wxXmlNode( wxXML_ELEMENT_NODE, wxT( "IPC-2581" ) );
    root->AddAttribute( wxT( "revision" ), wxT( "C" ) );
    root->AddAttribute( wxT( "quote" ), wxT( "a\"b<c>&\t\n\r" ) );
    doc.SetRoot( root );
    wxXmlNode* c1 = new wxXmlNode( root, wxXML_ELEMENT_NODE, wxT( "Content" ) );
    new wxXmlNode( c1, wxXML_TEXT_NODE, wxEmptyString, wxT( "x<y & \r\n" ) );
    wxXmlNode* c2 = new wxXmlNode( wxXML_ELEMENT_NODE, wxT( "Second" ) );
    root->AddChild( c2 );
    root->InsertChild( new wxXmlNode( wxXML_ELEMENT_NODE, wxT( "First" ) ), c1 );
    root->InsertChildAfter( new wxXmlNode( wxXML_COMMENT_NODE, wxT( "c" ), wxT( "note" ) ), c2 );
    OUT( root->DeleteAttribute( wxT( "quote" ) ) );
    OUT( root->DeleteAttribute( wxT( "none" ) ) );
    wxXmlNode* removed = new wxXmlNode( wxXML_ELEMENT_NODE, wxT( "Gone" ) );
    root->AddChild( removed );
    OUT( root->RemoveChild( removed ) );
    delete removed;

    wxMemoryOutputStream out;
    OUT( doc.Save( out ) );
    OUT( memText( out ) );

    wxXmlDocument copy( doc );
    wxMemoryOutputStream out2;
    copy.Save( out2, 1 );
    OUT( memText( out2 ) );

    // errors
    {
        wxLogNull           noLog;
        const char*         bad = "<root><unclosed></root>";
        wxMemoryInputStream in( bad, strlen( bad ) );
        wxXmlDocument       badDoc;
        OUT( badDoc.Load( in ) );
        OUT( badDoc.IsOk() );
    }

    // Latin-1 declared encoding
    {
        const char* latin = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><r v=\"\xe8\">\xe0</r>";
        wxMemoryInputStream in( latin, strlen( latin ) );
        wxXmlDocument       ldoc;
        OUT( ldoc.Load( in ) );
        OUT( ldoc.GetFileEncoding() );
        dumpXml( ldoc.GetDocumentNode(), 0 );
        wxMemoryOutputStream lout;
        ldoc.Save( lout );
        OUT( memText( lout ) );
    }
}


static void testRegex()
{
    SECTION( "regex" );

    struct CASE
    {
        const wchar_t* pattern;
        int            flags;
        const wchar_t* text;
    };

    const CASE cases[] = {
        { L"^(\\w+)(<|<=|=|>=|>)([-+]?[\\d.]*)(\\w*)$", wxRE_ADVANCED, L"R<=10k" },
        { L"(\\w+)[=:]([-+]?[\\d.]+)(\\w*)", wxRE_ADVANCED, L"value:4.7uF tol=5%" },
        { L"KICAD[0-9]+_[A-Z0-9_]+(_DIR)?", wxRE_DEFAULT, L"KICAD9_3DMODEL_DIR" },
        { L"(?i)#[ \t]+EESchema[ \t]+Netlist[ \t]+", wxRE_ADVANCED, L"# eeschema  NETLIST  v" },
        { L"(^|[^\\\\])\\$\\{DRC_WARNING\\s*([^}]*)\\}", wxRE_DEFAULT, L"x ${DRC_WARNING  msg here}" },
        { L"\\b(https?|ftp|file)://([-\\w+&@#/%?=~|!:,.;]*[^.,:;<>\\(\\)\\s\u00b6])", wxRE_ICASE,
          L"see HTTP://kicad.org/path?a=1. and more" },
        { L"a|ab", wxRE_DEFAULT, L"xabx" },
        { L"a.c", wxRE_DEFAULT, L"a\nc" },
        { L"a.c", wxRE_NEWLINE, L"a\nc" },
        { L"^b", wxRE_NEWLINE, L"a\nb" },
        { L"^b", wxRE_DEFAULT, L"a\nb" },
        { L"\\(a*\\)b", wxRE_BASIC, L"xaab" },
        { L"\\mword\\M", wxRE_ADVANCED, L"a word here" },
        { L"***=a.b", wxRE_DEFAULT, L"xa.by" },
        { L"(unclosed", wxRE_DEFAULT, L"" },
        { L"[[:alpha:]]+", wxRE_DEFAULT, L"123abc456" },
        { L"(a)(b)?(c)", wxRE_DEFAULT, L"ac" },
        { L"\u00e8+", wxRE_ICASE, L"x\u00c8\u00e8y" },
        { L"x*", wxRE_DEFAULT, L"abc" },
    };

    for( const CASE& c : cases )
    {
        wxLogNull noLog;
        wxRegEx   re( c.pattern, c.flags );
        OUT( wxString( c.pattern ) );
        OUT( re.IsValid() );

        if( !re.IsValid() )
            continue;

        OUT( re.GetMatchCount() );
        bool m = re.Matches( c.text );
        OUT( m );

        if( m )
        {
            for( size_t i = 0; i < re.GetMatchCount(); ++i )
            {
                size_t start = 0, len = 0;
                OUT( re.GetMatch( &start, &len, i ) );
                OUT( (long) start );
                OUT( (long) len );
                OUT( re.GetMatch( c.text, i ) );
            }
        }
    }

    wxString s = wxT( "a\n  b\n\n\n\nc   \nd" );
    wxRegEx( wxT( "\n +" ) ).ReplaceAll( &s, wxT( "\n" ) );
    wxRegEx( wxT( " +\n" ) ).ReplaceAll( &s, wxT( "\n" ) );
    wxRegEx( wxT( "\n{3,}" ) ).ReplaceAll( &s, wxT( "\n\n" ) );
    OUT( s );

    wxString t = wxT( "<b>bold</b> and <i>it</i>" );
    OUT( wxRegEx( wxT( "<[^>]*>" ) ).ReplaceAll( &t, wxEmptyString ) );
    OUT( t );

    wxString u = wxT( "x1 y22 z333" );
    wxRegEx  re( wxT( "([a-z])([0-9]+)" ) );
    OUT( re.Replace( &u, wxT( "[\\2-\\1-&]" ), 2 ) );
    OUT( u );
    wxString v = wxT( "aaa" );
    OUT( wxRegEx( wxT( "^a" ) ).ReplaceAll( &v, wxT( "b" ) ) );
    OUT( v );
    OUT( wxRegEx::QuoteMeta( wxT( "a.b*c(d)[e]{f}\\g^$|?+" ) ) );
    OUT( wxRegEx::ConvertFromBasic( wxT( "^*a\\(b\\)c+d?e|f{2}\\{3\\}$" ) ) );
}


static void testConfig()
{
    SECTION( "config" );

    const char* text = "; comment\n"
                       "top=1\n"
                       "[General]\n"
                       "Name = Some Value  \n"
                       "Quoted=\"  spaced \"\n"
                       "Escaped=a\\tb\\\\c\\\"d\\n\n"
                       "Number=42\n"
                       "Neg=-7\n"
                       "Dbl=3.25\n"
                       "Bool=1\n"
                       "Env=$HOME/x\n"
                       "[General/Sub]\n"
                       "Deep=yes\n"
                       "[Other]\n"
                       "k=v\n"
                       "k=dup\n"
                       "!imm=fixed\n";

    wxStringInputStream in( wxString::FromUTF8( text ) );
    wxFileConfig        cfg( in );
    cfg.SetExpandEnvVars( false );

    wxString str;
    long     l = 0;
    double   d = 0;
    bool     b = false;
    int      i = 0;

    OUT( cfg.Read( wxT( "top" ), &str ) );
    OUT( str );
    OUT( cfg.Read( wxT( "/General/Name" ), &str ) );
    OUT( str );
    OUT( cfg.Read( wxT( "General/Quoted" ), &str ) );
    OUT( str );
    OUT( cfg.Read( wxT( "General/Escaped" ), &str ) );
    OUT( str );
    OUT( cfg.Read( wxT( "General/Number" ), &l ) );
    OUT( l );
    OUT( cfg.Read( wxT( "General/Neg" ), &i ) );
    OUT( i );
    OUT( cfg.Read( wxT( "General/Dbl" ), &d ) );
    OUT( d );
    OUT( cfg.Read( wxT( "General/Bool" ), &b ) );
    OUT( b );
    OUT( cfg.Read( wxT( "General/Name" ), &l ) );
    OUT( cfg.Read( wxT( "General/Missing" ), &l, 99L ) );
    OUT( l );
    OUT( cfg.Read( wxT( "General/Env" ), wxT( "def" ) ) );
    OUT( cfg.ReadDouble( wxT( "General/Missing" ), 1.5 ) );
    OUT( cfg.ReadBool( wxT( "General/Bool" ), false ) );
    OUT( cfg.Read( wxT( "Other/k" ) ) );
    OUT( cfg.Read( wxT( "Other/imm" ) ) );
    OUT( cfg.HasGroup( wxT( "General" ) ) );
    OUT( cfg.HasGroup( wxT( "General/Sub" ) ) );
    OUT( cfg.HasEntry( wxT( "General/Sub/Deep" ) ) );
    OUT( cfg.Exists( wxT( "Nope" ) ) );
    OUT( (long) cfg.GetNumberOfEntries( true ) );
    OUT( (long) cfg.GetNumberOfGroups( true ) );

    cfg.SetPath( wxT( "/General" ) );
    OUT( cfg.GetPath() );

    wxString name;
    long     idx;

    for( bool ok = cfg.GetFirstEntry( name, idx ); ok; ok = cfg.GetNextEntry( name, idx ) )
        OUT( name );

    for( bool ok = cfg.GetFirstGroup( name, idx ); ok; ok = cfg.GetNextGroup( name, idx ) )
        OUT( name );

    cfg.SetPath( wxT( "Sub/../Sub" ) );
    OUT( cfg.GetPath() );
    cfg.SetPath( wxT( "/" ) );

    OUT( cfg.Write( wxT( "New/Key" ), wxT( "with space and \"quote\"\t" ) ) );
    OUT( cfg.Write( wxT( "New/Long" ), 123L ) );
    OUT( cfg.Write( wxT( "New/Dbl" ), 0.1 ) );
    OUT( cfg.Write( wxT( "New/Bool" ), true ) );
    OUT( cfg.Write( wxT( "General/Number" ), 43 ) );
    OUT( cfg.RenameEntry( wxT( "top" ), wxT( "top2" ) ) );
    OUT( cfg.RenameGroup( wxT( "Other" ), wxT( "Renamed" ) ) );
    OUT( cfg.DeleteEntry( wxT( "General/Dbl" ) ) );
    OUT( cfg.DeleteGroup( wxT( "General/Sub" ) ) );

    wxMemoryOutputStream out;
    OUT( cfg.Save( out ) );
    OUT( memText( out ) );

    // a file config on disk, as advanced_config.cpp reads it
    wxFFile f( wxT( "kiwx_cfg_test.ini" ), wxT( "w" ) );
    f.Write( wxT( "DrawArcAccuracy=10\nSmallDrillMarkSize=0.35\n" ) );
    f.Close();

    {
        wxFileConfig fc( wxEmptyString, wxEmptyString, wxT( "kiwx_cfg_test.ini" ), wxEmptyString,
                         wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_RELATIVE_PATH );
        OUT( fc.Read( wxT( "DrawArcAccuracy" ), 0L ) );
        OUT( fc.ReadDouble( wxT( "SmallDrillMarkSize" ), 0 ) );
        fc.Write( wxT( "Added" ), wxT( "z" ) );
        OUT( fc.Flush() );
    }

    wxFFile rf( wxT( "kiwx_cfg_test.ini" ) );
    wxString content;
    rf.ReadAll( &content );
    OUT( content );
    rf.Close();
    wxRemoveFile( wxT( "kiwx_cfg_test.ini" ) );

    OUT( wxFileConfig::GetLocalFileName( wxT( "kicad" ) ).AfterLast( '/' ) );
}


enum class COLOR_ENUM
{
    RED,
    GREEN
};


static void testAny()
{
    SECTION( "any" );

    wxAny a( 5 );
    OUT( a.CheckType<int>() );
    OUT( a.CheckType<long>() );
    OUT( a.CheckType<short>() );
    OUT( a.CheckType<unsigned>() );
    OUT( a.CheckType<double>() );
    OUT( a.As<long>() );

    wxString s;
    OUT( a.GetAs( &s ) );
    OUT( s );
    double d = 0;
    OUT( a.GetAs( &d ) );
    OUT( d );
    bool b = false;
    OUT( a.GetAs( &b ) );
    OUT( b );

    wxAny f( 2.5 );
    long  l = 0;
    OUT( f.GetAs( &l ) );
    OUT( l );
    OUT( f.GetAs( &s ) );
    OUT( s );

    wxAny str( wxString( wxT( "12" ) ) );
    OUT( str.GetAs( &l ) );
    OUT( l );
    OUT( str.GetAs( &d ) );
    OUT( d );
    OUT( str.CheckType<wxString>() );
    wxAny bad( wxString( wxT( "x12" ) ) );
    OUT( bad.GetAs( &l ) );

    wxAny boolean( true );
    OUT( boolean.GetAs( &s ) );
    OUT( s );
    OUT( boolean.GetAs( &l ) );
    OUT( l );

    wxAny big( (wxULongLong_t) 1 << 40 );
    int   small = 0;
    OUT( big.GetAs( &small ) );
    unsigned long long ull = 0;
    OUT( big.GetAs( &ull ) );
    OUT( ull );
    wxAny neg( -3 );
    unsigned u = 0;
    OUT( neg.GetAs( &u ) );

    wxAny opt( std::optional<int>( 4 ) );
    OUT( opt.CheckType<std::optional<int>>() );
    OUT( opt.CheckType<int>() );
    OUT( opt.As<std::optional<int>>().value() );

    wxAny e( COLOR_ENUM::GREEN );
    OUT( e.CheckType<COLOR_ENUM>() );
    OUT( e.CheckType<int>() );

    wxAny null;
    OUT( null.IsNull() );
    OUT( a == 5 );
    OUT( a == 6L );
    OUT( str == wxT( "12" ) );

    wxAny copy = str;
    OUT( copy.As<wxString>() );

    wxVariant v;
    OUT( a.GetAs( &v ) );
    OUT( v.GetType() );
    OUT( v.GetLong() );
    OUT( str.GetAs( &v ) );
    OUT( v.GetType() );
    OUT( v.GetString() );
}


static void testVariant()
{
    SECTION( "variant" );

    wxVariant l( 10L );
    wxVariant d( 2.5 );
    wxVariant s( wxT( "text" ) );
    wxVariant b( true );
    wxVariant n;

    for( const wxVariant* v : { &l, &d, &s, &b, &n } )
    {
        OUT( v->GetType() );
        OUT( v->IsNull() );
        OUT( v->MakeString() );
    }

    long   lv = 0;
    double dv = 0;
    bool   bv = false;
    OUT( d.Convert( &lv ) );
    OUT( lv );
    OUT( s.Convert( &dv ) );
    OUT( wxVariant( wxT( "3.5" ) ).Convert( &dv ) );
    OUT( dv );
    OUT( l.Convert( &bv ) );
    OUT( bv );
    OUT( l == 10L );
    OUT( s == wxT( "text" ) );
    OUT( l == d );
    OUT( l.GetDouble() );

    wxAny any = l;
    OUT( any.CheckType<long>() );
    OUT( any.As<long>() );
    wxAny anyS = s;
    OUT( anyS.As<wxString>() );

    wxVariant copy = s;
    copy = wxT( "changed" );
    OUT( s.GetString() );
    OUT( copy.GetString() );

    wxArrayString arr;
    arr.Add( wxT( "a" ) );
    arr.Add( wxT( "b" ) );
    wxVariant av( arr );
    OUT( av.GetType() );
    OUT( av.MakeString() );
    OUT( (long) av.GetCount() );
}


static void testBase64()
{
    SECTION( "base64" );

    for( const char* text : { "", "a", "ab", "abc", "abcd", "hello world\n", "\xff\xfe\x00\x01" } )
    {
        size_t   len = std::strlen( text );
        wxString enc = wxBase64Encode( text, len );
        OUT( enc );
        wxMemoryBuffer dec = wxBase64Decode( enc );
        OUT( std::string( static_cast<char*>( dec.GetData() ), dec.GetDataLen() ) );
    }

    size_t posErr = 0;
    OUT( wxBase64Decode( "YWJj ZA==", wxNO_LEN, wxBase64DecodeMode_Strict, &posErr ).GetDataLen() );
    OUT( (long) posErr );
    OUT( wxBase64Decode( "YWJj ZA==", wxNO_LEN, wxBase64DecodeMode_SkipWS ).GetDataLen() );
    OUT( wxBase64Decode( "Y*WJj!", wxNO_LEN, wxBase64DecodeMode_Relaxed ).GetDataLen() );
    OUT( wxBase64Decode( "YW=j" ).GetDataLen() );
    char buf[4];
    OUT( (long) wxBase64Decode( buf, 2, "YWJj" ) );
    OUT( (long) wxBase64EncodedSize( 10 ) );
    OUT( (long) wxBase64DecodedSize( 16 ) );
}


static void testUri()
{
    SECTION( "uri" );

    for( const wchar_t* text : { L"https://user:pw@kicad.org:8080/a/b/../c/./d?q=1&r=%20x#frag",
                                 L"file:///home/user/doc%20s/x.pdf", L"http://[::1]:80/",
                                 L"http://192.168.1.1/x", L"mailto:someone@example.com",
                                 L"relative/path?x", L"//host/share", L"http://h/\u00e8 space",
                                 L"${KIPRJMOD}/datasheet.pdf", L"C:\\dir\\file.pdf", L"" } )
    {
        wxURI uri;
        OUT( wxString( text ) );
        OUT( uri.Create( text ) );
        OUT( wxString::Format( wxT( "scheme=%d:%s user=%d:%s server=%d:%s port=%d:%s path=%d:%s "
                                    "query=%d:%s frag=%d:%s host=%d" ),
                               uri.HasScheme(), uri.GetScheme(), uri.HasUserInfo(),
                               uri.GetUserInfo(), uri.HasServer(), uri.GetServer(), uri.HasPort(),
                               uri.GetPort(), uri.HasPath(), uri.GetPath(), uri.HasQuery(),
                               uri.GetQuery(), uri.HasFragment(), uri.GetFragment(),
                               (int) uri.GetHostType() ) );
        OUT( uri.BuildURI() );
        OUT( uri.BuildUnescapedURI() );
        OUT( uri.GetUser() );
        OUT( uri.GetPassword() );
        OUT( uri.IsReference() );
    }

    OUT( wxURI::Unescape( wxT( "a%20b%zz%c3%a8%" ) ) );

    wxURI base( wxT( "http://a/b/c/d;p?q" ) );
    wxURI rel( wxT( "../g?y#s" ) );
    rel.Resolve( base );
    OUT( rel.BuildURI() );
}


int main()
{
    testXml();
    testRegex();
    testConfig();
    testAny();
    testVariant();
    testBase64();
    testUri();
    return 0;
}
