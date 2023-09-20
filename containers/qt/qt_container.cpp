// QT
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QUrl>
#include <QtGui/QPainter>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

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
static inline QColor ToQColor( const web_color& in_color )
{
    return { in_color.red, in_color.green, in_color.blue, in_color.alpha };
}

/**********************************************************************************************/
static inline QPen ToQPen( const litehtml::border& in_border )
{
    Qt::PenStyle style { Qt::NoPen };

    switch( in_border.style )
    {
        case border_style_dashed    : style = Qt::DashLine; break;
        case border_style_dotted    : style = Qt::DotLine; break;

        case border_style_solid     :
        case border_style_double    :
        case border_style_groove    :
        case border_style_ridge     :
        case border_style_inset     :
        case border_style_outset    : style = Qt::SolidLine; break;

        case border_style_hidden    :
        case border_style_none      :;
    }

    return { ToQColor( in_border.color ), double( in_border.width ), style };
}

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
qt_container::qt_container( QPaintDevice* in_paint_device )
:
    paint_device_( in_paint_device )
{
}

/**********************************************************************************************/
qt_container::qt_container( qt_litehtml* in_view )
:
    paint_device_ ( in_view ),
    view_         ( in_view )
{
}


//////////////////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark document_container
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
uint_ptr qt_container::create_font(
    const char*   in_face_name,
    int           in_size,
    int           in_weight,
    font_style    in_italic,
    unsigned int  in_decoration,
    font_metrics* out_font_metrics )
{
    auto* const r = new QFont;

    // Families
    QStringList families;
    for( auto family : QString( in_face_name ).split( ',' ) )
    {
        family = family.simplified();

        if( family.startsWith( '"' ) && family.endsWith( '"' ) )
            family = family.sliced( 1, family.length() - 2 );

        if( r->styleHint() == QFont::AnyStyle )
        {
            static const QHash<QString, QFont::StyleHint> hints
            {
                { "helvetica"       , QFont::Helvetica },
                { "sans-serif"      , QFont::SansSerif },
                { "times"           , QFont::Times },
                { "serif"           , QFont::Serif },
                { "courier"         , QFont::Courier },
                { "system"          , QFont::System },
                { "cursive"         , QFont::Cursive },
                { "monospace"       , QFont::Monospace }
            };

            if( const auto sh = hints.value( family.toLower(), QFont::AnyStyle ); sh != QFont::AnyStyle )
                r->setStyleHint( sh );
        }

        families.append( family );
    }

    r->setFamilies( families );

    // Size
    r->setPixelSize( in_size );

    // Weight
    r->setWeight( QFont::Weight( in_weight ) );

    // Style
    r->setStyle( ( in_italic == font_style::font_style_italic ) ? QFont::StyleItalic : QFont::StyleNormal );

    // Decorations
    if( in_decoration == font_decoration_underline )
        r->setUnderline( true );
    else if( in_decoration == font_decoration_overline )
        r->setOverline( true );
    else if( in_decoration == font_decoration_linethrough )
        r->setStrikeOut( true );

    // Font Metrics
    if( out_font_metrics )
    {
        const QFontMetrics metrics( *r );

        out_font_metrics->ascent      = metrics.ascent();
        out_font_metrics->descent     = metrics.descent();
        out_font_metrics->draw_spaces = true;
        out_font_metrics->height      = metrics.height();
        out_font_metrics->x_height    = metrics.xHeight();
    }

    return reinterpret_cast<uint_ptr>( r );
}

/**********************************************************************************************/
void qt_container::delete_font( uint_ptr in_font )
{
    delete reinterpret_cast<QFont*>( in_font );
}

/**********************************************************************************************/
int qt_container::text_width( const char* in_text, uint_ptr in_font )
{
    auto* const font = reinterpret_cast<QFont*>( in_font );

    return QFontMetrics( *font ).horizontalAdvance( in_text );
}

/**********************************************************************************************/
void qt_container::draw_text( uint_ptr in_dc, const char* in_text, uint_ptr in_font, web_color in_color, const position& in_pos )
{
    auto* const font    = reinterpret_cast<QFont*>( in_font );
    auto* const painter = reinterpret_cast<QPainter*>( in_dc );

    painter->setFont( *font );
    painter->setPen( ToQColor( in_color ) );
    painter->drawText( ToQRect( in_pos ), 0, in_text );
}

