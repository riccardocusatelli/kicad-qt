// The wxWidgets compatibility layer of the Qt build: <wx/colordlg.h>.
//
// wxColourDialog: the colour dialog of Qt (QColorDialog) with the colour of a wxColourData.
#pragma once

#include <QColorDialog>

#include <wx/kiwx_window.h>
#include <wx/kiwx_gdi.h>


class wxColourData
{
public:
    void            SetColour( const wxColour& aColour ) { m_colour = aColour; }
    const wxColour& GetColour() const { return m_colour; }

    void SetChooseFull( bool aFull ) { m_chooseFull = aFull; }
    bool GetChooseFull() const { return m_chooseFull; }

    void SetChooseAlpha( bool aAlpha ) { m_chooseAlpha = aAlpha; }
    bool GetChooseAlpha() const { return m_chooseAlpha; }

private:
    wxColour m_colour = wxColour( 0, 0, 0 );
    bool     m_chooseFull = true;
    bool     m_chooseAlpha = false;
};


class wxColourDialog
{
public:
    wxColourDialog( wxWidgetArgCompat aParent, const wxColourData* aData = nullptr ) :
            m_parent( aParent.get() )
    {
        if( aData )
            m_data = *aData;
    }

    int ShowModal()
    {
        const wxColour& colour = m_data.GetColour();
        QColor          initial( colour.Red(), colour.Green(), colour.Blue(), colour.Alpha() );

        QColorDialog::ColorDialogOptions options;

        if( m_data.GetChooseAlpha() )
            options |= QColorDialog::ShowAlphaChannel;

        QColor chosen = QColorDialog::getColor( initial, m_parent, QString(), options );

        if( !chosen.isValid() )
            return wxID_CANCEL;

        m_data.SetColour( wxColour( chosen.red(), chosen.green(), chosen.blue(), chosen.alpha() ) );
        return wxID_OK;
    }

    wxColourData& GetColourData() { return m_data; }

private:
    QWidget*     m_parent = nullptr;
    wxColourData m_data;
};
