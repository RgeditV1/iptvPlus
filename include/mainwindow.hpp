#pragma once

#include "moviedetail.hpp"

#include <QMainWindow>
#include <QPushButton>
#include <QTreeWidget>
#include <QStackedWidget>
#include <QPropertyAnimation>
#include <QEvent>

class TvWidget;
class MoviesWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupUi();
    void setupSidebar();
    void toggleSidebar();
    void toggleSidebar(bool show);
    void onNavigationItemClicked(QTreeWidgetItem* item, int column);

private:
    QWidget* m_centralContainer = nullptr;
    QStackedWidget* m_stackedWidget = nullptr;

    // Vistas principales
    MoviesWidget* m_moviesView = nullptr;
    MovieDetailWidget* m_movieDetailView = nullptr;
    TvWidget* m_tvView = nullptr;

    // Sidebar Widgets
    QWidget* m_sidebar = nullptr;
    QPushButton* m_menuButton = nullptr;
    QTreeWidget* m_navTree = nullptr;

    // Animación
    QPropertyAnimation* m_sidebarAnimation = nullptr;
    bool m_sidebarVisible = false;
    const int m_sidebarWidth = 250;
};