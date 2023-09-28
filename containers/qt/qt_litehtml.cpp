// QT
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QShortcut>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QStyle>

// QTLITEHTML
#include "qt_container.h"
#include "qt_litehtml.h"

// LITEHTML
#include "../../include/litehtml/render_item.h"

/**********************************************************************************************/
using namespace litehtml;


//////////////////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark helper functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static inline QRect to_qrect( const position& in_pos )
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
        new QShortcut( QKeySequence::Copy, this ),
        &QShortcut::activated,
        this,
        &qt_litehtml::copy );

    connect(
        new QShortcut( QKeySequence::SelectAll, this ),
        &QShortcut::activated,
        this,
        &qt_litehtml::selectAll );

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
void qt_litehtml::contextMenuEvent( QContextMenuEvent* in_event )
{
    QMenu m;

    // Copy

    auto* const ac = m.addAction( QObject::tr("Copy") );
    ac->setEnabled( !selection_.empty() );

    connect(
        ac,
        &QAction::triggered,
        this,
        &qt_litehtml::copy
    );

    // Copy Link

    if( document_ )
    {
        const auto pos = documentPos( in_event->pos() );

        if( const auto el = document_->root_render_item()->get_element_by_point( pos.x(), pos.y(), pos.x(), pos.y() ) )
        {
            if( const char* href = el->get_attr( "href" ); href && *href )
            {
                connect(
                    m.addAction( QObject::tr("Copy Link") ),
                    &QAction::triggered,
                    [s=href] { QApplication::clipboard()->setText( s ); }
                );
            }
        }
    }

    // Select All

    connect(
        m.addAction( QObject::tr("Select All") ),
        &QAction::triggered,
        this,
        &qt_litehtml::selectAll
    );

    m.exec( in_event->globalPos() );
}

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
            QRect rr = to_qrect( it ).translated( -spos );

            rr.moveTopLeft( rr.topLeft() * zoom_ );
            rr.setSize( rr.size() * zoom_ + QSize( 1, 1 ) );

            viewport()->update( rr );
        }
    }
}

