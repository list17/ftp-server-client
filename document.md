# FTP SERVER & CLIENT

李胜涛 2017013618 软件72

## 实验说明：

### 实验环境：
系统：linux（manjaro发行版）  
内核：linux 4.19.79  
gcc版本：9.2.0  
python版本（指客户端）：3.7.4  

### 运行方法：
server：
```bash
[sudo] ./server [OPTION...]
-port, --port=port set the server port
-root, --root=root set the root directory
```
client:
```bash
python client.py
```

### 备注：
客户端与服务端只支持anonymous登录。

## 代码说明：

### server
1.实现命令：  

2.架构说明：

server采用epoll完成，单线程实现多客户端登录，
