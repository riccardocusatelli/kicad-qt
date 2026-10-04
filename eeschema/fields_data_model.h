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

#pragma once

#include <map>
#include <vector>

#include <sch_reference_list.h>
#include <table_group_type.h>
#include <core/ki_assert.h>

class SCH_COMMIT;
class TEMPLATES;
struct BOM_FIELD;
struct BOM_PRESET;
struct BOM_FMT_PRESET;


struct DATA_MODEL_ROW
{
    DATA_MODEL_ROW( const SCH_REFERENCE& aFirstReference, GROUP_TYPE aType )
    {
        m_ItemNumber = 0;
        m_Refs.push_back( aFirstReference );
        m_Flag = aType;
    }

    int                        m_ItemNumber;
    GROUP_TYPE                 m_Flag;
    std::vector<SCH_REFERENCE> m_Refs;
};


struct DATA_MODEL_COL
{
    wxString m_fieldName;
    wxString m_label;
    bool     m_userAdded;
    bool     m_show;
    bool     m_group;
};


/**
 * The data of the symbol fields table and of the BOM: the symbols of a schematic as rows
 * (grouped and sorted), their fields as columns.
 *
 * It has no GUI: the BOM export of kicad-cli uses it directly, and the symbol fields table
 * dialog shows it in a grid through FIELDS_EDITOR_GRID_DATA_MODEL
 * (fields_editor_grid_data_model.h).
 */
class FIELDS_DATA_MODEL
{
public:
    enum SCOPE : int
    {
        SCOPE_ALL = 0,
        SCOPE_SHEET,
        SCOPE_SHEET_RECURSIVE
    };

    FIELDS_DATA_MODEL( const SCH_REFERENCE_LIST& aSymbolsList ) :
            m_symbolsList( aSymbolsList ),
            m_edited( false ),
            m_sortColumn( 0 ),
            m_sortAscending( false ),
            m_scope( SCOPE_ALL ),
            m_groupingEnabled( false ),
            m_excludeDNP( false ),
            m_includeExcluded( false ),
            m_rebuildsEnabled( true )
    {
        m_symbolsList.SplitReferences();
    }

    virtual ~FIELDS_DATA_MODEL() = default;

    static const wxString QUANTITY_VARIABLE;
    static const wxString ITEM_NUMBER_VARIABLE;

    void AddColumn( const wxString& aFieldName, const wxString& aLabel, bool aAddedByUser );
    void RemoveColumn( int aCol );
    void RenameColumn( int aCol, const wxString& newName );

    void MoveColumn( int aCol, int aNewPos )
    {
        KI_CHECK_RET( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), "Invalid Column Number" );

