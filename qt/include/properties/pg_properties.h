/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef PG_PROPERTIES_H
#define PG_PROPERTIES_H

/**
 * @file pg_properties.h
 * The properties of the property grid of the properties panels with Qt: the rows of the
 * PROPERTY_GRID, with their values (the wxVariant values of the properties of the items) and
 * the way they are shown and edited (see include/properties/pg_properties.h, the properties of
 * wxPropertyGrid).
 */

#include <functional>

#include <wx/string.h>
#include <wx/variant.h>

#include <gal/color4d.h>
#include <origin_transforms.h>
#include <properties/pg_factory.h>
#include <properties/pg_value_text.h>
#include <properties/property_choices.h>

class EDA_DRAW_FRAME;
class PROPERTY_BASE;
class PROPERTY_GRID;


/**
 * A property (a row) of a PROPERTY_GRID: a category, or a value with its editor.
 */
class PG_PROPERTY
{
public:
    /// How the value of the property is edited.
    enum class EDITOR
    {
        NONE,       ///< Not editable (a category)
        TEXT,       ///< A text (StringToValue())
        UNIT,       ///< A text with units (a unit binder)
        RATIO,      ///< A scale-free number
        CHECKBOX,   ///< A boolean, toggled by a click
        CHOICE,     ///< One of the choices of the property
        NET,        ///< A net (the editor given to the grid by the panel)
        COLOR,      ///< A color, chosen with the color picker
        FPID,       ///< A text, with a button opening the footprint chooser
        URL         ///< A text, with a button opening the link or choosing a file
    };

    PG_PROPERTY( EDITOR aEditor );

    virtual ~PG_PROPERTY() = default;

    PG_PROPERTY( const PG_PROPERTY& ) = delete;
    PG_PROPERTY& operator=( const PG_PROPERTY& ) = delete;

    void            SetLabel( const wxString& aLabel ) { m_label = aLabel; }
    const wxString& GetLabel() const { return m_label; }

    void            SetName( const wxString& aName ) { m_name = aName; }
    const wxString& GetName() const { return m_name; }

    void            SetHelpString( const wxString& aHelp ) { m_help = aHelp; }
    const wxString& GetHelpString() const { return m_help; }

    void           SetClientData( PROPERTY_BASE* aProperty ) { m_clientData = aProperty; }
    PROPERTY_BASE* GetClientData() const { return m_clientData; }

    void             SetValue( const wxVariant& aValue ) { m_value = aValue; }
    const wxVariant& GetValue() const { return m_value; }

    /// The selection has different values for the property.
    bool IsValueUnspecified() const { return m_value.IsNull(); }

    void SetReadOnly( bool aReadOnly ) { m_readOnly = aReadOnly; }
    bool IsReadOnly() const { return m_readOnly; }

    EDITOR GetEditor() const { return m_editor; }
    void   SetEditor( EDITOR aEditor ) { m_editor = aEditor; }

    /// The frame of the custom editors of the property (FPID, URL).
    void            SetEditorFrame( EDA_DRAW_FRAME* aFrame ) { m_editorFrame = aFrame; }
    EDA_DRAW_FRAME* GetEditorFrame() const { return m_editorFrame; }

    void                    SetChoices( const PROPERTY_CHOICES& aChoices ) { m_choices = aChoices; }
    const PROPERTY_CHOICES& GetChoices() const { return m_choices; }
    PROPERTY_CHOICES&       Choices() { return m_choices; }

    bool IsCategory() const { return m_editor == EDITOR::NONE; }

    /// The text of a value of the property.
    virtual wxString ValueToString( const wxVariant& aValue ) const;

    /// The text of the value of the property (empty when unspecified).
    wxString GetValueAsString() const;

    /**
     * The value of the property from a text (the TEXT editor).
     *
     * @return false if the text isn't a value of the property.
     */
    virtual bool StringToValue( wxVariant& aVariant, const wxString& aText ) const;

    /// The quantity edited by the UNIT editor, and the scale of the angles.
    virtual PG_UNIT_KIND GetUnitKind() const { return PG_UNIT_KIND::OTHER; }
    virtual double       GetAngleScale() const { return 1.0; }

    /// The coordinate type of the UNIT editor.
    virtual ORIGIN_TRANSFORMS::COORD_TYPES_T GetCoordType() const
    {
        return ORIGIN_TRANSFORMS::NOT_A_COORD;
    }

