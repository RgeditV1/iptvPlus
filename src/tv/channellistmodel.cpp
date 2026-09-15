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
    currentFilterText = text;
    applyFilters();
}

void ChannelListModel::filterByCountry(const QString& country)
{
    currentCountryFilter = country;
    applyFilters();
}

void ChannelListModel::applyFilters()
{
    beginResetModel();
    filteredIndexes.clear();

    for (int i = 0; i < allChannels.size(); ++i) {
        const M3UItem& item = allChannels.at(i);

        bool matchesText = currentFilterText.isEmpty() || 
                           item.title.contains(currentFilterText, Qt::CaseInsensitive);
        
        bool matchesCountry = currentCountryFilter.isEmpty() || 
                              currentCountryFilter.compare("Todos", Qt::CaseInsensitive) == 0 ||
                              item.country.trimmed().compare(currentCountryFilter.trimmed(), Qt::CaseInsensitive) == 0;

        if (matchesText && matchesCountry) {
            filteredIndexes.append(i);
        }
    }

    endResetModel();
}

QStringList ChannelListModel::getAvailableCountries() const
{
    QSet<QString> countries;
    for (const auto& item : allChannels) {
        if (!item.country.isEmpty()) {
            countries.insert(item.country);
        }
    }
    QStringList result = countries.values();
    result.sort();
    result.prepend("Todos");
    return result;
}