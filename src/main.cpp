#include <QApplication>
#include "ui/MainWindow.h"
#include "common/Theme.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("FluxTransfer");
    app.setOrganizationName("FluxTransfer");
    app.setApplicationDisplayName("FluxTransfer");

    MainWindow w;
    w.show();

    return app.exec();
}
