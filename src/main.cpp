#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QDir>
#include <QStandardPaths>
#include <qlogging.h>
#include "mainwindow.hpp"

// Variable global para proteger el archivo de logs en entornos multihilo
QMutex logMutex;

void customLogMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QMutexLocker locker(&logMutex);

    QString logPath = QCoreApplication::applicationDirPath() + "/debug.log";

    QFile logFile(logPath);
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&logFile);
    QString timeStr = QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss.zzz"); // Espanol fmt
    QString typeStr;

    switch (type) {
    case QtDebugMsg:    typeStr = "[DEBUG]"; break;
    case QtInfoMsg:     typeStr = "[INFO] "; break;
    case QtWarningMsg:  typeStr = "[WARN] "; break;
    case QtCriticalMsg: typeStr = "[CRIT] "; break;
    case QtFatalMsg:    typeStr = "[FATAL]"; break;
    }

    stream << timeStr << " " << typeStr << " " << msg << "\n";

    stream.flush();
    logFile.close();
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Redirigir todos los qMessage / qDebug a debug.log
    qInstallMessageHandler(customLogMessageHandler);

    qInfo() << "Iniciando la aplicación IPTV Plus...";

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}