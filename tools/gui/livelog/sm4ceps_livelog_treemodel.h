#ifndef SM4CEPS_LIVELOG_TREEMODEL_H
#define SM4CEPS_LIVELOG_TREEMODEL_H



#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <map>
#include <string>
#include <vector>
#include "core/include/livelog/livelogger.hpp"
#include "core/include/sockets/rdwrn.hpp"
#include "core/include/sm_livelog_storage_utils.hpp"


struct chunk_ext{
    livelog::Livelogger::Storage::len_t len = 0;
    livelog::Livelogger::Storage::id_t id = 0;
    livelog::Livelogger::Storage::what_t what = 0;
    char* data = nullptr;
};

class LivelogTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit LivelogTreeModel( QObject *parent = 0);
    ~LivelogTreeModel();

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
signals:
    void append_rows(int from, int to);
private slots:
    void do_append_rows(int from, int to);
private:
    livelog::Livelogger* log_entries_peer;
    sm4ceps::Livelogger_sink* livelogger_sink;
    mutable std::map<int,std::string> id2name;
    mutable std::vector<chunk_ext> chunks;
    void check_for_new_entries();
    void update_chunks();

};




#endif // SM4CEPS_LIVELOG_TREEMODEL_H