    /// The color shown with a choice (COLORENUM), or an unspecified color.
    virtual KIGFX::COLOR4D GetChoiceColor( int aValue ) const
    {
        return KIGFX::COLOR4D::UNSPECIFIED;
    }

protected:
    EDITOR           m_editor;
    wxString         m_label;
    wxString         m_name;
    wxString         m_help;
    PROPERTY_BASE*   m_clientData;
    wxVariant        m_value;
    bool             m_readOnly;
    PROPERTY_CHOICES m_choices;
    EDA_DRAW_FRAME*  m_editorFrame;
};


/// A category of properties (the group of the properties).
class PGPROPERTY_CATEGORY : public PG_PROPERTY
{
public:
    PGPROPERTY_CATEGORY( const wxString& aCaption ) :
            PG_PROPERTY( EDITOR::NONE )
    {
        m_label = aCaption;
        m_name = aCaption;
    }
};


/// An integer.
class PGPROPERTY_INT : public PG_PROPERTY
{
public:
    PGPROPERTY_INT() : PG_PROPERTY( EDITOR::TEXT ) {}

    wxString ValueToString( const wxVariant& aValue ) const override;
    bool     StringToValue( wxVariant& aVariant, const wxString& aText ) const override;
};


/// An unsigned integer.
class PGPROPERTY_UINT : public PG_PROPERTY
{
public:
    PGPROPERTY_UINT() : PG_PROPERTY( EDITOR::TEXT ) {}

    wxString ValueToString( const wxVariant& aValue ) const override;
    bool     StringToValue( wxVariant& aVariant, const wxString& aText ) const override;
};


/// A floating point number.
class PGPROPERTY_FLOAT : public PG_PROPERTY
{
public:
    PGPROPERTY_FLOAT() : PG_PROPERTY( EDITOR::TEXT ) {}

    wxString ValueToString( const wxVariant& aValue ) const override;
    bool     StringToValue( wxVariant& aVariant, const wxString& aText ) const override;
};


/// One of the choices of the property (an enumeration).
class PGPROPERTY_ENUM : public PG_PROPERTY
{
public:
    PGPROPERTY_ENUM( const PROPERTY_CHOICES& aChoices ) :
            PG_PROPERTY( EDITOR::CHOICE )
    {
        m_choices = aChoices;
    }

    wxString ValueToString( const wxVariant& aValue ) const override;
};


/// A choice shown with a color swatch (the layers, with their colors).
class PGPROPERTY_COLORENUM : public PGPROPERTY_ENUM
{
public:
    PGPROPERTY_COLORENUM( const PROPERTY_CHOICES& aChoices,
                          std::function<KIGFX::COLOR4D( int aValue )> aColorFunc ) :
            PGPROPERTY_ENUM( aChoices ),
            m_colorFunc( std::move( aColorFunc ) )
    {
    }

    KIGFX::COLOR4D GetChoiceColor( int aValue ) const override
    {
        return m_colorFunc ? m_colorFunc( aValue ) : KIGFX::COLOR4D::UNSPECIFIED;
    }

protected:
    std::function<KIGFX::COLOR4D( int aValue )> m_colorFunc;
};


/// A net (a choice of the nets of the board, edited by the net selector).
class PGPROPERTY_NET : public PGPROPERTY_ENUM
{
public:
    PGPROPERTY_NET( const PROPERTY_CHOICES& aChoices ) :
            PGPROPERTY_ENUM( aChoices )
    {
        m_editor = EDITOR::NET;
    }
};


/// A distance (a length or a coordinate), in the units of the frame.
class PGPROPERTY_DISTANCE : public PG_PROPERTY
{
public:
    PGPROPERTY_DISTANCE( EDA_DRAW_FRAME* aParentFrame,
                         ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType ) :
            PG_PROPERTY( EDITOR::UNIT ),
            m_parentFrame( aParentFrame ),
            m_coordType( aCoordType )
    {
    }

    ORIGIN_TRANSFORMS::COORD_TYPES_T CoordType() const { return m_coordType; }

    wxString ValueToString( const wxVariant& aValue ) const override
    {
        return PGDistanceToString( m_parentFrame, aValue, m_coordType );
    }

    PG_UNIT_KIND                     GetUnitKind() const override { return PG_UNIT_KIND::DISTANCE; }
    ORIGIN_TRANSFORMS::COORD_TYPES_T GetCoordType() const override { return m_coordType; }

protected:
    EDA_DRAW_FRAME*                  m_parentFrame;
    ORIGIN_TRANSFORMS::COORD_TYPES_T m_coordType;
};


/// A size (a distance that isn't a coordinate).
class PGPROPERTY_SIZE : public PGPROPERTY_DISTANCE
{
public:
    PGPROPERTY_SIZE( EDA_DRAW_FRAME* aParentFrame ) :
            PGPROPERTY_DISTANCE( aParentFrame, ORIGIN_TRANSFORMS::NOT_A_COORD )
    {
    }
};


