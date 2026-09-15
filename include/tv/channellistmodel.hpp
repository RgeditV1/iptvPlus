#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QStringList>
#include <QIcon>
#include <QSet>
#include "m3uparser.hpp"

class ChannelListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ChannelListModel(QObject* parent = nullptr);

    // Métodos obligatorios de QAbstractListModel
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    // Métodos de gestión de canales
    void setChannels(const QList<M3UItem>& channels);
    const M3UItem* channelAt(int row) const;

    // Métodos de filtrado
    void filter(const QString& text);
    void filterByCountry(const QString& country);
    QStringList getAvailableCountries() const;

private:
    void applyFilters();

    QList<M3UItem> allChannels;
    QList<int> filteredIndexes;
    QIcon channelIcon;

    QString currentFilterText;
    QString currentCountryFilter;
};