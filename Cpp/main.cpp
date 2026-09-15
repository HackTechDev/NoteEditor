#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // Forcé : certains styles natifs/GTK ignorent silencieusement des règles
    // de la feuille de style personnalisée (onglets, bouton +). Voir
    // Cpp/CLAUDE.md ou Python/CLAUDE.md.
    app.setStyle("Fusion");

    MainWindow window;
    window.show();

    return app.exec();
}
