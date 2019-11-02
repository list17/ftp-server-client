import sys
import re
from socket import *
from socket import _GLOBAL_DEFAULT_TIMEOUT
import ftplib

class FTPClient:
    def __init__(self):
        self.address = "127.0.0.1"
        self.port = 21
        self.write_oft = 0
        self.username = ""
        self.password = ""
        self.is_login = 0
        self.encoding = "utf-8"
        self.read_size = 20000
        self.read_dir = ""
        self.upload_filename = ""
        self.upload_filename_full = ""
        self.timeout = _GLOBAL_DEFAULT_TIMEOUT
        self.rest = 0

        self.get_file_size = 0
        self.put_file_size = 0
        self.get_file_size_already = 0
        self.put_file_size_already = 0

        self.socket_t = socket(AF_INET, SOCK_STREAM)
        self.directory = []
        self.passive = 0

    def cmd_connect(self):
        self.socket_t.connect((self.address, int(self.port)))
        self.socket_t.recv(self.read_size)

        self.af = self.socket_t.family
        self.data_socket_t = socket(AF_INET, SOCK_STREAM)

    def cmd_user(self):
        self.socket_t.send(("USER %s\r\n" % self.username).encode(self.encoding))
        # print(self.socket_t.recv(self.read_size).split(b' ')[0])
        if self.socket_t.recv(self.read_size).split(b' ')[0] != b"331":
            return 0
        return 1

    def cmd_pass(self):
        self.socket_t.send(("PASS %s\r\n" % self.password).encode(self.encoding))
        if self.socket_t.recv(self.read_size).split(b' ')[0] != b"230":
            return 0
        return 1

    def cmd_pwd(self):
        self.socket_t.send("PWD\r\n".encode(self.encoding))
        receive = self.socket_t.recv(self.read_size).split(b' ')
        if receive[0] != b'257':
            return 0
        else:
            self.read_dir = str(receive[1])
            return 1

    def cmd_quit(self):
        self.socket_t.send("QUIT\r\n".encode(self.encoding))
        receive = self.socket_t.recv(self.read_size)
        if receive.split(b' ')[0] == b'221':
            return 1
        return 0

    def cmd_rest(self):
        self.socket_t.send(("REST %ld\r\n" % self.rest).encode(self.encoding))
        receive = self.socket_t.recv(self.read_size)
        if receive.split(b' ')[0] == b'350':
            return 1
        return 0


    def cmd_delete(self, filename):
        self.socket_t.send(("DELE %s\r\n" % filename).encode(self.encoding))
        receive = self.socket_t.recv(self.read_size)
        if receive.split(b' ')[0] == b'250':
            return 1
        elif receive.split(b' ')[0] == b'550':
            return 0

    def cmd_rmd(self, directory):
        self.socket_t.send(("RMD %s\r\n" % directory).encode(self.encoding))
        receive = self.socket_t.recv(self.read_size)
        if receive.split(b' ')[0] == b'250':
            return 1
        elif receive.split(b' ')[0] == b'550':
            return 0

    def cmd_mkd(self, directory):
        self.socket_t.send(("MKD %s\r\n" % directory).encode(self.encoding))
        receive = self.socket_t.recv(self.read_size)
        if receive.split(b' ')[0] == b'257':
            return 1
        return 0

    def cmd_rename(self, nafr, nato):
        self.socket_t.send(("RNFR %s\r\n" % nafr).encode(self.encoding))
        receive = self.socket_t.recv(self.read_size).split(b' ')
        if receive[0] == b'350':
            self.socket_t.send(("RNTO %s\r\n" % nato).encode(self.encoding))
            receive = self.socket_t.recv(self.read_size).split(b' ')
            if receive[0] == b'250':
                return 1
            return 0
        return 0

    def cmd_size(self, filename):
        self.socket_t.send(("SIZE %s\r\n" % filename).encode(self.encoding))
        receive = self.socket_t.recv(self.read_size)
        return int(receive)

    def cmd_type(self, mode):
        self.socket_t.send(("TYPE %s\r\n" % mode).encode(self.encoding))
        receive = self.socket_t.recv(self.read_size)
        if receive.split(b' ')[0] == b'220':
            return 1
        return 0

    def cmd_syst(self):
        self.socket_t.send("SYST\r\n".encode(self.encoding))
        receive = self.socket_t.recv(self.read_size)
        if receive.split(b' ')[0] == b'215':
            return str(receive)
        return 0

    def cmd_cwd(self, directory):
        print(directory)
        self.socket_t.send(("CWD %s\r\n" % directory).encode(self.encoding))
        receive = self.socket_t.recv(self.read_size)
        if receive.split(b' ')[0] == b'250':
            return 1
        return 0

    def cmd_port(self, host, port):
        hbytes = host.split('.')
        pbytes = [repr(port // 256), repr(port % 256)]
        bytes = hbytes + pbytes
        self.socket_t.send(("PORT " + ','.join(bytes) + "\r\n").encode(self.encoding))
        receive = self.socket_t.recv(self.read_size)
        if receive.split(b' ')[0] == b'200':
            return receive
        return None

    def cmd_pasv(self):
        self.socket_t.send("PASV\r\n".encode(self.encoding))
        receive = self.socket_t.recv(self.read_size)
        if receive.split(b' ')[0] == b'227':
            return receive
        return None

    def cmd_list(self, blocksize=8192, rest=None):
        size = None
        conn = None
        if self.passive:
            host, port = self.makepasv()
            conn = create_connection((host, port), self.timeout, None)
        elif not self.passive:
            sock = self.makeport()
        self.socket_t.send("LIST\r\n".encode(self.encoding))
        receive1 = self.socket_t.recv(self.read_size)
        if not self.passive:
            conn, sockaddr = sock.accept()
        while 1:
            data = conn.recv(blocksize)
            if not data:
                break
            try:
                a = data.decode('utf-8').index('\r')
                for i in data.decode('utf-8').split('\r\n'):
                    self.directory.append(i.split(' '))
            except ValueError:
                for i in data.decode('utf-8').split('\n'):
                    self.directory.append(i.split(' '))
        if receive1.count(b'\r\n') == 1:
            receive2 = self.socket_t.recv(self.read_size)
        return 1

    def cmd_retr(self, filename, callback, dialog, blocksize=8192, rest=None):
        self.cmd_type('I')
        size = None
        conn = None
        if self.passive:
            host, port = self.makepasv()
            conn = create_connection((host, port), self.timeout, None)
        elif not self.passive:
            sock = self.makeport()
        self.socket_t.send(("RETR %s\r\n" % filename).encode(self.encoding))
        receive1 = self.socket_t.recv(self.read_size)
        if receive1.split(b' ')[0] == b'150':
            self.get_file_size = int(receive1.split(b'(')[1].split(b' ')[0].decode('utf-8'))
            if not self.passive:
                conn, sockaddr = sock.accept()
                if self.timeout is not _GLOBAL_DEFAULT_TIMEOUT:
                    conn.settimeout(self.timeout)
        while 1:
            data = conn.recv(blocksize)
            if not data:
                break
            self.get_file_size_already += sys.getsizeof(data)
            callback(data)
        if receive1.count(b'\r\n') == 1:
            receive2 = self.socket_t.recv(self.read_size)
        return 1

    def cmd_stor(self, fp, callback=None, blocksize=8192, rest=None):
        self.cmd_type('I')
        size = None
        conn = None
        if self.passive:
            host, port = self.makepasv()
            conn = create_connection((host, port), self.timeout, None)
        elif not self.passive:
            sock = self.makeport()
        self.socket_t.send(("STOR %s\r\n" % self.upload_filename).encode(self.encoding))
        receive1 = self.socket_t.recv(self.read_size)
        if not self.passive:
            conn, sockaddr = sock.accept()
        while 1:
            buf = fp.read(blocksize)
            if not buf:
                break
            conn.sendall(buf)
            if callback:
                callback(buf)
        conn.close()
        if receive1.count(b'\r\n') == 1:
            receive2 = self.socket_t.recv(self.read_size)
        return 1

    def makeport(self):
        err = None
        sock = None
        for res in getaddrinfo(None, 0, self.af, SOCK_STREAM, 0, AI_PASSIVE):
            af, socktype, proto, canonname, sa = res
            sock = socket(af, socktype, proto)
            sock.bind(sa)
        sock.listen(1)
        port = sock.getsockname()[1]  # Get proper port
        host = self.socket_t.getsockname()[0]  # Get proper host
        resp = self.cmd_port(host, port)
        if self.timeout is not _GLOBAL_DEFAULT_TIMEOUT:
            sock.settimeout(self.timeout)
        return sock

    def makepasv(self):
        resp = self.cmd_pasv().decode('utf-8')
        re_227 = re.compile(r'(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)', re.ASCII)
        m = re_227.search(resp)
        numbers = m.groups()
        host = '.'.join(numbers[:4])
        port = (int(numbers[4]) << 8) + int(numbers[5])
        return host, port

    def cmd_abor(self):
        self.socket_t.send("ABOR\r\n".encode(self.encoding))
        receive = self.socket_t.recv(self.read_size)
        if receive.split(b' ')[0] == b'225':
            return 1
        return 0

    def cmd_quit(self):
        self.socket_t.send("QUIT\r\n".encode(self.encoding))
        receive = self.socket_t.recv(self.read_size)
        if receive.split(b' ')[0] == b'221':
            return 1
        return 0