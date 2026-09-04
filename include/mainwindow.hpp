#pragma once

#include "update.hpp"
#include "m3uparser.hpp"
#include "channellistmodel.hpp"
#include "movies.hpp"

#include <QMainWindow>
#include <QTreeWidget>
#include <QListView>
#include <QTimer>
#include <QIcon>
#include <QFrame>
#include <QPropertyAnimation>
#include <QListWidget>
#include <QRandomGenerator>
#include <QStackedWidget>
#include <QComboBox>

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
    /**
     * @brief Intercepta eventos de objetos supervisados (ej. atajos de teclado o clicks externos).
     * @param watched Objeto receptor del evento.
     * @param event Información del evento a evaluar.
     * @return true si el evento fue gestionado completamente; false de lo contrario.
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    /**
     * @brief Abre o enfoca la ventana del reproductor multimedia.
     */
    void openPlayer();

    /**
     * @brief Alterna la visibilidad de la barra lateral de navegación mediante animación.
     */
    void toggleSidebar();

    /**
     * @brief Despliega u oculta el panel que muestra la lista de canales.
     */
    void toggleChannelPanel();

    /**
     * @brief Aplica un filtro sobre la lista de canales en tiempo real.
     * @param text Texto ingresado para la búsqueda.
     */
    void filterChannelItems(const QString& text);

    /**
     * @brief Maneja la selección de elementos en el árbol de categorías.
     * @param item Elemento del QTreeWidget presionado.
     * @param column Índice de la columna donde se realizó el clic.
     */
    void onItemClicked(QTreeWidgetItem* item, int column);

    /**
     * @brief Ajusta la interfaz según si el reproductor entra o sale del modo pantalla completa.
     * @param isFullScreen Indicacion del estado de pantalla completa.
     */
    void onFullScreenToggled(bool isFullScreen);

private:
    QLineEdit* txtUrl;
    QLineEdit* searchMoviesEdit;
    QComboBox* genreComboBox;
    QPushButton* btnOpenPlayer;
    QPushButton* btnToggleMenu;

    QWidget* sidebarWidget;
    QTreeWidget* treeMenu;
    QTreeWidgetItem* itemCanales;
    QTreeWidgetItem* itemPeliculas;
    QStackedWidget* stackedWidget;
    

    QFrame* channelPanel;
    QLineEdit* categorySearch;

    QListView* categoryList;
    ChannelListModel* channelModel;

    QPropertyAnimation* channelAnimation;

    VideoPlayerWindow* playerWindow;
    MoviesWidget* moviesWidget;

    M3UParser m3uParser;
    QList<M3UItem> currentChannels;

    UpdateNotifier* updateNotifier;

    MovieDetailsWidget* movieDetailsWidget;

    /**
     * @brief Configura la estructura visual y propiedades del panel lateral de canales.
     */
    void setupChannelPanel();

    /**
     * @brief Carga los canales disponibles en la lista gráfica.
     */
    void populateChannelPanel();

    /**
     * @brief Inicializa todos los componentes de la interfaz de usuario.
     */
    void setupUi();

    /**
     * @brief Selecciona y reproduce un canal aleatorio del listado actual.
     */
    void playRandomChannel();

    bool sidebarWasVisibleBeforeFS = false;
    bool channelPanelWasOpenBeforeFS = false;
};