/**********************************************************************************************/
int qt_container::pt_to_px( int in_pt ) const
{
    if( paint_device_ )
    {
        const auto ld = paint_device_->logicalDpiY();
        const auto pd = paint_device_->physicalDpiY();

        if( ld )
            return int( in_pt * pd * 11. / ld / 12 + .5 );
    }

    return 1;
}

/**********************************************************************************************/
int qt_container::get_default_font_size() const
{
    return default_font_.pointSize();
}

/**********************************************************************************************/
const char* qt_container::get_default_font_name() const
{
    default_font_name_ = default_font_.family().toUtf8();

    return default_font_name_.constData();
}

/**********************************************************************************************/
void qt_container::draw_list_marker( uint_ptr in_dc, const list_marker& in_marker )
{
    auto* const painter = reinterpret_cast<QPainter*>( in_dc );

    // Image
    if( !in_marker.image.empty() )
    {
        const QPixmap pixmap = loaded_image( in_marker.image.c_str(), in_marker.baseurl );

        painter->drawPixmap( ToQRect( in_marker.pos ), pixmap );
    }
    // Type
    else
    {
        const auto c = ToQColor( in_marker.color );
        const auto r = ToQRect( in_marker.pos );

        switch( in_marker.marker_type )
        {
            case list_style_type_circle             :
            {
                painter->setBrush( Qt::NoBrush );
                painter->setPen( c );
                painter->drawEllipse( r );
            }
            break;

            case list_style_type_disc               :
            {
                painter->setBrush( c );
                painter->setPen( Qt::NoPen );
                painter->drawEllipse( r );
            }
            break;

            case list_style_type_square             :
            {
                painter->setBrush( c );
                painter->setPen( Qt::NoPen );
                painter->drawRect( r );
            }
            break;

            default:;
                // Other containers do nothing, need test
        }
    }
}

/**********************************************************************************************/
void qt_container::load_image( const char* in_src, const char* in_base, bool /*in_redraw_on_ready*/ )
{
    const auto url = resolve_url( in_src, in_base );

    if( images_.contains( url ) )
        return;

    QPixmap image;

    if( view_ )
    {
        image.loadFromData( view_->loadData( url ) );
    }
    else if( url.isLocalFile() )
    {
        image.load( url.fileName() );
    }
    else
    {
        QNetworkAccessManager nm;
        auto* const           reply = nm.get( QNetworkRequest( url ) );

        QEventLoop loop;
        connect( reply, &QNetworkReply::finished, &loop, &QEventLoop::quit );
        loop.exec();

        image.loadFromData( reply->readAll() );
        reply->deleteLater();
    }

    images_.insert( url, image );
}

/**********************************************************************************************/
void qt_container::get_image_size( const char* in_src, const char* in_base, size& out_size )
{
    const QPixmap image = loaded_image( in_src, in_base );

    out_size.width  = image.width();
    out_size.height = image.height();
}

