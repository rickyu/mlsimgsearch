#include "reducer_result.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#include <fstream>
#include <bof_searcher/index_hinter.h>
#include <bof_searcher/indexer_serializer.h>
#include "image_quantized_serializer.h"

ReducerBucketResult::ReducerBucketResult(int bucket_id)
    : bucket_id_(bucket_id)
    , bucket_(NULL) {
}

ReducerBucketResult::~ReducerBucketResult() {
  if (bucket_) {
    delete bucket_;
    bucket_ = NULL;
  }
}

const int ReducerBucketResult::Init() {
  bucket_ = new BOFIndexBucket(bucket_id_);
  if (NULL == bucket_) {
    return -1;
  }
  return 0;
}

const int ReducerBucketResult::AddLine(const int64_t pic_id,
                                       const float q,
                                       const std::string &words1,
                                       const std::string &words2) {
  BOFQuantizeResult bof_info;
  bof_info.word_info_[0] = words1;
  bof_info.word_info_[1] = words2;
  bof_info.q_ = q ;
  return bucket_->AddItem(pic_id, bof_info);
}

void ReducerBucketResult::PrintResult() {
  std::stringstream ss;
  boost::archive::text_oarchive oa(ss);
  oa << *bucket_;
  // print the length of this bucket data
  std::cout << bucket_id_ << "\t"
            << ss.str().length() << std::endl;
  // print the data now
  std::cout << ss.str() ;
}

ReducerResultResolver::ReducerResultResolver(const int bucket_count)
    : hinter_(bucket_count) {
  cur_file_pos_ = 0;
}

ReducerResultResolver::~ReducerResultResolver() {
  fs_data_.close();
  fs_hint_.close();
}

const int ReducerResultResolver::Init(const std::string &index_data_path,
                                      const std::string &index_hint_path) {
  fs_data_.open(index_data_path.c_str(), std::fstream::out | std::fstream::binary);
  if (!fs_data_.is_open()) {
    return -1;
  }

  fs_hint_.open(index_hint_path.c_str(), std::fstream::out | std::fstream::binary);
  if (!fs_hint_.is_open()) {
    return -2;
  }
  return 0;
}

const int ReducerResultResolver::Normalize(const std::string &image_quantized_info,
                                           const BOFWeighter *weighter,
                                           int bucket_id,
                                           int feature_num,
                                           float &q) {
  ImageQuantizedData qdata;
  if (0 != ImageQuantizedSerialzer::Unserialize(image_quantized_info, qdata)) {
    return -1;
  }
  const_cast<BOFWeighter*>(weighter)->Weighting(bucket_id, q, &feature_num);
  float sum = 0.f;
  for (size_t i = 0; i < qdata.size(); ++i) {
    const_cast<BOFWeighter*>(weighter)->Weighting(qdata[i].word_id_,
                                                  qdata[i].weight_,
                                                  &feature_num);
    sum += qdata[i].weight_;
  }
  q = sqrt(q/sum);
  return 0;
}

const int ReducerResultResolver::ParseFile(const std::string &file_path,
                                           BOFWeighter *weighter) {
  printf("begin parse file: %s\n", file_path.c_str());

  std::fstream fs;
  fs.open(file_path.c_str(), std::fstream::in);
  if (!fs.is_open()) {
    printf("bad file\n");
    return -1;
  }

  std::vector<std::string> fields;
  while (!fs.eof() && fs.good()) {
    /*try*/ {
      // read/parse bucket_id and bucket_data
      char line[128] = {0};
      fs.getline(line, sizeof(line));
      fields.clear();
      boost::algorithm::split(fields, line, boost::algorithm::is_any_of("\t"));
      if (0 == strlen(line)) {
        printf("read an empty line\n");
        break;
      }
      if (fields.size() < 2 || fields[0].empty()) {
        printf("wrong file format, maybe at the end. file=%s, line=%s\n", file_path.c_str(), line);
        continue;
      }
      boost::algorithm::trim(fields[0]);
      boost::algorithm::trim(fields[1]);
      int64_t bucket_id = boost::lexical_cast<int64_t>(fields[0]);
      int64_t bucket_data_len = boost::lexical_cast<int64_t>(fields[1]);
      printf("read line from %s, bucket_id=%ld, bucket_data_len=%ld\n",
             file_path.c_str(), bucket_id, bucket_data_len);
      // set hint and write data
      char *data = new char[bucket_data_len+1];
      fs.read(data, bucket_data_len);

      BOFIndexBucket bucket;
      std::stringstream ss;
      ss.write(data, bucket_data_len);
      boost::archive::text_iarchive ia(ss);
      ia >> bucket;
    
      std::vector<BOFIndexItem> *pitems;
      bucket.GetItems(pitems);
      std::vector<BOFIndexItem> &items = *pitems;

      size_t item_index = 0;
      int num = 1;
      // float word_q = 1 ;
      // weighter->Weighting(bucket_id, word_q,&num); 
      // if (word_q <= 0.15 ) {
      //   continue;
      // }
      
      // while (item_index < items.size()) {
      //   // Normalize(items[item_index].bof_info_.word_info_[2],
      //   //           weighter,
      //   //           bucket_id,
      //   //           num,
      //   //           items[item_index].bof_info_.q_);
      //   weighter->Weighting(bucket_id,items[item_index].bof_info_.q_,&num); 
      //   //printf("data =  %ld %f \n",bucket_id,items[item_index].bof_info_.q_);
      //   item_index ++;
      // }
      if (data) {
        delete [] data;
        data = NULL;
      }

      long cur_pos = fs_data_.tellp();
      WordIndex windex;
      IndexerSerializer index_serializer;
      bucket.ConvertB2W(windex);
      char *index_data = NULL;
      int index_data_size = index_serializer.Serialize(windex, index_data);
      fs_data_.write(index_data, index_data_size);
      if (index_data) {
        delete [] index_data;
        index_data = NULL;
      }
      long end_pos = fs_data_.tellp();
      hinter_.SetHint(bucket_id, 0, cur_pos, end_pos);
      printf("bucket_id=%ld, cur_pos=%ld, end_pos=%ld, index_data_size=%d\n",
             bucket_id, cur_pos, end_pos, index_data_size);
    }
    /*catch (...) {
      printf("catch an exception\n");
      }*/
  }
  return 0;
}

void ReducerResultResolver::SaveIndex() {
  boost::archive::text_oarchive oa(fs_hint_);
  oa << hinter_;
}
