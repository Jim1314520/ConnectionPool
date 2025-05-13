#pragma once
#include <iostream>
#include <mysql.h>
using namespace std;
#include <chrono>
using namespace chrono;
class MysqlConn
{
public:
	//��ʼ�����ݿ�����
	MysqlConn();
	//�ͷ����ݿ�����
	~MysqlConn();
	//�������ݿ�
	bool connect(string user, string passwd, string dbname, string ip, unsigned short port = 3306);
	// �������ݿ�
	bool update(string sql);
	// ��ѯ���ݿ�
	bool query(string sql);
	// ������ѯ�õ��Ľ����
	bool next();
	//�õ�������е��ֶ�ֵ
	string value(int index);
	//�������
	bool transaction();
	// �ύ����
	bool commit();
	// ����ع�
	bool roollback();
	//ˢ����ʼ�Ŀ���ʱ���
	void refreshAliveTime();
	// �������Ӵ�����ʱ��
	long long getAliveTime();
private:
	MYSQL* m_conn = nullptr;
	MYSQL_RES* m_result = nullptr;
	MYSQL_ROW m_row = nullptr;
	void freeResult();
	steady_clock::time_point m_alivetime;
};

