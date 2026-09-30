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
 * Exercises core_base, kimath and sexpr in a process that has never seen wxWidgets.
 * Exit code 0 only if every check passes.
 */

#include <core/ki_assert.h>
#include <core/ki_log.h>
#include <core/utf8.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_line_chain.h>
#include <sexpr/sexpr.h>
#include <sexpr/sexpr_parser.h>

#include <cmath>
#include <cstdio>
#include <functional>
#include <map>
#include <string>


static int s_failures = 0;
static int s_asserts = 0;

#define CHECK( cond, ... )                                                  \
    do                                                                      \
    {                                                                       \
        bool ok_ = ( cond );                                                \
        std::printf( "  [%s] ", ok_ ? " ok " : "FAIL" );                    \
        std::printf( __VA_ARGS__ );                                         \
        std::printf( "\n" );                                                \
        s_failures += ok_ ? 0 : 1;                                          \
    } while( 0 )


static void countingAssertHandler( const char*, int, const char*, const char* aCond,
                                   std::string_view aMsg )
{
    s_asserts++;
    std::printf( "  (assert captured: %s — %.*s)\n", aCond, int( aMsg.size() ), aMsg.data() );
}


static void testUtf8()
{
    std::printf( "UTF8 (core_base)\n" );

    UTF8 s( L"Resistenza 10kΩ ±1% – €" );
    CHECK( std::string( s ) == "Resistenza 10kΩ ±1% – €", "wchar_t -> UTF-8: \"%s\"", s.c_str() );

    int count = 0;

    for( UTF8::uni_iter it = s.ubegin(); it < s.uend(); ++it )
        count++;

    CHECK( count == 23, "code points iterated: %d", count );

    UTF8 t;
    t += 0x1F50Cu;  // outside the BMP: 4 bytes
    CHECK( t.size() == 4, "U+1F50C encoded in %zu bytes", t.size() );
}


static void testAssertAndLog()
{
    std::printf( "Assertions and logging (core_base)\n" );

    KI_ASSERT_HANDLER previous = KiSetAssertHandler( &countingAssertHandler );

    auto checked = []( int v ) -> int
    {
        KI_CHECK_MSG( v > 0, -1, "value must be positive" );
        return v * 2;
    };

    CHECK( checked( 4 ) == 8 && s_asserts == 0, "KI_CHECK_MSG passes through" );
    CHECK( checked( -3 ) == -1 && s_asserts == 1, "KI_CHECK_MSG returns fallback and reports" );

    KiSetAssertHandler( previous );

    std::string msg = KiStrFormat( "%s=%d %.2f", "n", 42, 3.14159 );
    CHECK( msg == "n=42 3.14", "KiStrFormat: \"%s\"", msg.c_str() );
}


static void testGeometry()
{
    std::printf( "Geometry (kimath)\n" );

    // 20 x 10 mm board outline (nm) with a 4 mm square hole cut out
    SHAPE_POLY_SET board;
    board.NewOutline();
    board.Append( 0, 0 );
    board.Append( 20000000, 0 );
    board.Append( 20000000, 10000000 );
    board.Append( 0, 10000000 );

    SHAPE_POLY_SET hole;
    hole.NewOutline();
    hole.Append( 8000000, 3000000 );
    hole.Append( 12000000, 3000000 );
    hole.Append( 12000000, 7000000 );
    hole.Append( 8000000, 7000000 );

    board.BooleanSubtract( hole );
    double area_mm2 = board.Area() / 1e12;
    CHECK( std::fabs( area_mm2 - 184.0 ) < 1e-6, "board - hole area: %.3f mm² (expected 184)",
           area_mm2 );

    board.Fracture();
    board.CacheTriangulation();
    size_t triangles = 0;

    for( unsigned i = 0; i < board.TriangulatedPolyCount(); i++ )
        triangles += board.TriangulatedPolygon( i )->GetTriangleCount();

    CHECK( board.IsTriangulationUpToDate() && triangles >= 8,
           "triangulated: %u partition(s), %zu triangles", board.TriangulatedPolyCount(), triangles );

    SHAPE_POLY_SET grown = hole;
    grown.Inflate( 500000, CORNER_STRATEGY::ROUND_ALL_CORNERS, 5000 );
    double g = grown.Area() / 1e12;
    double expected = 16.0 + 4 * 4 * 0.5 + M_PI * 0.25;
    CHECK( std::fabs( g - expected ) < 0.01, "inflate 0.5 mm, round corners: %.3f mm² (expected %.3f)",
           g, expected );
}