        if( aCol == aNewPos )
        {
            return;
        }
        else if( aCol < aNewPos )
        {
            std::rotate( std::begin( m_cols ) + aCol, std::begin( m_cols ) + aCol + 1,
                         std::begin( m_cols ) + aNewPos + 1 );
        }
        else
        {
            std::rotate( std::begin( m_cols ) + aNewPos, std::begin( m_cols ) + aCol,
                         std::begin( m_cols ) + aCol + 1 );
        }
    }

    int GetNumberRows() { return (int) m_rows.size(); }
    int GetNumberCols() { return (int) m_cols.size(); }

    void SetColLabelValue( int aCol, const wxString& aLabel )
    {
        KI_CHECK_RET( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), "Invalid Column Number" );
        m_cols[aCol].m_label = aLabel;
    }

    wxString GetColLabelValue( int aCol )
    {
        KI_CHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), wxString() );
        return m_cols[aCol].m_label;
    }

    wxString GetColFieldName( int aCol )
    {
        KI_CHECK( aCol >= 0 && aCol < static_cast<int>( m_cols.size() ), wxString() );
        return m_cols[aCol].m_fieldName;
    }

    int GetFieldNameCol( const wxString& aFieldName ) const;

    std::vector<BOM_FIELD> GetFieldsOrdered();
    void                   SetFieldsOrder( const std::vector<wxString>& aNewOrder );

    bool IsEmptyCell( int aRow, int aCol )
    {
        return false; // don't allow adjacent cell overflow, even if we are actually empty
    }

    wxString GetValue( int aRow, int aCol );
    wxString        GetResolvedValue( int aRow, int aCol );

    /// The cell cannot be edited (the expander column, or an attribute set by a parent sheet).
    bool IsCellReadOnly( int aRow, int aCol );

    /// The cell has a variant value that differs from the default value of the field.
    bool CellDiffersFromDefault( int aRow, int aCol );

    /// The raw value of the cell contains a text variable (shown resolved).
    bool CellHasTextVariable( int aRow, int aCol );

    /// The texts of column @a aCol, for the column width.
    std::vector<wxString> GetColumnTexts( int aCol );

    wxString GetValue( const DATA_MODEL_ROW& group, int aCol,
                       const wxString& refDelimiter = wxT( ", " ),
                       const wxString& refRangDelimiter = wxT( "-" ),
                       bool resolveVars = false,
                       bool listMixedValues = false );

    wxString GetExportValue( int aRow, int aCol, const wxString& refDelimiter,
                             const wxString& refRangeDelimiter )
    {
        return GetValue( m_rows[aRow], aCol, refDelimiter, refRangeDelimiter, true, true );
    }

    void     SetValue( int aRow, int aCol, const wxString& aValue );

    GROUP_TYPE GetRowFlags( int aRow ) { return m_rows[aRow].m_Flag; }

    std::vector<SCH_REFERENCE> GetRowReferences( int aRow ) const
    {
        KI_CHECK( aRow >= 0 && aRow < (int) m_rows.size(), std::vector<SCH_REFERENCE>() );
        return m_rows[aRow].m_Refs;
    }

    bool ColIsReference( int aCol );
    bool ColIsValue( int aCol );
    bool ColIsQuantity( int aCol );
    bool ColIsItemNumber( int aCol );
    bool ColIsAttribute( int aCol );

    bool IsExpanderColumn( int aCol ) const;
    GROUP_TYPE GetGroupType( int aRow ) const
    {
        return m_rows[aRow].m_Flag;
    }

    void SetSorting( int aCol, bool ascending )
    {
        KI_CHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), "Invalid Column Number" );
        m_sortColumn = aCol;
        m_sortAscending = ascending;
    }

    int  GetSortCol() { return m_sortColumn; }
    bool GetSortAsc() { return m_sortAscending; }

    // These are used to disable the RebuildRows functionality while we're generating
    // lots of events in the UI, e.g. applying a BOM preset, that would thrash the grid.
    void EnableRebuilds();
    void DisableRebuilds();
    void RebuildRows();

    void ExpandRow( int aRow );
    void CollapseRow( int aRow );
    void ExpandCollapseRow( int aRow );
    void CollapseForSort();
    void ExpandAfterSort();

    void ApplyData( SCH_COMMIT& aCommit, TEMPLATES& aTemplateFieldnames, const wxString& aVariantName );

    bool IsEdited() { return m_edited; }

    void SetFilter( const wxString& aFilter ) { m_filter = aFilter; }
    const wxString& GetFilter() { return m_filter; }

    void  SetScope( SCOPE aScope ) { m_scope = aScope; }
    SCOPE GetScope() { return m_scope; }

    void                  SetPath( const SCH_SHEET_PATH& aPath ) { m_path = aPath; }
    const SCH_SHEET_PATH& GetPath() { return m_path; }

    void SetGroupingEnabled( bool group ) { m_groupingEnabled = group; }
    bool GetGroupingEnabled() { return m_groupingEnabled; }

    /* These contradictorily named functions force including symbols that
     * have the Exclude from BOM check box ticked. This is needed so we can view
     * these parts in the symbol fields table dialog, while also excluding from the
     * BOM export */
    void SetIncludeExcludedFromBOM( bool include ) { m_includeExcluded = include; }
    bool GetIncludeExcludedFromBOM() { return m_includeExcluded; }

    void SetExcludeDNP( bool exclude ) { m_excludeDNP = exclude; }
    bool GetExcludeDNP() { return m_excludeDNP; }

    void SetGroupColumn( int aCol, bool group )
    {
        KI_CHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), "Invalid Column Number" );
        m_cols[aCol].m_group = group;
    }

    bool GetGroupColumn( int aCol )
    {
        KI_CHECK_MSG( aCol >= 0 && aCol < (int) m_cols.size(), false, "Invalid Column Number" );
        return m_cols[aCol].m_group;
    }

    void SetShowColumn( int aCol, bool show )
    {
        KI_CHECK_RET( aCol >= 0 && aCol < (int) m_cols.size(), "Invalid Column Number" );
        m_cols[aCol].m_show = show;
    }

    bool GetShowColumn( int aCol )
    {
        KI_CHECK_MSG( aCol >= 0 && aCol < (int) m_cols.size(), false, "Invalid Column Number" );
        return m_cols[aCol].m_show;
    }

    void       ApplyBomPreset( const BOM_PRESET& preset );
    BOM_PRESET GetBomSettings();
    wxString Export( const BOM_FMT_PRESET& settings );

    void AddReferences( const SCH_REFERENCE_LIST& aRefs );
    void RemoveReferences( const SCH_REFERENCE_LIST& aRefs );
    void RemoveSymbol( const SCH_SYMBOL& aSymbol );
    void UpdateReferences( const SCH_REFERENCE_LIST& aRefs );

    // Identity-based undo serialization (keyed by symbol, not row position) for the dialog's
    // Ctrl+Z, so it stays correct as rows are grouped/sorted/reordered.
    wxString SerializeUndoState() const;
    void     RestoreUndoState( const wxString& aState );

    bool DeleteRows( size_t aPosition = 0, size_t aNumRows = 1 );

    const SCH_REFERENCE_LIST& GetReferenceList() const { return m_symbolsList; }

    /**
     * Set the current variant name for highlighting purposes.
     *
     * When a variant is set, cells that differ from the default (non-variant) value
     * will be highlighted.
     *
     * @param aVariantName The name of the current variant, or empty string for default.
     */
    void SetCurrentVariant( const wxString& aVariantName ) { m_currentVariant = aVariantName; }
    const wxString& GetCurrentVariant() const { return m_currentVariant; }

    void SetVariantNames( const std::vector<wxString>& aVariantNames ) { m_variantNames = aVariantNames; }
    const std::vector<wxString>& GetVariantNames() const { return m_variantNames; }