/**********************************************************************************************/
void qt_container::draw_background( uint_ptr in_dc, const std::vector<background_paint>& in_back )
{
    auto* const painter = reinterpret_cast<QPainter*>( in_dc );

    // BACKGROUND COLOR

    {
        const auto& back = in_back.back();

        painter->save();
        painter->setClipRect( ToQRect( back.clip_box ) );

        painter->setBrush( ToQColor( back.color ) );
        painter->setPen( Qt::NoPen );
        painter->drawRect( ToQRect( back.border_box ) );
    }

    // IMAGES

    const int count = (int) in_back.size();
    for( int i = count - 1 ; i >= 0 ; --i )
    {
        const auto& back = in_back[ i ];

        if( !back.image.empty() && back.image_size.height > 0 && back.image_size.width > 0 )
        {
            if( const QPixmap pixmap = loaded_image( back.image.data(), back.baseurl.data() ); !pixmap.isNull() )
            {
                switch( back.repeat )
                {
                    case background_repeat_no_repeat    :
                    {
                        painter->drawPixmap(
                            back.position_x,
                            back.position_y,
                            back.image_size.width,
                            back.image_size.height,
                            pixmap );
                    }
                    break;

                    case background_repeat_repeat       :
                    {
                        for( int x = back.border_box.x; x <= back.border_box.right(); x += back.image_size.width )
                        {
                            for( int y = back.border_box.y; y <= back.border_box.bottom(); y += back.image_size.height )
                            {
                                painter->drawPixmap(
                                    x,
                                    y,
                                    back.image_size.width,
                                    back.image_size.height,
                                    pixmap );
                            }
                        }
                    }
                    break;

                    case background_repeat_repeat_x     :
                    {
                        for( int x = back.border_box.x; x <= back.border_box.right(); x += back.image_size.width )
                        {
                            painter->drawPixmap(
                                x,
                                back.border_box.y,
                                back.image_size.width,
                                back.image_size.height,
                                pixmap );
                        }
                    }
                    break;

                    case background_repeat_repeat_y     :
                    {
                        for( int y = back.border_box.y; y <= back.border_box.bottom(); y += back.image_size.height )
                        {
                            painter->drawPixmap(
                                back.border_box.x,
                                y,
                                back.image_size.width,
                                back.image_size.height,
                                pixmap );
                        }
                    }
                    break;
                }
            }
        }
    }

    painter->restore();
}

/**********************************************************************************************/
void qt_container::draw_borders( uint_ptr in_dc, const borders& in_borders, const position& in_draw_pos, bool /*in_root*/ )
{
    auto* const painter = reinterpret_cast<QPainter*>( in_dc );

    if( const auto penTop = ToQPen( in_borders.top ); penTop.style() != Qt::NoPen )
    {
        painter->setPen( penTop );
        painter->drawLine( in_draw_pos.left(), in_draw_pos.top(), in_draw_pos.right(), in_draw_pos.top() );
    }

    if( const auto penLeft = ToQPen( in_borders.left ) ; penLeft.style() != Qt::NoPen )
    {
        painter->setPen( penLeft );
        painter->drawLine( in_draw_pos.left(), in_draw_pos.top(), in_draw_pos.left(), in_draw_pos.bottom() );
    }

    if( const auto penBottom = ToQPen( in_borders.bottom ) ; penBottom.style() != Qt::NoPen )
    {
        painter->setPen( penBottom );
        painter->drawLine( in_draw_pos.left(), in_draw_pos.bottom(), in_draw_pos.right(), in_draw_pos.bottom() );
    }

    if( const auto penRight = ToQPen( in_borders.right ) ; penRight.style() != Qt::NoPen )
    {
        painter->setPen( penRight );
        painter->drawLine( in_draw_pos.right(), in_draw_pos.top(), in_draw_pos.right(), in_draw_pos.bottom() );
    }
}


//////////////////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark static methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
QPixmap qt_container::render( const char* in_html, int in_width )
{
    QPixmap r;

    {
        QPixmap      pmp;
        qt_container ctr { &pmp };
        const auto   doc { document::createFromString( in_html, &ctr ) };

        doc->render( in_width );

        r = QPixmap( in_width, doc->content_height() );
        r.fill( Qt::white );

        QPainter p( &r );
        doc->draw( litehtml::uint_ptr( &p ), 0, 0, nullptr );
    }

    return r;
}


//////////////////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark internal methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
QPixmap qt_container::loaded_image( const QString& in_src, const QString& in_base ) const
{
    const auto url = resolve_url( in_src, in_base );

    return images_.value( url );
}

/**********************************************************************************************/
QUrl qt_container::resolve_url( const QString& in_src, const QString& in_base ) const
{
    // Anchor
    QUrl src( in_src );
    if( in_src.startsWith( '#' ) )
        return src;

    // Net-path
    const QUrl base( in_base.isEmpty() ? base_url_ : in_base );
    if( in_src.startsWith( "//" ) )
    {
        if( in_base.isEmpty() )
            return { "https:" + in_src };

        return { QUrl( in_base ).scheme() + ":" + in_src };
    }

    // Full URL with scheme
    if( !src.scheme().isEmpty() )
        return src;

    // Resolve
    return base.resolved( in_src );
}

