#pragma once
#include "pch.h"
#include <WinSock2.h>
#include <string>
#include <map>
#include <queue>
#include <vector>
#include <minwinbase.h>
#include <wx/event.h>
#include <functional>

// WebSocket 协议常量定义
#define WEB_SOCKET_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define FIN_RSV_CODE 0x80
#define RECV_BUFFER_SIZE 2048
#define SEND_BUFFER_SIZE 4096

// WebSocket OpCode 完整定义
#define OP_CODE_CONTINUATION 0x00   // 分片续传帧
#define OP_CODE_TEXT 0x01           // UTF - 8 文本数据帧
#define OP_CODE_BINARY 0x02         // 二进制数据帧
#define OP_CODE_RESERVE_3 0x03      // 保留
#define OP_CODE_RESERVE_4 0x04      // 保留
#define OP_CODE_RESERVE_5 0x05      // 保留
#define OP_CODE_RESERVE_6 0x06      // 保留
#define OP_CODE_RESERVE_7 0x07      // 保留
#define OP_CODE_CTRL_CLOSE 0x08     // 关闭连接帧
#define OP_CODE_CTRL_PING 0x09      // 心跳请求帧
#define OP_CODE_CTRL_PONG 0x0A      // 心跳响应帧
#define OP_CODE_CTRL_RESERVE_B 0x0B // 保留
#define OP_CODE_CTRL_RESERVE_C 0x0C // 保留
#define OP_CODE_CTRL_RESERVE_D 0x0D // 保留
#define OP_CODE_CTRL_RESERVE_E 0x0E // 保留
#define OP_CODE_CTRL_RESERVE_F 0x0F // 保留

// 动态延时配置
#define MIN_DELAY 1        // 高频通讯：最小延迟(ms)
#define MAX_DELAY 50       // 低频空闲：最大延迟(ms)
#define IDLE_THRESHOLD 500 // 空闲阈值(ms)，超过则开始增加延迟
#define DELAY_STEP 5       // 每次递增的延迟步长(ms)

// 安全与心跳配置
#define MAX_RECV_BUFFER_SIZE (10 * 1024 * 1024) // 单客户端接收缓冲区上限 10MB
#define MAX_MESSAGE_SIZE (5 * 1024 * 1024)      // 单条完整消息（含分片）上限 5MB
#define PING_INTERVAL 30000                     // 心跳发送间隔 30秒
#define HEARTBEAT_TIMEOUT 60000                 // 心跳超时时间 60秒
#define WORKER_THREAD_COUNT 4                   // 业务工作线程数

namespace
{
    constexpr const char *BRIDAGE_PROTOCOL_SCHEME = "webridge://";

    // 桥接回调函数类型：入参为 回调ID、请求数据
    using BridgeCallFunc = std::function<void(const std::string &callbackId, const std::string &data)>;

    // 客户端结构体
    struct ClientInfo
    {
        SOCKET socket;       // 客户端 socket 连接
        CWinThread *pThread; // 客户端线程句柄
        std::string ip;      // 客户端 IP
        UINT port;           // 客户端端口
        DWORD lastRecvTime;  // 最后一次接收数据的时间戳
        int currentDelay;    // 动态自适应延迟(ms)

        // 每个客户端独立接收缓冲区（解决粘包）
        std::string recvBuffer;

        // 分片消息拼接
        std::vector<uint8_t> fragmentBuffer; // 分片载荷拼接缓存
        BYTE fragmentOpcode;                 // 分片消息原始 Opcode
        bool inFragment;                     // 是否处于分片接收中

        // 心跳管理
        DWORD lastPongTime; // 上次收到 PONG 响应的时间
        DWORD lastPingTime; // 上次发送 PING 的时间

        // 发送临界区（多线程并发发送保护）
        CRITICAL_SECTION csSend;

        ClientInfo()
            : socket(INVALID_SOCKET), pThread(nullptr), port(0),
              lastRecvTime(0), currentDelay(MIN_DELAY),
              fragmentOpcode(0), inFragment(false),
              lastPongTime(0), lastPingTime(0)
        {
            InitializeCriticalSection(&csSend);
        }

