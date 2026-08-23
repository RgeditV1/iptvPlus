#ifndef CHANNELLISTMODEL_HPP
#define CHANNELLISTMODEL_HPP

#include "m3uparser.hpp"

#include <QAbstractListModel>
#include <QIcon>
#include <QList>

class ChannelListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ChannelListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(
        const QModelIndex& index,
        int role = Qt::DisplayRole
    ) const override;

    void setChannels(const QList<M3UItem>& channels);

    const M3UItem* channelAt(int row) const;

    void filter(const QString& text);

private:
    QList<M3UItem> allChannels;
    QList<int> filteredIndexes;

    QIcon channelIcon;
};

#endif // CHANNELLISTMODEL_HPP