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

// 返回一个监听套接字，并进入监听状态
SOCKET BindListen()
{
	// 创建一个监听套接字
	SOCKET sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == INVALID_SOCKET)
	{
		cout << "socket error" << WSAGetLastError() << endl;
		return INVALID_SOCKET;
	}
	cout << "socket !" << endl;

	// 填充本地套接字地址
	sockaddr_in saListen;
	saListen.sin_family = AF_INET;
	saListen.sin_addr.s_addr = htonl(INADDR_ANY);
	saListen.sin_port = htons(nDefaultServerport);

	// 调用bind把本地套接字地址绑定到监听套接字
	if (bind(sd, (sockaddr*)&saListen, sizeof(sockaddr_in)) == SOCKET_ERROR)
	{
		cout << "bind error" << WSAGetLastError() << endl;
		closesocket(sd);
		return INVALID_SOCKET;
	}
	cout << "bind !" << endl;

	// 开始监听
	if (listen(sd, 5) == SOCKET_ERROR)
	{
		cout << "listen error" << WSAGetLastError() << endl;
		closesocket(sd);
		return INVALID_SOCKET;
	}
	cout << "listen !" << endl;

	return sd;
}

// 接受一个客户端连接并返回对应于该连接的套接字句柄
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

// 服务一个客户端连接，实现回显服务业务逻辑
bool ProcessConnection(SOCKET sd)
{
	char buff[nBuffSize];
	int nRecv;

	// 循环直到客户端关闭数据连接
	do
	{
		// 接收客户端数据
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
			// 将数据发送回客户端
			while (nSent < nRecv)
			{
				// 这里的send也会阻塞
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
					// send 返回0，此时nSent<nRecv，此时数据没有发送出去，连接被客户端意外关闭
					cout << "Connection closed unexpectedly by peer." << endl;
					return true;
				}
			}
		}
	} while (nRecv != 0);
	cout << "Connection closed by peer." << endl;
	return true;
}

// 安全关闭一个TCP连接
bool ShutdownConnection(SOCKET sd)
{
	// 首先发送一个TCP FIN分段，向对方表明已经完成数据发送。
	if (shutdown(sd, SD_SEND) == SOCKET_ERROR)
	{
		cout << "shutDown error " << WSAGetLastError() << endl;
		return false;
	}
	cout << "shutDown ! " << endl;

	char buff[nBuffSize];
	int nRecv;
	// 继续接收对方数据，直到recv返回0为止
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
	// 关闭套接字
	if (closesocket(sd) == SOCKET_ERROR)
	{
		cout << "closesocket error " << WSAGetLastError() << endl;
		return false;
	}
	cout << "closesocket ! " << endl;
	return true;
}


// 服务一个客户端连接的线程函数，参数lpParam就是客户端连接的套接字句柄
DWORD __stdcall ThreadProc(LPVOID lpParam)
{
	SOCKET sd = (SOCKET) lpParam;

	// 第二阶段服务一个客户端
	if (ProcessConnection(sd) == false)
	{
		return -1;
	}

	// 第三阶段，关闭一个客户端连接
	if (ShutdownConnection(sd) == false)
	{
		return -1;
	}
	return 0;
}


void DoWork()
{
	// 获取套接字并进入监听状态
	SOCKET sdListen = BindListen();
	if (sdListen == INVALID_SOCKET)
	{
		printf("Bind ERROR");
		return;
	}
	printf("Bind OK\n");

	// 服务器的主循环
	while (true)
	{
		cout << "waiting and Listening " << endl;
		// 第一阶段，接受一个客户端连接，
		SOCKET sd = AcceptConnection(sdListen);
		if (sd == INVALID_SOCKET)
		{
			printf("Accept Error\n");
			break;
		}
		printf("--New connection\n");

		// 创建一个新的线程来服务刚接受的客户端连接
		DWORD dwThreadID;
		HANDLE hThread = CreateThread(0, 0, ThreadProc, (LPVOID) sd, 0, &dwThreadID);
		bool res = CloseHandle(hThread);
		cout << "res = " << res << endl;
	}
	// 关闭套接字
	if (closesocket(sdListen) == SOCKET_ERROR)
	{
		cout << "closesocket error" << WSAGetLastError() << endl;
	}
	printf("closedsocket Over\n____________________\n");
}