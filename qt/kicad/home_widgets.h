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

#include <utility>
#include <vector>

#include <QColor>
#include <QDateTime>
#include <QFont>
#include <QString>
#include <QStringList>
#include <QWidget>

class QLabel;


/// The colours of the home page: the tokens of UI_THEME of the wx project manager.
struct HOME_PALETTE
{
    QColor bg;           ///< Window / dock background
    QColor surface;      ///< Cards
    QColor surfaceHover; ///< Hovered card
    QColor border;       ///< Hairlines and outlines of the cards
    QColor text;         ///< Primary text
    QColor muted;        ///< Secondary text
    QColor accent;       ///< Primary action, section labels
    QColor accentText;   ///< Text on the accent colour
    QColor badge;        ///< Background of the monogram badges
};

/// The palette of the light or dark theme (the "appearance/dark" setting of kicad-qt).
const HOME_PALETTE& HomePalette();

/// The font for labels and badges, with a fixed pitch.
QFont HomeMonoFont( int aPointDelta, bool aBold );

/// "3 hours ago", "yesterday", ...
QString FormatRelativeTime( const QDateTime& aTime );


/// A page of the home: it paints the background and colours its labels from the palette.
class HOME_PAGE : public QWidget
{
    Q_OBJECT

public:
    enum class TEXT_ROLE
    {
        ACCENT,
        TEXT,
        MUTED
    };

    explicit HOME_PAGE( QWidget* aParent = nullptr );

    /// Section label in the accent colour, small caps (e.g. "DESIGN").
    QLabel* AddSectionLabel( const QString& aText );

    /// Large title at the top of a column.
    QLabel* AddTitle( const QString& aText );

    /// Label in one of the colours of the palette.
    QLabel* AddLabel( const QString& aText, TEXT_ROLE aRole );

protected:
    void paintEvent( QPaintEvent* aEvent ) override;
    void changeEvent( QEvent* aEvent ) override;

private:
    void applyColours();

    std::vector<std::pair<QLabel*, TEXT_ROLE>> m_labels;
};


/// Flat, rounded button.  Emits clicked().
class HOME_BUTTON : public QWidget
{
    Q_OBJECT

public:
    HOME_BUTTON( const QString& aLabel, bool aPrimary, QWidget* aParent = nullptr );

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

signals:
    void clicked();

protected:
    void paintEvent( QPaintEvent* aEvent ) override;
    void enterEvent( QEnterEvent* aEvent ) override;
    void leaveEvent( QEvent* aEvent ) override;
    void mousePressEvent( QMouseEvent* aEvent ) override;
    void mouseReleaseEvent( QMouseEvent* aEvent ) override;
    void changeEvent( QEvent* aEvent ) override;

private:
    QString m_label;
    bool    m_primary;
    bool    m_hover = false;
    bool    m_pressed = false;
};


/// Clickable card of an editor: monogram badge, title, one line of description.
class LAUNCHER_CARD : public QWidget
{
    Q_OBJECT

public:
    LAUNCHER_CARD( const QString& aMonogram, const QString& aTitle, const QString& aDescription,
                   bool aCompact, QWidget* aParent = nullptr );

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

signals:
    void clicked();

protected:
    void paintEvent( QPaintEvent* aEvent ) override;
    void enterEvent( QEnterEvent* aEvent ) override;
    void leaveEvent( QEvent* aEvent ) override;
    void mouseReleaseEvent( QMouseEvent* aEvent ) override;
    void changeEvent( QEvent* aEvent ) override;

private:
    QString m_monogram;
    QString m_title;
    QString m_description;
    bool    m_compact;
    bool    m_hover = false;
};


/// A recent project: name and time, folder, tags of the documents that exist.
class RECENT_PROJECT_ITEM : public QWidget
{
    Q_OBJECT

public:
    RECENT_PROJECT_ITEM( const QString& aProjectFile, QWidget* aParent = nullptr );

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

signals:
    void activated( const QString& aProjectFile );

protected:
    void paintEvent( QPaintEvent* aEvent ) override;
    void enterEvent( QEnterEvent* aEvent ) override;
    void leaveEvent( QEvent* aEvent ) override;
    void mouseReleaseEvent( QMouseEvent* aEvent ) override;

private:
    QString   m_projectFile;
    QString   m_name;
    QString   m_folder;
    QDateTime m_modified;
    bool      m_exists = false;
    bool      m_hasSchematic = false;
    bool      m_hasBoard = false;
    bool      m_hover = false;
};


/// The left column of the home: the recent projects as cards, with New and Open.
class RECENT_PROJECTS_PANEL : public HOME_PAGE
{
    Q_OBJECT

public:
    explicit RECENT_PROJECTS_PANEL( QWidget* aParent = nullptr );

    void SetProjects( const QStringList& aProjectFiles );

signals:
    void openRequested();
    void projectActivated( const QString& aProjectFile );

private:
    QWidget* m_list;
    QLabel*  m_footer;
};


/// The right column of the home: the editors, in the sections of the wx project manager.
class LAUNCHER_PANEL : public HOME_PAGE
{
    Q_OBJECT

public:
    explicit LAUNCHER_PANEL( QWidget* aParent = nullptr );

    /// Enable the editors for the documents of the project.
    void SetDocuments( bool aSchematic, bool aBoard );

signals:
    void schematicRequested();
    void boardRequested();
    void footprintEditorRequested();

private:
    LAUNCHER_CARD* m_schematic;
    LAUNCHER_CARD* m_board;
};
