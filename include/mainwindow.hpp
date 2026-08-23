#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QTreeWidget>
#include <QIcon>
#include "m3uparser.hpp"

class VideoPlayerWindow;
class QLineEdit;
class QPushButton;
class QWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void loadChannelsFromFolder(const QString& dirPath);

private slots:
    void openPlayer();
    void toggleSidebar();
    void filterChannels(const QString& text);
    void onItemClicked(QTreeWidgetItem* item, int column);

private:
    QLineEdit* txtUrl;
    QPushButton* btnOpenPlayer;
    QPushButton* btnToggleMenu;

    QWidget* sidebarWidget;
    QLineEdit* txtSearchChannel;
    QTreeWidget* treeMenu;
    QTreeWidgetItem* itemCanales;

    VideoPlayerWindow* playerWindow;
    M3UParser m3uParser;
    QList<M3UItem> currentChannels;

    void setupUi();
    void populateChannelSubmenu();
};

#endif // MAINWINDOW_HPP