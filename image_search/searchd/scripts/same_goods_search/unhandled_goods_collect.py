import redis
import MySQLdb
import sys
import time

class UnhandledGoodsCollector:
    db_conn = ""
    redis_conn = ""

    def __init__(self):
        self.db_conn = MySQLdb.connect(host="172.16.0.42",
                                       user="dbreader",
                                       passwd="wearefashions",
                                       db="shark",
                                       charset="utf8",
                                       port=3306)

        self.redis_conn = redis.StrictRedis(host='127.0.0.1', port=6379)

    def Run(self, data_file):
        data_file_reader = open(data_file, 'rb')
        start_goods_id = data_file_reader.read()
        data_file_reader.close()
        if (start_goods_id == ""):
            return -1

        print("add goods_id begin with %s" % (start_goods_id))
        while True:
            get_step = 100
            sql = "select goods_id from t_dolphin_fut_goods_info where goods_id > %s and goods_picture_id > 0 limit %s"
            param = (start_goods_id, get_step)
            db = self.db_conn.cursor()
            ret = db.execute(sql, param)
            goods_result = db.fetchall()
            max_goods_id = 0
            if (len(goods_result) == 0):
                break
            for goods_item in goods_result:
                self.redis_conn.lpush('same_goods_search_list', goods_item[0])
                print("add goods %s" % (goods_item[0]))
                max_goods_id = goods_item[0]
            start_goods_id = max_goods_id
            data_file_writer = open(data_file, 'w')
            data_file_writer.write(str(start_goods_id))
            data_file_writer.close()
            if (len(goods_result) < get_step):
                break

if __name__ == "__main__":
    while True:
        handler = UnhandledGoodsCollector()
        handler.Run(sys.argv[1])
        time.sleep(3)
