#include "mainwindow.hpp"
#include "videoplayerwindow.hpp"
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), playerWindow(nullptr)
{
    setWindowTitle("IPTV Plus - Panel Principal");
    resize(800, 450);

    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    playerWindow = new VideoPlayerWindow(this);

    QHBoxLayout* controlsLayout = new QHBoxLayout();

    txtUrl = new QLineEdit(this);
    txtUrl->setPlaceholderText("Ingresa la URL del stream .m3u8 aquí...");
    txtUrl->setText("https://soul-5mincrafteng-rakuten.amagi.tv/playlist.m3u8");

    btnOpenPlayer = new QPushButton("Reproducir Canal", this);

    controlsLayout->addWidget(txtUrl);
    controlsLayout->addWidget(btnOpenPlayer);

    mainLayout->addWidget(playerWindow);
    mainLayout->addLayout(controlsLayout);

    setCentralWidget(centralWidget);

    connect(btnOpenPlayer, &QPushButton::clicked, this, &MainWindow::openPlayer);
    connect(txtUrl, &QLineEdit::returnPressed, this, &MainWindow::openPlayer);
}

MainWindow::~MainWindow() {}

void MainWindow::openPlayer() {
    QString url = txtUrl->text().trimmed();

    if (url.isEmpty()) {
        qWarning() << "[MainWindow] Intento de reproducción con URL vacía.";
        return;
    }

    qDebug() << "[MainWindow] Botón/Enter presionado. Cargando URL:" << url;
    playerWindow->playMedia(url);
}