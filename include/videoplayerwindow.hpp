#pragma once

#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QSlider>
#include <QResizeEvent>
#include <QPushButton>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>

#include <mpv/client.h>
#include "channellistmodel.hpp"

class LoadingSpinner;

class VideoPlayerWindow : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Construye la ventana del reproductor de video libmpv.
     * @param parent Objeto QWidget primario.
     */
    explicit VideoPlayerWindow(QWidget* parent = nullptr);

    /**
     * @brief Destructor que libera los recursos de libmpv y la interfaz.
     */
    ~VideoPlayerWindow();

    /**
     * @brief Carga y reproduce un flujo multimedia dado por su URL.
     * @param url Dirección del flujo o archivo a reproducir.
     */
    void playMedia(const QString& url);

    /**
     * @brief Detiene inmediatamente la reproducción actual en libmpv.
     */
    void stopMedia();

    /**
     * @brief Enlaza el modelo de canales con la ventana del reproductor para permitir la navegación.
     * @param model Puntero al ChannelListModel activo.
     */
    void setModel(ChannelListModel* model);

    /**
     * @brief Inicia la reproducción del canal situado en una fila específica del modelo.
     * @param row Índice del canal a reproducir.
     */
    void playChannelAt(int row);

        /**
     * @brief Alterna entre el modo en ventana y pantalla completa.
     */
    void toggleFullScreen();
     
    void setNavigationButtonsVisible(bool visible);
    void setBackToDetailsVisible(bool visible);

protected:
    /**
     * @brief Evento para detectar movimiento de cursor u ocultamiento de controles sobre la ventana.
     * @param watched Objeto supervisado.
     * @param event Información del evento.
     * @return true si el evento se procesó completamente.
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

    /**
     * @brief Ajusta la disposición interna y capas superpuestas cuando la ventana cambia de tamaño.
     * @param event Evento de redimensionamiento.
     */
    void resizeEvent(QResizeEvent* event) override;

private slots:
    /**
     * @brief Procesa de manera periódica los eventos emitidos por la instancia de libmpv.
     */
    void onMpvEvents();

    /**
     * @brief Alterna el estado del reproductor entre pausa y reproducción.
     */
    void togglePlayPause();

    /**
     * @brief Ajusta el volumen del sistema de audio de libmpv.
     * @param value Nivel de volumen (0 a 100).
     */
    void setVolume(int value);

    /**
     * @brief Alterna el estado de silencio (mute) del reproductor.
     */
    void togleMute();

    /**
     * @brief Pasa al canal anterior registrado en el modelo.
     */
    void playPreviousChannel();

    /**
     * @brief Pasa al siguiente canal registrado en el modelo.
     */
    void playNextChannel();

signals:
    /**
     * @brief Señal emitida cuando cambia el canal en reproducción actual.
     * @param row Índice del nuevo canal en el modelo.
     */
    void channelChanged(int row);

    /**
     * @brief Señal emitida cuando cambia el estado de pantalla completa.
     * @param isFullScreen true si está en pantalla completa, false en caso contrario.
     */
    void fullScreenToggled(bool isFullScreen);

    void backToDetailsRequested();

private:
    mpv_handle* mpv;
    QTimer* mpvTimer;

    QWidget* videoContainer;
    QWidget* controlsContainer;

    QPushButton* btnPlayPause;
    QPushButton* btnStop;

    QWidget* loadingOverlay;
    LoadingSpinner* loadingSpinner;
    QTimer* spinnerTimer;
    
    QPushButton* btnPrevious;
    QPushButton* btnNext;
    QPushButton* btnFullScreen;

    QWidget* volumeContainer;
    QToolButton* btnVolume;
    QSlider* sliderVolume;

    QPushButton* btnBackToDetails;

    bool isPaused;
    bool isMuted;
    bool buffering = false;

    int volume;
    int previousVolume;
    int spinnerAngle;

    double cacheDuration = 0.0;

    ChannelListModel* channelModel;
    int currentChannelRow;

    QTimer* controlsTimer;

    /**
     * @brief Inicializa la instancia libmpv y ajusta sus propiedades iniciales de renderizado.
     */
    void initMpv();

    /**
     * @brief Construye la estructura visual de controles multimedia.
     */
    void setupUi();

    /**
     * @brief Configura la capa de superposición con el indicador de carga (spinner).
     */
    void setupLoadingSpinner();

    /**
     * @brief Muestra u oculta la animación de carga según el estado de la red/búfer.
     * @param visible true para mostrar, false para ocultar.
     */
    void setLoadingSpinnerVisible(bool visible);

    /**
     * @brief Actualiza la rotación del spinner en la animación de carga.
     */
    void updateSpinner();

    /**
     * @brief Evalúa los parámetros del búfer de libmpv para determinar si se requiere mostrar el spinner.
     */
    void updateBufferingState();

    /**
     * @brief Muestra u oculta la barra de controles inferior con temporizador automático.
     * @param show true para forzar la visibilidad de los controles.
     */
    void updateControls(bool show = true);

    /**
     * @brief Actualiza el icono del botón de volumen basándose en el nivel actual o silencio.
     */
    void updateVolumeIcon();
};

class LoadingSpinner : public QWidget
{
public:
    /**
     * @brief Construye el widget para la animación de carga circular.
     * @param parent Objeto QWidget primario.
     */
    explicit LoadingSpinner(QWidget* parent = nullptr);

    /**
     * @brief Define el ángulo de rotación actual del indicador visual.
     * @param angle Ángulo en grados.
     */
    void setAngle(int angle);

protected:
    /**
     * @brief Dibuja el gráfico del spinner sobre el canvas del widget.
     * @param event Información del evento de pintura.
     */
    void paintEvent(QPaintEvent* event) override;

private:
    int angle = 0;
};