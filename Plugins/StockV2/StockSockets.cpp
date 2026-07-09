#include "pch.h"
#include "StockSockets.h"
#include <wincrypt.h>
#include <WinSock2.h>
#include <afx.h>
#include <WS2tcpip.h>
#include <ws2def.h>
#include "Logger.h"
#include <afxstat_.h>
#include <inaddr.h>
#include <processthreadsapi.h>
#include <synchapi.h>
#include <handleapi.h>
#include <string>
#include <cstring>
#include <winnt.h>
#include <minwindef.h>
#include <vector>

#pragma intrinsic(_byteswap_uint64)

namespace
{
    struct OpMessage
    {
        BYTE opcode;
        std::vector<uint8_t> payload;
    };
};

LStockServerSocket &LStockServerSocket::GetInstance()
{
    static LStockServerSocket instance;
    return instance;
}

LStockServerSocket::LStockServerSocket()
    : m_ListenSocket(INVALID_SOCKET), m_ListenEvent(NULL), m_bRun(FALSE), m_pListenThread(NULL), m_webbridgeport(0), m_bWsaInit(FALSE)
{
    // 初始化线程锁
    InitializeCriticalSection(&m_csClient);
    InitializeCriticalSection(&m_csPending);
    m_bridge = new LSocketBridge();
}

LStockServerSocket::~LStockServerSocket()
{
    StopSocketServer();
    // 销毁线程锁
    DeleteCriticalSection(&m_csClient);
    DeleteCriticalSection(&m_csPending);
    SAFE_DELETE(m_bridge);
}

UINT LStockServerSocket::GetWebBridgePort() const
{
    return m_webbridgeport;
}

int LStockServerSocket::GetClientCount() const
{
    EnterCriticalSection(&m_csClient);
    int count = (int)m_mapClients.size();
    LeaveCriticalSection(&m_csClient);
    return count;
}

void LStockServerSocket::BroadcastMessage(const std::string &msg)
{
    EnterCriticalSection(&m_csClient);
    for (auto &pair : m_mapClients)
    {
        SendCommonFrame(pair.first, &pair.second.csSend, OP_CODE_TEXT, msg.data(), msg.size());
    }
    LeaveCriticalSection(&m_csClient);
}

