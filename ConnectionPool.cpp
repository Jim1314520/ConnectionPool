#include "ConnectionPool.h"
#include <json/json.h>
#include <fstream>
#include <thread>
using namespace Json;
// ��ȡ���ӳص�����������ʽ���̰߳�ȫ��
ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool;// C++11 �ֲ���̬�������̰߳�ȫ��ʼ��
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
// �����̺߳��������Ӳ���ʱ�Զ�����
void ConnectionPool::produceConnection()
{
	while (true) {
		unique_lock<mutex> locker(m_mutexQ);
		while (m_connectionQ.size() >= m_minSize) {
			m_cond.wait(locker); //  ��ǰ�߳��������Զ��ͷ�����ֱ��������
		}
		addConnection();
		m_cond.notify_all(); // ֪ͨ�ȴ��е��߳�
	}
}
void ConnectionPool::recycleConnection()
{
	while (true) {
		this_thread::sleep_for(chrono::milliseconds(500));  // ÿ 500ms ���һ��
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
// ���������Ӳ��������Ӷ���
void ConnectionPool::addConnection()
{
	MysqlConn* conn = new MysqlConn; // ����һ��MySQL�Ķ���
	conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port);
	conn->refreshAliveTime();  // ��¼��ǰ��Ծʱ��
	m_connectionQ.push(conn); // �������Ӷ���
}
shared_ptr< MysqlConn> ConnectionPool::getConnection()
{
	unique_lock<mutex> locker(m_mutexQ);
	while (m_connectionQ.empty()) {
		if (cv_status::timeout == m_cond.wait_for(locker, chrono::milliseconds(m_timeout))) { // �ȵȴ�һ��ʱ�䣬��ȥ����
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
	/*�����ӳ����ó�һ�����ӣ��� shared_ptr �������ҵ�����������걻�ͷŵ�
	ʱ�����ü���Ϊ0�����ͻ�ִ������д�� lambda �������������Զ��Żس��ӡ�*/
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