private:
    static bool cmp( const DATA_MODEL_ROW& lhGroup, const DATA_MODEL_ROW& rhGroup,
                     FIELDS_DATA_MODEL* dataModel, int sortCol, bool ascending );

    bool unitMatch( const SCH_REFERENCE& lhRef, const SCH_REFERENCE& rhRef );
    bool groupMatch( const SCH_REFERENCE& lhRef, const SCH_REFERENCE& rhRef );

    // Helper functions to deal with translating wxGrid values to and from
    // named field values like ${DNP}
    bool     isAttribute( const wxString& aFieldName );

    /**
     * Test whether a field's storage is common to every sheet path that reaches a symbol.
     *
     * Shared sheets present one symbol under several paths.  Edits to storage they have in
     * common must be recorded against all of those paths, while variant overrides belong to
     * a single symbol instance.
     *
     * @param aFieldName is the canonical field or attribute name.
     * @retval true if the field is stored on the symbol.
     * @retval false if the field is stored on the symbol instance.
     */
    bool storageIsSharedAcrossPaths( const wxString& aFieldName ) const;

    // True when an ancestor sheet forces this attribute on, not the symbol itself.
    bool attributeInheritedFromSheet( const SCH_REFERENCE& aRef, const wxString& aAttributeName ) const;
    bool rowAttributeInheritedFromSheet( const DATA_MODEL_ROW& aGroup, int aCol );

    wxString getAttributeValue( const SCH_REFERENCE& aRef, const wxString& aAttributeName,
                                const wxString& aVariantNames );

    /**
     * Get the default (non-variant) value for a field.
     *
     * This retrieves the field value as it would appear without any variant override.
     *
     * @param aRef The symbol reference.
     * @param aFieldName The name of the field.
     * @return The default field value.
     */
    wxString getDefaultFieldValue( const SCH_REFERENCE& aRef, const wxString& aFieldName );

    /**
     * Set the attribute value.
     *
     * @param aReference is a reference to the symbol to set the attribute.
     * @param aAttributeName is the name of the symbol attribute.
     * @param aValue is the value to set the attribute.
     * @param aVariantName is an optional variant name to set the variant attribute.
     * @retval true if the symbol attribute value has changed.
     * @retval false if the symbol attribute has **not** changed.
     */
    bool setAttributeValue( SCH_REFERENCE& aRef, const wxString& aAttributeName, const wxString& aValue,
                            const wxString& aVariantName = wxEmptyString );

    /* Helper function to get the resolved field value.
     * Handles symbols that are missing fields that would have a variable
     * in their value because their name is the same as a variable.
     * Example: BOM template provides ${DNP} as a field, but they symbol doesn't have the field. */
    wxString getFieldShownText( const SCH_REFERENCE& aRef, const wxString& aFieldName );

    void Sort();

    void updateDataStoreSymbolField( const SCH_REFERENCE& aSymbolRef, const wxString& aFieldName );

protected:
    /// Notifications for the grid showing the model (none without GUI).
    virtual void rowsAppended( size_t aCount ) {}
    virtual void rowsInserted( size_t aPos, size_t aCount ) {}
    virtual void rowsDeleted( size_t aPos, size_t aCount ) {}
    virtual void colsDeleted( size_t aPos, size_t aCount ) {}

    /// The rows are going to be rebuilt: finish the edits in progress.
    virtual void commitPendingEdits() {}

    /// The data has changed outside of the grid.
    virtual void dataChanged() {}

    /**
     * The flattened by hierarchy list of symbols.
     *
     * @warning This list **must** be kept sorted by symbol pointer.  Otherwise, the undo/redo
     *          commit actions will be broken.
     */
    SCH_REFERENCE_LIST m_symbolsList;
    bool               m_edited;
    int                m_sortColumn;
    bool               m_sortAscending;
    wxString           m_filter;
    SCOPE              m_scope;
    SCH_SHEET_PATH     m_path;
    bool               m_groupingEnabled;
    bool               m_excludeDNP;
    bool               m_includeExcluded;
    bool               m_rebuildsEnabled;
    wxString                m_currentVariant;  ///< Current variant name for highlighting
    std::vector<wxString>   m_variantNames;    ///< Variant names for multi-variant DNP filtering

    std::vector<DATA_MODEL_COL> m_cols;
    std::vector<DATA_MODEL_ROW> m_rows;

    // Data store
    // The data model is fundamentally m_componentRefs X m_fieldNames.
    // A map of compID : fieldSet, where fieldSet is a map of fieldName : fieldValue
    // The compID is now the full KIID_PATH (sheet path + symbol UUID) as a string
    std::map<KIID_PATH, std::map<wxString, wxString>> m_dataStore;
};
