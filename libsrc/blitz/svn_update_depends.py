#encoding:utf-8

import os
import sys
import subprocess
import time

def wait_process_end(process, timeout):
    start_time = time.time()
    end_time = start_time + timeout
    while 1:
        ret = process.poll()
        if ret == 0:
            return 0
        elif ret is None:
            cur_time = time.time()
            if cur_time >= end_time:
                return 1
            if end_time - cur_time >= 0.1:
                time.sleep(0.1)
            else:
                time.sleep(end_time-cur_time)
        else:
            return 2

def exec_command2(command, timeout):
    """ 执行一个shell命令
        @return (returncode, stdout, stderr)
    """
    process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, shell=True)
    ret = wait_process_end(process, timeout)
    if ret == 1:
        process.terminate()
        raise Exception, "terminated_for_timout"
    output = process.stdout.readlines()
    err = process.stderr.readlines()
    return process.returncode, output, err
def exec_command(command, timeout):
    """ 执行一个shell命令
        @return (returncode, stdout, stderr)
    """
    process = subprocess.Popen(command, shell=True)
    ret = wait_process_end(process, timeout)
    if ret == 1:
        process.terminate()
        raise Exception, "terminated_for_timout"
    return process.returncode, [], [] 

def stop_process(ssh_user, host, pid):
    """ 停止服务，返回停止的进程ID"""
    cmd = "ssh %s@%s \"kill %d\"" % (ssh_user, host, pid)
    returncode, stdout, stderr = exec_command(cmd, 5)
    if returncode != 0:
        err_msg = list_join(stderr)
        raise Exception, "stop_process pid=%d stderr=[%s]" % (pid, err_msg)
    return pid


CFG_DEPENDS=[]
CFG_WORKROOT = ""
CFG_SVN= {"url":"",
        "user":"",
        "passwd":""}
def SVN(url, user, passwd = None):
    CFG_SVN["url"] = url
    CFG_SVN["user"]  = user
    CFG_SVN["passwd"] = passwd
def WORKROOT(path):
    global CFG_WORKROOT
    CFG_WORKROOT=path
    
def ADD_DEPENDS(path):
    CFG_DEPENDS.append(path)
    pass


def print_lines(lines):
    for line in lines:
        sys.stdout.write(line)
    sys.stdout.flush()
class MModule(object):
    def __init__(self, svn_path, local_path):
        self.svn_path = svn_path
        self.local_path = local_path
        pass
    def __str__(self):
        return "svn:" + self.svn_path + ", local:" + self.local_path
    def update(self):
        # 如果目录不存在，svn co
        # 如果存在, svn up ，up失败的话， 删除， svn co.
        if os.path.isdir(self.local_path):
            print "local path %s exist" % (self.local_path)
            cwd = os.getcwd()
            os.chdir(self.local_path)
            ret, out, err = exec_command("svn up", 5000)
            os.chdir(cwd)
            if ret != 0:
                print_lines(err)
                exec_command("rm -rf " + self.local_path, 5000)
            else:
                print_lines(out)
                return
        ret, out, err = exec_command("svn co " + self.svn_path + " " + self.local_path, 5000)
        print_lines(out)
    def build(self):
        if not os.path.isdir(self.local_path):
            print "%s not exist" % (self.local_path)
            return 1
        cwd = os.getcwd()
        os.chdir(self.local_path)
        ret = 0
        err = []
        if os.path.isfile("Makefile"):
            ret, out, err = exec_command("make", 5000)
            print_lines(out)
        else:
            print "no Makefile"
        os.chdir(cwd)
        if ret == 0:
            return 0
        print_lines(err)
        return 1

if __name__ == "__main__":
    SVN("http://svn.meilishuo.com/repos", "zuoyu")
    ADD_DEPENDS("meilishuo/middleware/thirdlib/boost")
    ADD_DEPENDS("meilishuo/middleware/thirdlib/mysql")
    ADD_DEPENDS("meilishuo/middleware/thirdlib/thrift")
    ADD_DEPENDS("meilishuo/middleware/thirdlib/libevent")
    ADD_DEPENDS("meilishuo/middleware/thirdlib/hiredis")
    ADD_DEPENDS("meilishuo/middleware/thirdsrc/gtest-1.6.0")
    ADD_DEPENDS("meilishuo/middleware/libsrc/utils")
    WORKROOT("../../../../")
    print CFG_WORKROOT
    depends = []
    for path in CFG_DEPENDS:

        mod = MModule(CFG_SVN["url"] + "/" + path,
            os.path.abspath(CFG_WORKROOT + "/" + path))
        depends.append(mod)

    for mod in depends:
        print mod
        mod.update()
    print "start make ....."
    for mod in depends:
        mod.build()
