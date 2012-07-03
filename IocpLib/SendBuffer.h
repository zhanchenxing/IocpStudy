#pragma once

#include <string>

class CSendBuffer
{
public:
	CSendBuffer(void);
	~CSendBuffer(void);

public:
	void AppendData( void * pvData, unsigned nLen ){

		// TODO: ����Ӧ���и���󳤶ȣ����������󳤶Ⱦʹ����־��������
		// ��������һ����󳧵ġ������������ʧ�ܡ�
		// ������õ��������Ŀռ䣬����Ч���ϲ���̫�á�
		// ����һ��Ҫ����󳤶ȵ����ƣ�����Ͳ��ɿ��ˡ�
		m_strBuffer.append( (const char*)pvData, nLen );
	}

	/// ��ȡ��һ����Ҫ���͵�����
	/// ָ�����Ч�ڵ���һ�ε���GetNextData
	void * GetNextData( unsigned & nLen ){
		// ���ڷ��͵��Ѿ������ˡ�
		m_strSending.clear();

		// ��Ҫ���͵ı�ɵ�ǰҪ���͵ġ�
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
	std::string m_strSending;	// ��ǰ���ڷ��͵�
	std::string m_strBuffer;	// ��Ҫ���͵ġ�
};

