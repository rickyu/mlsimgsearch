#encoding: utf-8
import json
import MySQLdb
import sys
import urllib2

class Pic2GoodsConvertor:
    goods_conn = ""
    result_conn = ""
    def __init__(self):
        self.result_conn = MySQLdb.connect(host="172.16.0.216",
                                           user="meiliwork",
                                           passwd="Tqs2nHFn4pvgw",
                                           db="image_data",
                                           charset="utf8",
                                           port=3316)
        self.goods_conn = MySQLdb.connect(host="172.16.0.42",
                                           user="dbreader",
                                           passwd="wearefashions",
                                           db="shark",
                                           charset="utf8",
                                           port=3306)
        
    def GetGoodsIDs(self, picid):
        sql = "select goods_id from t_dolphin_fut_goods_info where goods_picture_id=%s"
        param = (picid)
        db = self.goods_conn.cursor()
        ret = db.execute(sql, param)
        return db.fetchall()

    def CheckResultExist(self, goods_id, search_goods_id):
        sql = "select * from t_goods_search_result where goods_id=%s and search_goods_id=%s"
        param = (goods_id, search_goods_id)
        db = self.result_conn.cursor()
        ret = db.execute(sql, param)
        results = db.fetchall()
        if (len(results) > 0):
            return 1
        return 0

    def InsertSearchResult(self, search_picid, result_item):
        # 得到搜索的picture对应的所有goods
        goods_ids = self.GetGoodsIDs(search_picid)
        db = self.result_conn.cursor()
        for goods_id in goods_ids:
            # 分别入库
            result_goods_ids = self.GetGoodsIDs(result_item['picid'])
            for result_goods_id in result_goods_ids:
                # 删除旧数据
                sql = "delete from t_goods_search_result where goods_id=%s and search_goods_id=%s"
                param = (goods_id[0], result_goods_id[0])
                db.execute(sql, param)
                self.result_conn.commit()
                if (result_goods_id == goods_id):
                    print("result_goods_id=%s, goods_id=%s is same, ignored" % (result_goods_id, goods_id))
                    continue
                sql = "insert into t_goods_search_result(goods_id, search_goods_id, sim, search_options) values(%s, %s, %s, %s)"
                param = (goods_id[0], result_goods_id[0], result_item['sim'], 1)
                print sql
                print param
                db.execute(sql, param)
                self.result_conn.commit()

class Searcher:
    converter = ""
    file_path = ""
    image_search_url_prefix = "http://172.16.0.57/search/common?user=work&url=http://172.16.0.57/"
    def __init__(self, file_path):
        self.file_path = file_path
        self.converter = Pic2GoodsConvertor()

    def Run(self):
        for line in open(self.file_path):
            try:
                parts = line.split('\t')
                url = self.image_search_url_prefix + parts[len(parts)-1].strip()
                print url
                pic_id = parts[0]
                response = urllib2.urlopen(url).read()
                decodejson = json.loads(response)
                for same_item in decodejson['data']['same']:
                    print("begin insert (%s:%s)" % (pic_id, same_item))
                    self.converter.InsertSearchResult(pic_id, same_item)
            except Exception, e:
                print("catch an exception: %s" % line)
                


if __name__ == "__main__":
    searcher = Searcher(sys.argv[1])
    searcher.Run()