static void testSexpr( const char* aFile )
{
    std::printf( "S-expressions (sexpr)\n" );

    SEXPR::PARSER parser;
    std::unique_ptr<SEXPR::SEXPR> root;

    try
    {
        root = parser.ParseFromFile( aFile );
    }
    catch( const std::exception& e )
    {
        CHECK( false, "parse %s: %s", aFile, e.what() );
        return;
    }

    CHECK( root && root->IsList(), "parsed %s", aFile );

    std::map<std::string, int> counts;
    std::map<std::string, std::string> refs;   // uuid-less: reference -> lib_id

    std::function<void( const SEXPR::SEXPR*, int )> walk = [&]( const SEXPR::SEXPR* aNode, int aDepth )
    {
        if( !aNode->IsList() || aNode->GetNumberOfChildren() == 0 )
            return;

        const SEXPR::SEXPR* head = aNode->GetChild( 0 );

        if( head->IsSymbol() && aDepth == 1 )
            counts[head->GetSymbol()]++;

        if( aDepth == 1 && head->IsSymbol() && head->GetSymbol() == "symbol" )
        {
            std::string libId, ref;

            for( size_t i = 1; i < aNode->GetNumberOfChildren(); i++ )
            {
                const SEXPR::SEXPR* c = aNode->GetChild( i );

                if( !c->IsList() || c->GetNumberOfChildren() < 2 || !c->GetChild( 0 )->IsSymbol() )
                    continue;

                if( c->GetChild( 0 )->GetSymbol() == "lib_id" )
                    libId = c->GetChild( 1 )->GetString();
                else if( c->GetChild( 0 )->GetSymbol() == "property" && c->GetNumberOfChildren() > 2
                         && c->GetChild( 1 )->GetString() == "Reference" )
                    ref = c->GetChild( 2 )->GetString();
            }

            refs[ref] = libId;
        }

        for( size_t i = 0; i < aNode->GetNumberOfChildren(); i++ )
            walk( aNode->GetChild( i ), aDepth + 1 );
    };

    walk( root.get(), 0 );

    const char* head = root->GetChild( 0 )->IsSymbol() ? root->GetChild( 0 )->GetSymbol().c_str() : "?";
    CHECK( std::string( head ) == "kicad_sch", "root token: %s", head );
    CHECK( counts["symbol"] > 0, "%d symbols, %d wires, %d labels, %d junctions",
           counts["symbol"], counts["wire"], counts["label"], counts["junction"] );

    int shown = 0;

    for( const auto& [ref, lib] : refs )
    {
        if( ref.empty() || ref[0] == '#' )
            continue;

        if( shown++ == 6 )
            break;

        std::printf( "         %-6s %s\n", ref.c_str(), lib.c_str() );
    }

    // Round trip: the tree printed back must parse to the same structure
    std::string text = root->AsString();
    std::unique_ptr<SEXPR::SEXPR> again = parser.Parse( text );
    CHECK( again && again->AsString() == text, "AsString() round trip (%zu bytes)", text.size() );
}


int main( int argc, char** argv )
{
    std::printf( "KiCad core libraries without wxWidgets\n\n" );

    testUtf8();
    testAssertAndLog();
    testGeometry();

    if( argc > 1 )
        testSexpr( argv[1] );

    std::printf( "\n%s (%d failure%s)\n", s_failures ? "FAILED" : "PASSED", s_failures,
                 s_failures == 1 ? "" : "s" );
    return s_failures ? 1 : 0;
}
