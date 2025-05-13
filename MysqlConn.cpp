#include "MysqlConn.h"

MysqlConn::MysqlConn()
{
	m_conn = mysql_init(nullptr);
	mysql_set_character_set(m_conn, "utf8");
}

MysqlConn::~MysqlConn()
{
	if (m_conn != nullptr) {
		mysql_close(m_conn);  // 关闭数据库连接
	}
	freeResult(); // 释放查询结果，避免内存泄漏
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
	// 执行非查询操作（INSERT, UPDATE, DELETE）
	if (mysql_query(m_conn, sql.c_str())) { // 查询成功返回0
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
	m_result = mysql_store_result(m_conn); // 获取结果集
	return true;
}

bool MysqlConn::next() 
{
	if (m_result != nullptr) {
		m_row = mysql_fetch_row(m_result); // 得到结果集的列数
		return m_row != nullptr;  // 如果有结果，返回 true
	}
	return false;
}

string MysqlConn::value(int index) //获取当前查询结果行中指定列的值
{
	int rowCount = mysql_num_fields(m_result); // 获取当前结果集的列数（字段数）
	if (index >= rowCount || index < 0) {
		return string();
	}
	char* val = m_row[index];
	unsigned long length = mysql_fetch_lengths(m_result)[index];
	return string(val, length);  // 精确拷贝指定长度的原始数据，不依赖 \0 结束符
}

bool MysqlConn::transaction() //// 开始事务，禁用自动提交
{
	return mysql_autocommit(m_conn, false);
}

bool MysqlConn::commit()
{ // 提交事务
	return mysql_commit(m_conn);
}

bool MysqlConn::roollback()
{
	return mysql_rollback(m_conn);
}

void MysqlConn::refreshAliveTime()
{
	m_alivetime = steady_clock::now(); // 刷新当前时间，记录连接活跃时间
}

long long MysqlConn::getAliveTime()
{
	nanoseconds res = steady_clock::now() - m_alivetime;
	milliseconds millsec = duration_cast<milliseconds> (res);
	return millsec.count(); // 返回连接存活的毫秒数
}

void MysqlConn::freeResult()
{ // 释放查询结果，避免内存泄漏
	if (m_result) {
		mysql_free_result(m_result);
		m_result = nullptr;
	}
}
