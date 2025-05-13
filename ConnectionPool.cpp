#include "ConnectionPool.h"
#include <json/json.h>
#include <fstream>
#include <thread>
using namespace Json;
// 获取连接池单例对象（懒汉式，线程安全）
ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool;// C++11 局部静态变量，线程安全初始化
	return &pool;
}

bool ConnectionPool::parseJosnFile()
{
	ifstream ifs("dbconfig.json");
	Reader rd;
	Value root;
	rd.parse(ifs, root);
	if (root.isObject()) {
		m_ip = root["ip"].asString();
		m_port = root["port"].asInt();
		m_user = root["userName"].asString();
		m_passwd = root["password"].asString();
		m_dbName = root["dbName"].asString();
		m_minSize = root["minSize"].asInt();
		
		
		m_timeout = root["timeout"].asInt();
		m_maxSize = root["maxSize"].asInt();
		m_maxIdleTime = root["maxIdleTime"].asInt();
		
		return true;
	}
	return false;
}
// 生产线程函数：连接不足时自动补充
void ConnectionPool::produceConnection()
{
	while (true) {
		unique_lock<mutex> locker(m_mutexQ);
		while (m_connectionQ.size() >= m_minSize) {
			m_cond.wait(locker); //  当前线程阻塞，自动释放锁，直到被唤醒
		}
		addConnection();
		m_cond.notify_all(); // 通知等待中的线程
	}
}
void ConnectionPool::recycleConnection()
{
	while (true) {
		this_thread::sleep_for(chrono::milliseconds(500));  // 每 500ms 检查一次
		lock_guard<mutex> locker(m_mutexQ);
		while (m_connectionQ.size() > m_minSize) {
			MysqlConn* conn = m_connectionQ.front();
			if (conn->getAliveTime() >= m_maxIdleTime) {
				m_connectionQ.pop();
				delete conn;
			}
			else {
				break;
			}
		}
	}
}
// 创建新连接并加入连接队列
void ConnectionPool::addConnection()
{
	MysqlConn* conn = new MysqlConn; // 创建一个MySQL的对象
	conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port);
	conn->refreshAliveTime();  // 记录当前活跃时间
	m_connectionQ.push(conn); // 加入连接队列
}
shared_ptr< MysqlConn> ConnectionPool::getConnection()
{
	unique_lock<mutex> locker(m_mutexQ);
	while (m_connectionQ.empty()) {
		if (cv_status::timeout == m_cond.wait_for(locker, chrono::milliseconds(m_timeout))) { // 先等待一段时间，再去访问
			if (m_connectionQ.empty()) {
				continue;
			}
		}
	}
	shared_ptr<MysqlConn> connptr(m_connectionQ.front(), [this](MysqlConn* conn) {
		lock_guard<mutex> locker(m_mutexQ);
		conn->refreshAliveTime();
		m_connectionQ.push(conn);
		});
	m_connectionQ.pop();
	m_cond.notify_all();
	return connptr;
	/*从连接池中拿出一个连接，用 shared_ptr 管理，并且当这个连接用完被释放的
	时候（引用计数为0），就会执行我们写的 lambda 函数，把连接自动放回池子。*/
}
ConnectionPool::~ConnectionPool()
{
	while (!m_connectionQ.empty()) {
		MysqlConn* conn = m_connectionQ.front();
		m_connectionQ.pop();
		delete conn;
	}
}
ConnectionPool::ConnectionPool(){
	if (!parseJosnFile()) {
		return;
	}
	for (int i = 0; i < m_minSize; i++) {
		addConnection();
	}
	thread producer(&ConnectionPool::produceConnection, this);
	thread recycler(&ConnectionPool::recycleConnection, this);
	producer.detach();
	recycler.detach();

}

