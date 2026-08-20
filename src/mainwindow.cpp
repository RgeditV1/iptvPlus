#include "mainwindow.hpp"
#include "videoplayerwindow.hpp"
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), playerWindow(nullptr)
{
    setWindowTitle("IPTV Plus - Panel Principal");
    resize(400, 300);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    btnOpenPlayer = new QPushButton("Abrir Reproductor y Ver Canal", this);
    layout->addWidget(btnOpenPlayer);

    setCentralWidget(centralWidget);

    connect(btnOpenPlayer, &QPushButton::clicked, this, &MainWindow::openPlayer);
}

MainWindow::~MainWindow() {
    if (playerWindow) {
        delete playerWindow;
    }
}

void MainWindow::openPlayer() {
    if (!playerWindow) {
        // Crear la ventana del reproductor de forma independiente
        playerWindow = new VideoPlayerWindow();
    }

    // Mostrar la ventana del reproductor si está oculta
    playerWindow->show();
    playerWindow->raise();
    playerWindow->activateWindow();

    // Reproducir canal
    playerWindow->playMedia("https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4");
}