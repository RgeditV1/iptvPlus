#include "mainwindow.hpp"
#include "videoplayerwindow.hpp"

#include <QPushButton>
#include <QLineEdit>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEasingCurve>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), playerWindow(nullptr)
{
    setWindowTitle("IPTV Plus - Panel Principal");
    resize(950, 550);

    setupUi();

    categoryAnimation = new QPropertyAnimation(categoryPanel, "maximumWidth", this);
    categoryAnimation->setDuration(200);
    categoryAnimation->setEasingCurve(QEasingCurve::OutCubic);

    updateNotifier = new UpdateNotifier(this);

    connect(updateNotifier, &UpdateNotifier::remoteChannelsLoaded,
        this, [this](const QList<M3UItem>& channels) {

            currentChannels = channels;

            populateCategoryPanel();
            playRandomChannel();
        });

    updateNotifier->fetchRemoteStreams();
}

MainWindow::~MainWindow() {}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == categoryPanel ||
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
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // =========================================================
    // 1. PANEL LATERAL
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



    // Árbol para categorías y submenús
    treeMenu = new QTreeWidget(sidebarWidget);
    treeMenu->setHeaderHidden(true);
    treeMenu->setIndentation(15);
    treeMenu->setIconSize(QSize(18, 18));

    // --- Categoría Principal: Canales ---
    itemCanales = new QTreeWidgetItem(treeMenu);
    itemCanales->setText(0, "Canales");
    itemCanales->setIcon(0, QIcon(":/resources/icons/tv.svg"));
    itemCanales->setExpanded(false);

    sidebarLayout->addWidget(treeMenu);

    sidebarWidget->hide();

    QWidget* rightWidget = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    categoryPanel = new QFrame(centralWidget);
    categoryPanel->setMinimumWidth(0);
    categoryPanel->setMaximumWidth(0);
    categoryPanel->setFrameShape(QFrame::NoFrame);

    setupCategoryPanel();

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

    txtUrl = new QLineEdit(this);
    txtUrl->setPlaceholderText("Ingresa la URL del stream .m3u8 aquí...");

    btnOpenPlayer = new QPushButton("Reproducir Canal", this);
    btnOpenPlayer->setCursor(Qt::PointingHandCursor);

    topBarLayout->addWidget(btnToggleMenu);
    topBarLayout->addWidget(txtUrl);
    topBarLayout->addWidget(btnOpenPlayer);

    // --- REPRODUCTOR ---
    playerWindow = new VideoPlayerWindow(this);

    // Añadir barra superior y el reproductor ocupando todo el resto de espacio
    rightLayout->addLayout(topBarLayout, 0);
    rightLayout->addWidget(playerWindow, 1);

    mainLayout->addWidget(sidebarWidget, 0);
    mainLayout->addWidget(categoryPanel, 0);
    mainLayout->addWidget(rightWidget, 1);

    setCentralWidget(centralWidget);

    // =========================================================
    // 3. CONEXIONES
    // =========================================================
    connect(btnToggleMenu, &QPushButton::clicked, this, &MainWindow::toggleSidebar);
    connect(btnOpenPlayer, &QPushButton::clicked, this, &MainWindow::openPlayer);
    connect(txtUrl, &QLineEdit::returnPressed, this, &MainWindow::openPlayer);
    connect(categorySearch, &QLineEdit::textChanged, this, &MainWindow::filterCategoryItems);
    connect(treeMenu, &QTreeWidget::itemClicked, this, &MainWindow::onItemClicked);

}

void MainWindow::toggleCategoryPanel()
{
    categoryAnimation->stop();

    const int currentWidth = categoryPanel->width();
    const int targetWidth = (currentWidth > 0) ? 0 : 280;

    categoryAnimation->setStartValue(currentWidth);
    categoryAnimation->setEndValue(targetWidth);

    categoryAnimation->start();
}

void MainWindow::setupCategoryPanel()
{
    QVBoxLayout* layout = new QVBoxLayout(categoryPanel);

    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    categorySearch = new QLineEdit(categoryPanel);

    categorySearch->setPlaceholderText(
        "Buscar canales..."
    );

    channelModel = new ChannelListModel(this);

    categoryList = new QListView(categoryPanel);

    categoryList->setModel(channelModel);

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
    categoryPanel->installEventFilter(this);

    connect(
        categoryList,
        &QListView::clicked,
        this,
        [this](const QModelIndex& index) {

            if (!index.isValid())
                return;

            const QString url =
                index.data(Qt::UserRole).toString();

            if (url.isEmpty())
                return;

            txtUrl->setText(url);
            openPlayer();
        }
    );
}

void MainWindow::onItemClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);

    if (!item)
        return;

    if (item == itemCanales) {
        toggleCategoryPanel();
        return;
    }

    QString url = item->data(0, Qt::UserRole).toString();

    if (!url.isEmpty()) {
        txtUrl->setText(url);
        openPlayer();
    }
}

void MainWindow::toggleSidebar() {
    sidebarWidget->setVisible(!sidebarWidget->isVisible());
}

void MainWindow::populateCategoryPanel()
{
    channelModel->setChannels(currentChannels);
}

void MainWindow::playRandomChannel() {
    if (currentChannels.isEmpty()) return;

    int randomIndex = QRandomGenerator::global()->bounded(currentChannels.size());
    const M3UItem& randomChannel = currentChannels.at(randomIndex);

    txtUrl->setText(randomChannel.url);
    openPlayer();
}

void MainWindow::filterCategoryItems(
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