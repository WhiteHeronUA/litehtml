#pragma once

// QT
#include <QtCore/QUrl>
#include <QtWidgets/QAbstractScrollArea>

/**********************************************************************************************/
namespace litehtml
{
    class document;
    class render_item;
} // namespace litehtml

/**********************************************************************************************/
class qt_container;


/**********************************************************************************************/
class qt_litehtml : public QAbstractScrollArea
{
    public://////////////////////////////////////////////////////////////////////////

                                            qt_litehtml( QWidget* in_parent = {} );
                                            ~qt_litehtml() override;


    protected://////////////////////////////////////////////////////////////////////////

// QWidget API:

        void                                contextMenuEvent( QContextMenuEvent* in_event ) override;
        void                                leaveEvent( QEvent* in_event ) override;
        void                                mouseDoubleClickEvent( QMouseEvent* in_event ) override;
        void                                mouseMoveEvent( QMouseEvent* in_event ) override;
        void                                mousePressEvent( QMouseEvent* in_event ) override;
        void                                mouseReleaseEvent( QMouseEvent* in_event ) override;
        void                                paintEvent( QPaintEvent* in_event ) override;
        void                                resizeEvent( QResizeEvent* in_event ) override;
        void                                wheelEvent( QWheelEvent* in_event ) override;


    public://////////////////////////////////////////////////////////////////////////

// this class API:

        void                                clear() { setHtml({}); }
        void                                copy() const;

        QFont                               defaultFont() const;
        void                                setDefaultFont( const QFont& in_font );

        QString                             defaultStyleSheet() const { return default_css_; }
        void                                setDefaultStyleSheet( const QString& in_css ) { default_css_ = in_css; }

        int                                 documentMargin() const { return doc_margin_; }
        void                                setDocumentMargin( int in_margin ) { doc_margin_ = in_margin; }

        QString                             html() const { return html_; }
        void                                setHtml( const QString& in_html );

        void                                selectAll();
        QString                             selectedText() const;

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

        void                                clearSelection();
        QRect                               clientRect() const;
        QPoint                              documentPos( const QPoint& in_pos ) const;
        bool                                inSelectionContains( int in_x, int in_y ) const;
        const char*                         masterCSS() const;
        void                                render();
        void                                updateDocumentRect( const QRect& in_rect );
        void                                updateSelection( QPoint in_from, QPoint in_to );


    private://////////////////////////////////////////////////////////////////////////

        struct selection_item
        {
            int                                    width_;
            int                                    height_;
            std::shared_ptr<litehtml::render_item> item_;
        };

        using selection_t = std::map<int, std::multimap<int, selection_item>>;


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
        QPoint                              drag_start_ { -1, -1 };
        selection_t                         selection_;
        QPoint                              selection_start_ { -1, -1 };
        QUrl                                url_;
        double                              zoom_ { 1. };


    protected://////////////////////////////////////////////////////////////////////////

        friend class qt_container;
};
