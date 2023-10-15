#include "../include/litehtml/html.h"
#include "../include/litehtml/el_style.h"
#include "../include/litehtml/document_litehtml.h"


litehtml::el_style::el_style(const std::shared_ptr<document>& doc) : element(doc)
{

}

void litehtml::el_style::parse_attributes()
{
    string text;

    for(auto& el : m_children)
    {
        el->get_text(text);
    }
    get_document()->add_stylesheet( text.c_str(), nullptr, get_attr("media") );
}

bool litehtml::el_style::appendChild(const ptr &el)
{
    m_children.push_back(el);
    return true;
}

litehtml::string_id litehtml::el_style::tag() const
{
    return _style_;
}

const char* litehtml::el_style::get_tagName() const
{
    return "style";
}
