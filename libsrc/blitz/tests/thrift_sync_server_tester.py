#coding: utf-8
import sys
import subprocess
import time
from test_thrift_sync_server.client import TestClient
class ThreadTimeoutTester(object):
    def __init__(self):
        self.service_process = None
        self.sender_cmd = 'python test_thrift_sync_server/sender.py'
        pass
    def setup(self):
       pass
    def test_long(self):
        cmd = './test_thrift_sync_server/test_thrift_sync_server -b tcp://*:1234 -t 2 -k 0'
        service_process = subprocess.Popen(cmd, shell=True)
        time.sleep(0.2)
        print 'service started, pid=%d' % (service_process.pid)
 
        cmd = self.sender_cmd + ' 10000 1 1234'
        long_sender = subprocess.Popen(cmd, shell=True)
        time.sleep(0.2)
        cmd = self.sender_cmd + ' 1 20 1234'
        short_count = 20
        short_sender = []
        for i in range(short_count):
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                    shell=True)
            short_sender.append(p)
        for p in short_sender:
            p.wait()
        long_sender.terminate()
        service_process.terminate()
        print 'service stop'
        for p in short_sender:
            out = p.stdout.readlines()
            print out
            if len(out) < 1 or int(out[0]) < 15:
                raise Exception('some client timeout')
    def test_timeout(self):
        """ 测试ThrifySyncServer的超时设置是否生效"""
        cmd = './test_thrift_sync_server/test_thrift_sync_server -b tcp://*:1235 -t 1 -k 1000'
        service_process = subprocess.Popen(cmd, shell=True)
        time.sleep(0.2)
        print 'service started, pid=%d' % (service_process.pid)
 
        cmd = self.sender_cmd + ' 2000 1 1235'
        long_sender = subprocess.Popen(cmd, shell=True)
        time.sleep(0.2)
        client = TestClient('127.0.0.1', 1235, 10000)
        ok = 1
        try:
            client.Wait(50)
            # 不正常，居然正确的处理了
            ok = 0
        except:
            ok = 1
            print "succeed: timeout"
        client.close()
        long_sender.terminate()
        service_process.terminate()
        if not ok:
            raise Exception("timeout doesn't work")

        cmd = './test_thrift_sync_server/test_thrift_sync_server -b tcp://*:1236 -t 1 -k 5000'
        service_process = subprocess.Popen(cmd, shell=True)
        time.sleep(0.1)
        print 'service started, pid=%d' % (service_process.pid)
 
        cmd = self.sender_cmd + ' 2000 1 1236'
        long_sender = subprocess.Popen(cmd, shell=True)
        time.sleep(0.2)
        client = TestClient('127.0.0.1', 1236, 10000)
        ok = 1
        try:
            client.Wait(50)
            ok = 1
        except:
            ok = 0
            print "fail: timeout"
        client.close()
        long_sender.terminate()
        service_process.terminate()
        if not ok:
            raise Exception("timeout doesn't work")

  
    def teardown(self):
        pass
if __name__ == '__main__':
    tester = ThreadTimeoutTester() 
    tester.setup()
    tester.test_long()
    tester.test_timeout()
    tester.teardown()
