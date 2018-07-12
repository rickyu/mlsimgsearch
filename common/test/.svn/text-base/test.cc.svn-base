#include <fstream>
#include <common/http_client.h>
#include <common/mysqldb.h>
#include <common/json_parser.h>
#include <common/constant.h>
#include <common/jpeg_code.h>
#include <image/graphics_magick.h>
#include <time.h>
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
/*int main() {
  HttpClient *client =new HttpClient;
  HttpClient::GlobalInit();
  client->Init();
  int a = 9;
  int ac  = 0;
  client->GetResponseCode(ac);
  printf("reponse a=%d\n", a);//*/
  /*for (int i = 0; i < 100; i++)*/ 
  /*{
    char *recv_buf = NULL;
    size_t len = 0;
    char *header_buf = NULL;
    size_t header_len = 0;
    int ret = client->SendRequest(HTTP_GET,
				  "http://www.meilishuo.com/guang/hot",
				  //"http://img01.taobaocdn.com/bao/uploaded/i1/T1WUDsXmdlXXcbV2Db_124032.jpg",
				  //"http://img03.taobaocdn.com/bao/uploaded/i3/735092396/T2TBtcXhdNXXXXXXXX_!!735092396.jpg_460x460.jpg",
				  //"http://img01.taobaocdn.com/bao/uploaded/i1/T1TwSzXlXYXXXKp_oT_012857.jpg",
				  &recv_buf,
				  &len,
				  NULL,
				  0,
				  5000,
				  5000,
				  &header_buf,
				  &header_len);
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

  delete client;

  curl_global_cleanup();

  return 0;
}*/


// test json
int main(int argc, char **argv) {
  const char *xpath = "/data[1]/img_id";
  std::string test_json;
  test_json = "{\"pic\":\"pic/_o/a7/32/1f017f8e4c08197f6c3768075448_451_680.jpeg\",\"x\":0,\"y\":0,\"cx\":98,\"cy\":101}";
  JsonParser jp;
  jp.LoadMem(test_json.c_str(), (size_t)test_json.length());
  int ret = 0;
  std::string val;
  ret = jp.GetString(xpath, val);
  if (CR_SUCCESS == ret)
    printf("%s=%s\n", xpath, val.c_str());
  const char *xpath_data = "/data";
  int arr_len = -1;
  ret = jp.GetArrayNodeLength(xpath_data, arr_len);
  printf("array_len=%d\n", arr_len);
  return 0;
}

