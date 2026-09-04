#include "mainwindow.hpp"
#include "videoplayerwindow.hpp"

#include <QPushButton>
#include <QLineEdit>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QEasingCurve>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), playerWindow(nullptr)
{
    setWindowTitle("IPTV Plus");
    
    QIcon icon(":/iptv-icon.png");

    qDebug() << "Icono válido:" << !icon.isNull();

    setWindowIcon(icon);

    resize(950, 550);

    setupUi();

    channelAnimation = new QPropertyAnimation(channelPanel, "maximumWidth", this);
    channelAnimation->setDuration(200);
    channelAnimation->setEasingCurve(QEasingCurve::OutCubic);

    updateNotifier = new UpdateNotifier(this);

    connect(updateNotifier, &UpdateNotifier::remoteChannelsLoaded,
        this, [this](const QList<M3UItem>& channels) {

            currentChannels = channels;

            populateChannelPanel();
            playRandomChannel();
        });

    updateNotifier->fetchRemoteStreams();
}

MainWindow::~MainWindow() {}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == channelPanel ||
        watched == categorySearch ||
        watched == categoryList) {

        if (event->type() == QEvent::Enter) {

            categoryList->setVerticalScrollBarPolicy(
                Qt::ScrollBarAsNeeded
            );
        }
        else if (event->type() == QEvent::Leave) {

            categoryList->setVerticalScrollBarPolicy(
                Qt::ScrollBarAlwaysOff
            );
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setupUi() {
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color: black;");

    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    

    // =========================================================
    // 1. PANEL LATERAL
    // =========================================================
    sidebarWidget = new QWidget(this);
    sidebarWidget->setFixedWidth(160);
    sidebarWidget->setStyleSheet(
        "QWidget { background-color: #1e1e24; color: white; }"
        "QLineEdit { background-color: #2b2b36; border: 1px solid #3a3a4c; "
        "            padding: 5px; border-radius: 4px; color: white; }"
        "QTreeWidget {"
        "    background-color: #18181c;"
        "    border: none;"
        "    outline: none;"
        "    color: white;"
        "}"
        "QTreeWidget::item {"
        "    padding: 8px 6px;"
        "    border: none;"
        "    outline: none;"
        "    border-bottom: 1px solid #282830;" // Borde entre categorías
        "}"
        "QTreeWidget::item:hover {"
        "    background-color: #2a2a35;"
        "}"
        /* Mantiene el azul azul vibrante activo y visible cuando el usuario hace clic en otro lado */
        "QTreeWidget::item:selected, QTreeWidget::item:selected:!active {"
        "    background-color: #007acc;"
        "    color: white;"
        "}"
    );

    QVBoxLayout* sidebarLayout = new QVBoxLayout(sidebarWidget);
    sidebarLayout->setContentsMargins(8, 8, 8, 8);
    sidebarLayout->setSpacing(6);

    // Árbol para categorías y submenús
    treeMenu = new QTreeWidget(sidebarWidget);
    treeMenu->setColumnCount(2);
    treeMenu->setHeaderHidden(true);
    treeMenu->setIndentation(10);
    treeMenu->setIconSize(QSize(18, 18));
    treeMenu->header()->setSectionResizeMode(0, QHeaderView::Fixed);
    treeMenu->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    treeMenu->setColumnWidth(1, 16);

    // --- Categoría Principal: Canales ---
    itemCanales = new QTreeWidgetItem(treeMenu);
    itemCanales->setText(0, "Canales");
    itemCanales->setIcon(0, QIcon(":/resources/icons/tv.svg"));
    itemCanales->setIcon(1, QIcon(":/resources/icons/arrow-right.svg"));
    itemCanales->setTextAlignment(1, Qt::AlignCenter);

    sidebarLayout->addWidget(treeMenu);
    treeMenu->setCurrentItem(itemCanales); // por defecto

    sidebarWidget->hide();

    QWidget* rightWidget = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    channelPanel = new QFrame(centralWidget);
    channelPanel->setMinimumWidth(0);
    channelPanel->setMaximumWidth(0);
    channelPanel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    channelPanel->setFrameShape(QFrame::NoFrame);

    setupChannelPanel();

    // --- BARRA SUPERIOR (TOP BAR) ---
    QHBoxLayout* topBarLayout = new QHBoxLayout();
    topBarLayout->setContentsMargins(8, 6, 8, 6);
    topBarLayout->setSpacing(8);

    btnToggleMenu = new QPushButton(this);
    btnToggleMenu->setIcon(QIcon(":/resources/icons/menu.svg"));
    btnToggleMenu->setIconSize(QSize(24, 24));
    btnToggleMenu->setCursor(Qt::PointingHandCursor);
    btnToggleMenu->setToolTip("Menu");
    btnToggleMenu->setFixedSize(36, 36);

    btnToggleMenu->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    border: none;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(255, 255, 255, 30);"
        "}"
        "QPushButton:pressed {"
        "    background-color: rgba(255, 255, 255, 50);"
        "}"
    );

    txtUrl = new QLineEdit(this);
    txtUrl->setPlaceholderText("Ingresa la URL del stream .m3u8 aquí...");

    btnOpenPlayer = new QPushButton("Reproducir Canal", this);
    btnOpenPlayer->setCursor(Qt::PointingHandCursor);

    topBarLayout->addWidget(btnToggleMenu);
    topBarLayout->addWidget(txtUrl);
    topBarLayout->addWidget(btnOpenPlayer);

    // --- REPRODUCTOR ---
    playerWindow = new VideoPlayerWindow(this);

    playerWindow->setModel(channelModel);

    rightLayout->addLayout(topBarLayout, 0);
    rightLayout->addWidget(playerWindow, 1);

    mainLayout->addWidget(sidebarWidget, 0);
    mainLayout->addWidget(channelPanel, 0);
    mainLayout->addWidget(rightWidget, 1);

    setCentralWidget(centralWidget);

    // =========================================================
    // 3. CONEXIONES
    // =========================================================
    connect(btnToggleMenu, &QPushButton::clicked, this, &MainWindow::toggleSidebar);
    connect(btnOpenPlayer, &QPushButton::clicked, this, &MainWindow::openPlayer);
    connect(txtUrl, &QLineEdit::returnPressed, this, &MainWindow::openPlayer);
    connect(categorySearch, &QLineEdit::textChanged, this, &MainWindow::filterChannelItems);
    connect(treeMenu, &QTreeWidget::itemClicked, this, &MainWindow::onItemClicked);
    connect(playerWindow, &VideoPlayerWindow::fullScreenToggled, this, &MainWindow::onFullScreenToggled);
    connect(
        categoryList,
        &QListView::clicked,
        this,
        [this](const QModelIndex& index) {

            if (!index.isValid())
                return;

            const QString url = index.data(Qt::UserRole).toString();

            if (url.isEmpty())
                return;

            txtUrl->setText(url);
            playerWindow->playChannelAt(index.row());
        }
    );
    connect( // Sincronizacion con la lista
        playerWindow,
        &VideoPlayerWindow::channelChanged,
        this,
        [this](int row) {

            if (!categoryList->model())
                return;

            QModelIndex index = categoryList->model()->index(row, 0);

            if (!index.isValid())
                return;

            categoryList->setCurrentIndex(index);
            categoryList->scrollTo(
                index,
                QAbstractItemView::PositionAtCenter
            );
			txtUrl->setText(index.data(Qt::UserRole).toString());
        }
    );

}

void MainWindow::toggleChannelPanel()
{
    channelAnimation->stop();

    const int currentWidth = channelPanel->width();
    const int targetWidth = (currentWidth > 0) ? 0 : 280;

    channelAnimation->setStartValue(currentWidth);
    channelAnimation->setEndValue(targetWidth);

    channelAnimation->start();
}

void MainWindow::setupChannelPanel()
{
    QVBoxLayout* layout = new QVBoxLayout(channelPanel);

    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    categorySearch = new QLineEdit(channelPanel);

    categorySearch->setPlaceholderText(
        "Buscar canales..."
    );

    channelModel = new ChannelListModel(this);

    categoryList = new QListView(channelPanel);

    categoryList->setModel(channelModel);

    categoryList->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding
    );

    categoryList->setVerticalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff
    );

    categoryList->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff
    );

    categoryList->setIconSize(
        QSize(18, 18)
    );

    categoryList->setSelectionMode(
        QAbstractItemView::SingleSelection
    );

    categoryList->setUniformItemSizes(true);

    layout->addWidget(categorySearch);
    layout->addWidget(categoryList);

    categorySearch->installEventFilter(this);
    categoryList->installEventFilter(this);
    channelPanel->installEventFilter(this);
}

