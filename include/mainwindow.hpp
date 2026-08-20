#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>

class VideoPlayerWindow;
class QPushButton;
class QLineEdit;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void openPlayer();

private:
    VideoPlayerWindow* playerWindow;
    QPushButton* btnOpenPlayer;
    QLineEdit* txtUrl;
};

#endif // MAINWINDOW_HPP