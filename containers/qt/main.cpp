// QT
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>

// LITEHTML
#include "qt_container.h"
#include "qt_litehtml.h"


/**********************************************************************************************/
constexpr int HTML_HEIGHT = 400;
constexpr int HTML_WIDTH  = 600;

/**********************************************************************************************/
static const char* const HTML_TEXT = R"(
<!DOCTYPE html>
<html>
<head>
<style>
table, th, td {
  border: 1px solid black;
}
</style>
</head>
<body>

<h2>Table With Border</h2>

<p>Use the CSS border property to add a border to the table.</p>

<table style="width:400px">
  <tr>
    <th>Firstname</th>
    <th>Lastname</th>
    <th>Age</th>
  </tr>
  <tr>
    <td>Jill</td>
    <td>Smith</td>
    <td>50</td>
  </tr>
  <tr>
    <td>Eve</td>
    <td>Jackson</td>
    <td>94</td>
  </tr>
  <tr>
    <td>John</td>
    <td>Doe</td>
    <td>80</td>
  </tr>
</table>

<hr>

<h2>Absolute URLs</h2>
<p><a href="https://www.google.com/">Google</a></p>
<p><a href="https://www.ukr.net/">Ukr.Net</a></p>

<h2>Relative URLs</h2>
<p><a href="html_images.asp">HTML Images</a></p>
<p><a href="/css/default.asp">CSS Tutorial</a></p>

</body>
</html>
)";


/**********************************************************************************************/
int main( int argc, char* argv[] )
{
    const QApplication app( argc, argv );

    auto* const view = new qt_litehtml;
    view->resize( HTML_WIDTH, HTML_HEIGHT );

    view->setHtml( HTML_TEXT );
    view->show();

#if 0
    const QPixmap pmp = qt_container::render( HTML_TEXT, HTML_WIDTH );

    auto* const l = new QLabel;
    l->resize( pmp.width(), pmp.height() );
    l->setAutoFillBackground( true );
    l->setBackgroundRole( QPalette::Base );
    l->setPixmap( pmp );
    l->show();
#endif // 0

    QApplication::exec();
}
