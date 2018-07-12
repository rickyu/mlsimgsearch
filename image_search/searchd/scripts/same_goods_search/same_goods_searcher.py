#encoding: utf-8
import json
import MySQLdb
import sys
import urllib2
import redis

class Pic2GoodsConvertor:
    goods_conn = ""
    result_conn = ""
    redis_conn = ""
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
        self.redis_conn = redis.StrictRedis(host='127.0.0.1', port=6379)

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
    db_conn = ""
    redis_conn = ""
    image_search_url_prefix = "http://172.16.0.57/search/common?user=yantao&url=http://172.16.0.57/"
    def __init__(self):
        self.converter = Pic2GoodsConvertor()
        self.redis_conn = redis.StrictRedis(host='127.0.0.1', port=6379)

    def Run(self):
        while True:
            try:
                goods_id = self.redis_conn.lpop('same_goods_search_list')
                if (goods_id <= 0):
                    print("can't get valid goods_id" % (goods_id))
                    break
                self.db_conn = MySQLdb.connect(host="172.16.0.42",
                                               user="dbreader",
                                               passwd="wearefashions",
                                               db="shark",
                                               charset="utf8",
                                               port=3306)
                sql = "select goods_picture_id from t_dolphin_fut_goods_info where goods_id=%s and goods_picture_id > 0"
                param = (goods_id)
                db = self.db_conn.cursor()
                ret = db.execute(sql, param)
                goods_result = db.fetchall()
                if (len(goods_result) <= 0):
                    print("can't get goods_picture_id [%s]" % (goods_id))
                    break
                picture_id = goods_result[0][0]
                sql = "select picid, n_pic_file from t_picture where picid=%s"
                param = (picture_id)
                ret = db.execute(sql, param)
                picture_result = db.fetchall()
                if (len(picture_result) <= 0):
                    print("can't get picture info [%s]" % (picture_id))
                    break
                url = self.image_search_url_prefix + picture_result[0][1]
                print("begin search [%s] %s - %s" % (goods_id, picture_result[0][1], url))
                response = urllib2.urlopen(url).read()
                decodejson = json.loads(response)
                for same_item in decodejson['data']['same']:
                    print("begin insert (%s:%s)" % (picture_id, picture_result[0][1]))
                    self.converter.InsertSearchResult(picture_id, same_item)
                db.close()
            except Exception, e:
                print("catch an exception")

if __name__ == "__main__":
    searcher = Searcher()
    searcher.Run()