/// A coordinate (with the origin transforms of the frame).
class PGPROPERTY_COORD : public PGPROPERTY_DISTANCE
{
public:
    PGPROPERTY_COORD( EDA_DRAW_FRAME* aParentFrame, ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType ) :
            PGPROPERTY_DISTANCE( aParentFrame, aCoordType )
    {
    }
};


/// An area, in the units of the frame.
class PGPROPERTY_AREA : public PG_PROPERTY
{
public:
    PGPROPERTY_AREA( EDA_DRAW_FRAME* aParentFrame ) :
            PG_PROPERTY( EDITOR::UNIT ),
            m_parentFrame( aParentFrame )
    {
    }

    wxString ValueToString( const wxVariant& aValue ) const override
    {
        return PGAreaToString( m_parentFrame, aValue );
    }

    PG_UNIT_KIND GetUnitKind() const override { return PG_UNIT_KIND::AREA; }

protected:
    EDA_DRAW_FRAME* m_parentFrame;
};


/// A time (a delay), in the units of the frame.
class PGPROPERTY_TIME : public PG_PROPERTY
{
public:
    PGPROPERTY_TIME( EDA_DRAW_FRAME* aParentFrame ) :
            PG_PROPERTY( EDITOR::UNIT ),
            m_parentFrame( aParentFrame )
    {
    }

    wxString ValueToString( const wxVariant& aValue ) const override
    {
        return PGTimeToString( m_parentFrame, aValue );
    }

    PG_UNIT_KIND GetUnitKind() const override { return PG_UNIT_KIND::TIME; }

protected:
    EDA_DRAW_FRAME* m_parentFrame;
};


/// An angle, in degrees (scaled for the angles in tenths of degrees).
class PGPROPERTY_ANGLE : public PG_PROPERTY
{
public:
    PGPROPERTY_ANGLE() :
            PG_PROPERTY( EDITOR::UNIT ),
            m_scale( 1.0 )
    {
    }

    void   SetScale( double aScale ) { m_scale = aScale; }
    double GetScale() const { return m_scale; }

    wxString ValueToString( const wxVariant& aValue ) const override
    {
        return PGAngleToString( aValue, m_scale );
    }

    bool StringToValue( wxVariant& aVariant, const wxString& aText ) const override
    {
        PGAngleFromString( aVariant, aText, m_scale );
        return true;
    }

    PG_UNIT_KIND GetUnitKind() const override { return PG_UNIT_KIND::ANGLE; }
    double       GetAngleScale() const override { return m_scale; }

protected:
    double m_scale;
};


/// A scale-free ratio.
class PGPROPERTY_RATIO : public PG_PROPERTY
{
public:
    PGPROPERTY_RATIO() : PG_PROPERTY( EDITOR::RATIO ) {}

    wxString ValueToString( const wxVariant& aValue ) const override
    {
        return PGRatioToString( aValue );
    }
};


/// A string (shown unescaped).
class PGPROPERTY_STRING : public PG_PROPERTY
{
public:
    PGPROPERTY_STRING() : PG_PROPERTY( EDITOR::TEXT ) {}

    wxString ValueToString( const wxVariant& aValue ) const override
    {
        return PGStringToString( aValue );
    }

    bool StringToValue( wxVariant& aVariant, const wxString& aText ) const override
    {
        aVariant = PGStringFromString( aText );
        return true;
    }
};


/// A boolean (a check box).
class PGPROPERTY_BOOL : public PG_PROPERTY
{
public:
    PGPROPERTY_BOOL() : PG_PROPERTY( EDITOR::CHECKBOX ) {}

    wxString ValueToString( const wxVariant& aValue ) const override;
};


/// A color (a swatch over the background color).
class PGPROPERTY_COLOR4D : public PG_PROPERTY
{
public:
    PGPROPERTY_COLOR4D() :
            PG_PROPERTY( EDITOR::COLOR ),
            m_backgroundColor( KIGFX::COLOR4D::UNSPECIFIED )
    {
    }

    wxString ValueToString( const wxVariant& aValue ) const override
    {
        return PGColorToString( aValue );
    }

    bool StringToValue( wxVariant& aVariant, const wxString& aText ) const override
    {
        aVariant = PGColorFromString( aText );
        return true;
    }

    /// The color of the value (unspecified for a null value).
    KIGFX::COLOR4D GetColor() const;

    void SetBackgroundColor( const KIGFX::COLOR4D& aColor ) { m_backgroundColor = aColor; }
    const KIGFX::COLOR4D& GetBackgroundColor() const { return m_backgroundColor; }

private:
    KIGFX::COLOR4D m_backgroundColor;
};

#endif // PG_PROPERTIES_H
