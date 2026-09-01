#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "update.hpp"
#include "m3uparser.hpp"
#include "channellistmodel.hpp"

#include <QMainWindow>
#include <QTreeWidget>
#include <QListView>
#include <QTimer>
#include <QIcon>
#include <QFrame>
#include <QPropertyAnimation>
#include <QListWidget>
#include <QRandomGenerator>

class VideoPlayerWindow;
class QLineEdit;
class QPushButton;
class QWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void openPlayer();
    void toggleSidebar();
    void toggleChannelPanel();
    void filterChannelItems(const QString& text);
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onFullScreenToggled(bool isFullScreen);

private:
    QLineEdit* txtUrl;
    QPushButton* btnOpenPlayer;
    QPushButton* btnToggleMenu;

    QWidget* sidebarWidget;
    QTreeWidget* treeMenu;
    QTreeWidgetItem* itemCanales;

    QFrame* channelPanel;
    QLineEdit* categorySearch;

    QListView* categoryList;
    ChannelListModel* channelModel;

    QPropertyAnimation* channelAnimation;

    VideoPlayerWindow* playerWindow;
    M3UParser m3uParser;
    QList<M3UItem> currentChannels;

    UpdateNotifier* updateNotifier;

    void setupChannelPanel();
    void populateChannelPanel();

    void setupUi();
    void playRandomChannel();

    bool sidebarWasVisibleBeforeFS = false;
    bool channelPanelWasOpenBeforeFS = false;
};

#endif // MAINWINDOW_HPP