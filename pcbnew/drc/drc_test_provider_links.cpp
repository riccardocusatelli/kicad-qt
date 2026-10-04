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
 * @file drc_test_provider_links.cpp
 *
 * The list of the DRC test providers of pcbnew (see LinkDrcTestProviders()).
 */

#include <drc/drc_test_provider.h>


#define DRC_TEST_PROVIDERS( X ) \
    X( annular_width ) \
    X( connection_width ) \
    X( connectivity ) \
    X( copper_clearance ) \
    X( courtyard_clearance ) \
    X( creepage ) \
    X( diff_pair_coupling ) \
    X( disallow ) \
    X( edge_clearance ) \
    X( footprint_checks ) \
    X( hole_size ) \
    X( hole_to_hole ) \
    X( library_parity ) \
    X( matched_length ) \
    X( misc ) \
    X( physical_clearance ) \
    X( schematic_parity ) \
    X( silk_clearance ) \
    X( sliver_checker ) \
    X( solder_mask ) \
    X( text_dims ) \
    X( text_mirroring ) \
    X( track_angle ) \
    X( track_segment_length ) \
    X( track_width ) \
    X( via_diameter ) \
    X( zone_connections )

#define DECLARE_LINK_ANCHOR( name ) void drcTestProviderLinkAnchor_##name();
DRC_TEST_PROVIDERS( DECLARE_LINK_ANCHOR )


void LinkDrcTestProviders()
{
#define CALL_LINK_ANCHOR( name ) drcTestProviderLinkAnchor_##name();
    DRC_TEST_PROVIDERS( CALL_LINK_ANCHOR )
}
