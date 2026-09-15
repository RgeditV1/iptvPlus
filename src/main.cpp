#include "mainwindow.hpp"
#include "databasemanager.hpp"
#include "torrentengine.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <iostream>
#include <csignal>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

static bool g_verboseMode = false;

void signalHandler(int signal)
{
    qDebug() << "[System] Señal de cierre recibida (" << signal << "). Limpiando archivos temporales...";
    TorrentEngine::cleanTempDirectory();
    std::exit(signal);
}

void customLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);

    if (!g_verboseMode) {
        if (type != QtWarningMsg && type != QtCriticalMsg && type != QtFatalMsg) {
            return;
        }
    }

    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    
    QString typeStr;
    switch (type) {
        case QtDebugMsg:    typeStr = "[DEBUG]"; break;
        case QtInfoMsg:     typeStr = "[INFO] "; break;
        case QtWarningMsg:  typeStr = "[WARN] "; break;
        case QtCriticalMsg: typeStr = "[CRIT] "; break;
        case QtFatalMsg:    typeStr = "[FATAL]"; break;
    }

    QString formattedMsg = QString("[%1] %2 %3").arg(timeStr, typeStr, msg);

    std::cout << formattedMsg.toStdString() << std::endl;
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // Adjuntar la consola si se ejecutó desde una terminal existente (Windows)
    bool launchedFromCmd = false;
#ifdef Q_OS_WIN
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        launchedFromCmd = true;
    }
#endif

    // Parser de argumentos por consola
    QCommandLineParser parser;
    parser.setApplicationDescription("iptvPlus Media Player");
    parser.addHelpOption();
    
    QCommandLineOption verboseOption(QStringList() << "v" << "verbose", "Activa el modo verbose con todos los logs en consola.");
    parser.addOption(verboseOption);
    parser.process(app);

    // Determinar si se activa el modo verbose completo
    g_verboseMode = launchedFromCmd || parser.isSet(verboseOption);

    // Registrar el handler global de logs
    qInstallMessageHandler(customLogHandler);

    TorrentEngine::cleanTempDirectory();

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    qDebug() << "========================================";
    qDebug() << "Iniciando iptvPlus... (Verbose:" << (g_verboseMode ? "SI" : "NO") << ")";

    if (!DatabaseManager::instance().initDatabase()) {
        qWarning("No se pudo conectar a la base de datos SQLite.");
    }

    MainWindow window;
    window.show();

    int exitCode = app.exec();

#ifdef Q_OS_WIN
    if (launchedFromCmd) {
        FreeConsole();
    }
#endif

    return exitCode;
}