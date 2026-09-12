#include "channellistmodel.hpp"

ChannelListModel::ChannelListModel(QObject* parent)
    : QAbstractListModel(parent),
      channelIcon(":/resources/icons/play-circle.svg")
{
}

int ChannelListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return filteredIndexes.size();
}

QVariant ChannelListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= filteredIndexes.size()) {
        return QVariant();
    }

    int realIndex = filteredIndexes.at(index.row());
    const M3UItem& item = allChannels.at(realIndex);

    switch (role) {
    case Qt::DisplayRole:
        return item.title;
    case Qt::DecorationRole:
        return channelIcon;
    case Qt::UserRole:
        return item.url;
    default:
        return QVariant();
    }
}

void ChannelListModel::setChannels(const QList<M3UItem>& channels)
{
    beginResetModel();
    allChannels = channels;
    filteredIndexes.clear();
    filteredIndexes.reserve(allChannels.size());
    for (int i = 0; i < allChannels.size(); ++i) {
        filteredIndexes.append(i);
    }
    endResetModel();
}

const M3UItem* ChannelListModel::channelAt(int row) const
{
    if (row < 0 || row >= filteredIndexes.size()) return nullptr;
    return &allChannels.at(filteredIndexes.at(row));
}

void ChannelListModel::filter(const QString& text)
{
    beginResetModel();
    filteredIndexes.clear();
    
    if (text.trimmed().isEmpty()) {
        filteredIndexes.reserve(allChannels.size());
        for (int i = 0; i < allChannels.size(); ++i) {
            filteredIndexes.append(i);
        }
    } else {
        for (int i = 0; i < allChannels.size(); ++i) {
            if (allChannels.at(i).title.contains(text, Qt::CaseInsensitive)) {
                filteredIndexes.append(i);
            }
        }
    }
    endResetModel();
}