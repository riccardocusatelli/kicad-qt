// The wxWidgets compatibility layer of the Qt build: <wx/laywin.h>, the windows laid out by a
// wxLayoutAlgorithm.  KiCad uses a wxSashLayoutWindow as a panel of an AUI manager only: it is a
// wxPanel here.
#pragma once
#include <wx/kiwx_gui.h>

#include <QChildEvent>
#include <QPointer>
#include <QResizeEvent>

class wxSashLayoutWindow : public wxPanel
{
public:
    using wxPanel::wxPanel;

protected:
    /*
     * As with wxWidgets, a window without a sizer and with a single child gives it its whole
     * client area: the project tree of the project manager covers its pane (Qt would leave it
     * at the size hint of the tree, in the top left corner).
     */
    void resizeEvent( QResizeEvent* aEvent ) override
    {
        wxPanel::resizeEvent( aEvent );
        fillWithChildCompat();
    }

    void childEvent( QChildEvent* aEvent ) override
    {
        wxPanel::childEvent( aEvent );

        // The child is not built yet when it is added: it is laid out once it is
        if( aEvent->added() && aEvent->child()->isWidgetType() )
        {
            QPointer<QWidget> self( this );
            QMetaObject::invokeMethod(
                    this,
                    [self]()
                    {
                        if( self )
                            static_cast<wxSashLayoutWindow*>( self.data() )->fillWithChildCompat();
                    },
                    Qt::QueuedConnection );
        }
    }

private:
    void fillWithChildCompat()
    {
        if( layout() )
            return;

        QWidget* only = nullptr;

        for( QObject* object : children() )
        {
            QWidget* child = qobject_cast<QWidget*>( object );

            if( !child || child->isWindow() || child->isHidden() )
                continue;

            if( only )
                return;

            only = child;
        }

        if( only && only->geometry() != rect() )
            only->setGeometry( rect() );
    }
};
