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

/**********************************************************************************************/
using namespace litehtml;


//////////////////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark helper functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static inline QColor ToQColor( const web_color& inColor )
{
    return { inColor.red, inColor.green, inColor.blue, inColor.alpha };
}

/**********************************************************************************************/
static inline QPen ToQPen( const litehtml::border& inBorder )
{
    Qt::PenStyle style { Qt::NoPen };

    switch( inBorder.style )
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

    return { ToQColor( inBorder.color ), double( inBorder.width ), style };
}

/**********************************************************************************************/
static inline QRect ToQRect( const position& inPos )
{
    return { inPos.x, inPos.y, inPos.width, inPos.height };
}


//////////////////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark construction
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
qt_container::qt_container( QPainter* inPainter )
:
    mpPainter( inPainter )
{
}

/**********************************************************************************************/
qt_container::qt_container(
    QWidget*  inWidget,
    QPainter* inPainter )
:
    mpPainter ( inPainter ),
    mpWidget  ( inWidget )
{
}


//////////////////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark document_container
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
uint_ptr qt_container::create_font(
    const char*   inFaceName,
    int           inSize,
    int           inWeight,
    font_style    inItalic,
    unsigned int  inDecoration,
    font_metrics* outFontMetrics )
{
    auto* const r = new QFont;

    // Families
    QStringList families;
    for( auto family : QString( inFaceName ).split( ',' ) )
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
    r->setPixelSize( inSize );

    // Weight
    r->setWeight( QFont::Weight( inWeight ) );

    // Style
    r->setStyle( ( inItalic == font_style::font_style_italic ) ? QFont::StyleItalic : QFont::StyleNormal );

    // Decorations
    if( inDecoration == font_decoration_underline )
        r->setUnderline( true );
    else if( inDecoration == font_decoration_overline )
        r->setOverline( true );
    else if( inDecoration == font_decoration_linethrough )
        r->setStrikeOut( true );

    // Font Metrics
    if( outFontMetrics )
    {
        const QFontMetrics metrics( *r );

        outFontMetrics->ascent      = metrics.ascent();
        outFontMetrics->descent     = metrics.descent();
        outFontMetrics->draw_spaces = true;
        outFontMetrics->height      = metrics.height();
        outFontMetrics->x_height    = metrics.xHeight();
    }

    return reinterpret_cast<uint_ptr>( r );
}

/**********************************************************************************************/
void qt_container::delete_font( uint_ptr inFont )
{
    delete reinterpret_cast<QFont*>( inFont );
}

/**********************************************************************************************/
int qt_container::text_width( const char* inText, uint_ptr inFont )
{
    auto* const font = reinterpret_cast<QFont*>( inFont );

    return QFontMetrics( *font ).horizontalAdvance( inText );
}

/**********************************************************************************************/
void qt_container::draw_text( uint_ptr inDC, const char* inText, uint_ptr inFont, web_color inColor, const position& inPos )
{
    auto* const font    = reinterpret_cast<QFont*>( inFont );
    auto* const painter = reinterpret_cast<QPainter*>( inDC );

    painter->setFont( *font );
    painter->setPen( ToQColor( inColor ) );
    painter->drawText( ToQRect( inPos ), 0, inText );
}

/**********************************************************************************************/
int qt_container::pt_to_px( int inPt ) const
{
    if( mpPainter )
    {
        if( auto* const device = mpPainter->device() )
        {
            const auto ld = device->logicalDpiY();
            const auto pd = device->physicalDpiY();

            if( ld )
                return int( inPt * pd * 11. / ld / 12 + .5 );
        }
    }

    return 1;
}

/**********************************************************************************************/
int qt_container::get_default_font_size() const
{
    return mDefaultFont.pointSize();
}

/**********************************************************************************************/
const char* qt_container::get_default_font_name() const
{
    mDefaultFontName = mDefaultFont.family().toUtf8();

    return mDefaultFontName.constData();
}

