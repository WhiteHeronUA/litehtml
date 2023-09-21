#pragma once

// QT
#include <QtCore/QUrl>
#include <QtWidgets/QAbstractScrollArea>

/**********************************************************************************************/
class QNetworkAccessManager;

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

        QFont                               defaultFont() const;
        void                                setDefaultFont( const QFont& in_font );

        void                                setHtml( const char* in_html );

virtual void                                setURL( QUrl in_url );
        QUrl                                url() const { return url_; }

        void                                setZoom( double in_zoom );
        double                              zoom() const { return zoom_; }
        void                                zoomIn();
        void                                zoomOut();


    protected://////////////////////////////////////////////////////////////////////////

virtual void                                onError( const QString& in_msg );
virtual QByteArray                          loadData( const QUrl& in_url );


    private://////////////////////////////////////////////////////////////////////////

        void                                render();


    private://////////////////////////////////////////////////////////////////////////

// cached data:

        QHash<QUrl, QByteArray>             cache_;


// references:

        std::unique_ptr<qt_container>           container_;
        std::shared_ptr<litehtml::document>     document_;
        std::unique_ptr<QNetworkAccessManager>  network_manager_;


// state:

        QUrl                                url_;
        double                              zoom_ { 1 };


    protected://////////////////////////////////////////////////////////////////////////

        friend class qt_container;
};
