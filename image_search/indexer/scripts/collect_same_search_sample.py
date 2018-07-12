#encoding: utf-8
import json
import MySQLdb
import sys
import urllib2

class Pic2GoodsConvertor:
    shark_conn = ""
    data_conn = ""
    def __init__(self):
        self.shark_conn = MySQLdb.connect(host="172.16.0.216",
                                           user="meiliwork",
                                           passwd="Tqs2nHFn4pvgw",
                                           db="image_data",
                                           charset="utf8",
                                           port=3316)
        self.data_conn = MySQLdb.connect(host="172.16.0.42",
                                           user="dbreader",
                                           passwd="wearefashions",
                                           db="shark",
                                           charset="utf8",
                                           port=3306)
        
    def GetGoodsIDs(self, picid):
        sql = "select picid, n_pic_file from t_picture where picid=%s"
        param = (picid)
        ret = self.shark_conn.cursor().execute(sql, param)
        for item in self.shark_conn.cursor().fetchall()
            return item
        return false

    def GetNextNRecords(self, start_id, limit):
        sql = "select search_picid, result_picid from t_same_search_samples where id > %s limit %s"
        param = (start_id, limit)
        ret = self.data_conn.cursor().execute(sql, param)
        return self.data_conn.cursor().fetchall()

class Collector:
    def Run(self):
        for line in open(self.file_path):
            parts = line.split(' ')
            url = self.image_search_url_prefix + parts[1]
            pic_id = parts[0]
            urllib2.urlopen(self.url).read()
            decodejson = json.loads(line)
            for same_item in decodejson['data']['same']:
                self.converter.InsertSearchResult(pic_id, same_item)



if __name__ == "__main__":
    searcher = Searcher(sys.argv[1])
    searcher.Run()
