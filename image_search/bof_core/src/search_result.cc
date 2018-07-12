#include <bof_searcher/search_result.h>

SearchResultItem::SearchResultItem() {
  pic_id_ = 0;
  sim_ = 0.0f;
}

SearchResult::SearchResult() {
  items_.clear();
}

void SearchResult::AddItem(int pic_id, float sim) {
  items_.push_back(SearchResultItem(pic_id, sim));
}
