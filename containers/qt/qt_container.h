#pragma once

// STD
#include <mutex>

// LITEHTML
#include "../../include/litehtml.h"

// QT
#include <QtCore/QPointer>
#include <QtCore/QUrl>
#include <QtWidgets/QWidget>

/**********************************************************************************************/
class QPainter;
class QPixmap;


/**********************************************************************************************/
class qt_container final :
    public litehtml::document_container,
    public QObject
{
    public://////////////////////////////////////////////////////////////////////////

                                qt_container( QPainter* inPainter );

                                qt_container(
                                    QWidget*  inWidget,
                                    QPainter* inPainter );


    public://////////////////////////////////////////////////////////////////////////

// document_container API:

        litehtml::uint_ptr      create_font(
                                    const char*             inFaceName,
                                    int                     inSize,
                                    int                     inWeight,
                                    litehtml::font_style    inItalic,
                                    unsigned int            inDecoration,
                                    litehtml::font_metrics* outFontMetrics ) override;

        void                    delete_font( litehtml::uint_ptr inFont ) override;
        int                     text_width( const char* inText, litehtml::uint_ptr inFont ) override;
        void                    draw_text( litehtml::uint_ptr inDC, const char* inText, litehtml::uint_ptr inFont, litehtml::web_color inColor, const litehtml::position& inPos ) override;
        int                     pt_to_px( int inPt ) const override;
        int                     get_default_font_size() const override;
        const char*             get_default_font_name() const override;
        void                    draw_list_marker( litehtml::uint_ptr inDC, const litehtml::list_marker& inMarker ) override;
        void                    load_image( const char* inSrc, const char* inBase, bool inRedrawOnReady ) override;
        void                    get_image_size( const char* inSrc, const char* inBase, litehtml::size& outSize ) override;
        void                    draw_background( litehtml::uint_ptr inDC, const std::vector<litehtml::background_paint>& inBack ) override;
        void                    draw_borders( litehtml::uint_ptr inDC, const litehtml::borders& inBorders, const litehtml::position& inDrawPos, bool inRoot ) override;
        void                    set_caption( const char* inCaption ) override;
        void                    set_base_url( const char* inBaseUrl ) override;
        void                    link( const std::shared_ptr<litehtml::document>& inDoc, const litehtml::element::ptr& inEl ) override;
        void                    on_anchor_click( const char* inUrl, const litehtml::element::ptr& inEl ) override;
        void                    set_cursor( const char* in_cursor ) override;
        void                    transform_text( std::string& ioText, litehtml::text_transform inTt ) override;
        void                    import_css( std::string& inText, const std::string& inUrl, std::string& inBase ) override;
        void                    set_clip( const litehtml::position& inPos, const litehtml::border_radiuses& inRadius ) override;
        void                    del_clip() override;
        void                    get_client_rect( litehtml::position& outClient ) const override;

        litehtml::element::ptr	create_element(
                                    const char* inTagName,
                                    const litehtml::string_map& inAttributes,
                                    const std::shared_ptr<litehtml::document>& inDoc ) override;

        void                    get_media_features( litehtml::media_features& outMedia ) const override;
        void                    get_language( std::string& outLanguage, std::string& outCulture ) const override;
        std::string             resolve_color( const std::string& inColor ) const override;


    private://////////////////////////////////////////////////////////////////////////

// this class API:

        QPixmap                 loaded_image( const QString& inSrc, const QString& inBase ) const;
        QUrl                    resolve_url( const QString& inSrc, const QString& inBase ) const;


    private://////////////////////////////////////////////////////////////////////////

        using clips_t = QList<std::pair<litehtml::position, litehtml::border_radiuses>>;


    private://////////////////////////////////////////////////////////////////////////

// cached data:

        clips_t                 mClips;
mutable QHash<QUrl, QPixmap>    mImages;
mutable std::mutex              mImagesLock;


// properties:

        QString                 mBaseURL;
        QString                 mCaption;
        QFont                   mDefaultFont;
mutable QByteArray              mDefaultFontName;


// references:

        QPainter*               mpPainter {};
        QPointer<QWidget>       mpWidget;
};
