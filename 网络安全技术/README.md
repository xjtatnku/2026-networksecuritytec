# Lab01 - 基于 DES 加密的 TCP 聊天程序

## 功能说明

- 基础实验
  - 手写实现 DES 加解密
  - 基于 TCP 的单客户端聊天程序
  - 发送内容先经 DES 加密，再通过网络传输

## 目录结构

```text
Lab01
├── CMakeLists.txt
├── README.md
├── include
│   ├── DES_Operation.h
│   └── chat.h
├── main.cpp
└── src
    ├── DES_Operation.cpp
    └── chat.cpp
```

## WSL 编译

```bash
wsl -d Ubuntu-OSLab
cd /mnt/c/Users/xjt26/Desktop/网络安全技术
sudo apt update
sudo apt install -y cmake g++
cmake -S . -B build
cmake --build build
```

## 运行方式

服务器端：

```bash
./bin/DES_chat server --port 8888 --key SecLab01 --show-cipher
```

客户端：

```bash
./bin/DES_chat client --ip 127.0.0.1 --port 8888 --key SecLab01 --show-cipher
```

如果命令行中不写 `--show-cipher`，程序会在建立聊天前询问是否显示密文：

```text
Current identity: Server
Show cipher text during chat? (yes/no):
```

输入 `quit` 时程序会直接关闭本端连接，不会将 `quit` 当作聊天内容加密发送。
