# Lab02 - 基于 RSA 算法自动分配密钥的加密聊天程序

## 功能说明

- 保留 Lab1 的核心能力
  - 手写实现 DES 加解密
  - 基于 TCP 的单客户端全双工聊天
  - 使用长度头解决 TCP 粘包和拆包问题
- 新增 Lab2 的核心能力
  - 服务器端自动生成 RSA 公钥/私钥对
  - 客户端自动生成 8 字节 DES 会话密钥
  - 客户端使用 RSA 公钥加密 DES 会话密钥并发送给服务器
  - 服务器使用 RSA 私钥解密并恢复 DES 会话密钥
  - 双方随后自动切换到 DES 加密聊天，无需手动输入密钥
- 交互增强
  - 支持命令行指定 `server/client`、IP、端口
  - 支持在建立聊天前选择是否显示十六进制密文
  - 输入提示符明确标注当前身份 `[Server]` 或 `[Client]`
  - 输入 `quit` 时直接关闭本端连接，不再将其作为普通消息发送

## 目录结构

```text
Lab02
├── CMakeLists.txt
├── README.md
├── include
│   ├── chat.h
│   ├── DES_Operation.h
│   └── RSA_Operation.h
├── main.cpp
└── src
    ├── chat.cpp
    ├── DES_Operation.cpp
    └── RSA_Operation.cpp
```

## WSL 编译

```bash
wsl -d Ubuntu
cd /mnt/c/Users/xjt26/Desktop/2313226肖俊涛网络安全技术lab1
sudo apt update
sudo apt install -y cmake g++
cmake -S . -B build
cmake --build build
```

如果当前 WSL 中还没有安装 `cmake`，也可以先使用 `g++` 直接验证：

```bash
mkdir -p bin
g++ -std=c++17 -pthread -Iinclude main.cpp src/DES_Operation.cpp src/chat.cpp src/RSA_Operation.cpp -o bin/RSA_DES_chat
```

## 运行方式

服务器端：

```bash
./bin/RSA_DES_chat server --port 8888 --show-cipher yes
```

客户端：

```bash
./bin/RSA_DES_chat client --ip 127.0.0.1 --port 8888 --show-cipher yes
```

如果命令行中不写 `--show-cipher`，程序会在建立聊天前询问是否显示密文：

```text
Current identity: Server
Show cipher text during chat? (yes/no):
```

## Lab2 握手流程

1. 服务器端生成 RSA 公钥/私钥对
2. 服务器端将 RSA 公钥发送给客户端
3. 客户端自动生成随机 8 字节 DES 会话密钥
4. 客户端使用 RSA 公钥加密该 DES 会话密钥
5. 客户端将加密后的结果发送给服务器
6. 服务器端使用 RSA 私钥解密，恢复 DES 会话密钥
7. 双方使用该 DES 会话密钥进行后续聊天

## 退出方式

输入 `quit` 时程序会直接关闭本端连接，不会将 `quit` 当作聊天内容加密发送。

## 建议演示与截图步骤

下面给出一套适合实验报告截图展示的测试流程。建议打开两个 WSL 终端，左侧作为服务端，右侧作为客户端，终端字体适当调大，便于截图。

### 1. 编译命令

```bash
wsl -d Ubuntu-OSLab-Recovered
cd /mnt/c/Users/xjt26/Desktop/2313226肖俊涛网络安全技术lab1
cmake -S . -B build
cmake --build build -j
```

### 2. 启动命令

服务端终端：

```bash
./bin/RSA_DES_chat server --port 8888 --show-cipher yes
```

客户端终端：

```bash
./bin/RSA_DES_chat client --ip 127.0.0.1 --port 8888 --show-cipher yes
```

### 3. 建议截图顺序

#### 图 1：程序启动与 RSA 握手成功

先启动服务端，再启动客户端。此时建议截图，界面中应尽量包含以下关键输出：

服务端侧应出现：

```text
Current identity: Server
Show cipher text: yes
Role: [Server]
Server is listening on port 8888.
Waiting for client...
Client connected: 127.0.0.1:xxxxx
RSA handshake completed. DES session key was distributed automatically.
Secure channel is ready.
```

客户端侧应出现：

```text
Current identity: Client
Show cipher text: yes
Role: [Client]
Connected to 127.0.0.1:8888
RSA handshake completed. DES session key was generated and shared automatically.
Secure channel is ready.
```

#### 图 2：客户端向服务端发送消息

在客户端输入：

```text
hello from client
```

服务端应看到类似输出：

```text
[Client cipher hex] xx xx xx ...
Client: hello from client
```

这一张截图建议同时截到客户端输入区和服务端接收区，用于说明：

- 客户端成功发送消息
- 网络中传输的是密文
- 服务端能够正确解密显示明文

#### 图 3：服务端向客户端发送中文消息

在服务端输入：

```text
你好，这是 lab2
```

客户端应看到类似输出：

```text
[Server cipher hex] xx xx xx ...
Server: 你好，这是 lab2
```

这一张图适合证明：

- 程序支持中文消息
- DES 加密与解密对 UTF-8 文本仍然有效
- 双方实现了全双工通信

#### 图 4：退出测试

在任意一端输入：

```text
quit
```

例如服务端输入后，服务端应显示：

```text
Server is closing the connection.
```

客户端应显示：

```text
Server disconnected.
```

这一张图适合说明：

- `quit` 被作为本地控制命令处理
- 程序能够正常结束连接
- 不会把 `quit` 当作普通聊天内容加密发送

### 4. 一套可直接照抄的演示输入

如果你想快速完成演示和截图，推荐按以下顺序操作：

客户端输入：

```text
hello from client
```

服务端输入：

```text
你好，这是 lab2
```

服务端最后输入：

```text
quit
```

### 5. 不想显示密文时的命令

如果你只想截图界面更简洁的聊天效果，可以把 `--show-cipher yes` 改成 `--show-cipher no`：

服务端：

```bash
./bin/RSA_DES_chat server --port 8888 --show-cipher no
```

客户端：

```bash
./bin/RSA_DES_chat client --ip 127.0.0.1 --port 8888 --show-cipher no
```

这种模式更适合截“聊天界面”，而显示密文的模式更适合截“加密效果展示”。实验报告中通常两种截图各保留一张会更完整。
