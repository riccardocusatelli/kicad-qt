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


#include "ui_hooks.h"

#include <memory>
#include <vector>

#include <QApplication>
#include <QPointer>
#include <QTimer>
#include <QVariant>
#include <QWidget>


namespace
{

/// The event filter of a hook (a child of the widget, so that it is deleted with it).
class EVENT_HOOK : public QObject
{
public:
    EVENT_HOOK( QWidget* aWidget, QEvent::Type aType, std::function<void( QEvent* )> aHandler ) :
            QObject( aWidget ),
            m_type( aType ),
            m_handler( std::move( aHandler ) )
    {
        aWidget->installEventFilter( this );
    }

protected:
    bool eventFilter( QObject* aWatched, QEvent* aEvent ) override
    {
        if( aEvent->type() == m_type && aWatched == parent() )
            m_handler( aEvent );

        return false;
    }

private:
    QEvent::Type                    m_type;
    std::function<void( QEvent* )>  m_handler;
};


/// An event filter of a widget calling a function (a child of the widget).
class EVENT_FILTER : public QObject
{
public:
    EVENT_FILTER( QWidget* aWidget, std::function<bool( QEvent* )> aFilter ) :
            QObject( aWidget ),
            m_filter( std::move( aFilter ) )
    {
        aWidget->installEventFilter( this );
    }

protected:
    bool eventFilter( QObject* aWatched, QEvent* aEvent ) override
    {
        return aWatched == parent() && m_filter( aEvent );
    }

private:
    std::function<bool( QEvent* )> m_filter;
};


/**
 * The update UI functions, and the application event filter that runs them (once, from the
 * event loop) after the events of the user.
 */
class UPDATE_UI_REGISTRY : public QObject
{
public:
    static UPDATE_UI_REGISTRY& Get()
    {
        static UPDATE_UI_REGISTRY* registry = new UPDATE_UI_REGISTRY;
        return *registry;
    }

    void Add( QWidget* aOwner, std::function<void()> aUpdate )
    {
        m_entries.push_back( { QPointer<QWidget>( aOwner ), std::move( aUpdate ) } );
        Schedule();
    }

    void Schedule()
    {
        if( !m_timer.isActive() )
            m_timer.start();
    }

    void Run()
    {
        // The entries of deleted owners go away
        std::erase_if( m_entries,
                       []( const ENTRY& aEntry )
                       {
                           return aEntry.m_owner.isNull();
                       } );

        // An update can register other functions: run the ones registered now
        std::vector<ENTRY> entries = m_entries;

        for( const ENTRY& entry : entries )
        {
            if( entry.m_owner && entry.m_owner->isVisible() )
                entry.m_update();
        }
    }

protected:
    bool eventFilter( QObject* aWatched, QEvent* aEvent ) override
    {
        switch( aEvent->type() )
        {
        case QEvent::MouseButtonRelease:
        case QEvent::KeyRelease:
        case QEvent::FocusIn:
        case QEvent::Wheel:
        case QEvent::Show:
            if( !m_entries.empty() )
                Schedule();

            break;

        default:
            break;
        }

        return false;
    }

private:
    UPDATE_UI_REGISTRY()
    {
        m_timer.setSingleShot( true );
        m_timer.setInterval( 0 );
        QObject::connect( &m_timer, &QTimer::timeout, this, [this]() { Run(); } );

        qApp->installEventFilter( this );
    }

    struct ENTRY
    {
        QPointer<QWidget>     m_owner;
        std::function<void()> m_update;
    };

    std::vector<ENTRY> m_entries;
    QTimer             m_timer;
};

} // namespace


void KIQT::OnEvent( QWidget* aWidget, QEvent::Type aType,
                    std::function<void( QEvent* aEvent )> aHandler )
{
    new EVENT_HOOK( aWidget, aType, std::move( aHandler ) );
}


void KIQT::FilterEvents( QWidget* aWidget, std::function<bool( QEvent* aEvent )> aFilter )
{
    new EVENT_FILTER( aWidget, std::move( aFilter ) );
}


void KIQT::RegisterUpdateUI( QWidget* aOwner, std::function<void()> aUpdate )
{
    UPDATE_UI_REGISTRY::Get().Add( aOwner, std::move( aUpdate ) );
}


void KIQT::UpdateUINow()
{
    UPDATE_UI_REGISTRY::Get().Run();
}


void KIQT::SetWindowId( QWidget* aWidget, int aId )
{
    aWidget->setProperty( "kicadWindowId", aId );
}


int KIQT::GetWindowId( const QWidget* aWidget )
{
    QVariant id = aWidget->property( "kicadWindowId" );
    return id.isValid() ? id.toInt() : -1;
}