BOOL LStockServerSocket::StartSocketServer()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (m_bRun)
    {
        LLOG_WARN("Socket Server is already running!");
        return TRUE;
    }

    // 初始化异步业务任务队列
    m_taskQueue.Init();

    // 初始化Socket（仅一次）
    if (!m_bWsaInit)
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            LLOG_ERROR("Socket INIT FAILED!");
            return FALSE;
        }
        m_bWsaInit = TRUE;
    }

    // 创建监听套接字
    m_ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_ListenSocket == INVALID_SOCKET)
    {
        LLOG_ERROR("Socket Server Create FAILED!");
        return FALSE;
    }

    // 绑定地址（0端口=系统自动分配随机端口）
    SOCKADDR_IN sin{};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(0);
    sin.sin_addr.s_addr = INADDR_ANY;

    if (bind(m_ListenSocket, (SOCKADDR *)&sin, sizeof(sin)) == SOCKET_ERROR)
    {
        LLOG_ERROR("Socket Server Bind FAILED!");
        closesocket(m_ListenSocket);
        m_ListenSocket = INVALID_SOCKET;
        return FALSE;
    }

    // 获取系统分配的随机端口
    int nLen = sizeof(sin);
    getsockname(m_ListenSocket, (SOCKADDR *)&sin, &nLen);
    m_webbridgeport = ntohs(sin.sin_port);

    // 创建事件对象，绑定监听事件
    m_ListenEvent = WSACreateEvent();
    if (m_ListenEvent == WSA_INVALID_EVENT)
    {
        LLOG_ERROR("WSACreateEvent FAILED!");
        closesocket(m_ListenSocket);
        m_ListenSocket = INVALID_SOCKET;
        return FALSE;
    }
    WSAEventSelect(m_ListenSocket, m_ListenEvent, FD_ACCEPT);

    // 开始监听
    if (listen(m_ListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        LLOG_ERROR("Socket Server Listen FAILED!");
        WSACloseEvent(m_ListenEvent);
        m_ListenEvent = NULL;
        closesocket(m_ListenSocket);
        m_ListenSocket = INVALID_SOCKET;
        return FALSE;
    }

    // 启动后台监听线程
    m_bRun = TRUE;
    CWinThread* hThread = AfxBeginThread(ListenThread, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
    if (hThread == NULL)
    {
        m_bRun = FALSE;
        m_pListenThread = NULL;
        LLOG_ERROR("Create Listen Thread FAILED!");
        WSACloseEvent(m_ListenEvent);
        m_ListenEvent = NULL;
        closesocket(m_ListenSocket);
        m_ListenSocket = INVALID_SOCKET;
        return FALSE;
    }
    m_pListenThread = hThread;
    m_pListenThread->m_bAutoDelete = TRUE;
    m_pListenThread->ResumeThread();

    LLOG_INFO("Socket Server Start!, Port %d", m_webbridgeport);
    return TRUE;
}

void LStockServerSocket::StopSocketServer()
{
    if (!m_bRun)
        return;

    LLOG_INFO("Stopping Socket Server...");

    // 停止线程循环
    m_bRun = FALSE;
    CloseAllClients();

    // 停止业务任务队列
    m_taskQueue.Uninit();

    // 等待监听线程退出
    if (m_pListenThread)
    {
        WaitForSingleObject(m_pListenThread->m_hThread, 3000);
    }

    // 关闭监听套接字
    if (m_ListenSocket != INVALID_SOCKET)
    {
        closesocket(m_ListenSocket);
        m_ListenSocket = INVALID_SOCKET;
    }

    // 关闭事件句柄
    if (m_ListenEvent)
    {
        WSACloseEvent(m_ListenEvent);
        m_ListenEvent = NULL;
    }

    // 清理Socket环境
    if (m_bWsaInit)
    {
        WSACleanup();
        m_bWsaInit = FALSE;
    }
    LLOG_INFO("Socket Server Stopped Successfully");
}

void LStockServerSocket::AddClient(const ClientInfo &client)
{
    EnterCriticalSection(&m_csClient);
    m_mapClients[client.socket] = client;
    LLOG_INFO("Client Added: %s:%d, Total: %d",
              client.ip.c_str(), client.port, (int)m_mapClients.size());
    LeaveCriticalSection(&m_csClient);
}

UINT LStockServerSocket::MarkClientForRemove(SOCKET sock)
{
    closesocket(sock); // 主动关闭套接字，避免连接残留
    EnterCriticalSection(&m_csPending);
    m_pendingRemoveList.push_back(sock);
    LeaveCriticalSection(&m_csPending);
    return 0;
}

void LStockServerSocket::ProcessPendingRemove()
{
    std::vector<SOCKET> tmpList;

    EnterCriticalSection(&m_csPending);
    if (m_pendingRemoveList.empty())
    {
        LeaveCriticalSection(&m_csPending);
        return;
    }
    // 批量取出，减少锁持有时间
    tmpList.swap(m_pendingRemoveList);
    LeaveCriticalSection(&m_csPending);

    // 锁外逐个清理（RemoveClient 内部会加 m_csClient 锁）
    for (SOCKET sock : tmpList)
    {
        RemoveClient(sock);
    }
}

void LStockServerSocket::RemoveClient(SOCKET sock)
{
    EnterCriticalSection(&m_csClient);
    auto it = m_mapClients.find(sock);
    if (it != m_mapClients.end())
    {
        // 安全等待客户端线程退出
        if (it->second.pThread)
        {
            WaitForSingleObject(it->second.pThread->m_hThread, 500);
        }
        // 关闭套接字
        if (it->second.socket != INVALID_SOCKET)
        {
            closesocket(it->second.socket);
        }
        // 移除记录（自动调用 ClientInfo 析构，释放 csSend）
        m_mapClients.erase(it);
        LLOG_INFO("Client Removed, Total: %d", (int)m_mapClients.size());
    }
    LeaveCriticalSection(&m_csClient);
}

void LStockServerSocket::CloseAllClients()
{
    EnterCriticalSection(&m_csClient);
    for (auto &pair : m_mapClients)
    {
        if (pair.second.pThread)
        {
            WaitForSingleObject(pair.second.pThread->m_hThread, 500);
        }
        if (pair.second.socket != INVALID_SOCKET)
        {
            closesocket(pair.second.socket);
        }
    }
    m_mapClients.clear();
    LeaveCriticalSection(&m_csClient);
}

/**
 * WebSocket 握手协议: 生成 Sec-WebSocket-Accept
 *
 * Sec-WebSocket-Accept标头至关重要，因为服务器必须根据客户端发送给它的Sec-WebSocket-Key生成该标头。
 * 生成方法为：将客户端的Sec-WebSocket-Key与字符串"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"（这是一个魔术字符串）拼接在一起，对结果进行 SHA-1 哈希计算，再将哈希结果进行base64编码后返回。
 */
std::string LStockServerSocket::MakeAccept(const std::string &key)
{
    const std::string GUID = WEB_SOCKET_GUID;
    std::string combined = key + GUID;
    return CommonUtils::StringHelper::Sha1ToBase64(combined);
}

/**
 * 解析接收的 WebSocket 帧
 *
 ```
  0               1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-------+-+-------------+-------------------------------+
 |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 | |1|2|3|       |K|             |                               |
 +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 |     Extended payload length continued, if payload len == 127  |
 + - - - - - - - - - - - - - - - +-------------------------------+
 |                               |Masking-key, if MASK set to 1  |
 +-------------------------------+-------------------------------+
 | Masking-key (continued)       |          Payload Data         |
 +-------------------------------- - - - - - - - - - - - - - - - +
 :                     Payload Data continued ...                :
 + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 |                     Payload Data continued ...                |
 +---------------------------------------------------------------+
 ```
 * - FIN:  1bit
 *    为1表示是消息的最后一帧，为0表示此消息还有更多后续帧。如果消息只有一帧那么第一帧也就是最后一帧。
 * - RSV1, RSV2, RSV3:  1 bit each
 *    必须是0，除非扩展定义为非零。如果接受到的是非零值但是扩展没有定义，则需要关闭连接。
 * - Opcode:  4 bits
 *    解释Payload数据，规定有以下不同的状态，如果是未知的，接收方必须马上关闭连接。状态如下：
 *    0x0(分片消息延续帧)
 *    0x1(UTF-8 文本数据帧)
 *    0x2(二进制数据帧)
 *    0x3-0x7(保留用于后续非控制帧)
 *    0x8(连接关闭)
 *    0x9(心跳请求)
 *    0xA(心跳响应)
 *    0xB-0xF(保留用于后续控制帧)
 * - Mask:  1 bit
 *    掩码，定义payload数据是否进行了掩码处理，如果是1表示进行了掩码处理。Masking-key域的数据即是掩码密钥，用于解码PayloadData。客户端发出的数据帧需要进行掩码处理，所以此位是1。
 * - Payload length:  7 bits, 7+16 bits, or 7+64 bits
 *    "Payload data"的长度（单位：字节）：
 *    0-125: 该数值即为Payload长度
 *    126:   接下来的16bits为有效载荷长度（最大65,535字节）
 *    127:   接下来的64bits为有效载荷长度（最大2^63-1字节）
 *    多字节长度值采用网络字节序表示。需注意，无论何种情况，都必须使用最少字节数编码长度，例如，长度为124字节的字符串不能编码为126、0、124这一序列。Payload长度为"Payload data"长度与"Application data"长度之和。"Extension data"长度可为0，此时Payload长度即为"Application data"的长度。
    ```javascript
    // Payload length encoding example
    function encodePayloadLength(length) {
      if (length <= 125) {
        return [length];
      } else if (length <= 65535) {
        return [126, (length >> 8) & 0xff, length & 0xff];
      } else {
        // For lengths > 65535, use 64-bit encoding
        return [127]; // 8 bytes of length
      }
    }
    ```
 * - Masking-key:  0 or 4 bytes
 *    如果MASK位设为1则有4个字节的掩码解密密钥，否则就没有。
    ```javascript
    // Masking/unmasking payload data
    function maskPayload(payload, maskingKey) {
      const masked = new Uint8Array(payload.length);
      for (let i = 0; i < payload.length; i++) {
        masked[i] = payload[i] ^ maskingKey[i % 4];
      }
      return masked;
    }
    ```
 * - Payload data:  (x+y) bytes
 *    "Payload data"定义为"Extension data"与"Application data"的拼接，如果没有定义扩展则没有此项，仅含有Application data。
 * - Extension data:  x bytes
 *    除非已协商扩展，否则"Extension data"长度为 0 字节。任何扩展均必须指定"Extension data"的长度、该长度的计算方式，以及在握手阶段必须如何协商该扩展的使用。若存在"Extension data"，其长度将计入总负载长度。
 * - Application data:  y bytes
 *    任意"Application data"，占据帧中"Extension data"之后的剩余部分。"Application data"的长度等于负载长度减去"Extension data"的长度。
 */
int LStockServerSocket::ParseFrame(const char *frame, size_t frameLen, BYTE &outOpcode, bool &outFin, std::vector<uint8_t> &outPayload)
{
    outPayload.clear();
    if (frameLen < 2)
        return 0;

    // 解析首字节：FIN + RSV + Opcode
    BYTE firstByte = (BYTE)frame[0];
    outFin = (firstByte & FIN_RSV_CODE) != 0;
    // 取出 OpCode
    outOpcode = firstByte & 0x0F;

    // RSV位校验（未协商扩展必须为0）
    if ((firstByte & 0x70) != 0)
    {
        return -1;
    }

    // 解析掩码标志与载荷长度
    BYTE secondByte = (BYTE)frame[1];
    bool hasMask = (secondByte & FIN_RSV_CODE) != 0;
    // 客户端发出的数据帧必须带掩码，没有掩码，无效数据帧
    if (!hasMask)
    {
        return -1;
    }

    // 取出载荷长度
    uint64_t payloadLen = secondByte & 0x7F;
    int headerLen = 2;

    // 解析扩展长度（网络字节序转主机字节序）
    if (payloadLen == 126)
    {
        if (frameLen < 4)
            return 0;
        payloadLen = ntohs(*(const uint16_t *)(frame + 2));
        headerLen = 4;
    }
    else if (payloadLen == 127)
    {
        if (frameLen < 10)
            return 0;
        uint32_t high = ntohl(*(const uint32_t *)(frame + 2));
        uint32_t low = ntohl(*(const uint32_t *)(frame + 6));
        payloadLen = ((uint64_t)high << 32) | low;
        headerLen = 10;
    }

    // 控制帧校验
    if (outOpcode >= OP_CODE_CTRL_CLOSE)
    {
        // RFC6455规范: 禁止控制帧进行分片及其载荷长度超过125
        if (!outFin || payloadLen > 125)
        {
            return -1;
        }
    }

    // 校验完整帧长度
    uint64_t totalFrameLen = headerLen + 4 + payloadLen;
    if (frameLen < (int)totalFrameLen)
        return 0;

    // 单帧载荷超限直接拒绝
    if (payloadLen > MAX_MESSAGE_SIZE)
    {
        return -1;
    }

    // 获取掩码密钥
    const BYTE *maskKey = (const BYTE *)(frame + headerLen);
    int payloadOffset = headerLen + 4;

    // 解掩码
    outPayload.resize((size_t)payloadLen);
    for (uint64_t i = 0; i < payloadLen; i++)
    {
        outPayload[i] = frame[payloadOffset + i] ^ maskKey[i % 4];
    }

    return (int)totalFrameLen;
}

bool LStockServerSocket::SendCommonFrame(SOCKET sock, CRITICAL_SECTION *pSendCs, BYTE opcode, const char *data, size_t len)
{
    if (sock == INVALID_SOCKET || !pSendCs)
        return false;

    // 计算帧头长度
    size_t headerLen = 2;
    if (len > 65535)
        headerLen += 8;
    else if (len > 125)
        headerLen += 2;

    // 构造帧数据
    std::string frame;
    frame.resize(headerLen + len);

    // 首字节：FIN=1 + RSV=0 + opcode
    frame[0] = (char)(FIN_RSV_CODE | opcode);

    // 第二字节: 载荷长度编码
    if (len <= 125)
    {
        frame[1] = (char)len;
    }
    else if (len <= 65535)
    {
        frame[1] = 126;
        uint16_t netLen = htons((uint16_t)len);
        memcpy(&frame[2], &netLen, 2);
    }
    else
    {
        frame[1] = 127;
        uint64_t netLen = _byteswap_uint64(len); // 64位大端转换
        memcpy(&frame[2], &netLen, 8);
    }

    // 拷贝载荷
    if (len > 0 && data != nullptr)
    {
        memcpy(&frame[headerLen], data, len);
    }

    // 加锁发送，防止多线程并发 send 导致数据错乱
    EnterCriticalSection(pSendCs);
    int sent = send(sock, frame.data(), (int)frame.size(), 0);
    LeaveCriticalSection(pSendCs);

    return sent == (int)frame.size();
}

bool LStockServerSocket::SendPong(SOCKET sock, CRITICAL_SECTION *pSendCs, const char *pingData, size_t len)
{
    return SendCommonFrame(sock, pSendCs, OP_CODE_CTRL_PONG, pingData, len);
}

bool LStockServerSocket::SendPing(SOCKET sock, CRITICAL_SECTION *pSendCs)
{
    return SendCommonFrame(sock, pSendCs, OP_CODE_CTRL_PING, NULL, 0);
}

bool LStockServerSocket::SendClose(SOCKET sock, CRITICAL_SECTION *pSendCs)
{
    return SendCommonFrame(sock, pSendCs, OP_CODE_CTRL_CLOSE, NULL, 0);
}

bool LStockServerSocket::SendText(SOCKET sock, CRITICAL_SECTION *pSendCs, const std::string &data)
{
    return SendCommonFrame(sock, pSendCs, OP_CODE_TEXT, data.data(), data.size());
}

bool LStockServerSocket::SendToSocket(SOCKET sock, const std::string &data)
{
    // 加全局锁查找客户端，拿到发送锁指针后立即释放全局锁
    EnterCriticalSection(&m_csClient);
    auto it = m_mapClients.find(sock);
    if (it == m_mapClients.end())
    {
        LeaveCriticalSection(&m_csClient);
        return FALSE;
    }
    CRITICAL_SECTION *pSendCs = &it->second.csSend;
    LeaveCriticalSection(&m_csClient);
    return SendCommonFrame(sock, pSendCs, OP_CODE_TEXT, data.data(), data.size());
}

void LStockServerSocket::HandleClientTask(ConstSharedClientTask task)
{
    // 注意：此函数在后台线程执行，操作UI需转发到主线程
    if (task->opcode == OP_CODE_TEXT)
    {
        // 文本消息：转字符串，解析JSON等
        std::string content = task->text();
        wxLogDebug("HandleClientTask: %s", content);
        size_t scheme_pos = content.find(BRIDAGE_PROTOCOL_SCHEME);
        if (scheme_pos != std::string::npos)
        {
            wxLogDebug("HandleClientTask: BRIDAGE PROTOCOL");
            m_bridge->HandleBridgeData(task, content, scheme_pos);
        }
    }
    else if (task->opcode == OP_CODE_BINARY)
    {
        // 二进制消息：按协议解析
        wxLogDebug("Handle client binary message, length: %zu", task->opcode, task->message.size());
    }
}

/**
 * 处理单个客户端连接
 *   ┌──────────┐                               ┌──────────┐
 *   │  Client  │                               │  Server  │
 *   └──────┬───┘                               └──────┬───┘
 *          │                                          │
 *          │  GET /chat HTTP/1.1                      │
 *          │  Host: example.com                       │
 *          │  Upgrade: websocket                      │
 *          │  Connection: Upgrade                     │
 *          │  Sec-WebSocket-Key: [base64]             │
 *          │  Sec-WebSocket-Version: 13               │
 *          ├─────────────────────────────────────────>│
 *          │                                          │
 *          │  HTTP/1.1 101 Switching Protocols        │
 *          │  Upgrade: websocket                      │
 *          │  Connection: Upgrade                     │
 *          │  Sec-WebSocket-Accept: [hash]            │
 *          │<─────────────────────────────────────────┤
 *          │                                          │
 *          │       WebSocket Connection               │
 *          │<────────────────────────────────────────>│
 *          │                                          │
 */
UINT __cdecl LStockServerSocket::HandleClient(LPVOID lpParam)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    SOCKET clientSocket = (SOCKET)lpParam;
    LStockServerSocket &server = LStockServerSocket::GetInstance();

    char recvBuf[RECV_BUFFER_SIZE] = {0};
    int totalRecv = 0;
    bool bClose = false;

    // 接收握手请求
    while (totalRecv < sizeof(recvBuf) - 1)
    {
        int recvLen = recv(clientSocket, recvBuf + totalRecv, RECV_BUFFER_SIZE - 1 - totalRecv, 0);
        // 连接异常直接退出
        if (recvLen <= 0)
        {
            return server.MarkClientForRemove(clientSocket);
        }
        totalRecv += recvLen;
        recvBuf[totalRecv] = '\0';

        // 检查是否收到完整HTTP头（\r\n\r\n）
        char *headerEnd = strstr(recvBuf, "\r\n\r\n");
        if (headerEnd != NULL)
        {
            LLOG_INFO("Start HANDSHAKE...");

            // 握手残留的WebSocket数据转入客户端缓冲区
            int headerLen = (int)(headerEnd - recvBuf) + 4;
            int remainLen = totalRecv - headerLen;

            EnterCriticalSection(&server.m_csClient);
            auto it = server.m_mapClients.find(clientSocket);
            if (it != server.m_mapClients.end() && remainLen > 0)
            {
                ClientInfo &client = it->second;
                // 缓冲区超限检查
                if (client.recvBuffer.size() + remainLen <= MAX_RECV_BUFFER_SIZE)
                {
                    client.recvBuffer.append(recvBuf + headerLen, remainLen);
                }
            }
            LeaveCriticalSection(&server.m_csClient);
            break;
        }
    }

    // 提取 Sec-WebSocket-Key（大小写不敏感）
    std::string key;
    const char *keyTag = "Sec-WebSocket-Key: ";
    if (char *pos = strstr(recvBuf, keyTag))
    {
        pos += strlen(keyTag);
        char *end = strstr(pos, "\r\n");
        if (end)
        {
            key = std::string(pos, end - pos);
        }
    }

    if (key.empty())
    {
        // 非WebSocket请求，返回400错误
        const char *badReq = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        send(clientSocket, badReq, (int)strlen(badReq), 0);
        LLOG_INFO("HANDSHAKE FAIL!");
        return server.MarkClientForRemove(clientSocket);
    }

    // 发送握手响应
    std::string acceptKey = MakeAccept(key);
    LLOG_INFO("Server Sec Accept: %s", acceptKey.c_str());
    std::string response =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " +
        acceptKey + "\r\n\r\n";
    send(clientSocket, response.c_str(), (int)response.size(), 0);
    LLOG_INFO("Client Handshake Success");

    // 初始化客户端状态
    DWORD now = GetTickCount();
    EnterCriticalSection(&server.m_csClient);
    auto itInit = server.m_mapClients.find(clientSocket);
    if (itInit != server.m_mapClients.end())
    {
        ClientInfo &client = itInit->second;
        client.lastRecvTime = now;
        client.lastPongTime = now;
        client.lastPingTime = now;
        client.inFragment = false;
        client.fragmentOpcode = 0;
        client.fragmentBuffer.clear();
        client.currentDelay = MIN_DELAY;
    }
    LeaveCriticalSection(&server.m_csClient);

    // 循环接收WebSocket数据
    LLOG_INFO("Wait Client Data...");
    while (server.m_bRun && !bClose)
    {
        now = GetTickCount();
        CRITICAL_SECTION *pSendCs = nullptr;

        bool needSendPing = false;

        // 心跳检测与发送
        EnterCriticalSection(&server.m_csClient);
        auto itHeart = server.m_mapClients.find(clientSocket);
        if (itHeart == server.m_mapClients.end())
        {
            LeaveCriticalSection(&server.m_csClient);
            break;
        }
        ClientInfo &clientHeart = itHeart->second;
        pSendCs = &clientHeart.csSend;

        // 心跳超时检测：无数据且无PONG超过阈值则断开
        DWORD lastActive = max(clientHeart.lastRecvTime, clientHeart.lastPongTime);
        if (now - lastActive > HEARTBEAT_TIMEOUT)
        {
            LeaveCriticalSection(&server.m_csClient);
            LLOG_ERROR("Heartbeat timeout, close connection");
            break;
        }

        // 定时发送 PING
        if (now - clientHeart.lastPingTime > PING_INTERVAL)
        {
            needSendPing = true;
            clientHeart.lastPingTime = now;
        }
        LeaveCriticalSection(&server.m_csClient);

        if (needSendPing)
        {
            SendPing(clientSocket, pSendCs);
        }

        // 接收数据
        int recvLen = recv(clientSocket, recvBuf, sizeof(recvBuf), 0);

        if (recvLen == 0)
        {
            LLOG_INFO("Client Disconnected!");
            break;
        }

        if (recvLen == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            // 10035：无数据可读（正常现象），继续循环
            if (err == WSAEWOULDBLOCK)
            {
                // 动态调整延迟
                EnterCriticalSection(&server.m_csClient);
                auto itDelay = server.m_mapClients.find(clientSocket);
                if (itDelay == server.m_mapClients.end())
                {
                    LeaveCriticalSection(&server.m_csClient);
                    break;
                }
                ClientInfo &clientDelay = itDelay->second;
                // 计算距离上次接收数据的空闲时间
                DWORD idleTime = now - clientDelay.lastRecvTime;

                // 空闲超时 → 逐步增加延迟（低频通讯）
                if (idleTime > IDLE_THRESHOLD)
                {
                    clientDelay.currentDelay = min(clientDelay.currentDelay + DELAY_STEP, MAX_DELAY);
                }
                // 未超时 → 保持最小延迟（高频通讯）
                else
                {
                    clientDelay.currentDelay = MIN_DELAY;
                }

                int delay = clientDelay.currentDelay;
                LeaveCriticalSection(&server.m_csClient);

                // 执行动态延时
                Sleep(delay);
                continue;
            }

            // 连接错误
            LLOG_ERROR("Client disconnected, error: %d", err);
            break;
        }

        // 收到新数据：更新状态 + 追加到专属缓冲区 + 批量解析 + 分片拼接 + 缓冲区上限校验
        std::vector<OpMessage> readyMessages; // 完整业务消息列表（原始字节）
        bool parseError = false;
        // RFC 6455 规范要求：每收到一个 PING 帧，都应该回复一个对应载荷的 PONG 帧。
        std::vector<std::vector<uint8_t>> pongPayloads; // 收集所有待回复的 PING 载荷
        bool needSendClose = false;

        EnterCriticalSection(&server.m_csClient);
        auto itParse = server.m_mapClients.find(clientSocket);
        if (itParse == server.m_mapClients.end())
        {
            LeaveCriticalSection(&server.m_csClient);
            break;
        }
        // 重置延迟
        ClientInfo &client = itParse->second;
        pSendCs = &client.csSend;
        client.lastRecvTime = now;
        client.currentDelay = MIN_DELAY;

        // 接收缓冲区超限保护
        if (client.recvBuffer.size() + recvLen > MAX_RECV_BUFFER_SIZE)
        {
            parseError = true;
            LLOG_ERROR("Recv buffer overflow, close connection");
        }
        else
        {
            client.recvBuffer.append(recvBuf, recvLen);
        }

        // 循环解析所有完整帧
        while (!parseError)
        {
            BYTE opcode = 0;
            bool fin = false;
            std::vector<uint8_t> payload;
            int consumed = ParseFrame(
                client.recvBuffer.data(),
                client.recvBuffer.size(),
                opcode,
                fin,
                payload);

            if (consumed > 0)
            {
                client.recvBuffer.erase(0, consumed);
            }
            else if (consumed == 0)
            {
                // 半包，等待更多数据，退出解析循环
                break;
            }
            else
            {
                // 帧解析错误，数据已错位，继续接收无意义
                parseError = true;
                break;
            }

            // 按 Opcode 分发处理
            switch (opcode)
            {
            case OP_CODE_TEXT:
            case OP_CODE_BINARY:
                if (fin)
                {
                    // 单帧完整消息
                    if (client.inFragment)
                    {
                        // 上一分片未结束就来新消息，协议错误
                        parseError = true;
                    }
                    else if (payload.size() <= MAX_MESSAGE_SIZE)
                    {
                        // 分片接收完成
                        readyMessages.push_back({opcode, std::move(payload)});
                    }
                    else
                    {
                        // 单条消息超限直接拒绝（含分片单帧）
                        parseError = true;
                    }
                }
                else
                {
                    // 分片消息
                    if (client.inFragment)
                    {
                        // 重叠分片，非法
                        parseError = true;
                    }
                    else
                    {
                        // 保存分片起始帧
                        client.inFragment = true;
                        client.fragmentOpcode = opcode;
                        client.fragmentBuffer = std::move(payload);
                    }
                }
                break;

            case OP_CODE_CONTINUATION:
                if (!client.inFragment)
                {
                    parseError = true; // 无起始帧的续传帧，非法
                }
                else
                {
                    // 分片超限检查
                    if (client.fragmentBuffer.size() + payload.size() > MAX_MESSAGE_SIZE)
                    {
                        parseError = true;
                    }
                    else
                    {
                        client.fragmentBuffer.insert(
                            client.fragmentBuffer.end(),
                            payload.begin(),
                            payload.end());
                        if (fin)
                        {
                            // 分片接收完成
                            readyMessages.push_back({client.fragmentOpcode, std::move(client.fragmentBuffer)});
                            client.inFragment = false;
                            client.fragmentBuffer.clear();
                            client.fragmentOpcode = 0;
                        }
                    }
                }
                break;

            case OP_CODE_CTRL_PING:
                // 收到PING立刻回PONG（RFC6455规范）
                pongPayloads.push_back(std::move(payload));
                break;

            case OP_CODE_CTRL_PONG:
                // 收到PONG，更新心跳时间
                client.lastPongTime = now;
                break;

            case OP_CODE_CTRL_CLOSE:
                // 收到关闭帧，回复关闭帧后断开
                needSendClose = true;
                bClose = true;
                break;

            default:
                // 未知Opcode
                parseError = true;
                break;
            }

            if (bClose)
                break;
        }

        // 解析错误清空所有缓冲区
        if (parseError)
        {
            client.recvBuffer.clear();
            client.fragmentBuffer.clear();
            client.inFragment = false;
            client.fragmentOpcode = 0;
        }
        LeaveCriticalSection(&server.m_csClient);

        // 批量 PONG
        for (auto &payload : pongPayloads)
        {
            SendPong(clientSocket, pSendCs, reinterpret_cast<const char *>(payload.data()), payload.size());
        }

        if (needSendClose)
        {
            SendClose(clientSocket, pSendCs);
        }

        // 分发消息队列
        for (auto &msg : readyMessages)
        {
            LLOG_INFO("RECV Client Data, op: %d len: %zu", msg.opcode, msg.payload.size());
            auto task = std::make_shared<ClientTask>();
            task->sock = clientSocket;
            task->opcode = msg.opcode;
            task->message = std::move(msg.payload);
            server.m_taskQueue.PushTask(task);

            // 响应客户端 自动回复 success
            SendText(clientSocket, pSendCs, R"({"code":0,"msg":"success"})");
        }

        if (parseError)
        {
            LLOG_ERROR("Invalid frame, close connection");
            break;
        }
    }

    return server.MarkClientForRemove(clientSocket);
}

