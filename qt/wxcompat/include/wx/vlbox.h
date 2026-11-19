// The wxWidgets compatibility layer of the Qt build: <wx/vlbox.h>.
//
// wxVListBox: a list box whose items are drawn by the class (OnDrawItem, OnDrawBackground) with
// the heights that it measures (OnMeasureItem). The items are painted on a panel in the paint
// events of the layer, scrolled with the wheel when they don't fit.
#pragma once

#include <algorithm>

#include <wx/kiwx_gui.h>
#include <wx/kiwx_gdi.h>

class wxVListBox : public wxPanel
{
public:
    wxVListBox( wxWidgetArgCompat aParent, int aId = wxID_ANY,
                const wxPoint& aPos = wxDefaultPosition, const wxSize& aSize = wxDefaultSize,
                long aStyle = 0, const wxString& aName = wxEmptyString ) :
            wxPanel( aParent, aId, aPos, aSize, aStyle, aName )
    {
        Bind( wxEVT_PAINT, [this]( wxPaintEvent& ) { paint(); } );

        Bind( wxEVT_MOUSEWHEEL,
              [this]( wxMouseEvent& aEvent )
              {
                  int step = aEvent.GetWheelRotation() > 0 ? -1 : 1;
                  ScrollToRow( std::clamp<int>( (int) m_first + step, 0,
                                                std::max<int>( 0, (int) m_count - 1 ) ) );
              } );

        Bind( wxEVT_LEFT_DCLICK,
              [this]( wxMouseEvent& aEvent )
              {
                  wxCommandEvent event( wxEVT_LISTBOX_DCLICK, GetId() );
                  event.SetEventObject( this );
                  event.SetInt( m_selection );
                  ProcessWindowEvent( event );
                  aEvent.Skip();
              } );
    }

    void SetItemCount( size_t aCount )
    {
        m_count = aCount;
        m_first = 0;

        if( m_selection >= (int) m_count )
            m_selection = -1;

        Refresh();
    }

    size_t GetItemCount() const { return m_count; }

    void RefreshAll() { Refresh(); }
    void RefreshRow( size_t ) { Refresh(); }

    int  GetSelection() const { return m_selection; }
    bool IsSelected( size_t aItem ) const { return (int) aItem == m_selection; }
    bool IsCurrent( size_t aItem ) const { return IsSelected( aItem ); }

    void SetSelection( int aSelection )
    {
        m_selection = ( aSelection >= 0 && aSelection < (int) m_count ) ? aSelection : -1;

        if( m_selection >= 0 )
            ensureVisible( m_selection );

        Refresh();
    }

    bool ScrollToRow( size_t aRow )
    {
        m_first = std::min( aRow, m_count ? m_count - 1 : 0 );
        Refresh();
        return true;
    }

    size_t GetVisibleRowsBegin() const { return m_first; }

    /// The rectangle of an item in the client area (empty if it is scrolled out).
    wxRect GetItemRect( size_t aItem ) const
    {
        if( aItem < m_first || aItem >= m_count )
            return wxRect();

        int y = 0;

        for( size_t ii = m_first; ii < aItem; ++ii )
            y += OnMeasureItem( ii );

        return wxRect( 0, y, GetClientSize().x, OnMeasureItem( aItem ) );
    }

    int VirtualHitTest( wxCoord aY ) const
    {
        int y = 0;

        for( size_t ii = m_first; ii < m_count; ++ii )
        {
            int h = OnMeasureItem( ii );

            if( aY >= y && aY < y + h )
                return (int) ii;

            y += h;
        }

        return wxNOT_FOUND;
    }

protected:
    virtual void    OnDrawItem( wxDC& aDC, const wxRect& aRect, size_t aItem ) const = 0;
    virtual wxCoord OnMeasureItem( size_t aItem ) const = 0;

    virtual void OnDrawBackground( wxDC& aDC, const wxRect& aRect, size_t aItem ) const
    {
        if( IsSelected( aItem ) )
        {
            wxColour background = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT );
            aDC.SetBrush( wxBrush( background ) );
            aDC.SetPen( wxPen( background ) );
            aDC.DrawRectangle( aRect );
        }
    }

private:
    void paint()
    {
        wxPaintDC dc( this );
        wxSize    client = GetClientSize();
        int       y = 0;

        for( size_t ii = m_first; ii < m_count && y < client.y; ++ii )
        {
            wxRect rect( 0, y, client.x, OnMeasureItem( ii ) );

            OnDrawBackground( dc, rect, ii );
            OnDrawItem( dc, rect, ii );
            y += rect.height;
        }
    }

    void ensureVisible( size_t aItem )
    {
        if( aItem < m_first )
        {
            m_first = aItem;
            return;
        }

        // Scroll down until the item fits under the first visible one
        int height = GetClientSize().y;

        while( m_first < aItem )
        {
            int y = 0;

            for( size_t ii = m_first; ii <= aItem; ++ii )
                y += OnMeasureItem( ii );

            if( y <= height )
                break;

            ++m_first;
        }
    }

    size_t m_count = 0;
    size_t m_first = 0;
    int    m_selection = -1;
};
