#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QListView>
#include <QPropertyAnimation>
#include <QEvent>

#include "channellistmodel.hpp"
#include "update.hpp"

class VideoPlayerWindow;

class TvWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TvWidget(QWidget* parent = nullptr);
    ~TvWidget() override;

    VideoPlayerWindow* player() const { return m_player; }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupUi();
    void setupRightSidebar();
    void toggleRightSidebar(bool show);

private slots:
    void onChannelClicked(const QModelIndex& index);

private:
    VideoPlayerWindow* m_player = nullptr;

    // Barra lateral derecha de canales
    QWidget* m_rightSidebar = nullptr;
    QLineEdit* m_searchBox = nullptr;
    QListView* m_channelListView = nullptr;

    // Modelo de datos y Actualizador remoto
    ChannelListModel* m_channelModel = nullptr;
    UpdateNotifier* m_updateNotifier = nullptr;

    // Control de animación y visibilidad
    QPropertyAnimation* m_sidebarAnimation = nullptr;
    bool m_sidebarVisible = false;
    const int m_sidebarWidth = 300;
};