// 监听客户端连接线程
UINT __cdecl LStockServerSocket::ListenThread(LPVOID lpParam)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    // 获取类实例指针
    LStockServerSocket *pThis = (LStockServerSocket *)lpParam;
    if (!pThis)
        return 0;

    SOCKADDR_IN clientAddr{};
    int addrLen = sizeof(clientAddr);

    while (pThis->m_bRun)
    {
        pThis->ProcessPendingRemove();

        // 等待客户端连接事件（100ms超时，防止卡死）
        DWORD ret = WSAWaitForMultipleEvents(1, &pThis->m_ListenEvent, FALSE, 100, FALSE);
        if (ret == WSA_WAIT_TIMEOUT)
            continue;

        // 接受客户端连接
        SOCKET clientSocket = accept(pThis->m_ListenSocket, (SOCKADDR *)&clientAddr, &addrLen);
        if (clientSocket == INVALID_SOCKET)
            continue;

        // 获取客户端IP/端口
        char clientIP[256] = {0};
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, 256);
        UINT clientPort = ntohs(clientAddr.sin_port);

        ClientInfo client(clientSocket, nullptr, clientIP, clientPort);
        pThis->AddClient(client);

        // 为每个客户端创建独立线程，不阻塞Server线程
        CWinThread *hClientThread = AfxBeginThread(HandleClient, (LPVOID)clientSocket, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
        if (hClientThread)
        {
            hClientThread->m_bAutoDelete = TRUE;
            hClientThread->ResumeThread();

            // 回填线程句柄
            EnterCriticalSection(&pThis->m_csClient);
            auto it = pThis->m_mapClients.find(clientSocket);
            if (it != pThis->m_mapClients.end())
                it->second.pThread = hClientThread;
            LeaveCriticalSection(&pThis->m_csClient);
        }
        else
        {
            pThis->RemoveClient(clientSocket);
        }
    }
    return 0;
}

