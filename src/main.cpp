#include "mainwindow.hpp"
#include "databasemanager.hpp"
#include "torrentengine.hpp"

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <iostream>
#include <csignal>

#include <csignal>

void signalHandler(int signal)
{
    qDebug() << "[System] Señal de cierre recibida (" << signal << "). Limpiando archivos temporales...";
    TorrentEngine::cleanTempDirectory();
    std::exit(signal);
}

void customLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // Construir la ruta directamente en la carpeta del ejecutable
    QString logPath = QCoreApplication::applicationDirPath() + "/app.log";
    QFile logFile(logPath);

    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&logFile);
        QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        
        QString typeStr;
        switch (type) {
            case QtDebugMsg:    typeStr = "[DEBUG]"; break;
            case QtWarningMsg:  typeStr = "[WARN] "; break;
            case QtCriticalMsg: typeStr = "[CRIT] "; break;
            case QtFatalMsg:    typeStr = "[FATAL]"; break;
            case QtInfoMsg:     typeStr = "[INFO] "; break;
        }

        QString formattedMsg = QString("[%1] %2 %3").arg(timeStr, typeStr, msg);
        stream << formattedMsg << "\n";
        
        std::cout << formattedMsg.toStdString() << std::endl;
    }
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    TorrentEngine::cleanTempDirectory();

    
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Activar el log handler
    qInstallMessageHandler(customLogHandler);

    qDebug() << "========================================";
    qDebug() << "Iniciando iptvPlus...";

    if (!DatabaseManager::instance().initDatabase()) {
        qWarning("No se pudo conectar a la base de datos SQLite.");
    }

    MainWindow window;
    window.show();

    return app.exec();
}