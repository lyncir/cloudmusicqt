#include "musicdownloadmodel.h"

#include "musicdownloader.h"
#include <QFile>
#include <QTimer>

MusicDownloadModel::MusicDownloadModel(QObject *parent) :
    QAbstractListModel(parent), mDataType(ProcessingData)
{
    QHash<int, QByteArray> names;
    names[IdRole] = "id";
    names[NameRole] = "name";
    names[ArtistRole] = "artist";
    names[StatusRole] = "status";
    names[ProgressRole] = "progress";
    names[SizeRole] = "size";
    names[ErrCodeRole] = "errcode";
    setRoleNames(names);
    refresh();

    connect(MusicDownloader::Instance(), SIGNAL(dataChanged(MusicDownloadItem*)),
            SLOT(refresh(MusicDownloadItem*)));
}

MusicDownloadModel::~MusicDownloadModel()
{
    qDeleteAll(mDataList);
}

MusicDownloadModel::DataType MusicDownloadModel::dataType() const
{
    return mDataType;
}

void MusicDownloadModel::setDataType(const DataType &type)
{
    if (mDataType != type) {
        mDataType = type;
        emit dataTypeChanged();
        refresh();
    }
}

int MusicDownloadModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return mDataList.size();
}

QVariant MusicDownloadModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row < 0 || row >= mDataList.size())
        return QVariant();

    const MusicDownloadItem* ptr = mDataList.at(row);
    switch (role)
    {
    case IdRole: return ptr->id;
    case NameRole: return ptr->name;
    case ArtistRole: return ptr->artist;
    case StatusRole:
        if (ptr->status == MusicDownloadItem::Completed && !QFile::exists(ptr->fileName))
            return MusicDownloadItem::Failed;
        else
            return ptr->status;
    case ProgressRole: return ptr->progress;
    case SizeRole: return ptr->size;
    case ErrCodeRole:
        if (ptr->status == MusicDownloadItem::Completed && !QFile::exists(ptr->fileName))
            return MusicDownloadItem::FileRemovedError;
        else
            return ptr->errcode;
    default: return QVariant();
    }
}

int MusicDownloadModel::getIndexByMusicId(const QString &musicId) const
{
    for (int i = 0; i < mDataList.size(); i++) {
        if (mDataList.at(i)->id == musicId)
            return i;
    }
    return -1;
}

QList<MusicDownloadItem*> MusicDownloadModel::getDataList() const
{
    QList<MusicDownloadItem*> list;
    foreach (MusicDownloadItem* item, mDataList) {
        MusicDownloadItem* copy = new MusicDownloadItem;
        copy->id = item->id;
        copy->name = item->name;
        copy->artist = item->artist;
        copy->status = item->status;
        copy->progress = item->progress;
        copy->size = item->size;
        copy->remoteUrl = item->remoteUrl;
        copy->fileName = item->fileName;
        copy->errcode = item->errcode;
        copy->rawData = item->rawData;
        list.append(copy);
    }
    return list;
}

void MusicDownloadModel::refresh(MusicDownloadItem *item)
{
    if (item) {
        for (int i = 0; i < mDataList.size(); i++) {
            MusicDownloadItem* myData = mDataList.at(i);
            if (myData->id == item->id) {
                myData->status = item->status;
                myData->errcode = item->errcode;
                myData->progress = item->progress;
                myData->size = item->size;
                emit dataChanged(index(i), index(i));
                return;
            }
        }
        if (mDataType == CompletedData)
            return;
    }

    beginResetModel();
    qDeleteAll(mDataList);
    mDataList = MusicDownloader::Instance()->getCompletedRecords(mDataType == CompletedData);
    endResetModel();

    QTimer::singleShot(0, this, SIGNAL(loadFinished()));
}
