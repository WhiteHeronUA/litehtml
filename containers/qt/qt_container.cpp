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
static QString dequote( const QString& in_str )
{
    if( in_str.length() > 1 )
    {
        if( in_str.startsWith( '"' ) && in_str.endsWith( '"' ) )
            return in_str.sliced( 1, in_str.length() - 2 );

        if( in_str.startsWith( '\'' ) && in_str.endsWith( '\'' ) )
            return in_str.sliced( 1, in_str.length() - 2 );
    }

    return in_str;
}

/**********************************************************************************************/
static inline QColor to_qcolor( const web_color& in_color )
{
    return { in_color.red, in_color.green, in_color.blue, in_color.alpha };
}

/**********************************************************************************************/
static QPainterPath to_qpath(
    const position&        in_pos,
    const border_radiuses& in_radius )
{
    QPainterPath path;

    //   _____________
    path.moveTo( in_pos.x + in_radius.top_left_x * 2, in_pos.y );
    path.lineTo( in_pos.right() - in_radius.top_right_x * 2, in_pos.y );

    //                 \-
    path.arcTo( in_pos.right() - in_radius.top_right_x * 2, in_pos.y, in_radius.top_right_x * 2, in_radius.top_right_y * 2, 90, -90 );

    //                  |
    path.lineTo( in_pos.right(), in_pos.bottom() - in_radius.bottom_right_y * 2 );

    //                 /-
    path.arcTo( in_pos.right() - in_radius.bottom_right_x * 2, in_pos.bottom() - in_radius.bottom_right_y * 2, in_radius.bottom_right_x * 2, in_radius.bottom_right_y * 2, 0, -90 );

    //   -------------
    path.lineTo( in_pos.x + in_radius.bottom_left_x * 2, in_pos.bottom() );

    // \-
    path.arcTo( in_pos.x, in_pos.bottom() - in_radius.bottom_left_y * 2, in_radius.bottom_left_x * 2, in_radius.bottom_left_y * 2, 270, -90 );

    // |
    path.lineTo( in_pos.x, in_pos.y + in_radius.top_left_y * 2 );

    // /
    path.arcTo( in_pos.x, in_pos.y, in_radius.top_left_x * 2, in_radius.top_left_y * 2, 180, -90 );

    path.closeSubpath();
    return path;
}

/**********************************************************************************************/
static inline QPen to_qpen( const litehtml::border& in_border )
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

    return { to_qcolor( in_border.color ), double( in_border.width ), style };
}

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

        if( family.length() > 1 && family.startsWith( '"' ) && family.endsWith( '"' ) )
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
    r->setPointSize( in_size );

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

    painter->save();

    apply_clip( painter );

    painter->setFont( *font );
    painter->setPen( to_qcolor( in_color ) );
    painter->drawText( to_qrect( in_pos ), 0, in_text );

    painter->restore();
}

/**********************************************************************************************/
int qt_container::pt_to_px( int in_pt ) const
{
    if( paint_device_ )
        return int( in_pt / 72. * paint_device_->physicalDpiX() );

    return int( in_pt / 72. * 96. );
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

    painter->save();

    apply_clip( painter );

    // Image
    if( !in_marker.image.empty() )
    {
        const QPixmap pixmap = loaded_image( in_marker.image.c_str(), in_marker.baseurl );

        if( !pixmap.isNull() )
            painter->drawPixmap( to_qrect( in_marker.pos ), pixmap );
    }
    // Type
    else
    {
        const auto c = to_qcolor( in_marker.color );
        const auto r = to_qrect( in_marker.pos );

        switch( in_marker.marker_type )
        {
            case list_style_type_circle                 :
            {
                painter->setBrush( Qt::NoBrush );
                painter->setPen( c );
                painter->drawEllipse( r );
            }
            break;

            case list_style_type_disc                   :
            {
                painter->setBrush( c );
                painter->setPen( Qt::NoPen );
                painter->drawEllipse( r );
            }
            break;

            case list_style_type_square                 :
            {
                painter->setBrush( c );
                painter->setPen( Qt::NoPen );
                painter->drawRect( r );
            }
            break;

            // Other types handled by litehtml itself
            default:;
        }
    }

    painter->restore();
}

/**********************************************************************************************/
void qt_container::load_image( const char* in_src, const char* in_base, bool /*in_redraw_on_ready*/ )
{
    const auto url = resolve_url( in_src, in_base );

    if( images_.contains( url ) )
        return;

    QPixmap image;
    if( !image.loadFromData( load_data( url ) ) )
        on_error( QString( "'load_image' failed: " ) + url.toString() );

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

    // COLOR

    {
        const auto& back = in_back.back();

        painter->save();

        apply_clip( painter );
        painter->setClipRect( to_qrect( back.clip_box ), Qt::IntersectClip );

        painter->setBrush( to_qcolor( back.color ) );
        painter->setPen( Qt::NoPen );

        auto path = to_qpath( back.border_box, back.border_radius );
        if( painter->renderHints() & QPainter::Antialiasing )
            path.translate( .5F, .5F );

        painter->drawPath( path );

        painter->restore();
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
                painter->save();

                apply_clip( painter );
                painter->setClipRect( to_qrect( back.clip_box ), Qt::IntersectClip );

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

                painter->restore();
            }
        }
    }
}

