#include <bof_searcher/indexer.h>
#include <boost/lexical_cast.hpp>

static int merger_words(const WordIndex &index1,
                        const WordIndex &index2,
                        WordIndex &result);

/**
 * Usage: index_merger K index_old.hint index_old.data
 *        index_new.hint index_new.data index_save.hint index_save.data
 */
int main(int argc, char **argv) {
  if (argc != 8) {
    printf("Usage: index_merger K index_old.hint index_old.data "
           "index_new.hint index_new.data "
           "index_save.hint index_save.data\n");
    return -1;
  }

  int K = boost::lexical_cast<int>(argv[1]);
  char *index_old_hint_path = argv[2], *index_old_data_path = argv[3];
  char *index_new_hint_path = argv[4], *index_new_data_path = argv[5];
  char *index_save_hint_path = argv[6], *index_save_data_path = argv[7];

  BOFIndex old_indexer;
  int ret = old_indexer.Init(K, index_old_hint_path, index_old_data_path);
  if (0 != ret) {
    printf("old indexer init failed, ret=%d\n", ret);
    return -2;
  }
  BOFIndex new_indexer;
  ret = new_indexer.Init(K, index_new_hint_path, index_new_data_path);
  if (0 != ret) {
    printf("new indexer init failed, ret=%d\n", ret);
    return -3;
  }
  IndexHinter hint(K);
  std::fstream fs_save_hint;
  fs_save_hint.open(index_save_hint_path,
                    std::fstream::out | std::fstream::binary);
  if (!fs_save_hint.is_open()) {
    return -4;
  }
  std::fstream fs_save_data;
  fs_save_data.open(index_save_data_path,
                    std::fstream::out | std::fstream::binary);
  if (!fs_save_data.is_open()) {
    return -5;
  }
  for (int i = 0; i < K; ++i) {
    printf("begin merge word %d\n", i);
    WordIndex old_words, new_words, save_words;
    char *old_data = NULL, *new_data = NULL;
    if (0 != old_indexer.LoadWord(i, old_words, old_data)) {
      printf("load old data failed, k=%d\n", i);
      return -6;
    }
    if (0 != new_indexer.LoadWord(i, new_words, new_data)) {
      printf("load new data failed, k=%d\n", i);
      return -7;
    }
    merger_words(old_words, new_words, save_words);
    long cur_pos = fs_save_data.tellp();
    char *index_data = NULL;
    IndexerSerializer index_serializer;
    int index_data_size = index_serializer.Serialize(save_words, index_data);
    fs_save_data.write(index_data, index_data_size);
    if (index_data) {
      delete [] index_data;
      index_data = NULL;
    }
    long end_pos = fs_save_data.tellp();
    hint.SetHint(i, 0, cur_pos, end_pos);

    save_words.items_.clear();
    old_words.ClearItems();
    delete [] old_data;
    new_words.ClearItems();
    delete [] new_data;
  }

  boost::archive::text_oarchive oa(fs_save_hint);
  oa << hint;
  fs_save_hint.close();
  fs_save_data.close();
}

int merger_words(const WordIndex &index_old,
                 const WordIndex &index_new,
                 WordIndex &result) {
  std::vector<IndexItem*>::const_iterator it_old = index_old.items_.begin();
  std::vector<IndexItem*>::const_iterator it_new = index_new.items_.begin();
  while (it_old != index_old.items_.end() && it_new != index_new.items_.end()) {
    if ((*it_old)->pic_id_ == (*it_new)->pic_id_) {
      result.items_.push_back(*it_new);
      ++it_old;
      ++it_new;
    }
    else if ((*it_old)->pic_id_ < (*it_new)->pic_id_) {
      result.items_.push_back(*it_old);
      ++it_old;
    }
    else {
      result.items_.push_back(*it_new);
      ++it_new;
    }
  }

  while (it_old != index_old.items_.end()) {
    result.items_.push_back(*it_old);
    ++it_old;
  }
  while (it_new != index_new.items_.end()) {
    result.items_.push_back(*it_new);
    ++it_new;
  }
  return 0;
}