void MainWindow::onItemClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);

    if (!item)
        return;

    if (item == itemCanales) {
        toggleChannelPanel();
        return;
    }

    QString url = item->data(0, Qt::UserRole).toString();

    if (!url.isEmpty()) {
        txtUrl->setText(url);
        openPlayer();
    }
}

void MainWindow::toggleSidebar() {
    bool willBeVisible = !sidebarWidget->isVisible();

    if (!willBeVisible && channelPanel->maximumWidth() > 0) {
        toggleChannelPanel();
    }

    sidebarWidget->setVisible(willBeVisible);
}

void MainWindow::onFullScreenToggled(bool isFullScreen)
{
    if (isFullScreen) {
        // Guardar el estado previo de los paneles
        sidebarWasVisibleBeforeFS = sidebarWidget->isVisible();
        channelPanelWasOpenBeforeFS = (channelPanel->width() > 0);

        // Ocultar la barra superior (Top Bar)
        btnToggleMenu->hide();
        txtUrl->hide();
        btnOpenPlayer->hide();

        sidebarWidget->hide();

        channelAnimation->stop();
        channelPanel->setMaximumWidth(0);

    } else {
        btnToggleMenu->show();
        txtUrl->show();

        btnOpenPlayer->show();
        if (sidebarWasVisibleBeforeFS) {
            sidebarWidget->show();
        }

        if (channelPanelWasOpenBeforeFS) {
            channelAnimation->stop();
            channelAnimation->setStartValue(channelPanel->width());
            channelAnimation->setEndValue(280);
            channelAnimation->start();
        }
    }
}

void MainWindow::populateChannelPanel()
{
    channelModel->setChannels(currentChannels);
}

void MainWindow::playRandomChannel() {
    if (currentChannels.isEmpty() || !channelModel) return; //[cite: 7]

    int totalRows = channelModel->rowCount(); //[cite: 3, 5]
    if (totalRows == 0) return;

    int randomIndex = QRandomGenerator::global()->bounded(totalRows);

    // Seleccionar y reproducir mediante el índice del modelo
    const M3UItem* item = channelModel->channelAt(randomIndex); //[cite: 3, 5]
    if (item) {
        txtUrl->setText(item->url); //[cite: 7]
        playerWindow->playChannelAt(randomIndex); // <--- CAMBIADO[cite: 7]
    }
}

void MainWindow::filterChannelItems(
    const QString& text)
{
    channelModel->filter(text);
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