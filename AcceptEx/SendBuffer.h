#pragma once

#include <string>

class CSendBuffer
{
public:
	CSendBuffer(void);
	~CSendBuffer(void);

public:
	void AppendData( void * pvData, unsigned nLen ){
		m_strBuffer.append( (const char*)pvData, nLen );
	}

	/// 获取下一个需要发送的数据
	/// 指针的有效期到下一次调用GetNextData
	void * GetNextData( unsigned & nLen ){
		// 正在发送的已经发好了。
		m_strSending.clear();

		// 需要发送的变成当前要发送的。
		std::swap( m_strSending, m_strBuffer );

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

