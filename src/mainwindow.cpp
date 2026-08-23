#include "mainwindow.hpp"
#include "videoplayerwindow.hpp"

#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), playerWindow(nullptr)
{
    setWindowTitle("IPTV Plus - Panel Principal");
    resize(950, 550);

    setupUi();
    loadChannelsFromFolder("./3rdparty/iptv/streams");
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi() {
    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // =========================================================
    // 1. PANEL LATERAL / MENÚ DESPLEGABLE CON SUBMENÚS
    // =========================================================
    sidebarWidget = new QWidget(this);
    sidebarWidget->setFixedWidth(260);
    sidebarWidget->setStyleSheet(
        "QWidget { background-color: #1e1e24; color: white; }"
        "QLineEdit { background-color: #2b2b36; border: 1px solid #3a3a4c; "
        "            padding: 5px; border-radius: 4px; color: white; }"
        "QTreeWidget { background-color: #18181c; border: none; color: white; }"
        "QTreeWidget::item { padding: 6px; border-bottom: 1px solid #282830; }"
        "QTreeWidget::item:hover { background-color: #2a2a35; }"
        "QTreeWidget::item:selected { background-color: #007acc; color: white; }"
    );

    QVBoxLayout* sidebarLayout = new QVBoxLayout(sidebarWidget);
    sidebarLayout->setContentsMargins(8, 8, 8, 8);
    sidebarLayout->setSpacing(6);

    txtSearchChannel = new QLineEdit(sidebarWidget);
    txtSearchChannel->setPlaceholderText("Buscar en canales...");

    // Árbol para categorías y submenús
    treeMenu = new QTreeWidget(sidebarWidget);
    treeMenu->setHeaderHidden(true);
    treeMenu->setIndentation(15);
    treeMenu->setIconSize(QSize(18, 18));

    // --- Categoría Principal: Canales ---
    itemCanales = new QTreeWidgetItem(treeMenu);
    itemCanales->setText(0, "Canales");
    itemCanales->setIcon(0, QIcon(":/resources/icons/tv.svg"));
    itemCanales->setExpanded(true);

    sidebarLayout->addWidget(txtSearchChannel);
    sidebarLayout->addWidget(treeMenu);

    sidebarWidget->hide();

    // =========================================================
    // 2. PANEL DERECHO (REPRODUCTOR Y CONTROLES)
    // =========================================================
    QWidget* rightWidget = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    playerWindow = new VideoPlayerWindow(this);

    QHBoxLayout* controlsLayout = new QHBoxLayout();
    controlsLayout->setContentsMargins(8, 6, 8, 6);

    btnToggleMenu = new QPushButton(this);
    btnToggleMenu->setIcon(QIcon(":/resources/icons/menu.svg"));
    btnToggleMenu->setIconSize(QSize(24, 24));
    btnToggleMenu->setCursor(Qt::PointingHandCursor);
    btnToggleMenu->setToolTip("Abrir/Cerrar Menú");

    txtUrl = new QLineEdit(this);
    txtUrl->setPlaceholderText("Ingresa la URL del stream .m3u8 aquí...");

    btnOpenPlayer = new QPushButton("Reproducir Canal", this);
    btnOpenPlayer->setCursor(Qt::PointingHandCursor);

    controlsLayout->addWidget(btnToggleMenu);
    controlsLayout->addWidget(txtUrl);
    controlsLayout->addWidget(btnOpenPlayer);

    rightLayout->addWidget(playerWindow, 1);
    rightLayout->addLayout(controlsLayout, 0);

    mainLayout->addWidget(sidebarWidget, 0);
    mainLayout->addWidget(rightWidget, 1);

    setCentralWidget(centralWidget);

    // =========================================================
    // 3. CONEXIONES
    // =========================================================
    connect(btnToggleMenu, &QPushButton::clicked, this, &MainWindow::toggleSidebar);
    connect(btnOpenPlayer, &QPushButton::clicked, this, &MainWindow::openPlayer);
    connect(txtUrl, &QLineEdit::returnPressed, this, &MainWindow::openPlayer);
    connect(txtSearchChannel, &QLineEdit::textChanged, this, &MainWindow::filterChannels);
    connect(treeMenu, &QTreeWidget::itemClicked, this, &MainWindow::onItemClicked);
}

void MainWindow::toggleSidebar() {
    sidebarWidget->setVisible(!sidebarWidget->isVisible());
}

void MainWindow::loadChannelsFromFolder(const QString& dirPath) {
    currentChannels = m3uParser.parseDirectory(dirPath);
    populateChannelSubmenu();
}

void MainWindow::populateChannelSubmenu() {
    qDeleteAll(itemCanales->takeChildren());

    QIcon channelIcon(":/resources/icons/play-circle.svg");

    for (const auto& channel : currentChannels) {
        QString displayText = channel.title.isEmpty() ? channel.url : channel.title;

        QTreeWidgetItem* subItem = new QTreeWidgetItem(itemCanales);
        subItem->setText(0, displayText);
        subItem->setIcon(0, channelIcon);
        subItem->setData(0, Qt::UserRole, channel.url);
    }
}

void MainWindow::filterChannels(const QString& text) {
    for (int i = 0; i < itemCanales->childCount(); ++i) {
        QTreeWidgetItem* child = itemCanales->child(i);
        bool matches = child->text(0).contains(text, Qt::CaseInsensitive);
        child->setHidden(!matches);
    }
}

void MainWindow::onItemClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (!item) return;

    QString url = item->data(0, Qt::UserRole).toString();

    if (!url.isEmpty()) {
        txtUrl->setText(url);
        openPlayer();
    }
}

void MainWindow::openPlayer() {
    QString url = txtUrl->text().trimmed();

    if (url.isEmpty()) {
        qWarning() << "[MainWindow] Intento de reproducción con URL vacía.";
        return;
    }

    qDebug() << "[MainWindow] Cargando URL:" << url;
    playerWindow->playMedia(url);
}