//----------------------------------------------------------------------
// TaskQueue
//----------------------------------------------------------------------
LStockServerSocket::TaskQueue::TaskQueue() : m_hEvent(NULL), m_bRun(FALSE)
{
}

LStockServerSocket::TaskQueue::~TaskQueue()
{
    Uninit();
}

void LStockServerSocket::TaskQueue::Init()
{
    if (m_bRun)
        return;

    InitializeCriticalSection(&m_cs);
    m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_bRun = TRUE;

    // 启动业务工作线程
    for (int i = 0; i < WORKER_THREAD_COUNT; i++)
    {
        CWinThread *pThread = AfxBeginThread(WorkerThread, this,
                                             THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
        if (pThread)
        {
            pThread->m_bAutoDelete = TRUE;
            pThread->ResumeThread();
        }
    }
}

void LStockServerSocket::TaskQueue::Uninit()
{
    if (!m_bRun)
        return;

    m_bRun = FALSE;
    SetEvent(m_hEvent);
    Sleep(100); // 等待线程自然退出

    EnterCriticalSection(&m_cs);
    while (!m_queue.empty())
        m_queue.pop();
    LeaveCriticalSection(&m_cs);

    DeleteCriticalSection(&m_cs);
    CloseHandle(m_hEvent);
    m_hEvent = NULL;
}

void LStockServerSocket::TaskQueue::PushTask(SharedClientTask task)
{
    EnterCriticalSection(&m_cs);
    m_queue.push(task);
    LeaveCriticalSection(&m_cs);
    SetEvent(m_hEvent);
}

UINT __cdecl LStockServerSocket::TaskQueue::WorkerThread(LPVOID lpParam)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    TaskQueue *pThis = (TaskQueue *)lpParam;
    LStockServerSocket &server = LStockServerSocket::GetInstance();

    while (pThis->m_bRun)
    {
        SharedClientTask task = std::make_shared<ClientTask>();
        bool hasTask = false;

        EnterCriticalSection(&pThis->m_cs);
        if (!pThis->m_queue.empty())
        {
            task = pThis->m_queue.front();
            pThis->m_queue.pop();
            hasTask = true;
        }
        LeaveCriticalSection(&pThis->m_cs);

        if (hasTask)
        {
            server.HandleClientTask(task);
        }
        else
        {
            WaitForSingleObject(pThis->m_hEvent, INFINITE);
        }
    }
    return 0;
}
