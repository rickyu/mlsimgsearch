#ifndef IMAGE_STORAGE_BEANSDB_CLIENT_BEANSDB_CLUSTERS_H_
#define IMAGE_STORAGE_BEANSDB_CLIENT_BEANSDB_CLUSTERS_H_

#include <beansdb_client/beansdb_client.h>
#include <map>
#include <string>

#define DEFAULT_PIC_TYPE_NAME "default"
#define DEFAULT_CLUSTER_NAME "default"

class BeansdbClusters {
 private:
  typedef struct {
    std::string ip;
    int port;
  } ServerAddress;
  typedef struct _ClusterInfo {
    std::string pic_type;
    std::string cluster_flag;
    int pool_size;
    size_t cluster_scale;
    std::vector<ServerAddress> servers;
    int bucket_count;
    std::vector<int> buckets;

    _ClusterInfo() {
      pic_type = DEFAULT_PIC_TYPE_NAME;
      cluster_flag = DEFAULT_CLUSTER_NAME;
      cluster_scale = 1;
      pool_size = 0;
      servers.clear();
      bucket_count = 0;
      buckets.clear();
    }
  } ClusterInfo;
  typedef struct _PicKeyInfo {
    std::string type;
    std::string cluster;
  } PicKeyInfo;
  typedef struct _ClusterScale {
    std::string kind;
    std::string flag;
    size_t scale;
    _ClusterScale() {
      kind = DEFAULT_PIC_TYPE_NAME;
      flag = DEFAULT_CLUSTER_NAME;
      scale = 0;
    }
  } ClusterScale;
 private:
  //cluster<->beansdbclient
  typedef std::map<std::string, BeansdbClientPool*> BeansdbSingleCluster;
  //kind<->singlecluster
  typedef std::map<std::string, BeansdbSingleCluster*> BeansdbClustersNode;

  //集群分配信息
  typedef std::vector<ClusterScale> ClusterLocation;

 public:
  BeansdbClusters();
  ~BeansdbClusters();

  static BeansdbClusters* GetInstance();
  void Finalize();

  /**
   * @param conf [in] 格式：
   * type=picdetail;scale=10;cluster_flag=c1;pool_size=5;servers=172.16.0.61:7900,172.16.0.62:7900;buckets_num=16;buckets=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
   */
  beansdb_client_ret_t AddCluster(const std::string &conf);
  beansdb_client_ret_t AddClusters(const std::vector<std::string> &confs);

  beansdb_client_ret_t GenClusterLocate(const std::string &bytes_md5, 
                                        const std::string &kind,
                                        std::string &cluster_locate);
  beansdb_client_ret_t GenerateOriginPicId(const std::string &bytes_md5,
                                           const std::string &kind,
                                           const std::string &ext,
                                           const int width,
                                           const int height,
                                           std::string &orig_picid);
  beansdb_client_ret_t Set(const std::string& key,
                           const char *val,
                           const size_t val_size,
                           const long time = 0);
  beansdb_client_ret_t Get(const std::string& key,
                           char *&val,
                           size_t &val_size,
                           long timeout = 0);

 protected:
  beansdb_client_ret_t ParseConf(const std::string &conf, ClusterInfo &cluster);
  beansdb_client_ret_t ParsePicKey(const std::string &key, PicKeyInfo &key_info);
  beansdb_client_ret_t GetCluster(const std::string &key, BeansdbClientPool *&pool);

 private:
  static CLogger &logger_;
  BeansdbClustersNode clusters_;
  ClusterLocation cluster_location_;
  static BeansdbClusters *instance_;
};

#endif
