#pragma once
#include <mutex>
#include <queue>
#include "MysqlConn.h"
#include <condition_variable>
class ConnectionPool
{
public: 
	// ��ȡ���ӳ�Ψһʵ��������ʽ������
	static ConnectionPool* getConnectionPool();

	ConnectionPool(const ConnectionPool& obj) = delete; // ɾ���������캯��

	// ɾ����ֵ��������أ���ֹ��ֵ
	ConnectionPool& operator = (const ConnectionPool& obj) = delete;

	// �����ӳ��л�ȡ�������ӣ�ʹ�� shared_ptr �Զ��黹��
	shared_ptr< MysqlConn> getConnection();
	~ConnectionPool();
private:

	// ˽�й��캯������ϵ���ģʽ������ʱ��ʼ�����ӳز����������߳�
	ConnectionPool();

	// �� JSON �����ļ��ж�ȡ���ӳ����ò������� IP���û�������������ȣ�
	bool parseJosnFile();

	// ���̣߳�������������ʱ�Զ��������ӣ�������С������
	void produceConnection();

	// ���̣߳����ڼ�鲢�ͷſ���ʱ�䳬����ֵ������
	void recycleConnection();

	// ����һ���µ����Ӳ��������
	void addConnection();

	string m_ip;
	string m_user;
	string m_passwd;
	string m_dbName;
	unsigned short m_port;
	int m_minSize;
	int m_timeout;  // �ȴ����ӳ�ʱʱ�䣨���룩
	int m_maxSize;
	int m_maxIdleTime; // ������ʱ�䣨��ʱ�����ӽ����ͷţ�

	// ========== ���ӳ�״̬ ==========
	queue<MysqlConn*> m_connectionQ;   // �ɸ������Ӷ���
	mutex m_mutexQ;                    // ���л��������������Ӷ����̰߳�ȫ
	condition_variable m_cond;         // �����������������ӵȴ�/֪ͨ����

};

