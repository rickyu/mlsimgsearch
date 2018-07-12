#include <dirent.h>
#include <sys/stat.h>
#include "reducer_result.h"
#include <boost/lexical_cast.hpp>
#include <bof_feature_analyzer/idf_weighter.h>


/**
 * index_formatter bucket_count mapreduce_result_local_path
 *                 save_index_data_path save_index_hint_path
 */
int main(int argc, char **argv) {
  if (argc < 5) {
    printf("Wrong format!\n");
    printf("Usage: index_formatter bucket_count mapreduce_result_local_path "
           "save_index_data_path save_index_hint_path\n");
    return -1;
  }

  /*try*/ {
    int bucket_count = boost::lexical_cast<int>(argv[1]);
    std::string mapreduce_result_local_path(argv[2]),
      index_data_path(argv[3]),
      index_hint_path(argv[4]),
      idf_word_frequence(argv[5]),
      vocabulary_file_path(argv[6]);

    
    cv::Mat vocabulary;
    cv::FileStorage fs(vocabulary_file_path, cv::FileStorage::READ);      
    fs["vocabulary"] >> vocabulary;
    fs.release();
    
    IDFWeighter * weighter = new IDFWeighter;
    weighter->SetConf(idf_word_frequence);
    weighter->SetWordsNum(vocabulary.rows);
    weighter->Init();
    ReducerResultResolver resolver(bucket_count);// K 10000
    if (0 != resolver.Init(index_data_path, index_hint_path)) {
      printf("wrong data/hint path\n");
      return -2;
    }
    struct dirent *dirp;
    struct stat filestat;
    DIR *dp = opendir(mapreduce_result_local_path.c_str());
    if (NULL == dp) {
      printf("error path %s", mapreduce_result_local_path.c_str());
      return -3;
    }
    std::string filepath;
    while ((dirp = readdir( dp ))) {
      filepath = mapreduce_result_local_path + "/" + dirp->d_name;
                
      // If the file is a directory (or is in some way invalid) we'll skip it 
      if (stat( filepath.c_str(), &filestat )) {
        continue;
      }
      if (S_ISDIR( filestat.st_mode )) {
        continue;
      }
      if (dirp->d_name[0] == '.') {
        continue; //hidden file!
      }
      if (0 != resolver.ParseFile(filepath,weighter) ){
        return -4;
      }
    }
    resolver.SaveIndex();
    delete weighter;
  }
  // catch (...) {
  //   printf("catch an exception\n");
  //   return -10;
  // }
  return 0;
}
