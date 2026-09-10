#include "mainwindow.hpp"
#include "videoplayerwindow.hpp"
#include "tv.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QIcon>
#include <QHeaderView>
#include <QMouseEvent>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("IPTV++");
    resize(1280, 720);

    setupUi();
    setupSidebar();

    m_centralContainer->installEventFilter(this);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    m_centralContainer = new QWidget(this);
    m_centralContainer->setStyleSheet("background-color: black;");
    setCentralWidget(m_centralContainer);

    QVBoxLayout* mainContainerLayout = new QVBoxLayout(m_centralContainer);
    mainContainerLayout->setContentsMargins(0, 0, 0, 0);
    mainContainerLayout->setSpacing(0);

    // Layout superior para la barra de herramientas / botón menú
    QHBoxLayout* topBarLayout = new QHBoxLayout();
    topBarLayout->setContentsMargins(10, 10, 10, 10);

    m_menuButton = new QPushButton(m_centralContainer);
    m_menuButton->setText(" Menú");
    m_menuButton->setIcon(QIcon(":/resources/icons/menu.svg"));
    m_menuButton->setCursor(Qt::PointingHandCursor);
    m_menuButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #313244;"
        "   color: #cdd6f4;"
        "   border: none;"
        "   padding: 8px 12px;"
        "   border-radius: 5px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #45475a;"
        "}"
    );

    topBarLayout->addWidget(m_menuButton);
    topBarLayout->addStretch();
    mainContainerLayout->addLayout(topBarLayout);

    // QStackedWidget para gestionar las diferentes vistas en centralContainer
    m_stackedWidget = new QStackedWidget(m_centralContainer);

    // Vista Películas
    m_moviesView = new QWidget(this);
    QVBoxLayout* moviesLayout = new QVBoxLayout(m_moviesView);
    QLabel* moviesLabel = new QLabel("Sección de Películas (Vacía)", m_moviesView);
    moviesLabel->setAlignment(Qt::AlignCenter);
    moviesLabel->setStyleSheet("color: #a6adc8; font-size: 18px;");
    moviesLayout->addWidget(moviesLabel);

    // Vista TV integrada con reproductor y barra lateral de canales
    m_tvView = new TvWidget(this);

    // Añadir vistas al StackedWidget
    m_stackedWidget->addWidget(m_tvView);     // Índice 0: TV
    m_stackedWidget->addWidget(m_moviesView); // Índice 1: Películas

    connect(m_stackedWidget, &QStackedWidget::currentChanged, this, [this](int newIndex) {
        if (newIndex != 0 && m_tvView && m_tvView->player()) {
            m_tvView->player()->pause();
        }
    });

    mainContainerLayout->addWidget(m_stackedWidget, 1);

    connect(m_menuButton, &QPushButton::clicked, this, [this]() {
        toggleSidebar();
    });
}

void MainWindow::setupSidebar()
{
    m_sidebar = new QWidget(m_centralContainer);
    m_sidebar->setStyleSheet(
        "QWidget { background-color: #1e1e2e; color: #ffffff; }"
        "QTreeWidget { background-color: #181825; border: none; color: #cdd6f4; outline: none; }"
        "QTreeWidget::item { padding: 12px; border-radius: 4px; }"
        "QTreeWidget::item:hover { background-color: #313244; }"
        "QTreeWidget::item:selected { background-color: #45475a; color: #ffffff; font-weight: bold; }"
    );

    QVBoxLayout* sidebarLayout = new QVBoxLayout(m_sidebar);
    sidebarLayout->setContentsMargins(10, 10, 10, 10);

    m_navTree = new QTreeWidget(m_sidebar);
    m_navTree->setHeaderHidden(true);
    m_navTree->setColumnCount(1);

    // Crear elementos de menú vertical (Tree)
    QTreeWidgetItem* tvItem = new QTreeWidgetItem(m_navTree);
    tvItem->setIcon(0, QIcon(":/resources/icons/tv.svg"));
    tvItem->setText(0, "TV");
    tvItem->setData(0, Qt::UserRole, 0); // Índice para el QStackedWidget

    QTreeWidgetItem* moviesItem = new QTreeWidgetItem(m_navTree);
    moviesItem->setIcon(0, QIcon(":/resources/icons/film.svg"));
    moviesItem->setText(0, "Películas");
    moviesItem->setData(0, Qt::UserRole, 1); // Índice para el QStackedWidget

    m_navTree->setCurrentItem(tvItem);

    sidebarLayout->addWidget(m_navTree);

    connect(m_navTree, &QTreeWidget::itemClicked, this, &MainWindow::onNavigationItemClicked);

    m_sidebar->setGeometry(-m_sidebarWidth, 0, m_sidebarWidth, height());
    m_sidebar->raise();

    m_sidebarAnimation = new QPropertyAnimation(m_sidebar, "geometry", this);
    m_sidebarAnimation->setDuration(200);
}

void MainWindow::onNavigationItemClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    if (!item)
        return;

    int targetIndex = item->data(0, Qt::UserRole).toInt();
    m_stackedWidget->setCurrentIndex(targetIndex);

    toggleSidebar(false);
}

void MainWindow::toggleSidebar()
{
    toggleSidebar(!m_sidebarVisible);
}

void MainWindow::toggleSidebar(bool show)
{
    if (m_sidebarVisible == show)
        return;

    m_sidebarVisible = show;
    m_sidebarAnimation->stop();
    m_sidebarAnimation->setStartValue(m_sidebar->geometry());

    int targetX = show ? 0 : -m_sidebarWidth;
    m_sidebarAnimation->setEndValue(QRect(targetX, 0, m_sidebarWidth, height()));
    m_sidebarAnimation->start();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_centralContainer) {
        if (event->type() == QEvent::MouseMove) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            int mouseX = mouseEvent->pos().x();

            if (!m_sidebarVisible && mouseX <= 20) {
                toggleSidebar(true);
            } else if (m_sidebarVisible && mouseX > m_sidebarWidth) {
                toggleSidebar(false);
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    int currentX = m_sidebarVisible ? 0 : -m_sidebarWidth;
    m_sidebar->setGeometry(currentX, 0, m_sidebarWidth, height());
}