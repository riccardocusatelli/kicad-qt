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
 * @file text_ctrl_eval.h
 * TEXT_CTRL_EVAL of the Qt build: a text control evaluating the numeric expressions typed in
 * (see include/widgets/text_ctrl_eval.h).
 */

#pragma once

#include <functional>
#include <optional>

#include <QLineEdit>

#include <libeval/numeric_evaluator.h>
#include <wx/string.h>


/**
 * A text control that evaluates the numeric expression it contains when it loses focus or
 * Enter is pressed (Enter also accepts the dialog).  Back in the control, the expression is
 * shown again.
 */
class TEXT_CTRL_EVAL : public QLineEdit
{
public:
    TEXT_CTRL_EVAL( QWidget* aParent, const wxString& aValue = wxEmptyString );

    ~TEXT_CTRL_EVAL() override = default;

    /**
     * Set a function that will be called when the text control loses focus (instead of the
     * evaluation of the expression).
     */
    void SetCustomEval( std::function<void( TEXT_CTRL_EVAL* aCtrl )> aCustomEval )
    {
        m_customEval = std::move( aCustomEval );
    }

    /// Set a new value, forgetting the expression.
    void SetValue( const wxString& aValue );

    wxString GetValue() const;

    /// Set a new value without the change events (as ChangeValue() of wxWidgets).
    void ChangeValue( const wxString& aValue );

protected:
    void focusInEvent( QFocusEvent* aEvent ) override;
    void focusOutEvent( QFocusEvent* aEvent ) override;

    /// Numeric expression evaluator.
    NUMERIC_EVALUATOR m_eval;

    void onTextEnter();

    void evaluate();

private:
    std::optional<std::function<void( TEXT_CTRL_EVAL* aCtrl )>> m_customEval;
};