        ClientInfo(SOCKET s, CWinThread *t, const std::string &ip_, UINT p)
            : socket(s), pThread(t), ip(ip_), port(p),
              lastRecvTime(GetTickCount()), currentDelay(MIN_DELAY),
              fragmentOpcode(0), inFragment(false),
              lastPongTime(GetTickCount()), lastPingTime(GetTickCount())
        {
            InitializeCriticalSection(&csSend);
        }

        // 析构释放临界区
        ~ClientInfo()
        {
            DeleteCriticalSection(&csSend);
        }

        // 拷贝构造（兼容 std::map 插入，临界区不可拷贝，重新初始化）
        ClientInfo(const ClientInfo &other)
        {
            socket = other.socket;
            pThread = other.pThread;
            ip = other.ip;
            port = other.port;
            lastRecvTime = other.lastRecvTime;
            currentDelay = other.currentDelay;
            recvBuffer = other.recvBuffer;
            fragmentBuffer = other.fragmentBuffer;
            fragmentOpcode = other.fragmentOpcode;
            inFragment = other.inFragment;
            lastPongTime = other.lastPongTime;
            lastPingTime = other.lastPingTime;
            InitializeCriticalSection(&csSend);
        }

        // 赋值运算符
        ClientInfo &operator=(const ClientInfo &other)
        {
            if (this != &other)
            {
                DeleteCriticalSection(&csSend);

                socket = other.socket;
                pThread = other.pThread;
                ip = other.ip;
                port = other.port;
                lastRecvTime = other.lastRecvTime;
                currentDelay = other.currentDelay;
                recvBuffer = other.recvBuffer;
                fragmentBuffer = other.fragmentBuffer;
                fragmentOpcode = other.fragmentOpcode;
                inFragment = other.inFragment;
                lastPongTime = other.lastPongTime;
                lastPingTime = other.lastPingTime;

                InitializeCriticalSection(&csSend);
            }
            return *this;
        }
    };

    struct ClientTask
    {
        SOCKET sock;
        uint8_t opcode;
        std::vector<uint8_t> message;

        mutable std::string m_textCache;
        mutable bool m_hasCache = false;

        std::string text() const
        {
            if (!m_hasCache)
            {
                m_textCache.assign(message.begin(), message.end());
                m_hasCache = true;
            }
            return m_textCache;
        }
    };

    using SharedClientTask = std::shared_ptr<ClientTask>;
    using ConstSharedClientTask = std::shared_ptr<const ClientTask>;

    class LSocketBridge;
}

/**
 * @brief 股票服务端Socket类
 */
class LStockServerSocket
{
public:
    // 单例获取入口
    static LStockServerSocket &GetInstance();

    // 禁用拷贝构造和赋值运算符
    LStockServerSocket(const LStockServerSocket &) = delete;
    LStockServerSocket &operator=(const LStockServerSocket &) = delete;

public:
    // 启动Socket服务器
    BOOL StartSocketServer();
    // 停止Socket服务器
    void StopSocketServer();
    // 获取监听端口
    UINT GetWebBridgePort() const;
    // 获取当前客户端连接数
    int GetClientCount() const;
    // 广播消息给所有客户端
    void BroadcastMessage(const std::string &msg);
    LSocketBridge *GetBridge()
    {
        return m_bridge;
    }
    // 给指定 socket 发送数据
    bool SendToSocket(SOCKET sock, const std::string &data);

private:
    class TaskQueue
    {
    public:
        TaskQueue();
        ~TaskQueue();

        void Init();
        void Uninit();
        void PushTask(SharedClientTask task);

    private:
        static UINT __cdecl WorkerThread(LPVOID lpParam);

        std::queue<SharedClientTask> m_queue;
        CRITICAL_SECTION m_cs;
        HANDLE m_hEvent;
        BOOL m_bRun;
    };

private:
    // 监听套接字
    SOCKET m_ListenSocket;
    // 监听事件句柄
    HANDLE m_ListenEvent;
    // 服务运行标志
    BOOL m_bRun;
    // 监听线程句柄
    CWinThread *m_pListenThread;
    // WebBridge监听端口
    UINT m_webbridgeport;
    // 标记WSA是否初始化
    BOOL m_bWsaInit;

    LSocketBridge *m_bridge;

