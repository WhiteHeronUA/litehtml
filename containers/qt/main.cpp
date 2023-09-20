// QT
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>

// LITEHTML
#include "qt_container.h"


/**********************************************************************************************/
constexpr int HTML_WIDTH = 600;

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

<table style="width:100%">
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

</body>
</html>
)";


/**********************************************************************************************/
int main( int argc, char* argv[] )
{
    const QApplication app( argc, argv );

    const QPixmap pmp = qt_container::render( HTML_TEXT, HTML_WIDTH );

    auto* const l = new QLabel;
    l->resize( pmp.width(), pmp.height() );
    l->setAutoFillBackground( true );
    l->setBackgroundRole( QPalette::Base );
    l->setPixmap( pmp );
    l->show();

    QApplication::exec();
}