/**********************************************************************************************/
void qt_container::draw_list_marker( uint_ptr inDC, const list_marker& inMarker )
{
    auto* const painter = reinterpret_cast<QPainter*>( inDC );

    // Image
    if( !inMarker.image.empty() )
    {
        const QPixmap pixmap = loaded_image( inMarker.image.c_str(), inMarker.baseurl );

        painter->drawPixmap( ToQRect( inMarker.pos ), pixmap );
    }
    // Type
    else
    {
        const auto c = ToQColor( inMarker.color );
        const auto r = ToQRect( inMarker.pos );

        switch( inMarker.marker_type )
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
void qt_container::load_image( const char* inSrc, const char* inBase, bool inRedrawOnReady )
{
    const auto url = resolve_url( inSrc, inBase );

    const std::lock_guard _ { mImagesLock };

    if( mImages.contains( url ) )
        return;

    if( url.isLocalFile() )
    {
        QPixmap image;
        image.load( url.fileName() );

        mImages.insert( url, image );
    }
    else
    {
        auto* const nm = new QNetworkAccessManager;
        nm->get( QNetworkRequest( url ) );

        QObject::connect( nm, &QNetworkAccessManager::finished, this, [inRedrawOnReady, nm, url, this]( QNetworkReply* inReply )
        {
            {
                const std::lock_guard _ { mImagesLock };

                QPixmap image;
                image.loadFromData( inReply->readAll() );

                mImages.insert( url, image );

            }

            if( inRedrawOnReady && mpWidget )
                mpWidget->update();

            inReply->deleteLater();
            nm->deleteLater();
        } );
    }
}

/**********************************************************************************************/
void qt_container::get_image_size( const char* inSrc, const char* inBase, size& outSize )
{
    const QPixmap image = loaded_image( inSrc, inBase );

    outSize.width  = image.width();
    outSize.height = image.height();
}

/**********************************************************************************************/
void qt_container::draw_background( uint_ptr inDC, const std::vector<background_paint>& inBack )
{
    auto* const painter = reinterpret_cast<QPainter*>( inDC );
    const auto& back    = inBack.back();

    painter->save();
    painter->setClipRect( ToQRect( back.clip_box ) );

    painter->setBrush( ToQColor( back.color ) );
    painter->setPen( Qt::NoPen );
    painter->drawRect( ToQRect( back.border_box ) );
    painter->restore();
}

/**********************************************************************************************/
void qt_container::draw_borders( uint_ptr inDC, const borders& inBorders, const position& inDrawPos, bool /*inRoot*/ )
{
    auto* const painter = reinterpret_cast<QPainter*>( inDC );

    if( const auto penTop = ToQPen( inBorders.top ); penTop.style() != Qt::NoPen )
    {
        painter->setPen( penTop );
        painter->drawLine( inDrawPos.left(), inDrawPos.top(), inDrawPos.right(), inDrawPos.top() );
    }

    if( const auto penLeft = ToQPen( inBorders.left ) ; penLeft.style() != Qt::NoPen )
    {
        painter->setPen( penLeft );
        painter->drawLine( inDrawPos.left(), inDrawPos.top(), inDrawPos.left(), inDrawPos.bottom() );
    }

    if( const auto penBottom = ToQPen( inBorders.bottom ) ; penBottom.style() != Qt::NoPen )
    {
        painter->setPen( penBottom );
        painter->drawLine( inDrawPos.left(), inDrawPos.bottom(), inDrawPos.right(), inDrawPos.bottom() );
    }

    if( const auto penRight = ToQPen( inBorders.right ) ; penRight.style() != Qt::NoPen )
    {
        painter->setPen( penRight );
        painter->drawLine( inDrawPos.right(), inDrawPos.top(), inDrawPos.right(), inDrawPos.bottom() );
    }
}


//////////////////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark internal methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
QPixmap qt_container::loaded_image( const QString& inSrc, const QString& inBase ) const
{
    const auto url = resolve_url( inSrc, inBase );

    const std::lock_guard _ { mImagesLock };

    return mImages.value( url );
}

/**********************************************************************************************/
QUrl qt_container::resolve_url( const QString& inSrc, const QString& inBase ) const
{
    QUrl src( inSrc );

    // Anchor
    if( inSrc.startsWith( '#' ) )
        return src;

    // Full URL with scheme
    if( !src.scheme().isEmpty() )
        return src;

    // Resolve
    const QUrl base( inBase.isEmpty() ? mBaseURL : inBase );

    return base.resolved( inSrc );
}

/**********************************************************************************************/
void qt_container::set_caption( const char* inCaption )
{
    mCaption = inCaption;
}

/**********************************************************************************************/
void qt_container::set_base_url( const char* inBaseUrl )
{
    mBaseURL = inBaseUrl;
}

/**********************************************************************************************/
void qt_container::link( const std::shared_ptr<document>& /*inDoc*/, const element::ptr& /*inEl*/ )
{
}

/**********************************************************************************************/
void qt_container::on_anchor_click( const char* /*inUrl*/, const litehtml::element::ptr& /*inEl*/ )
{
}

/**********************************************************************************************/
void qt_container::set_cursor( const char* /*in_cursor*/ )
{
}

/**********************************************************************************************/
void qt_container::transform_text( std::string& ioText, text_transform inTt )
{
    switch( inTt )
    {
        case text_transform_capitalize  :
        {
            auto s = QString::fromStdString( ioText ).toLower();
            if( !s.isEmpty() )
            {
                s.front() = s.front().toUpper();
                ioText = s.toUtf8().data();
            }
        }
        break;

        case text_transform_uppercase   :
        {
            ioText = QString::fromStdString( ioText ).toUpper().toUtf8().data();
        }
        break;

        case text_transform_lowercase   :
        {
            ioText = QString::fromStdString( ioText ).toLower().toUtf8().data();
        }
        break;

        case text_transform_none        :;
    }
}

/**********************************************************************************************/
void qt_container::import_css( std::string& outText, const std::string& inUrl, std::string& ioBase )
{
    const QUrl url = resolve_url( inUrl.c_str(), ioBase.c_str() );

    ioBase = url.toString( QUrl::None ).section( '/', 0, -2 ).toUtf8().data();

    if( url.isLocalFile() )
    {
        QFile f( url.toLocalFile() );
        f.open( QIODevice::ReadOnly );
        outText = f.readAll().data();
    }
    else
    {
        QNetworkAccessManager nm;
        auto* const reply = nm.get( QNetworkRequest( url ) );

        QEventLoop loop;
        connect( reply, &QNetworkReply::finished, &loop, &QEventLoop::quit );
        loop.exec();

        outText = reply->readAll().data();
        reply->deleteLater();
    }
}

/**********************************************************************************************/
void qt_container::set_clip( const position& inPos, const border_radiuses& inRadius )
{
    mClips.emplaceBack( inPos, inRadius );
}

/**********************************************************************************************/
void qt_container::del_clip()
{
    if( !mClips.empty() )
        mClips.pop_back();
}

/**********************************************************************************************/
void qt_container::get_client_rect( position& outClient ) const
{
    if( mpPainter )
    {
        if( auto* const device = mpPainter->device() )
        {
            outClient.x      = 0;
            outClient.y      = 0;
            outClient.width  = device->width();
            outClient.height = device->height();
        }
    }
}

/**********************************************************************************************/
element::ptr qt_container::create_element(
    const char* /*inTagName*/,
    const string_map& /*inAttributes*/,
    const std::shared_ptr<document>& /*inDoc*/ )
{
    return {};
}

/**********************************************************************************************/
void qt_container::get_media_features( media_features& outMedia ) const
{
    outMedia.type = litehtml::media_type_screen;
}

/**********************************************************************************************/
void qt_container::get_language( std::string& /*outLanguage*/, std::string& /*outCulture*/ ) const
{
}

/**********************************************************************************************/
std::string qt_container::resolve_color( const std::string& inColor ) const
{
    const QColor c( inColor.c_str() );
    if( c.isValid() )
        return c.name().toUtf8().data();

    return {};
}
