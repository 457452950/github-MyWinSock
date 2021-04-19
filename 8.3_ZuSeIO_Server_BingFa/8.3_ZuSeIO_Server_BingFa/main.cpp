#include <iostream>
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const int nDefaultServerport = 4000;
const int nBuffSize = 1024;

SOCKET BindListen();

SOCKET AcceptConnection(SOCKET sdListen);
bool ProcessConnection(SOCKET sd);
bool ShutdownConnection(SOCKET sd);

DWORD WINAPI ThreadProc(LPVOID lpParam);

void DoWork();

int main(void)
{
	WSADATA wsaData = { 0 };
	WORD wVer = MAKEWORD(2, 2);
	int nRet = WSAStartup(wVer, &wsaData);
	if (nRet)
	{
		cout << "WSAStartup error" << endl;
	}

	DoWork();


	WSACleanup();
	return 0;
}

// ����һ�������׽��֣����������״̬
SOCKET BindListen()
{
	// ����һ�������׽���
	SOCKET sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == INVALID_SOCKET)
	{
		cout << "socket error" << WSAGetLastError() << endl;
		return INVALID_SOCKET;
	}
	cout << "socket !" << endl;

	// ��䱾���׽��ֵ�ַ
	sockaddr_in saListen;
	saListen.sin_family = AF_INET;
	saListen.sin_addr.s_addr = htonl(INADDR_ANY);
	saListen.sin_port = htons(nDefaultServerport);

	// ����bind�ѱ����׽��ֵ�ַ�󶨵������׽���
	if (bind(sd, (sockaddr*)&saListen, sizeof(sockaddr_in)) == SOCKET_ERROR)
	{
		cout << "bind error" << WSAGetLastError() << endl;
		closesocket(sd);
		return INVALID_SOCKET;
	}
	cout << "bind !" << endl;

	// ��ʼ����
	if (listen(sd, 5) == SOCKET_ERROR)
	{
		cout << "listen error" << WSAGetLastError() << endl;
		closesocket(sd);
		return INVALID_SOCKET;
	}
	cout << "listen !" << endl;

	return sd;
}

// ����һ���ͻ������Ӳ����ض�Ӧ�ڸ����ӵ��׽��־��
SOCKET AcceptConnection(SOCKET sdListen)
{
	sockaddr_in saRemote;
	int nSize = sizeof(sockaddr_in);
	SOCKET sd = accept(sdListen, (sockaddr*) &saRemote, &nSize);
	if (sd == INVALID_SOCKET)
	{
		cout << "accept error" << WSAGetLastError() << endl;
	}
	return sd;
}

// ����һ���ͻ������ӣ�ʵ�ֻ��Է���ҵ���߼�
bool ProcessConnection(SOCKET sd)
{
	char buff[nBuffSize];
	int nRecv;

	// ѭ��ֱ���ͻ��˹ر���������
	do
	{
		// ���տͻ�������
		nRecv = recv(sd, buff, nBuffSize, 0);
		if (nRecv == SOCKET_ERROR)
		{
			cout << "recv error " << WSAGetLastError() << endl;
			return false;
		}
		else if (nRecv > 0 )
		{
			int nSent = 0;
			buff[nRecv] = 0;
			cout << buff << endl;
			// �����ݷ��ͻؿͻ���
			while (nSent < nRecv)
			{
				// �����sendҲ������
				int nTemp = send(sd, &buff[nSent], nRecv - nSent, 0);
				if (nTemp > 0)
				{
					nSent += nTemp;
				}
				else if (nTemp == SOCKET_ERROR)
				{
					cout << "send error " << WSAGetLastError() << endl;
					return false;
				}
				else
				{
					// send ����0����ʱnSent<nRecv����ʱ����û�з��ͳ�ȥ�����ӱ��ͻ�������ر�
					cout << "Connection closed unexpectedly by peer." << endl;
					return true;
				}
			}
		}
	} while (nRecv != 0);
	cout << "Connection closed by peer." << endl;
	return true;
}

// ��ȫ�ر�һ��TCP����
bool ShutdownConnection(SOCKET sd)
{
	// ���ȷ���һ��TCP FIN�ֶΣ���Է������Ѿ�������ݷ��͡�
	if (shutdown(sd, SD_SEND) == SOCKET_ERROR)
	{
		cout << "shutDown error " << WSAGetLastError() << endl;
		return false;
	}
	cout << "shutDown ! " << endl;

	char buff[nBuffSize];
	int nRecv;
	// �������նԷ����ݣ�ֱ��recv����0Ϊֹ
	do
	{
		nRecv = recv(sd, buff, nBuffSize, 0);
		if (nRecv == SOCKET_ERROR)
		{
			cout << "recv error " << WSAGetLastError() << endl;
			return false;
		}
		else if(nRecv > 0)
		{
			cout << nRecv << "unexpected bytes received." << endl;
		}
	} while (nRecv != 0);
	cout << "recv over ! " << endl;
	// �ر��׽���
	if (closesocket(sd) == SOCKET_ERROR)
	{
		cout << "closesocket error " << WSAGetLastError() << endl;
		return false;
	}
	cout << "closesocket ! " << endl;
	return true;
}


// ����һ���ͻ������ӵ��̺߳���������lpParam���ǿͻ������ӵ��׽��־��
DWORD __stdcall ThreadProc(LPVOID lpParam)
{
	SOCKET sd = (SOCKET) lpParam;

	// �ڶ��׶η���һ���ͻ���
	if (ProcessConnection(sd) == false)
	{
		return -1;
	}

	// �����׶Σ��ر�һ���ͻ�������
	if (ShutdownConnection(sd) == false)
	{
		return -1;
	}
	return 0;
}


void DoWork()
{
	// ��ȡ�׽��ֲ��������״̬
	SOCKET sdListen = BindListen();
	if (sdListen == INVALID_SOCKET)
	{
		printf("Bind ERROR");
		return;
	}
	printf("Bind OK\n");

	// ����������ѭ��
	while (true)
	{
		cout << "waiting and Listening " << endl;
		// ��һ�׶Σ�����һ���ͻ������ӣ�
		SOCKET sd = AcceptConnection(sdListen);
		if (sd == INVALID_SOCKET)
		{
			printf("Accept Error\n");
			break;
		}
		printf("--New connection\n");

		// ����һ���µ��߳�������ս��ܵĿͻ�������
		DWORD dwThreadID;
		HANDLE hThread = CreateThread(0, 0, ThreadProc, (LPVOID) sd, 0, &dwThreadID);
		bool res = CloseHandle(hThread);
		cout << "res = " << res << endl;
	}
	// �ر��׽���
	if (closesocket(sdListen) == SOCKET_ERROR)
	{
		cout << "closesocket error" << WSAGetLastError() << endl;
	}
	printf("closedsocket Over\n____________________\n");
}