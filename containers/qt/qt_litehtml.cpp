// QT
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QShortcut>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QStyle>

// QTLITEHTML
#include "qt_container.h"
#include "qt_litehtml.h"

/**********************************************************************************************/
using namespace litehtml;


//////////////////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark helper functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static inline QRect ToQRect( const position& in_pos )
{
    return { in_pos.x, in_pos.y, in_pos.width, in_pos.height };
}


//////////////////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark construction
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
qt_litehtml::qt_litehtml( QWidget* in_parent )
:
    QAbstractScrollArea( in_parent )
,
    container_( new qt_container( this ) )
{
    setMouseTracking( true );

    // background

    auto pl = viewport()->palette();
    pl.setColor( QPalette::Base, Qt::white );
    viewport()->setPalette( pl );

    // scrollbars

    const int step = fontMetrics().height();

    horizontalScrollBar()->setSingleStep( step );
    verticalScrollBar()->setSingleStep( step );

    // shortcuts

    connect(
        new QShortcut( QKeySequence::ZoomIn, this ),
        &QShortcut::activated,
        this,
        &qt_litehtml::zoomIn );

    connect(
        new QShortcut( QKeySequence::ZoomOut, this ),
        &QShortcut::activated,
        this,
        &qt_litehtml::zoomOut );
}

/**********************************************************************************************/
qt_litehtml::~qt_litehtml()
{
}


//////////////////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark QWidget
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
void qt_litehtml::leaveEvent( QEvent* in_event )
{
    QAbstractScrollArea::leaveEvent( in_event );

    setToolTip({});

    if( document_ )
    {
        const auto spos = QPoint( horizontalScrollBar()->value(), verticalScrollBar()->value() );

        position::vector redraw;
        document_->on_mouse_leave( redraw );

        for( const auto& it : redraw )
        {
            QRect rr = ToQRect( it ).translated( -spos );

            rr.moveTopLeft( rr.topLeft() * zoom_ );
            rr.moveBottomRight( rr.bottomRight() * zoom_ + QPoint( 1, 1 ) );

            viewport()->update( rr );
        }
    }
}

/**********************************************************************************************/
void qt_litehtml::mouseMoveEvent( QMouseEvent* in_event )
{
    QAbstractScrollArea::mouseMoveEvent( in_event );

    if( document_ )
    {
        const auto spos = QPoint( horizontalScrollBar()->value(), verticalScrollBar()->value() );
        const auto vpos = viewport()->mapFromParent( in_event->pos() ) / zoom_;
        const auto dpos = vpos + spos;

        position::vector redraw;
        document_->on_mouse_over( dpos.x(), dpos.y(), dpos.x(), dpos.y(), redraw );

        for( const auto& it : redraw )
        {
            QRect rr = ToQRect( it ).translated( -spos );

            rr.moveTopLeft( rr.topLeft() * zoom_ );
            rr.moveBottomRight( rr.bottomRight() * zoom_ + QPoint( 1, 1 ) );

            viewport()->update( rr );
        }

        if( const auto over = document_->get_over_element() )
        {
            if( const char* const href = over->get_attr( "href" ); href && *href )
                setToolTip( href );
            else
                setToolTip({});
        }
        else
        {
            setToolTip({});
        }
    }
}

/**********************************************************************************************/
void qt_litehtml::mousePressEvent( QMouseEvent* in_event )
{
    if( document_ )
    {
        if( in_event->button() == Qt::LeftButton )
        {
            const auto spos = QPoint( horizontalScrollBar()->value(), verticalScrollBar()->value() );
            const auto vpos = viewport()->mapFromParent( in_event->pos() ) / zoom_;
            const auto dpos = vpos + spos;

            position::vector redraw;
            document_->on_lbutton_down( dpos.x(), dpos.y(), dpos.x(), dpos.y(), redraw );

            for( const auto& it : redraw )
            {
                QRect rr = ToQRect( it ).translated( -spos );

                rr.moveTopLeft( rr.topLeft() * zoom_ );
                rr.moveBottomRight( rr.bottomRight() * zoom_ + QPoint( 1, 1 ) );

                viewport()->update( rr );
            }

            in_event->accept();

            return;
        }
    }

    QAbstractScrollArea::mousePressEvent( in_event );
}

/**********************************************************************************************/
void qt_litehtml::mouseReleaseEvent( QMouseEvent* in_event )
{
    if( document_ )
    {
        if( in_event->button() == Qt::LeftButton )
        {
            const auto spos = QPoint( horizontalScrollBar()->value(), verticalScrollBar()->value() );
            const auto vpos = viewport()->mapFromParent( in_event->pos() ) / zoom_;
            const auto dpos = vpos + spos;

            position::vector redraw;
            document_->on_lbutton_up( dpos.x(), dpos.y(), dpos.x(), dpos.y(), redraw );

            for( const auto& it : redraw )
            {
                QRect rr = ToQRect( it ).translated( -spos );

                rr.moveTopLeft( rr.topLeft() * zoom_ );
                rr.moveBottomRight( rr.bottomRight() * zoom_ + QPoint( 1, 1 ) );

                viewport()->update( rr );
            }

            in_event->accept();

            return;
        }
    }

    QAbstractScrollArea::mousePressEvent( in_event );
}

