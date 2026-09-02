#pragma once
#include "m3uparser.hpp"

#include <QAbstractListModel>
#include <QIcon>
#include <QList>

class ChannelListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    /**
     * @brief Construye el modelo de la lista de canales.
     * @param parent Objeto Qt primario opcional.
     */
    explicit ChannelListModel(QObject* parent = nullptr);

    /**
     * @brief Obtiene el número de filas presentes en el modelo según el filtro actual.
     * @param parent Índice del elemento padre (no utilizado en modelos de lista).
     * @return Cantidad de elementos actualmente visibles.
     */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Devuelve los datos almacenados en un índice y rol determinados.
     * @param index Índice del elemento solicitado.
     * @param role Rol del cual se extraen los datos (ej. DisplayRole, DecorationRole).
     * @return QVariant con la información solicitada para la interfaz gráfica.
     */
    QVariant data(
        const QModelIndex& index,
        int role = Qt::DisplayRole
    ) const override;

    /**
     * @brief Asigna la lista completa de canales al modelo y reinicia la vista.
     * @param channels Lista con las estructuras M3UItem a cargar.
     */
    void setChannels(const QList<M3UItem>& channels);

    /**
     * @brief Recupera un puntero al canal situado en una fila específica.
     * @param row Índice de la fila del canal dentro de la vista filtrada.
     * @return Puntero constante al M3UItem correspondiente o nullptr si el índice es inválido.
     */
    const M3UItem* channelAt(int row) const;

    /**
     * @brief Aplica un filtro de texto sobre los nombres de los canales.
     * @param text Criterio de búsqueda para filtrar la lista.
     */
    void filter(const QString& text);

private:
    QList<M3UItem> allChannels;
    QList<int> filteredIndexes;

    QIcon channelIcon;
};