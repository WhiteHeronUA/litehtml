#pragma once

// QT
#include <QtCore/QUrl>
#include <QtWidgets/QAbstractScrollArea>

/**********************************************************************************************/
namespace litehtml { class document; }

/**********************************************************************************************/
class qt_container;


/**********************************************************************************************/
class qt_litehtml : public QAbstractScrollArea
{
    public://////////////////////////////////////////////////////////////////////////

                                            qt_litehtml( QWidget* in_parent = {} );
                                            ~qt_litehtml() override;


    public://////////////////////////////////////////////////////////////////////////

// QWidget API:

        void                                leaveEvent( QEvent* in_event ) override;
        void                                mouseMoveEvent( QMouseEvent* in_event ) override;
        void                                mousePressEvent( QMouseEvent* in_event ) override;
        void                                mouseReleaseEvent( QMouseEvent* in_event ) override;
        void                                paintEvent( QPaintEvent* in_event ) override;
        void                                resizeEvent( QResizeEvent* in_event ) override;
        void                                wheelEvent( QWheelEvent* in_event ) override;


    public://////////////////////////////////////////////////////////////////////////

// this class API:

        void                                clear() { setHtml({}); }

        QFont                               defaultFont() const;
        void                                setDefaultFont( const QFont& in_font );

        QString                             defaultStyleSheet() const { return default_css_; }
        void                                setDefaultStyleSheet( const QString& in_css ) { default_css_ = in_css; }

        int                                 documentMargin() const { return doc_margin_; }
        void                                setDocumentMargin( int in_margin ) { doc_margin_ = in_margin; }

        QString                             html() const { return html_; }
        void                                setHtml( const QString& in_html );

virtual void                                setURL( const QUrl& in_url );
        QUrl                                url() const { return url_; }

        void                                setZoom( double in_zoom );
        double                              zoom() const { return zoom_; }
        void                                zoomIn();
        void                                zoomOut();


    protected://////////////////////////////////////////////////////////////////////////

virtual void                                onAnchorClicked( const QUrl& in_url ) { setURL( in_url ); }
virtual void                                onError( const QString& in_msg );
virtual QByteArray                          loadData( const QUrl& in_url );


    private://////////////////////////////////////////////////////////////////////////

        QRect                               clientRect() const;
        const char*                         masterCSS() const;
        void                                render();


    private://////////////////////////////////////////////////////////////////////////

// cached data:

        QHash<QUrl, QByteArray>             cache_;
        QString                             html_;

mutable std::string                         master_css_;
mutable int                                 master_css_margin_ { -1 };

// references:

        std::unique_ptr<qt_container>       container_;
        std::shared_ptr<litehtml::document> document_;


// state:

        QString                             default_css_;
        int                                 doc_margin_ { 8 };
        QUrl                                url_;
        double                              zoom_ { 1. };


    protected://////////////////////////////////////////////////////////////////////////

        friend class qt_container;
};
