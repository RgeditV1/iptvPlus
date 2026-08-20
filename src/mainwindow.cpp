#include "mainwindow.hpp"
#include "videoplayerwindow.hpp"
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), playerWindow(nullptr)
{
    setWindowTitle("IPTV Plus - Panel Principal");
    resize(800, 400);

    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);

    playerWindow = new VideoPlayerWindow(this);
    btnOpenPlayer = new QPushButton("Reproducir Canal", this);

    layout->addWidget(playerWindow);
    layout->addWidget(btnOpenPlayer);

    setCentralWidget(centralWidget);

    connect(btnOpenPlayer, &QPushButton::clicked, this, &MainWindow::openPlayer);
}

MainWindow::~MainWindow() {}

void MainWindow::openPlayer() {
    qDebug() << "[MainWindow] Botón 'Reproducir Canal' presionado.";
    playerWindow->playMedia("https://vdopanel.jlahozconsulting.com:3648/live/atabaltvlive.m3u8");
}