/**********************************************************************************************/
void qt_container::set_caption( const char* in_caption )
{
    caption_ = in_caption;
}

/**********************************************************************************************/
void qt_container::set_base_url( const char* in_base_url )
{
    base_url_ = in_base_url;
}

/**********************************************************************************************/
void qt_container::link( const std::shared_ptr<document>& /*in_doc*/, const element::ptr& /*in_el*/ )
{
}

/**********************************************************************************************/
void qt_container::on_anchor_click( const char* in_url, const litehtml::element::ptr& /*in_el*/ )
{
    if( view_ )
        view_->setURL( resolve_url( in_url, base_url_ ) );
}

/**********************************************************************************************/
void qt_container::set_cursor( const char* in_cursor )
{
    if( view_ )
    {
        QCursor cr;

        // TODO(I.N.): https://developer.mozilla.org/en-US/docs/Web/CSS/cursor

        if( !strcmp( in_cursor, "pointer" ) )
            cr = Qt::PointingHandCursor;
        else if( !strcmp( in_cursor, "text" ) )
            cr = Qt::IBeamCursor;

        view_->viewport()->setCursor( cr );
    }
}

/**********************************************************************************************/
void qt_container::transform_text( std::string& io_text, text_transform in_tt )
{
    switch( in_tt )
    {
        case text_transform_capitalize  :
        {
            auto s = QString::fromStdString( io_text ).toLower();
            if( !s.isEmpty() )
            {
                s.front() = s.front().toUpper();
                io_text = s.toUtf8().data();
            }
        }
        break;

        case text_transform_uppercase   :
        {
            io_text = QString::fromStdString( io_text ).toUpper().toUtf8().data();
        }
        break;

        case text_transform_lowercase   :
        {
            io_text = QString::fromStdString( io_text ).toLower().toUtf8().data();
        }
        break;

        case text_transform_none        :;
    }
}

/**********************************************************************************************/
void qt_container::import_css( std::string& out_text, const std::string& in_url, std::string& io_base )
{
    QString base = io_base.c_str();
    if( base.isEmpty() )
        base = base_url_;

    const QUrl url = resolve_url( in_url.c_str(), base );

    io_base = url.toString( QUrl::None ).section( '/', 0, -2 ).toUtf8().data();

    if( view_ )
    {
        out_text = view_->loadData( url ).data();
    }
    else if( url.isLocalFile() )
    {
        QFile f( url.toLocalFile() );
        f.open( QIODevice::ReadOnly );
        out_text = f.readAll().data();
    }
    else
    {
        QNetworkAccessManager nm;
        auto* const reply = nm.get( QNetworkRequest( url ) );

        QEventLoop loop;
        connect( reply, &QNetworkReply::finished, &loop, &QEventLoop::quit );
        loop.exec();

        out_text = reply->readAll().data();
        reply->deleteLater();
    }
}

/**********************************************************************************************/
void qt_container::set_clip( const position& in_os, const border_radiuses& in_radius )
{
    clips_.emplaceBack( in_os, in_radius );
}

/**********************************************************************************************/
void qt_container::del_clip()
{
    if( !clips_.empty() )
        clips_.pop_back();
}

/**********************************************************************************************/
void qt_container::get_client_rect( position& out_client ) const
{
    if( paint_device_ )
    {
        out_client.x      = 0;
        out_client.y      = 0;
        out_client.width  = paint_device_->width();
        out_client.height = paint_device_->height();
    }
}

/**********************************************************************************************/
element::ptr qt_container::create_element(
    const char* /*in_tag_name*/,
    const string_map& /*in_attributes*/,
    const std::shared_ptr<document>& /*in_doc*/ )
{
    return {};
}

/**********************************************************************************************/
void qt_container::get_media_features( media_features& out_media ) const
{
    out_media.type = litehtml::media_type_screen;
}

/**********************************************************************************************/
void qt_container::get_language( std::string& /*out_language*/, std::string& /*out_culture*/ ) const
{
}

/**********************************************************************************************/
std::string qt_container::resolve_color( const std::string& in_color ) const
{
    const QColor c( in_color.c_str() );
    if( c.isValid() )
        return c.name().toUtf8().data();

    return {};
}
