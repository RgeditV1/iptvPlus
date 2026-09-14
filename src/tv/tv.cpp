#include "tv.hpp"
#include "videoplayerwindow.hpp"

#include <QVBoxLayout>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QDebug>

TvWidget::TvWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    setupRightSidebar();

    // Actualizador para descargar canales de iptv-org
    m_updateNotifier = new UpdateNotifier(this);
    
    connect(m_updateNotifier, &UpdateNotifier::remoteChannelsLoaded,
                this, [this](const QList<M3UItem>& channels) {
                    m_channelModel->setChannels(channels);

                    if (!channels.isEmpty()) {
                        int randomIndex = QRandomGenerator::global()->bounded(channels.size());
                        const M3UItem& randomChannel = channels.at(randomIndex);
                        
                        if (!randomChannel.url.isEmpty()) {
                            qDebug() << "[TvWidget] Canal al azar seleccionado:" << randomChannel.title;
                            m_player->play(randomChannel.url);
                        }
                    }
                });

    // Cargar canales en segundo plano
    m_updateNotifier->fetchRemoteStreams();

    setMouseTracking(true);
    installEventFilter(this);
}

TvWidget::~TvWidget() = default;

void TvWidget::restoreLayerOrder()
{
    if (m_player)
        m_player->raise();

    if (m_rightSidebar)
        m_rightSidebar->raise();
}

void TvWidget::setupUi()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_player = new VideoPlayerWindow(this);
    layout->addWidget(m_player);

    connect(m_player, &VideoPlayerWindow::fullscreenToggled,
            this, [this](bool fullscreen) {
        if (fullscreen) {
            return;
        }

        QTimer::singleShot(0, this, &TvWidget::restoreLayerOrder);
    });
}

void TvWidget::setupRightSidebar()
{
    m_rightSidebar = new QWidget(this);
    m_rightSidebar->setStyleSheet(
        "QWidget { background-color: #1e1e2e; color: #ffffff; }"
        "QLineEdit {"
        "   background-color: #313244;"
        "   color: #cdd6f4;"
        "   border: 1px solid #45475a;"
        "   border-radius: 6px;"
        "   padding: 6px 10px;"
        "   font-size: 13px;"
        "}"
        "QLineEdit:focus { border: 1px solid #89b4fa; }"
        "QListView {"
        "   background-color: #181825;"
        "   border: none;"
        "   color: #cdd6f4;"
        "   outline: none;"
        "}"
        "QListView::item {"
        "   padding: 10px;"
        "   border-radius: 4px;"
        "}"
        "QListView::item:hover { background-color: #313244; }"
        "QListView::item:selected { background-color: #45475a; color: #ffffff; font-weight: bold; }"
    );

    QVBoxLayout* sidebarLayout = new QVBoxLayout(m_rightSidebar);
    sidebarLayout->setContentsMargins(10, 10, 10, 10);
    sidebarLayout->setSpacing(8);

    // QLineEdit para filtrar canales por texto
    m_searchBox = new QLineEdit(m_rightSidebar);
    m_searchBox->setPlaceholderText("Buscar canal...");
    sidebarLayout->addWidget(m_searchBox);

    m_channelModel = new ChannelListModel(this);
    m_channelListView = new QListView(m_rightSidebar);
    m_channelListView->setModel(m_channelModel);
    m_channelListView->setIconSize(QSize(20, 20));
    sidebarLayout->addWidget(m_channelListView);

    // Conexiones de búsqueda y selección de canales
    connect(m_searchBox, &QLineEdit::textChanged,
            m_channelModel, &ChannelListModel::filter);

    connect(m_channelListView, &QListView::clicked,
            this, &TvWidget::onChannelClicked);

    // Posición inicial fuera de pantalla (lado derecho)
    m_rightSidebar->setGeometry(width(), 0, m_sidebarWidth, height());
    m_rightSidebar->raise();

    m_sidebarAnimation = new QPropertyAnimation(m_rightSidebar, "geometry", this);
    m_sidebarAnimation->setDuration(200);
}

void TvWidget::onChannelClicked(const QModelIndex& index)
{
    const M3UItem* channel = m_channelModel->channelAt(index.row());
    if (channel && !channel->url.isEmpty()) {
        m_player->play(channel->url);
    }
}

void TvWidget::toggleRightSidebar(bool show)
{
    if (m_sidebarVisible == show) {
        return;
    }

    m_sidebarVisible = show;
    m_sidebarAnimation->stop();

    if (show) {
        m_rightSidebar->raise();
    }

    m_sidebarAnimation->setStartValue(m_rightSidebar->geometry());

    int targetX = show ? (width() - m_sidebarWidth) : width();
    m_sidebarAnimation->setEndValue(QRect(targetX, 0, m_sidebarWidth, height()));

    m_sidebarAnimation->start();
}

bool TvWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == this) {
        if (event->type() == QEvent::MouseMove) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            int mouseX = mouseEvent->pos().x();

            // Abrir al colocar el mouse cerca del borde derecho (<= 20 px)
            if (!m_sidebarVisible && mouseX >= (width() - 20)) {
                toggleRightSidebar(true);
            }
            // Ocultar cuando el mouse sale de la barra lateral
            else if (m_sidebarVisible && mouseX < (width() - m_sidebarWidth)) {
                toggleRightSidebar(false);
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void TvWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    int currentX = m_sidebarVisible ? (width() - m_sidebarWidth) : width();

    m_rightSidebar->setGeometry(currentX, 0, m_sidebarWidth, height());
    m_rightSidebar->raise();
}