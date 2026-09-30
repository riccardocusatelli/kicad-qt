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

#ifndef GEOMETRY_TUNING_H
#define GEOMETRY_TUNING_H

#include <core/kicore_rt.h>

/**
 * Advanced tuning values used by the geometry code in kimath.
 *
 * kimath used to read them from ADVANCED_CFG, which lives in kicommon (a layer above) and
 * pulled wxConfig into the math library.  Now the application installs a provider (see
 * advanced_config.cpp), queried lazily exactly like ADVANCED_CFG::GetCfg() was.  Without a
 * provider the ADVANCED_CFG defaults apply.  The provider is process-wide (kicore_rt).
 */
struct GEOMETRY_TUNING
{
    int  triangulateSimplificationLevel = 50;
    int  triangulateMinimumArea = 1000;
    bool enableCacheFriendlyFracture = true;
};

using GEOMETRY_TUNING_PROVIDER = const GEOMETRY_TUNING& ( * )();

KICORE_RT_API void SetGeometryTuningProvider( GEOMETRY_TUNING_PROVIDER aProvider );

KICORE_RT_API const GEOMETRY_TUNING& GeometryTuning();

#endif // GEOMETRY_TUNING_H
