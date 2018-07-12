#coding:utf-8
"""
python % wait_time count
"""
import sys
import client
wait_time=int(sys.argv[1])
count=int(sys.argv[2])
host='127.0.0.1'
port=int(sys.argv[3])
timeout = wait_time + 50
client = client.TestClient(host, port, timeout)
return_count = 0
for x in range(count):
    client.Wait(wait_time)
    return_count = return_count + 1
client.close()
print return_count
