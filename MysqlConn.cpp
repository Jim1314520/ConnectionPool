#include "MysqlConn.h"

MysqlConn::MysqlConn()
{
	m_conn = mysql_init(nullptr);
	mysql_set_character_set(m_conn, "utf8");
}

MysqlConn::~MysqlConn()
{
	if (m_conn != nullptr) {
		mysql_close(m_conn);  // �ر����ݿ�����
	}
	freeResult(); // �ͷŲ�ѯ����������ڴ�й©
}

bool MysqlConn::connect(string user, string passwd, string dbname, string ip, unsigned short port)
{
	MYSQL* ptr = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), passwd.c_str(), dbname.c_str(), port, nullptr, 0);
	//return ptr != nullptr;
	if (!ptr) {
		//std::cerr << "Connect failed: " << mysql_error(m_conn) << std::endl;
		return false;
	}
	//if (mysql_autocommit(m_conn, true)) {
	////	std::cerr << "[MysqlConn] failed to enable autocommit: "
	//	//	<< mysql_error(m_conn) << std::endl;
	//}

	return true;

}




bool MysqlConn::update(string sql)
{
	/*if (mysql_query(m_conn, sql.c_str())) {
		return false;
	}
	return true;*/
	// ִ�зǲ�ѯ������INSERT, UPDATE, DELETE��
	if (mysql_query(m_conn, sql.c_str())) { // ��ѯ�ɹ�����0
		std::cerr << "Update failed: " << sql
			<< " | error: " << mysql_error(m_conn) << std::endl;
		return false;
	}
	return true;

}

bool MysqlConn::query(string sql)
{
	freeResult();
	if (mysql_query(m_conn, sql.c_str())) {
		return false;
	}
	m_result = mysql_store_result(m_conn); // ��ȡ�����
	return true;
}

bool MysqlConn::next() 
{
	if (m_result != nullptr) {
		m_row = mysql_fetch_row(m_result); // �õ������������
		return m_row != nullptr;  // ����н�������� true
	}
	return false;
}

string MysqlConn::value(int index) //��ȡ��ǰ��ѯ�������ָ���е�ֵ
{
	int rowCount = mysql_num_fields(m_result); // ��ȡ��ǰ��������������ֶ�����
	if (index >= rowCount || index < 0) {
		return string();
	}
	char* val = m_row[index];
	unsigned long length = mysql_fetch_lengths(m_result)[index];
	return string(val, length);  // ��ȷ����ָ�����ȵ�ԭʼ���ݣ������� \0 ������
}

bool MysqlConn::transaction() //// ��ʼ���񣬽����Զ��ύ
{
	return mysql_autocommit(m_conn, false);
}

bool MysqlConn::commit()
{ // �ύ����
	return mysql_commit(m_conn);
}

bool MysqlConn::roollback()
{
	return mysql_rollback(m_conn);
}

void MysqlConn::refreshAliveTime()
{
	m_alivetime = steady_clock::now(); // ˢ�µ�ǰʱ�䣬��¼���ӻ�Ծʱ��
}

long long MysqlConn::getAliveTime()
{
	nanoseconds res = steady_clock::now() - m_alivetime;
	milliseconds millsec = duration_cast<milliseconds> (res);
	return millsec.count(); // �������Ӵ��ĺ�����
}

void MysqlConn::freeResult()
{ // �ͷŲ�ѯ����������ڴ�й©
	if (m_result) {
		mysql_free_result(m_result);
		m_result = nullptr;
	}
}
