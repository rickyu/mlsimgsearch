#include <fstream>
#include <common/json_parser.h>
#include <common/xml_parser.h>
#include <common/constant.h>
#include <time.h>
#include <common/gearman_worker.h>
#include <common/http_client.h>
#include <string.h>

// test mysql
/*int main() {
  MysqlDB *mysql;
  MysqlDBPool *db_pool = new MysqlDBPool(10);
  for (int i = 0; i < 2; i++) {
  mysql = new MysqlDB;
  mysql->Init();
  //mysql->Connet();
  db_pool->Add(mysql);
  }
  mysql = const_cast<MysqlDB*>(db_pool->Borrow());
  // DO
  db_pool->Release(mysql);
  delete db_pool;
  }*/

// test httpclient
int main() {
  HttpClient *client =new HttpClient;
  HttpClient::GlobalInit();
  client->Init();
  int a = 9;
  int ac  = 0;
  client->GetResponseCode(ac);
  printf("reponse a=%d\n", a);//*/
  /*for (int i = 0; i < 100; i++)*/ 
  {
    char *recv_buf = NULL;
    size_t len = 0;
    char *header_buf = NULL;
    size_t header_len = 0;
    std::map<std::string, std::string> pairs;
    pairs.insert(std::pair<std::string, std::string>("mix_key", "mix/jk/b9/jkfldjsklfjsklfjsfjdskfjsklf.jpg"));
    std::fstream ifs("f3e84ca01dc4f2b8d47970f0be71.jpg");
    ifs.seekg (0, std::ios::end);
    size_t length = ifs.tellg();
    ifs.seekg (0, std::ios::beg);
    char *bytes = new char[length];
    ifs.read(bytes, length);
    ifs.close();
    char *recv = NULL;
    size_t recv_len = 0;
    std::vector<multipart_formdata_item_st> items;
    multipart_formdata_item_st item;
    item.name = "mix_key";
    item.data = "mix/jk/b9/jkfldjsklfjsklfjsfjdskfjsklf.jpg";
    item.data_len = strlen(item.data);
    items.push_back(item);
    item.name = "bytes";
    item.data = bytes;
    item.data_len = length;
    item.content_type = "image/jpg";
    items.push_back(item);
    int ret = client->PostMultipartFormData("http://192.168.1.6:8080/pic/savemix",
					    items, &recv, &recv_len);

    if (recv_buf) {
      std::ofstream fs;
      fs.open("1.html");
      fs.write(recv_buf, len);
      fs.close();
      free(recv_buf);
      printf("received %ld bytes\n", len);
      int resp_code  = 0;
      client->GetResponseCode(resp_code);
      printf("reponse code =%d, a=%d\n", resp_code, a);
    }
    else {
      printf("ret=%d\n", ret);
    }
  }

//   delete client;

//   curl_global_cleanup();

  return 0;
}


// test json
/*
int main(int argc, char **argv) {
  const char *xpath = "/ret";
  JsonParser jp;
  int ret = 0;
  if (CR_SUCCESS != jp.LoadFile(argv[1])) {
    printf("parse json file failed\n");
    return 1;
  }
  std::string val;
  ret = jp.GetString(xpath, val);
  if (CR_SUCCESS == ret)
    printf("%s=%s\n", xpath, val.c_str());
  const char *xpath_data = "/data/";
  int arr_len = -1;
  ret = jp.GetArrayNodeLength(xpath_data, arr_len);
  printf("array_len=%d\n", arr_len);
  return 0;
}
*/
