#include "../include/litehtml/html.h"
#include "../include/litehtml/el_base.h"
#include "../include/litehtml/document_litehtml.h"

litehtml::el_base::el_base(const std::shared_ptr<document>& doc) : html_tag(doc)
{

}

void litehtml::el_base::parse_attributes()
{
    get_document()->container()->set_base_url(get_attr("href"));
}
