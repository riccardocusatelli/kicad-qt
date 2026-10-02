/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef RC_ITEM_H
#define RC_ITEM_H

#include <deque>
#include <memory>
#include <vector>

#include <units_provider.h>
#include <kiid.h>
#include <reporter.h>
#include <math/vector2d.h>
#include <wx/intl.h>

class MARKER_BASE;
class EDA_BASE_FRAME;
class RC_ITEM;
class EDA_ITEM;
class EDA_DRAW_FRAME;

namespace RC_JSON
{
struct VIOLATION;
}

/**
 * Provide an abstract interface of a RC_ITEM* list manager.
 *
 * The details of the actual list architecture are hidden from the caller.  Any class that
 * implements this interface can then be used by a RC_TREE_MODEL class without it knowing
 * the actual architecture of the list.
 */
class RC_ITEMS_PROVIDER
{
public:
    virtual void SetSeverities( int aSeverities ) = 0;

    virtual int GetSeverities() const = 0;

    virtual int GetCount( int aSeverity = -1 ) const = 0;

    /**
     * Retrieve a RC_ITEM by index.
     */
    virtual std::shared_ptr<RC_ITEM> GetItem( int aIndex ) const = 0;

    /**
     * Remove (and optionally deletes) the indexed item from the list.
     * @param aDeep If true, the source item should be deleted as well as its entry in the list.
     */
    virtual void DeleteItem( int aIndex, bool aDeep ) = 0;

    virtual ~RC_ITEMS_PROVIDER() { }
};


/**
 * A holder for a rule check item, DRC in Pcbnew or ERC in Eeschema.
 *
 * RC_ITEMs can have zero, one, or two related EDA_ITEMs.
 */
class RC_ITEM
{
public:
    typedef std::vector<KIID> KIIDS;

    RC_ITEM() :
        m_errorCode( 0 ),
        m_parent( nullptr )
    {
    }

    RC_ITEM( const std::shared_ptr<RC_ITEM>& aItem )
    {
        m_errorCode    = aItem->m_errorCode;
        m_errorMessage = aItem->m_errorMessage;
        m_errorTitle   = aItem->m_errorTitle;
        m_settingsKey  = aItem->m_settingsKey;
        m_parent       = aItem->m_parent;
        m_ids          = aItem->m_ids;
    }

    virtual ~RC_ITEM() { }

    void SetErrorMessage( const wxString& aMessage ) { m_errorMessage = aMessage; }

    void SetErrorDetail( const wxString& aMsg ) { SetErrorMessage( GetErrorText( true ) + wxS( " " ) + aMsg ); }

    void SetItems( const KIIDS& aIds ) { m_ids = aIds; }

    void AddItem( EDA_ITEM* aItem );

    void SetItems( const EDA_ITEM* aItem, const EDA_ITEM* bItem = nullptr,
                   const EDA_ITEM* cItem = nullptr, const EDA_ITEM* dItem = nullptr );

    void SetItems( const KIID& aItem, const KIID& bItem = niluuid, const KIID& cItem = niluuid,
                   const KIID& dItem = niluuid )
    {
        m_ids.clear();

        m_ids.push_back( aItem );
        m_ids.push_back( bItem );
        m_ids.push_back( cItem );
        m_ids.push_back( dItem );
    }

    virtual KIID GetMainItemID() const { return m_ids.size() > 0 ? m_ids[0] : niluuid; }
    virtual KIID GetAuxItemID() const { return m_ids.size() > 1 ? m_ids[1] : niluuid; }
    virtual KIID GetAuxItem2ID() const { return m_ids.size() > 2 ? m_ids[2] : niluuid; }
    virtual KIID GetAuxItem3ID() const { return m_ids.size() > 3 ? m_ids[3] : niluuid; }

    std::vector<KIID> GetIDs() const { return m_ids; }

    void SetParent( MARKER_BASE* aMarker ) { m_parent = aMarker; }
    MARKER_BASE* GetParent() const { return m_parent; }


    /**
     * Translate this object into a text string suitable for saving to disk in a report.
     *
     * @return wxString - the simple multi-line report text.
     */
    virtual wxString ShowReport( UNITS_PROVIDER* aUnitsProvider, SEVERITY aSeverity,
                                 const std::map<KIID, EDA_ITEM*>& aItemMap ) const;

    /**
     * Translate this object into an RC_JSON::VIOLATION object
     *
     * @param aViolation is the violation to be populated by info from this item
     * @param aUnitsProvider is the units provider that will be used to output coordinates
     * @param aSeverity is the severity of this item
     * @param aItemMap is a map allowing the lookup of items from KIIDs
     *
     * @return None
     */
    virtual void GetJsonViolation( RC_JSON::VIOLATION& aViolation, UNITS_PROVIDER* aUnitsProvider,
                                   SEVERITY aSeverity,
                                   const std::map<KIID, EDA_ITEM*>& aItemMap ) const;

    int GetErrorCode() const { return m_errorCode; }
    void SetErrorCode( int aCode ) { m_errorCode = aCode; }

    /**
     * @return the error message describing the specific details of a RC_ITEM.  For instance,
     * "Clearance violation (netclass '100ohm' clearance 0.4000mm; actual 0.3200mm)"
     */
    virtual wxString GetErrorMessage( bool aTranslate ) const;

    /**
     * @return the error text for the class of error of this RC_ITEM represents.  For instance,
     * "Clearance violation".
     */
    wxString GetErrorText( bool aTranslate ) const
    {
        if( aTranslate )
            return wxGetTranslation( m_errorTitle );
        else
            return m_errorTitle;
    }

    wxString GetSettingsKey() const
    {
        return m_settingsKey;
    }

    virtual wxString GetViolatingRuleDesc( bool aTranslate ) const
    {
        return wxEmptyString;
    }

protected:
    static wxString getSeverityString( SEVERITY aSeverity );

    /**
     * Resolve the description string used for an affected item in ShowReport and
     * GetJsonViolation.  Subclasses that need per-context formatting (e.g. ERC's
     * per-sheet-instance symbol references) override this rather than reimplementing
     * the surrounding report layout.
     *
     * @param aItem is the affected item being described
     * @param aIndex is 0 for the main item and 1 for the aux item
     */
    virtual wxString getItemDescription( EDA_ITEM* aItem, int aIndex,
                                         UNITS_PROVIDER* aUnitsProvider ) const;

    int           m_errorCode;         ///< The error code's numeric value
    wxString      m_errorMessage;      ///< A message describing the details of this specific error
    wxString      m_errorTitle;        ///< The string describing the type of error
    wxString      m_settingsKey;       ///< The key used to describe this type of error in settings
    MARKER_BASE*  m_parent;            ///< The marker this item belongs to, if any

    KIIDS         m_ids;

};


#endif      // RC_ITEM_H
