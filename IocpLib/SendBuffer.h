#pragma once

#include <string>

class CSendBuffer
{
public:
	CSendBuffer(void);
	~CSendBuffer(void);

public:
	void AppendData( void * pvData, unsigned nLen ){

		// TODO: 这里应该有个最大长度，超过这个最大长度就打出日志，报警。
		// 或者设置一个最大厂的。如果超过，就失败。
		// 这里采用的是连续的空间，可能效率上不是太好。
		// 所以一定要有最大长度的限制，否则就不可控了。
		m_strBuffer.append( (const char*)pvData, nLen );
	}

	/// 获取下一个需要发送的数据
	/// 指针的有效期到下一次调用GetNextData
	void * GetNextData( unsigned & nLen ){
		// 正在发送的已经发好了。
		m_strSending.clear();

		// 需要发送的变成当前要发送的。
		m_strSending.swap( m_strBuffer );
		//std::swap( m_strSending, m_strBuffer );

		nLen = m_strSending.size();
		return (void*)m_strSending.data();
	}

	void ClearData() {
		m_strSending.clear();
		m_strBuffer.clear();
	}

private:
	std::string m_strSending;	// 当前正在发送的
	std::string m_strBuffer;	// 需要发送的。
};

