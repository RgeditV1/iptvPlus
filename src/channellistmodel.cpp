#include "channellistmodel.hpp"

ChannelListModel::ChannelListModel(QObject* parent)
    : QAbstractListModel(parent),
    channelIcon(":/resources/icons/play-circle.svg")
{
}

int ChannelListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return filteredIndexes.size();
}

QVariant ChannelListModel::data(
    const QModelIndex& index,
    int role) const
{
    if (!index.isValid())
        return {};

    if (index.row() < 0 ||
        index.row() >= filteredIndexes.size()) {
        return {};
    }

    const int originalIndex =
        filteredIndexes.at(index.row());

    const M3UItem& channel =
        allChannels.at(originalIndex);

    switch (role) {

    case Qt::DisplayRole:
        return channel.title.isEmpty()
            ? channel.url
            : channel.title;

    case Qt::DecorationRole:
        return channelIcon;

    case Qt::UserRole:
        return channel.url;

    default:
        return {};
    }
}

void ChannelListModel::setChannels(
    const QList<M3UItem>& channels)
{
    beginResetModel();

    allChannels = channels;

    filteredIndexes.clear();
    filteredIndexes.reserve(allChannels.size());

    for (int i = 0; i < allChannels.size(); ++i)
        filteredIndexes.append(i);

    endResetModel();
}

const M3UItem* ChannelListModel::channelAt(int row) const
{
    if (row < 0 || row >= filteredIndexes.size())
        return nullptr;

    return &allChannels.at(filteredIndexes.at(row));
}

void ChannelListModel::filter(const QString& text)
{
    const QString searchText =
        text.trimmed();

    beginResetModel();

    filteredIndexes.clear();

    if (searchText.isEmpty()) {

        filteredIndexes.reserve(allChannels.size());

        for (int i = 0; i < allChannels.size(); ++i)
            filteredIndexes.append(i);

    }
    else {

        for (int i = 0; i < allChannels.size(); ++i) {

            const M3UItem& channel =
                allChannels.at(i);

            const QString title =
                channel.title.isEmpty()
                ? channel.url
                : channel.title;

            if (title.contains(
                searchText,
                Qt::CaseInsensitive)) {

                filteredIndexes.append(i);
            }
        }
    }

    endResetModel();
}