/**********************************************************************************************/
void qt_litehtml::paintEvent( QPaintEvent* in_event )
{
    if( document_ )
    {
        QPainter p( viewport() );
        p.setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform );
        p.setWorldTransform( QTransform::fromScale( zoom_, zoom_ ) );

        const int x = horizontalScrollBar()->value();
        const int y = verticalScrollBar()->value();

        QRect qclip = in_event->rect();
        qclip.setTopLeft( qclip.topLeft() / zoom_ );
        qclip.setSize( qclip.size() / zoom_ );

        const position clip { qclip.x(), qclip.y(), qclip.width(), qclip.height() };

        document_->draw( reinterpret_cast<uint_ptr>( &p ), -x, -y, &clip );
    }
}

/**********************************************************************************************/
void qt_litehtml::resizeEvent( QResizeEvent* in_event )
{
    QAbstractScrollArea::resizeEvent( in_event );
    render();
}

/**********************************************************************************************/
void qt_litehtml::wheelEvent( QWheelEvent* in_event )
{
    if( ( in_event->modifiers() & Qt::CTRL ) )
    {
        const auto dy = in_event->pixelDelta().y();

        if( dy > 0 )
        {
            zoomIn();
            in_event->accept();
            return;
        }

        if( dy < 0 )
        {
            zoomOut();
            in_event->accept();
            return;
        }
    }

    QAbstractScrollArea::wheelEvent( in_event );
}


//////////////////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark QWidget
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
QFont qt_litehtml::defaultFont() const
{
    return container_->default_font();
}

/**********************************************************************************************/
void qt_litehtml::setDefaultFont( const QFont& in_font )
{
    container_->set_default_font( in_font );
    render();
}

/**********************************************************************************************/
void qt_litehtml::setHtml( const char* in_html )
{
    document_ = document::createFromString( in_html, container_.get(), litehtml::master_css );
    render();
}

/**********************************************************************************************/
void qt_litehtml::setURL( QUrl in_url )
{
    // Anchor
    if( auto name = in_url.toString(); name.startsWith( '#' ) )
    {
        if( document_ )
        {
            if( const auto root = document_->root() )
            {
                auto el = root->select_one( name.toUtf8().data() );
                if( !el )
                {
                    name = "[name=" + name.sliced( 1 ) + "]";
                    el = root->select_one( name.toUtf8().data() );
                }

                if( el )
                {
                    if( const int y = el->get_placement().y; y >= 0 )
                    {
                        auto* const vs = verticalScrollBar();
                        vs->setValue( std::min( y, vs->maximum() ) );
                    }
                }
            }
        }

        return;
    }

    // URL
    if( in_url != url_ )
    {
        url_ = in_url;

        container_->set_base_url( url_.toString().toUtf8().data() );
        setWindowTitle( url_.toString() );

        const auto old  = document_;
        const auto html = loadData( url_ );

        document_ = document::createFromString( html.data(), container_.get(), litehtml::master_css );
        render();

        if( old )
            QTimer::singleShot( 0, [old]{} );
    }
}

/**********************************************************************************************/
void qt_litehtml::setZoom( double in_zoom )
{
    if( in_zoom != zoom_ && zoom_ > 0. && zoom_ < 16. )
    {
        zoom_ = in_zoom;
        render();
    }
}

/**********************************************************************************************/
void qt_litehtml::zoomIn()
{
    // TODO(I.N.):
    if( zoom_ < 16. )
        setZoom( std::min( 16., zoom_ * 1.25 ) );
}

/**********************************************************************************************/
void qt_litehtml::zoomOut()
{
    // TODO(I.N.):
    if( zoom_ > 0.125 )
        setZoom( std::max( 0.125, zoom_ / 1.25 ) );
}


//////////////////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark internal methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
void qt_litehtml::onError( const QString& in_msg )
{
    qWarning() << in_msg;
}

/**********************************************************************************************/
QByteArray qt_litehtml::loadData( const QUrl& in_url )
{
    if( const auto it = cache_.find( in_url ); it != cache_.end() )
        return it.value();

    if( url_.isLocalFile() )
    {
        QFile f( url_.toLocalFile() );
        f.open( QIODevice::ReadOnly );

        auto r = f.readAll();
        cache_.insert( in_url, r );
        return r;
    }

    QNetworkAccessManager nm;

    auto* const reply = nm.get( QNetworkRequest( in_url ) );

    QEventLoop loop;
    connect( reply, &QNetworkReply::finished, &loop, &QEventLoop::quit );
    loop.exec( QEventLoop::ExcludeUserInputEvents );

    if( reply->error() != QNetworkReply::NoError )
        onError( reply->errorString() );

    reply->deleteLater();

    auto r = reply->readAll();
    cache_.insert( in_url, r );
    return r;
}

/**********************************************************************************************/
void qt_litehtml::render()
{
    if( document_ )
    {
        const int   vw = width() / zoom_;
        const int   cw = vw - style()->pixelMetric( QStyle::PM_ScrollBarExtent, {}, this ) - 2;
        auto* const hs = horizontalScrollBar();

        document_->render( cw );

        hs->setPageStep( vw );
        hs->setRange( 0, std::max( 0, document_->width() - cw ) );

        // -----------------------------

        const int   vh = viewport()->height() / zoom_;
        auto* const vs = verticalScrollBar();

        vs->setPageStep( vh );
        vs->setRange( 0, std::max( 0, document_->height() - vh ) );

        viewport()->update();
    }
}
