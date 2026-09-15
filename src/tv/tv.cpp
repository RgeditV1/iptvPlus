#include "tv.hpp"
#include "videoplayerwindow.hpp"

#include <QVBoxLayout>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QComboBox>
#include <QDebug>

TvWidget::TvWidget(QWidget* parent)
    : QWidget(parent)
{
    m_updateNotifier = new UpdateNotifier(this);

    setupUi();
    setupRightSidebar();

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
        "QLineEdit, QComboBox {"
        "   background-color: #313244;"
        "   color: #cdd6f4;"
        "   border: 1px solid #45475a;"
        "   border-radius: 6px;"
        "   padding: 6px 10px;"
        "   font-size: 13px;"
        "}"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView {"
        "   background-color: #181825;"
        "   color: #cdd6f4;"
        "   selection-background-color: #45475a;"
        "}"
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

    m_searchBox = new QLineEdit(m_rightSidebar);
    m_searchBox->setPlaceholderText("Buscar canal...");
    sidebarLayout->addWidget(m_searchBox);

    QComboBox* countryCombo = new QComboBox(m_rightSidebar);
    sidebarLayout->addWidget(countryCombo);

    m_channelModel = new ChannelListModel(this);
    m_channelListView = new QListView(m_rightSidebar);
    m_channelListView->setModel(m_channelModel);
    m_channelListView->setIconSize(QSize(20, 20));
    sidebarLayout->addWidget(m_channelListView);

    // Conexiones de búsqueda y filtros
    connect(m_searchBox, &QLineEdit::textChanged,
            m_channelModel, &ChannelListModel::filter);

    connect(countryCombo, &QComboBox::currentTextChanged,
            m_channelModel, &ChannelListModel::filterByCountry);

    connect(m_channelListView, &QListView::clicked,
            this, &TvWidget::onChannelClicked);

    // Respuesta a la carga remota de canales
    connect(m_updateNotifier, &UpdateNotifier::remoteChannelsLoaded,
            this, [countryCombo, this](const QList<M3UItem>& channels) {
                m_channelModel->setChannels(channels);

                countryCombo->blockSignals(true);
                countryCombo->clear();
                countryCombo->addItems(m_channelModel->getAvailableCountries());
                countryCombo->setCurrentIndex(0);
                countryCombo->blockSignals(false);

                if (!channels.isEmpty()) {
                    int randomIndex = QRandomGenerator::global()->bounded(m_channelModel->rowCount());
                    QModelIndex targetIndex = m_channelModel->index(randomIndex, 0);
                    
                    m_channelListView->setCurrentIndex(targetIndex);
                    
                    const M3UItem* randomChannel = m_channelModel->channelAt(randomIndex);
                    if (randomChannel && !randomChannel->url.isEmpty()) {
                        m_player->play(randomChannel->url);
                    }
                }
            });

    // Configuración visual de la barra lateral
    m_rightSidebar->setGeometry(width(), 0, m_sidebarWidth, height());
    m_rightSidebar->raise();

    m_sidebarAnimation = new QPropertyAnimation(m_rightSidebar, "geometry", this);
    m_sidebarAnimation->setDuration(200);
}

void TvWidget::onChannelClicked(const QModelIndex& index)
{
    if (!index.isValid()) return;

    m_channelListView->setCurrentIndex(index);

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

            if (!m_sidebarVisible && mouseX >= (width() - 20)) {
                toggleRightSidebar(true);
            }
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