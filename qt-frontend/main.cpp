#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "dnacontroller.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("Simian Detector");
    app.setOrganizationName("CISS");

    QQmlApplicationEngine engine;

    // Cria o controller e expoe pro QML como "controller"
    DnaController controller;
    engine.rootContext()->setContextProperty("controller", &controller);

    // Carrega o QML (qrc:/ = embutido no executavel via .qrc)
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML";
        return -1;
    }

    return app.exec();
}
