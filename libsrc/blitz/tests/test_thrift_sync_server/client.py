#coding: utf-8
from thrift.transport import TTransport
from thrift.transport import TSocket
from thrift.transport import THttpClient
from thrift.protocol import TBinaryProtocol
from thrift_genpy.mytest import  TestTime

class TestClient(object):
    def __init__(self, host, port, timeout):
        socket = TSocket.TSocket(host, port)
        socket.setTimeout(timeout)
        self.transport = TTransport.TFramedTransport(socket)
        protocol = TBinaryProtocol.TBinaryProtocol(self.transport)
        self.client = TestTime.Client(protocol)
        self.transport.open()
    def Wait(self, sleep_time):
        return self.client.Wait(sleep_time)
    def close(self):
        self.transport.close()
        self.transport = None
        self.client = None
