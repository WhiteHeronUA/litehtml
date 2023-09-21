#pragma once

// LITEHTML
#include "../../include/litehtml.h"

// QT
#include <QtCore/QPointer>
#include <QtCore/QUrl>
#include <QtGui/QPainterPath>
#include <QtWidgets/QWidget>

/**********************************************************************************************/
class QPainter;
class QPixmap;

/**********************************************************************************************/
class qt_litehtml;


/**********************************************************************************************/
class qt_container :
    public litehtml::document_container,
    public QObject
{
    public://////////////////////////////////////////////////////////////////////////

// document_container API:

        litehtml::uint_ptr      create_font(
                                    const char*             in_face_name,
                                    int                     in_size,
                                    int                     in_weight,
                                    litehtml::font_style    in_italic,
                                    unsigned int            in_decoration,
                                    litehtml::font_metrics* out_font_metrics ) override;

        void                    delete_font( litehtml::uint_ptr in_font ) override;
        int                     text_width( const char* in_text, litehtml::uint_ptr in_font ) override;
        void                    draw_text( litehtml::uint_ptr in_dc, const char* in_text, litehtml::uint_ptr in_font, litehtml::web_color in_color, const litehtml::position& in_pos ) override;
        int                     pt_to_px( int in_pt ) const override;
        int                     get_default_font_size() const override;
        const char*             get_default_font_name() const override;
        void                    draw_list_marker( litehtml::uint_ptr in_dc, const litehtml::list_marker& in_marker ) override;
        void                    load_image( const char* in_src, const char* in_base, bool in_redraw_on_ready ) override;
        void                    get_image_size( const char* in_src, const char* in_base, litehtml::size& out_size ) override;
        void                    draw_background( litehtml::uint_ptr in_dc, const std::vector<litehtml::background_paint>& in_back ) override;
        void                    draw_borders( litehtml::uint_ptr in_dc, const litehtml::borders& in_borders, const litehtml::position& in_draw_pos, bool in_root ) override;
        void                    set_caption( const char* in_caption ) override;
        void                    set_base_url( const char* in_base_url ) override;
        void                    link( const std::shared_ptr<litehtml::document>& in_doc, const litehtml::element::ptr& in_el ) override;
        void                    on_anchor_click( const char* in_url, const litehtml::element::ptr& in_el ) override;
        void                    set_cursor( const char* in_cursor ) override;
        void                    transform_text( std::string& io_text, litehtml::text_transform in_tt ) override;
        void                    import_css( std::string& in_text, const std::string& in_url, std::string& in_base ) override;
        void                    set_clip( const litehtml::position& in_pos, const litehtml::border_radiuses& in_radius ) override;
        void                    del_clip() override;
        void                    get_client_rect( litehtml::position& out_client ) const override;

        litehtml::element::ptr	create_element(
                                    const char* in_tag_name,
                                    const litehtml::string_map& in_attributes,
                                    const std::shared_ptr<litehtml::document>& in_doc ) override;

        void                    get_media_features( litehtml::media_features& out_media ) const override;
        void                    get_language( std::string& out_language, std::string& out_culture ) const override;
        std::string             resolve_color( const std::string& in_color ) const override;


    public://////////////////////////////////////////////////////////////////////////

// this class API:

        QFont                   default_font() const { return default_font_; }
        void                    set_default_font( const QFont& in_font ) { default_font_ = in_font; }

static  QPixmap                 render( const char* in_html, int in_width );


    protected://////////////////////////////////////////////////////////////////////////

virtual void                    on_error( const QString& in_msg );


    private://////////////////////////////////////////////////////////////////////////

        void                    apply_clip( QPainter* in_painter );
        QByteArray              load_data( const QUrl& in_url );
        QPixmap                 loaded_image( const QString& in_src, const QString& in_base ) const;
        QUrl                    resolve_url( const QString& in_src, const QString& in_base ) const;


    protected://////////////////////////////////////////////////////////////////////////

                                qt_container( QPaintDevice* in_paint_device );
                                qt_container( qt_litehtml* in_view );


    private://////////////////////////////////////////////////////////////////////////

// cached data:

        QList<QPainterPath>     clips_;
mutable QHash<QUrl, QPixmap>    images_;


// properties:

        QString                 base_url_;
        QString                 caption_;
        QFont                   default_font_;
mutable QByteArray              default_font_name_;


// references:

        QPaintDevice*           paint_device_ {};
        QPointer<qt_litehtml>   view_;


    protected://////////////////////////////////////////////////////////////////////////

        friend class qt_litehtml;
};