    // 多客户端管理
    mutable CRITICAL_SECTION m_csClient;       // 线程锁
    std::map<SOCKET, ClientInfo> m_mapClients; // 客户端连接池
    TaskQueue m_taskQueue;                     // 业务任务队列实例
    std::vector<SOCKET> m_pendingRemoveList;   // 待清理客户端队列
    CRITICAL_SECTION m_csPending;              // 线程锁

private:
    // 私有化构造/析构（单例）
    LStockServerSocket();
    ~LStockServerSocket();

    void AddClient(const ClientInfo &client);
    void RemoveClient(SOCKET sock);
    void CloseAllClients();

    // WebSocket 握手协议: 生成 Sec-WebSocket-Accept
    static std::string MakeAccept(const std::string &key);

    // 解析 WebSocket 单帧（返回消耗字节数：>0成功，=0半包，<0错误）
    static int ParseFrame(const char *frame, size_t frameLen, BYTE &outOpcode, bool &outFin, std::vector<uint8_t> &outPayload);

    // 发送PONG响应（PING载荷原样带回）
    static bool SendPong(SOCKET sock, CRITICAL_SECTION *pSendCs, const char *pingData, size_t len);
    // 发送PING心跳
    static bool SendPing(SOCKET sock, CRITICAL_SECTION *pSendCs);
    // 发送关闭
    static bool SendClose(SOCKET sock, CRITICAL_SECTION *pSendCs);
    // 发送文本
    static bool SendText(SOCKET sock, CRITICAL_SECTION *pSendCs, const std::string &data);
    // 通用帧发送
    static bool SendCommonFrame(SOCKET sock, CRITICAL_SECTION *pSendCs, BYTE opcode, const char *data, size_t len);

    // 业务消息处理（工作线程中异步执行）
    void HandleClientTask(ConstSharedClientTask task);

    // 处理单个客户端连接（线程函数）
    static UINT __cdecl HandleClient(LPVOID lpParam);
    // 监听客户端连接线程（线程函数）
    static UINT __cdecl ListenThread(LPVOID lpParam);

    // 标记客户端待清理（客户端线程内部调用）
    UINT MarkClientForRemove(SOCKET sock);
    // 处理待清理队列（监听线程定时调用）
    void ProcessPendingRemove();
};

namespace
{

    class LSocketBridge
    {
    public:
        LSocketBridge()
        {
            InitializeCriticalSection(&m_csCall);

            InitializeCriticalSection(&m_csCB);
        }
        ~LSocketBridge()
        {
            DeleteCriticalSection(&m_csCall);
            DeleteCriticalSection(&m_csCB);
        }

    private:
        std::map<std::string, BridgeCallFunc> m_callFuncs;
        CRITICAL_SECTION m_csCall;

        std::map<std::string, ConstSharedClientTask> m_callbacks;
        CRITICAL_SECTION m_csCB;

    public:
        // 注册回调函数
        void RegisterCallFunc(const std::string &callName, BridgeCallFunc func)
        {
            if (callName.empty() || !func)
                return;

            EnterCriticalSection(&m_csCall);
            m_callFuncs[callName] = std::move(func);
            LeaveCriticalSection(&m_csCall);
        }

        // 移除回调
        void UnregisterCallFunc(const std::string &callName)
        {
            EnterCriticalSection(&m_csCall);
            m_callFuncs.erase(callName);
            LeaveCriticalSection(&m_csCall);
        }

        // 查找回调函数
        BridgeCallFunc GetCallFunc(const std::string &callName)
        {
            BridgeCallFunc func;
            EnterCriticalSection(&m_csCall);
            auto it = m_callFuncs.find(callName);
            if (it != m_callFuncs.end())
            {
                func = it->second;
            }
            LeaveCriticalSection(&m_csCall);
            return func;
        }

        // 发送回调数据
        BOOL BridgeSendDataWithCallback(const std::string &callback_id, const std::string &data)
        {
            EnterCriticalSection(&m_csCB);
            auto callbackIt = m_callbacks.find(callback_id);
            if (callbackIt == m_callbacks.end())
            {
                LeaveCriticalSection(&m_csCB);
                return FALSE;
            }
            auto task = callbackIt->second;
            m_callbacks.erase(callback_id);
            LeaveCriticalSection(&m_csCB);
            if (task)
            {
                std::string text = MakeBridgeResponse(callback_id, data);
                return LStockServerSocket::GetInstance().SendToSocket(task->sock, text);
            }
            else
            {
                return FALSE;
            }
        }

