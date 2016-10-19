#include "sm4ceps_livelog_treemodel.h"
#include <thread>
#include <mutex>
#include <strstream>


LivelogTreeModel::LivelogTreeModel( QObject *parent ): QAbstractItemModel(parent){
    connect(this,SIGNAL(append_rows(int,int)),this,SLOT(do_append_rows(int,int)),Qt::QueuedConnection);
    log_entries_peer = new livelog::Livelogger(200,20000000);
    log_entries_peer->register_storage(sm4ceps::STORAGE_IDX2FQS,new livelog::Livelogger::Storage (4096*1000));
    livelogger_sink = new sm4ceps::Livelogger_sink(log_entries_peer);
    livelogger_sink->run();
    new std::thread{&LivelogTreeModel::check_for_new_entries,this};
    //std::this_thread::sleep_for(std::chrono::milliseconds(2500));
}

LivelogTreeModel::~LivelogTreeModel(){}

QVariant LivelogTreeModel::data(const QModelIndex &index, int role) const{
    if (!index.isValid())
      return QVariant();

    if (role != Qt::DisplayRole)
      return QVariant();

    {
     std::lock_guard<std::mutex> lk1(log_entries_peer->mutex_trans2consumer());
     if (chunks.size() <= index.row()) return QVariant();
     if (index.column() == 0) return index.row();
     //std::cout << index.row()<< "/" << index.column() << std::endl;
     auto& ch = chunks[index.row()];
     if (ch.what == sm4ceps::STORAGE_WHAT_CURRENT_STATES){
         if (index.column() == 1) return QVariant("Active States");
         if (index.column() == 2){
           std::stringstream ss;
           auto & t = id2name;
           sm4ceps::extract_current_states_raw(ch.data,ch.len,
             [&](std::vector<int> states){
               for(auto const & e : states) ss << t[e] << " ";
             });
           return ss.str().c_str();
         }
     }
    }
    return QVariant();
}

Qt::ItemFlags LivelogTreeModel::flags(const QModelIndex &index) const {
    if (!index.isValid())
            return 0;

    return QAbstractItemModel::flags(index);
}

QVariant LivelogTreeModel::headerData(int section, Qt::Orientation orientation,
                    int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole){
        if (section == 0) return QVariant("No.");
        if (section == 1) return QVariant("What");
        if (section == 2) return QVariant("Summary");
        if (section == 3) return QVariant("Time");
    }
    return QVariant();
}
QModelIndex LivelogTreeModel::LivelogTreeModel::index(int row, int column,
                  const QModelIndex &parent) const {
   if (!hasIndex(row, column, parent))
         return QModelIndex();

   return createIndex(row, column, nullptr);
}
QModelIndex LivelogTreeModel::parent(const QModelIndex &index) const {
 return QModelIndex();
}

int LivelogTreeModel::rowCount(const QModelIndex &parent) const {
 if (parent.column() > 0)
  return 0;
 if (!parent.isValid()){
  std::lock_guard<std::mutex> lk1(log_entries_peer->mutex_trans2consumer());
  return chunks.size();
 }
 return 0;
}
int LivelogTreeModel::columnCount(const QModelIndex &parent ) const {
 return 3;
}

void LivelogTreeModel::update_chunks(){
 chunks.resize(log_entries_peer->trans_storage().size());
 std::size_t j = 0;
 auto& ch = chunks;
 for_each(log_entries_peer->trans_storage(),
  [&](char* data,
      livelog::Livelogger::Storage::id_t id,
      livelog::Livelogger::Storage::what_t what,
      livelog::Livelogger::Storage::len_t len )
     {
      ch[j].id = id;ch[j].what = what;ch[j].len = len;ch[j].data = data;++j;
     });
}

void LivelogTreeModel::do_append_rows(int from, int to){
    QModelIndex idx;
    this->beginInsertRows(idx,from,to);
    this->endInsertRows();
}

void LivelogTreeModel::check_for_new_entries(){
 for(;;){
  log_entries_peer->flush();
  {
   auto r = log_entries_peer->find_storage_by_id(sm4ceps::STORAGE_IDX2FQS);
   livelog::Livelogger::Storage* s =  std::get<0>(r->second);
   std::mutex& m = *std::get<1>(r->second);
   std::lock_guard<std::mutex> lk(m);
   livelog::for_each(*s,
    [&](void* data,
        livelog::Livelogger::Storage::id_t id,
        livelog::Livelogger::Storage::what_t what,
        livelog::Livelogger::Storage::len_t len)
       {
         if (what == sm4ceps::STORAGE_WHAT_INT32_TO_STRING_MAP)
          sm4ceps::storage_read_entry(id2name,(char*)data);
       }
   );
  }
  int delta = 0;int old_size=0;
  {
   std::lock_guard<std::mutex> lk1(log_entries_peer->mutex_trans2consumer());
   old_size = chunks.size();
   update_chunks();
   delta = chunks.size() - old_size;
  }
  if (delta > 0){
      emit do_append_rows(old_size,old_size+delta);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
 }//for
}