/**********************************************************************************************/
void qt_container::draw_borders( uint_ptr in_dc, const borders& in_borders, const position& in_pos, bool /*in_root*/ )
{
    auto* const painter = reinterpret_cast<QPainter*>( in_dc );

    painter->save();

    apply_clip( painter );

    if( painter->renderHints() & QPainter::Antialiasing )
        painter->translate( .5F, .5F );

    const auto& radius = in_borders.radius;

    //  /_____________\-
    if( const auto penTop = to_qpen( in_borders.top ); penTop.style() != Qt::NoPen )
    {
        painter->setPen( penTop );

        painter->drawArc( in_pos.x, in_pos.y, radius.top_left_x * 2, radius.top_left_y * 2, 180 * 16, -90 * 16 );
        painter->drawLine( in_pos.x + radius.top_left_x, in_pos.y, in_pos.right() - radius.top_right_x, in_pos.y );
        painter->drawArc( in_pos.right() - radius.top_right_x * 2, in_pos.y, radius.top_right_x * 2, radius.top_right_y * 2, 90 * 16, -90 * 16 );
    }

    //  \_____________/
    if( const auto penBottom = to_qpen( in_borders.bottom ); penBottom.style() != Qt::NoPen )
    {
        painter->setPen( penBottom );

        painter->drawArc( in_pos.x, in_pos.bottom() - radius.bottom_left_y * 2, radius.bottom_left_x * 2, radius.bottom_left_y * 2, 270 * 16, -90 * 16 );
        painter->drawLine( in_pos.x + radius.bottom_left_x, in_pos.bottom(), in_pos.right() - radius.bottom_right_x, in_pos.bottom() );
        painter->drawArc( in_pos.right() - radius.bottom_right_x * 2, in_pos.bottom() - radius.bottom_right_y * 2, radius.bottom_right_x * 2, radius.bottom_right_y * 2, 0, -90 * 16 );
    }

    // |
    if( const auto penLeft = to_qpen( in_borders.left ); penLeft.style() != Qt::NoPen )
    {
        painter->setPen( penLeft );

        painter->drawLine( in_pos.x, in_pos.y + radius.top_left_y, in_pos.x, in_pos.bottom() - radius.bottom_left_y );
    }

    //                  |
    if( const auto penRight = to_qpen( in_borders.right ); penRight.style() != Qt::NoPen )
    {
        painter->setPen( penRight );

        painter->drawLine( in_pos.right(), in_pos.y + radius.top_right_y, in_pos.right(), in_pos.bottom() - radius.bottom_right_y );
    }

    painter->restore();
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
void qt_container::on_error( const QString& in_msg )
{
    qWarning() << in_msg;
}

/**********************************************************************************************/
void qt_container::apply_clip( QPainter* in_painter )
{
    for( const auto& ipath : clips_ )
        in_painter->setClipPath( ipath, Qt::IntersectClip );
}

/**********************************************************************************************/
QByteArray qt_container::load_data( const QUrl& in_url )
{
    if( view_ )
        return view_->loadData( in_url );

    if( in_url.isLocalFile() )
    {
        QFile f( in_url.toLocalFile() );
        f.open( QIODevice::ReadOnly );

        return f.readAll();
    }

    QNetworkAccessManager nm;

    auto* const reply = nm.get( QNetworkRequest( in_url ) );

    QEventLoop loop;
    connect( reply, &QNetworkReply::finished, &loop, &QEventLoop::quit );
    loop.exec( QEventLoop::ExcludeUserInputEvents );

    if( reply->error() != QNetworkReply::NoError )
        on_error( reply->errorString() );

    reply->deleteLater();
    return reply->readAll();
}

/**********************************************************************************************/
QPixmap qt_container::loaded_image( const QString& in_src, const QString& in_base ) const
{
    const auto url = resolve_url( in_src, in_base );

    return images_.value( url );
}

/**********************************************************************************************/
QUrl qt_container::resolve_url( const QString& in_src, const QString& in_base ) const
{
    const QString prepared_src = dequote( in_src );

    // Anchor
    QUrl src( prepared_src );
    if( in_src.startsWith( '#' ) )
        return src;

    QUrl base( dequote( in_base.isEmpty() ? base_url_ : in_base ) );

    // Net-path
    if( prepared_src.startsWith( "//" ) )
    {
        QString scheme = base.scheme();
        if( scheme.isEmpty() )
            scheme = "https:";

        return { scheme + ":" + prepared_src };
    }

    // Invalid base or full source
    if( !base.isValid() || !src.scheme().isEmpty() )
        return src;

    // Absolute
    if( prepared_src.startsWith( '/' ) )
        base = QUrl( base.toString( QUrl::RemovePath | QUrl::RemoveQuery ) + prepared_src );
    // Relative
    else
        base = QUrl( base.toString() + "/" + prepared_src );

    return base.adjusted( QUrl::FullyEncoded | QUrl::NormalizePathSegments );
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

    io_base  = url.toString( QUrl::None ).section( '/', 0, -2 ).toUtf8().data();
    out_text = load_data( url ).data();
}

/**********************************************************************************************/
void qt_container::set_clip( const position& in_os, const border_radiuses& in_radius )
{
    //clips_.emplaceBack( in_os, in_radius );
    (void) in_os;
    (void) in_radius;
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