        // 处理 bridge 协议数据
        void HandleBridgeData(ConstSharedClientTask task, const std::string &content, size_t scheme_pos)
        {
            std::string json_data = content.substr(scheme_pos + std::strlen(BRIDAGE_PROTOCOL_SCHEME));
            if (json_data.empty())
            {
                wxLogDebug("HandleBridgeData: INVALID BRIDAGE DATA!");
                return;
            }
            yyjson_doc* doc = yyjson_read(json_data.c_str(), json_data.size(), 0);
            if (doc == nullptr)
            {
                wxLogDebug("HandleBridgeData: json parse error!");
                return;
            }
            yyjson_val* root = yyjson_doc_get_root(doc);
            if (root == nullptr)
            {
                wxLogDebug("HandleBridgeData: root");
                yyjson_doc_free(doc);
                return;
            }

            std::string cateName = UtilJsonHlp::GetString(root, "cate").ToStdString();
            yyjson_val* container = yyjson_obj_get(root, "container");
            if (cateName.empty())
            {
                wxLogDebug("HandleBridgeData: NOT FOUND cate");
                yyjson_doc_free(doc);
                return;
            }

            if (cateName == "state")
            {
                int code = UtilJsonHlp::GetInt(container, "code", -1);
                if (code == 1)
                {
                    wxLogDebug("HandleBridgeData: state ready!");
                }
                else
                {
                    wxLogDebug("HandleBridgeData: state: %d", code);
                }
            }
            else if (cateName == "js_call_native")
            {
                std::string native_call = UtilJsonHlp::GetString(container, "native_call").ToStdString();
                std::string callback_id = UtilJsonHlp::GetString(container, "callback_id").ToStdString();
                std::string data = UtilJsonHlp::GetString(container, "data").ToStdString();
                wxLogDebug("HandleBridgeData: call: %s, cb: %s, data: %s", native_call.c_str(), callback_id.c_str(), data.c_str());
                if (native_call.empty())
                {
                    yyjson_doc_free(doc);
                    return;
                }

                auto func = GetCallFunc(native_call);
                if (func)
                {
                    try
                    {
                        if (!callback_id.empty())
                        {
                            EnterCriticalSection(&m_csCB);
                            m_callbacks[callback_id] = task;
                            LeaveCriticalSection(&m_csCB);
                        }
                        func(callback_id, data);
                    }
                    catch (const std::exception& e)
                    {
                        wxLogDebug("Call handler exception: %s", e.what());
                    }
                }
            }
            else if (cateName == "js_send_to_native")
            {
                wxString callback_id = UtilJsonHlp::GetString(container, "callback_id");
                wxString data = UtilJsonHlp::GetString(container, "data");
                wxLogDebug("HandleBridgeData: cb: %s, data: %s", callback_id, data);
            }
            else
            {
                wxLogDebug("HandleBridgeData: not support %s:", cateName);
            }
            yyjson_doc_free(doc);
        }

    private:
        // 生成标准 webridge 响应字符串
        std::string MakeBridgeResponse(const std::string &callbackId, const std::string &data)
        {
            yyjson_mut_doc* doc = yyjson_mut_doc_new(nullptr);
            yyjson_mut_val* root = yyjson_mut_obj(doc);
            yyjson_mut_doc_set_root(doc, root);

            yyjson_mut_obj_add_strcpy(doc, root, "cate", "native_response");
            yyjson_mut_obj_add_strcpy(doc, root, "callback_id", callbackId.c_str());
            yyjson_mut_obj_add_strcpy(doc, root, "container", data.c_str());

            size_t jsonLen = 0;
            char* jsonStr = yyjson_mut_write(doc, 0, &jsonLen);

            std::string result;
            if (jsonStr && jsonLen > 0)
            {
                result.append(jsonStr, jsonLen);
                free(jsonStr);
            }
            yyjson_mut_doc_free(doc);
            return result;
        }
    };
}
