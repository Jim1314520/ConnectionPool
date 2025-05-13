#pragma once
#include <mutex>
#include <queue>
#include "MysqlConn.h"
#include <condition_variable>
class ConnectionPool
{
public: 
	// 获取连接池唯一实例（懒汉式单例）
	static ConnectionPool* getConnectionPool();

	ConnectionPool(const ConnectionPool& obj) = delete; // 删除拷贝构造函数

	// 删除赋值运算符重载，防止赋值
	ConnectionPool& operator = (const ConnectionPool& obj) = delete;

	// 从连接池中获取可用连接（使用 shared_ptr 自动归还）
	shared_ptr< MysqlConn> getConnection();
	~ConnectionPool();
private:

	// 私有构造函数，配合单例模式，构造时初始化连接池并启动管理线程
	ConnectionPool();

	// 从 JSON 配置文件中读取连接池配置参数（如 IP、用户、最大连接数等）
	bool parseJosnFile();

	// 子线程：连接数量不足时自动创建连接，保持最小连接数
	void produceConnection();

	// 子线程：定期检查并释放空闲时间超过阈值的连接
	void recycleConnection();

	// 创建一个新的连接并加入队列
	void addConnection();

	string m_ip;
	string m_user;
	string m_passwd;
	string m_dbName;
	unsigned short m_port;
	int m_minSize;
	int m_timeout;  // 等待连接超时时间（毫秒）
	int m_maxSize;
	int m_maxIdleTime; // 最大空闲时间（超时的连接将被释放）

	// ========== 连接池状态 ==========
	queue<MysqlConn*> m_connectionQ;   // 可复用连接队列
	mutex m_mutexQ;                    // 队列互斥锁，保护连接队列线程安全
	condition_variable m_cond;         // 条件变量，用于连接等待/通知机制

};

