/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (src/common/gdicmn.cpp (the geometry classes), wxWindows Library Licence 3.1),
 * Copyright (C) the wxWidgets authors.
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

#include <wx/gdicmn.h>

const wxSize  wxDefaultSize( wxDefaultCoord, wxDefaultCoord );
const wxPoint wxDefaultPosition( wxDefaultCoord, wxDefaultCoord );

wxRect::wxRect(const wxPoint& point1, const wxPoint& point2)
{
    x = point1.x;
    y = point1.y;
    width = point2.x - point1.x;
    height = point2.y - point1.y;

    if (width < 0)
    {
        width = -width;
        x = point2.x;
    }
    width++;

    if (height < 0)
    {
        height = -height;
        y = point2.y;
    }
    height++;
}

wxRect& wxRect::Union(const wxRect& rect)
{
    // ignore empty rectangles: union with an empty rectangle shouldn't extend
    // this one to (0, 0)
    if ( !width || !height )
    {
        *this = rect;
    }
    else if ( rect.width && rect.height )
    {
        int x1 = wxMin(x, rect.x);
        int y1 = wxMin(y, rect.y);
        int y2 = wxMax(y + height, rect.height + rect.y);
        int x2 = wxMax(x + width, rect.width + rect.x);

        x = x1;
        y = y1;
        width = x2 - x1;
        height = y2 - y1;
    }
    //else: we're not empty and rect is empty

    return *this;
}

wxRect& wxRect::Inflate(wxCoord dx, wxCoord dy)
{
     if (-2*dx>width)
     {
         // Don't allow deflate to eat more width than we have,
         // a well-defined rectangle cannot have negative width.
         x+=width/2;
         width=0;
     }
     else
     {
         // The inflate is valid.
         x-=dx;
         width+=2*dx;
     }

     if (-2*dy>height)
     {
         // Don't allow deflate to eat more height than we have,
         // a well-defined rectangle cannot have negative height.
         y+=height/2;
         height=0;
     }
     else
     {
         // The inflate is valid.
         y-=dy;
         height+=2*dy;
     }

    return *this;
}

bool wxRect::Contains(int cx, int cy) const
{
    return ( (cx >= x) && (cy >= y)
          && ((cy - y) < height)
          && ((cx - x) < width)
          );
}

bool wxRect::Contains(const wxRect& rect) const
{
    return Contains(rect.GetTopLeft()) && Contains(rect.GetBottomRight());
}

wxRect& wxRect::Intersect(const wxRect& rect)
{
    int x2 = GetRight(),
        y2 = GetBottom();

    if ( x < rect.x )
        x = rect.x;
    if ( y < rect.y )
        y = rect.y;
    if ( x2 > rect.GetRight() )
        x2 = rect.GetRight();
    if ( y2 > rect.GetBottom() )
        y2 = rect.GetBottom();

    width = x2 - x + 1;
    height = y2 - y + 1;

    if ( width <= 0 || height <= 0 )
    {
        width =
        height = 0;
    }

    return *this;
}

bool wxRect::Intersects(const wxRect& rect) const
{
    wxRect r = Intersect(rect);

    // if there is no intersection, both width and height are 0
    return r.width != 0;
}

wxRect& wxRect::operator+=(const wxRect& rect)
{
    *this = *this + rect;
    return *this;
}


wxRect& wxRect::operator*=(const wxRect& rect)
{
    *this = *this * rect;
    return *this;
}


wxRect operator+(const wxRect& r1, const wxRect& r2)
{
    int x1 = wxMin(r1.x, r2.x);
    int y1 = wxMin(r1.y, r2.y);
    int y2 = wxMax(r1.y+r1.height, r2.height+r2.y);
    int x2 = wxMax(r1.x+r1.width, r2.width+r2.x);
    return wxRect(x1, y1, x2-x1, y2-y1);
}

wxRect operator*(const wxRect& r1, const wxRect& r2)
{
    int x1 = wxMax(r1.x, r2.x);
    int y1 = wxMax(r1.y, r2.y);
    int y2 = wxMin(r1.y+r1.height, r2.height+r2.y);
    int x2 = wxMin(r1.x+r1.width, r2.width+r2.x);
    return wxRect(x1, y1, x2-x1, y2-y1);
}

wxRealPoint::wxRealPoint(const wxPoint& pt)
 : x(pt.x), y(pt.y)
{
}
