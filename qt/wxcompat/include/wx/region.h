// The wxWidgets compatibility layer of the Qt build: wxRegion on QRegion.

#ifndef KIWX_REGION_H
#define KIWX_REGION_H

#include <wx/kiwx_gdi.h>

#include <QPainter>
#include <QPolygon>
#include <QRegion>


class wxRegion
{
public:
    wxRegion() = default;
    wxRegion( int aX, int aY, int aWidth, int aHeight ) : m_region( aX, aY, aWidth, aHeight ) {}
    explicit wxRegion( const wxRect& aRect ) :
            m_region( aRect.x, aRect.y, aRect.width, aRect.height )
    {
    }

    wxRegion( size_t aCount, const wxPoint* aPoints )
    {
        QPolygon poly;

        for( size_t i = 0; i < aCount; ++i )
            poly << QPoint( aPoints[i].x, aPoints[i].y );

        m_region = QRegion( poly, Qt::OddEvenFill );
    }

    bool Subtract( const wxRegion& aOther )
    {
        m_region = m_region.subtracted( aOther.m_region );
        return true;
    }

    bool Union( const wxRegion& aOther )
    {
        m_region = m_region.united( aOther.m_region );
        return true;
    }

    bool Intersect( const wxRegion& aOther )
    {
        m_region = m_region.intersected( aOther.m_region );
        return true;
    }

    bool IsEmpty() const { return m_region.isEmpty(); }

    const QRegion& GetQRegion() const { return m_region; }

private:
    QRegion m_region;
};

#endif // KIWX_REGION_H