/**********************************************************************************************/
void qt_litehtml::mouseDoubleClickEvent( QMouseEvent* in_event )
{
    const auto dpos = documentPos( in_event->pos() );

    updateSelection( dpos, dpos );
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

        if( drag_start_.x() >= 0 && selection_start_.x() < 0 && ( drag_start_ - dpos ).manhattanLength() >= QApplication::startDragDistance() )
            selection_start_ = drag_start_;

        updateSelection( selection_start_, dpos );

        for( const auto& it : redraw )
        {
            QRect rr = to_qrect( it ).translated( -spos );

            rr.moveTopLeft( rr.topLeft() * zoom_ );
            rr.setSize( rr.size() * zoom_ + QSize( 1, 1 ) );

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

            drag_start_ = dpos;

            if( !inSelectionContains( dpos.x(), dpos.y() ) )
                clearSelection();

            position::vector redraw;
            document_->on_lbutton_down( dpos.x(), dpos.y(), dpos.x(), dpos.y(), redraw );

            for( const auto& it : redraw )
            {
                QRect rr = to_qrect( it ).translated( -spos );

                rr.moveTopLeft( rr.topLeft() * zoom_ );
                rr.setSize( rr.size() * zoom_ + QSize( 1, 1 ) );

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
    if( const auto doc = document_ )
    {
        if( in_event->button() == Qt::LeftButton )
        {
            const auto spos = QPoint( horizontalScrollBar()->value(), verticalScrollBar()->value() );
            const auto vpos = viewport()->mapFromParent( in_event->pos() ) / zoom_;
            const auto dpos = vpos + spos;

            position::vector redraw;
            doc->on_lbutton_up( dpos.x(), dpos.y(), dpos.x(), dpos.y(), redraw );

            updateSelection( selection_start_, dpos );

            drag_start_      = QPoint( -1, -1 );
            selection_start_ = QPoint( -1, -1 );

            for( const auto& it : redraw )
            {
                QRect rr = to_qrect( it ).translated( -spos );

                rr.moveTopLeft( rr.topLeft() * zoom_ );
                rr.setSize( rr.size() * zoom_ + QSize( 1, 1 ) );

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

        p.setPen( Qt::NoPen );
        p.setBrush( palette().color( QPalette::Highlight ) );

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
void qt_litehtml::copy() const
{
    QApplication::clipboard()->setText( selectedText() );
}

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
void qt_litehtml::setHtml( const QString& in_html )
{
    if( in_html != html_ )
    {
        document_ = document::createFromString( in_html.toUtf8().data(), container_.get(), masterCSS(), default_css_.toUtf8().data() );
        html_     = in_html;
        url_      = QUrl();

        render();
    }
}

/**********************************************************************************************/
void qt_litehtml::selectAll()
{
    for( const auto& kv1 : selection_ )
    {
        for( const auto& kv2 : kv1.second )
            kv2.second.item_->set_selected( false );
    }

    selection_.clear();

    if( document_ )
    {
        const std::function<void(const std::shared_ptr<render_item>&, int, int)> process = [&]( const std::shared_ptr<render_item>& in_parent, int in_x, int in_y )
        {
            const QRect pr = to_qrect( in_parent->pos() ).translated( in_x, in_y );

            const auto& children = in_parent->children();
            const auto  el       = in_parent->src_el();

            if( el->is_text() || el->is_space() )
            {
                in_parent->set_selected( true );
                selection_[ pr.y() ].emplace( pr.x(), selection_item { pr.width(), pr.height(), in_parent } );

                updateDocumentRect( pr.adjusted( 0, 0, 1, 1 ) );
            }
            else
            {
                for( const auto& it : children )
                    process( it, pr.x(), pr.y() );
            }
        };

        process( document_->root_render_item(), 0, 0 );
    }

    viewport()->update();
}

/**********************************************************************************************/
QString qt_litehtml::selectedText() const
{
    QString r;

    for( const auto& kv1 : selection_ )
    {
        if( !r.isEmpty() && r.back() != '\n' )
        {
            while( !r.isEmpty() && r.back().isSpace() )
                r.removeLast();

            r.push_back( '\n' );
        }

        for( const auto& kv2 : kv1.second )
        {
            const auto el = kv2.second.item_->src_el();

            std::string s;
            el->get_text( s );

            if( !r.isEmpty() && !r.back().isSpace() )
                r.append( ' ' );

            r.append( QString::fromUtf8( s.c_str(), s.size() ) .trimmed() );
        }
    }

    return r;
}

/**********************************************************************************************/
void qt_litehtml::setURL( const QUrl& in_url )
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

        document_ = document::createFromString( html.data(), container_.get(), masterCSS(), default_css_.toUtf8().data() );
        html_     = QString::fromUtf8( html );

        render();

        if( old )
            QTimer::singleShot( 0, [old]{} );
    }
}

/**********************************************************************************************/
void qt_litehtml::setZoom( double in_zoom )
{
    if( in_zoom != zoom_ && in_zoom > 0. && in_zoom <= 16. )
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
void qt_litehtml::clearSelection()
{
    for( const auto& kv1 : selection_ )
    {
        for( const auto& kv2 : kv1.second )
        {
            kv2.second.item_->set_selected( false );
            updateDocumentRect({ kv2.first, kv1.first, kv2.second.width_ + 1, kv2.second.height_ + 1 });
        }
    }

    selection_.clear();
}

/**********************************************************************************************/
QPoint qt_litehtml::documentPos( const QPoint& in_pos ) const
{
    const auto spos = QPoint( horizontalScrollBar()->value(), verticalScrollBar()->value() );
    const auto vpos = viewport()->mapFromParent( in_pos ) / zoom_;

    return vpos + spos;
}

/**********************************************************************************************/
QRect qt_litehtml::clientRect() const
{
    const auto spos  = QPoint( horizontalScrollBar()->value(), verticalScrollBar()->value() );
    const auto vpos  = viewport()->mapFromParent( QPoint() ) / zoom_;
    const auto dpos  = vpos + spos;
    const auto csize = viewport()->size() / zoom_;

    return { dpos, csize + QSize( 1, 1 ) };
}

/**********************************************************************************************/
bool qt_litehtml::inSelectionContains( int in_x, int in_y ) const
{
    for( const auto& kv1 : selection_ )
    {
        for( const auto& kv2 : kv1.second )
        {
            const QRect r( kv2.first, kv1.first, kv2.second.width_, kv2.second.height_ );

            if( r.contains( in_x, in_y ) )
                return true;
        }
    }

    return false;
}

/**********************************************************************************************/
const char* qt_litehtml::masterCSS() const
{
    if( doc_margin_ != 8 )
    {
        if( doc_margin_ != master_css_margin_ )
        {
            master_css_ = std::string( master_css );
            master_css_.append( "\nbody { display:block; margin:" );
            master_css_.append( std::to_string( doc_margin_ ) );
            master_css_.append( "px; }" );

            master_css_margin_ = doc_margin_;
        }

        return master_css_.c_str();
    }

    return master_css;
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

/**********************************************************************************************/
void qt_litehtml::updateDocumentRect( const QRect& in_rect )
{
    const auto spos = QPoint( horizontalScrollBar()->value(), verticalScrollBar()->value() );

    QRect r = in_rect.translated( -spos );

    r.moveTopLeft( r.topLeft() * zoom_ );
    r.setSize( r.size() * zoom_ + QSize( 1, 1 ) );

    viewport()->update( r );
}

/**********************************************************************************************/
void qt_litehtml::updateSelection( QPoint in_from, QPoint in_to )
{
    if( in_from.x() < 0 || in_from.y() < 0 )
        return;

    const QRect r = QRect( in_from, in_to ).normalized();

    clearSelection();

    if( document_ )
    {
        const std::function<void(const std::shared_ptr<render_item>&, int, int)> process = [&]( const std::shared_ptr<render_item>& in_parent, int in_x, int in_y )
        {
            const QRect pr = to_qrect( in_parent->pos() ).translated( in_x, in_y );
            if( !pr.intersects( r ) && !pr.isEmpty() )
                return;

            const auto& children = in_parent->children();
            const auto  el       = in_parent->src_el();

            if( el->is_text() || el->is_space() )
            {
                in_parent->set_selected( true );
                selection_[ pr.y() ].emplace( pr.x(), selection_item { pr.width(), pr.height(), in_parent } );

                updateDocumentRect( pr.adjusted( 0, 0, 1, 1 ) );
            }
            else
            {
                for( const auto& it : children )
                    process( it, pr.x(), pr.y() );
            }
        };

        process( document_->root_render_item(), 0, 0 );
    }
}
