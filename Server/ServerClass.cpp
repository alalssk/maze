#include "ServerClass.h"
int ServerClass::TotalConnectedClientCount = 0;
int ServerClass::TotalCreateRoomCount = 0;
bool ServerClass::ExitFlag = false;
int ServerClass::ChatRoomCount = 0;
LogClass ServerClass::Chatlog;
LogClass ServerClass::DBLog;
ServerDB ServerClass::sDB;
CRITICAL_SECTION ServerClass::cs;
ServerClass::ServerClass()
{
	shareData = new Shared_DATA;
	//memset(lpComPort, 0, sizeof(ComPort)); 왜 이렇게 초기화하면 push_back 부분에서 크래쉬가 나는지...?
	shareData->flags = 0;
	shareData->Clients_Num = 0;
}


ServerClass::~ServerClass()
{
	delete shareData;
	cout << "Delete CompletionPort" << endl;
}
void ServerClass::printConnectClientNum()
{
	cout << TotalConnectedClientCount << "명 접속중" << endl;
}
bool ServerClass::ServerClassMain()
{
	Chatlog.OpenFile("ChatLog.txt");
	DBLog.OpenFile("DBLog.txt");
	//start DB
	if (!sDB.StartDB())
	{
		return false;
	}
	//start socket
	shareData->hServSock = GetListenSock(9191, SOMAXCONN);
	if (shareData->hServSock == INVALID_SOCKET)
	{
		cout << "GetListenSockError(" << GetLastError() << ')' << endl;
		return false;
	}
	else{
		cout << "GetListenSock() is ok" << endl;
	}

	if (!Create_IOCP_ThreadPool())
	{
		cout << "Create ThreadPool for IOCP is failed..." << endl;

		return false;
	}
	else{
		cout << "Create ThreadPool for IOCP is Ok ..." << endl;
		cout << "Start AcceptThread." << endl;
	}
	InitializeCriticalSection(&cs);
	hTheards[0] = (HANDLE)_beginthreadex(NULL, 0, &AcceptThread, (void*)shareData, 0, NULL);//Begin Accept Thread
	cout << "servermain ok" << endl;

	return true;
}
unsigned ServerClass::AcceptThread(PVOID pComPort)
{
	LPShared_DATA lpComPort = (LPShared_DATA)pComPort;
	CLIENT_DATA client_data;

	int addrLen = sizeof(client_data.clntAdr);

	LPOVER_DATA ioInfo;

	while (!ExitFlag)
	{
		printf("%d명 접속중....", lpComPort->Clients_Num);
		printf("%d 번 째 클라이언트 Accept 대기중\n", TotalConnectedClientCount);
		client_data.hClntSock = accept(
			lpComPort->hServSock,
			(SOCKADDR*)&client_data.clntAdr,
			&addrLen
			);

		cout << "Accept 처리중..." << endl;
		//
		if (CreateIoCompletionPort(
			(HANDLE)client_data.hClntSock,
			lpComPort->hComPort,
			(DWORD)client_data.hClntSock,//컴플리션키로 
			0
			) == NULL)
		{
			cout << "Client socket connect to IOCP handle error : " << GetLastError() << endl;
			continue;
		}

		ioInfo = new OVER_DATA;
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = BUF_SIZE;
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->Mode = FIRST_READ;//첫 접속 처리를 위한 모드(ID password 처리)
		/*IO_PENDING 에러처리*/
		cout << "client sock: " << client_data.hClntSock << '-' << inet_ntoa(client_data.clntAdr.sin_addr) << endl;
		if (WSARecv(
			client_data.hClntSock,
			&(ioInfo->wsaBuf),
			1,
			&lpComPort->recvBytes,
			&lpComPort->flags,
			&(ioInfo->overlapped),
			NULL
			) == SOCKET_ERROR)
		{
			switch (WSAGetLastError())
			{
			case WSA_IO_PENDING:
				cout << "An overlapped operation was successfully initiated and completion will be indicated at a later time." << endl; break;
			default:
				cout << "WSAGetLastError code(" << WSAGetLastError() << ')' << endl;
				break;
			}
		}



	}


	return 0;
}
bool ServerClass::Create_IOCP_ThreadPool()
{

	shareData->hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (shareData->hComPort == INVALID_HANDLE_VALUE)
	{
		cout << "CreateIoCompletionPort Error(" << GetLastError() << ')' << endl;
		return false;
	}
	GetSystemInfo(&sysInfo);
	for (int i = 0; i < (int)sysInfo.dwNumberOfProcessors; i++)
		_beginthreadex(NULL, 0, &IOCPWorkerThread, (LPVOID)shareData, 0, NULL);

	return true;

}
void ServerClass::SendMsgFunc(char* buf, LPShared_DATA lpComPort, DWORD RecvSz)
{
	list<CLIENT_DATA>::iterator iter;
	iter = lpComPort->Clients.begin();
	while (iter != lpComPort->Clients.end())
	{
		send(iter->hClntSock, buf, RecvSz, 0);
		iter++;//ㅅㅂ뺴먹지 말자
	}
}
unsigned  __stdcall ServerClass::IOCPWorkerThread(LPVOID CompletionPortIO)
{
	//HANDLE hComPort = (HANDLE)pComPort;
	LPShared_DATA shareData = (LPShared_DATA)CompletionPortIO;

	/*컴플리션키*/
	SOCKET sock;
	/*컴플리션키*/

	DWORD bytesTrans;
	LPOVER_DATA ioInfo;
	DWORD flags = 0;
	char SendMsg[1024];
	char clntName[MAX_NAME_SIZE];
	memset(SendMsg, 0, (sizeof(SendMsg)));
	memset(clntName, 0, (sizeof(clntName)));
	CLIENT_DATA client_data;

	list<CLIENT_DATA>::iterator iter;
	list<ChatRoom>::iterator iter_ChatRoom;
	while (1)
	{
		BOOL bGQCS = GetQueuedCompletionStatus(
			shareData->hComPort,
			&bytesTrans,
			&sock,//컴플리션키
			(LPOVERLAPPED*)&ioInfo,
			INFINITE
			);//errorㅊ리 꼭 해줘야함 
		if (bGQCS)//gqcs 성공
		{

			if (ioInfo->Mode == FIRST_READ)//ID_PASS 입력된걸 DB처리
			{
				char first_send[5] = "";
				int DBcode;
				if (sDB.Check_Password(ioInfo->buffer))
				{
					DBcode = 0;//접속ㅇㅋ
					EnterCriticalSection(&cs);

					client_data.hClntSock = sock;
					strcpy(client_data.name, strtok(ioInfo->buffer, "_"));
					client_data.MyRoom = 0;
					shareData->Clients.push_back(client_data);//list
					shareData->Clients_Num++;
					cout << '[' << client_data.name << ']' << client_data.hClntSock << "님이 접속함 - " << endl;
					TotalConnectedClientCount++;
					LeaveCriticalSection(&cs);
				}
				else {
					DBcode = 2;//비번다름코드;
					strcpy(client_data.name, strtok(ioInfo->buffer, "_"));//얘를 cs로 감쌀 필요가 있는지...?
					cout << '[' << client_data.name << "]비번틀림" << endl;

				}
				//

				//

				//shareData->Clients.push_back()
				sprintf(first_send, "@%d", DBcode);
				//SendMsgFunc(first_send, shareData, 5);
				//당연히 해당 소캣에만 보내야되느데 전체send(SnedMsgFunc)를 해가지고 ㅅㅂ
				send(sock, first_send, 5, 0);
				delete ioInfo;
				//여기다 ioInfo->Mode = ROOM_READ 로 설정하고 룸 정보만 recv send 함
				//특정 코드(방생성 방입장 종료코드 등)을 받으면 그에대한 모드 처리

				/*WSARecv*/
				ioInfo = new OVER_DATA;
				memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
				if (DBcode == 2)ioInfo->Mode = FIRST_READ; //비번다르면 다시 읽어야하니까
				else ioInfo->Mode = READ;
			}
			else if (ioInfo->Mode == READ)
			{
				if (ioInfo->buffer[0] == '@')//방생성(@R), 방입장(@J)-MyRoom이 0인 경우만, 방 나가기(@E)-MyRoom이 0이 아닌 경우만
				{
					if (ioInfo->buffer[1] == 'R')	//방 생성 요청이 오면
					{								//방 생성 완료 후 모든 클라이언트에 새로운 방들에 대한 정보를 send함
						if (CreateRoomFunc(shareData, sock))//방 생성
						{
							//그리고 모든클라에 새로운 방정보 send
							cout << "방 생성완료" << endl;
							send(sock, "@R1", 3, 0);
						}
						else 
						{
							cout << "방 생성 실패" << endl;
							send(sock, "@R0", 3, 0);
						}

						
					}
					else if (ioInfo->buffer[1] == 'J')	//방 입장 요청 Join
					{//@J_[방번호]
						char *cRoomNum;
						int iRoomNum;
						strtok(ioInfo->buffer, "_");
						cRoomNum = strtok(NULL, "_");
						iRoomNum = atoi(cRoomNum);
						cout << "방 입장 패킷 받음" << iRoomNum << endl;
						//func(shareData, sock, roomNum)


					}
					else if (ioInfo->buffer[1] == 'E')
					{
						cout << "방 나가기 요청 패킷 받음" << endl;
					}
				}
				/*WSARecv*/
				delete ioInfo;
				ioInfo = new OVER_DATA;
				memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
				ioInfo->Mode = READ;

			}

			ioInfo->wsaBuf.len = BUF_SIZE;
			ioInfo->wsaBuf.buf = ioInfo->buffer;//버퍼의 포인터를 담음....?여기서 wsaBuf의 필요성에대해 알아보도록 하자
			WSARecv(sock, &(ioInfo->wsaBuf),
				1, NULL, &flags, &(ioInfo->overlapped), NULL);
		}
		else
		{
			/*gqcs 실패
			* - 1. GQCS의 4번째인자(inInfo) 가 NULL 인 경우 -
			* 2,3번 인자 역시 정의되지 않는다.
			* 함수 자체 에러로, 타임아웃발생(WAIT_TIMEOUT) 이나 IOCP핸들이 닫힌 경우(ERROR_ABANDONED_WAIT_0)인 경우 발생
			*
			* - 2. GQCS의 4번째 인자(inInfo) 가 NULL이 아닌 경우 -
			* IOCP와 연결된 장치의 입출력돠정에 에러가 발생한 경우임
			* iocp큐로부터 해당 입출력 완료 패킷이 dequeue된 상태다. 따라서 2,3번 매개변수 모두 적절한 값으로 채워짐.
			* ipOverlapped의 Internal 필드는 장치의 상태코드를 담고 있음.
			* GetLastError를 호출하면 해당 상태코드에 매치되는 win32 에러코드를 획득할 수 있음.
			***** NULL이 아닌 경우에는 해당 장치 핸들에 대한 에러처리만 수행 후 IOCP에 연결된 다른 장치의 입출력을 받아들이기 위해 계속 루프를 돌려야 햔다. *****
			*/
			DWORD dwErrCode = GetLastError();
			if (ioInfo != NULL)
			{
				switch (dwErrCode)
				{
				case ERROR_NETNAME_DELETED:
					cout << "소캣 연결 해제됨: ";
					CloseClientSock(sock, ioInfo, shareData);
					break;
				default:
					cout << "GQCS Linked file handle error(" << dwErrCode << ')' << endl; break;
				}

				continue;
			}
			else
			{
				if (dwErrCode == WAIT_TIMEOUT)
				{//지정한 시간 경과, INFINITE로 설정해놨기 때문에 굳이 정의할 필요없음.
					cout << "GQCS Error: Time out(" << dwErrCode << ')' << endl;
					break;
				}
				else
				{//gqcs 호출자체 문제 (IOCP핸들이 닫힌 경우)
					cout << "GQCS Error: IOCP handle has been closed" << '(' << dwErrCode << ')' << endl;
					break;
				}
			}
		}
	}//end while
	return 0;
}
const SOCKET ServerClass::GetListenSock(const int Port, const int Backlog = SOMAXCONN)
{
	SOCKET hServSock;
	//SOCKADDR_IN servAdr;//얘는 바인드까지만 쓰고 안쓰기 때문에? 생성하고 없어져도됨?
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "WSAStartup error(" << WSAGetLastError() << ')' << endl;
		return INVALID_SOCKET;
	}

	hServSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (hServSock == INVALID_SOCKET)
	{
		cout << "Socket failed, code(" << WSAGetLastError() << ')' << endl;
		return INVALID_SOCKET;
	}

	//memset(&servAdr, 0, sizeof(servAdr));
	memset(&shareData->servAdr, 0, sizeof(shareData->servAdr));
	shareData->servAdr.sin_family = AF_INET;
	shareData->servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	shareData->servAdr.sin_port = htons(Port);
	LONG lSock;
	lSock = bind(
		hServSock,
		(SOCKADDR*)&shareData->servAdr,
		sizeof(shareData->servAdr)
		);
	if (lSock == SOCKET_ERROR)
	{
		cout << "bind failed, code : " << WSAGetLastError() << endl;
		return INVALID_SOCKET;
	}

	lSock = listen(hServSock, Backlog);

	if (lSock == SOCKET_ERROR)
	{
		cout << "listen failed, code : " << WSAGetLastError() << endl;
		closesocket(hServSock);
		return INVALID_SOCKET;
	}
	return hServSock;
}
void ServerClass::ExitIOCP()
{
	ExitFlag = true;
}
void ServerClass::CloseClientSock(SOCKET sock, LPOVER_DATA ioInfo, LPShared_DATA lpComp)
{

	char CloseName[MAX_NAME_SIZE];
	char tmp[MAX_NAME_SIZE + 128];
	memset(tmp, 0, sizeof(tmp));
	memset(CloseName, 0, sizeof(CloseName));

	list<CLIENT_DATA>::iterator iter;
	iter = lpComp->Clients.begin();
	while (iter != lpComp->Clients.end())
	{
		if (iter->hClntSock == sock)
		{
			EnterCriticalSection(&cs);//cs
			if (iter->MyRoom != 0)
			{
				ExitRoomFunc(lpComp, iter->MyRoom, iter->name);
			}
			
			strcpy(CloseName, iter->name);
			iter = lpComp->Clients.erase(iter);
			lpComp->Clients_Num--;
			TotalConnectedClientCount--;
			LeaveCriticalSection(&cs);//cs
			break;
		}
		else
		{
			iter++;
		}
	}
	closesocket(sock);

	sprintf(tmp, "[%s] is disconnected...\n", CloseName);
	SendMsgFunc(tmp, lpComp, strlen(tmp));
	delete ioInfo;
	cout << tmp;
	//puts("DisConnect Client!");
}
const bool ServerClass::CreateRoomFunc(LPShared_DATA lpComp, SOCKET sock)
{
	ChatRoom room;
	/*
	*RoomName
	*RoomNumber
	*Client_IDs[3]
	*UserCount --> max=3
	*/
	char tmpRoomName[40] = "";
	list<CLIENT_DATA>::iterator iter;
	iter = lpComp->Clients.begin();
	while (iter != lpComp->Clients.end())
	{
		if (iter->hClntSock == sock)
		{
			if (iter->MyRoom == 0)
			{
				sprintf(tmpRoomName, "[%s]님의 방입니다.", iter->name);
				strcpy(room.chatRoomName, tmpRoomName);
				room.ChatRoomNum = TotalCreateRoomCount + 1;
				strcpy(room.ClientsID[0], iter->name);
				memset(room.ClientsID[1], 0, sizeof(room.ClientsID[1]));
				memset(room.ClientsID[2], 0, sizeof(room.ClientsID[2]));
				room.UserCount = 1;

				iter->MyRoom = room.ChatRoomNum;

				EnterCriticalSection(&cs);//cs
				lpComp->ChatRoomList.push_back(room);
				TotalCreateRoomCount++;
				LeaveCriticalSection(&cs);//cs
				return true;
			}
			else
			{
				return false;//이미 방에 입장중
			}
		}
		else
		{
			iter++;
		}
	}
	return false;//ID를 찾을수 없음.


}

const bool ServerClass::ExitRoomFunc(LPShared_DATA lpComp, int RoomNum, char *id)
{//이 함수는 항상 cs안에있어야함
	list<ChatRoom>::iterator iter;
	iter = lpComp->ChatRoomList.begin();
	while (iter != lpComp->ChatRoomList.end())
	{
		if (iter->ChatRoomNum == RoomNum)
		{
			if (iter->UserCount <= 1)
			{//방에 나뿐이없으면 그냥 방 삭제
				iter = lpComp->ChatRoomList.erase(iter);
				//방이 없어졌으니 새로운 방 정보들을 모든 클라에 send
				return true;
			}
			else
			{//방에 나말고 다른사람도 있으면....그냥 나만 나가
				for (int i = 0; i < 3; i++)
				{//대기방 ID리스트에서 해당ID 지우는 과정
					if (strcmp(iter->ClientsID[i], id) == 0)
					{
						memset(iter->ClientsID[i], 0, sizeof(iter->ClientsID[i]));
						for (int j = i; j < 3; j++)
						{
							if (j == 3 - 1)memset(iter->ClientsID[j], 0, sizeof(iter->ClientsID[j]));
							else strcpy(iter->ClientsID[j], iter->ClientsID[j + 1]);
						}
					}
				}
				iter->UserCount--;
			}
		
		}
		else iter++;
	}
	return false;
}
const bool ServerClass::JoinRoomFunc(LPShared_DATA lpComp, SOCKET sock, int RoomNum)
{
	list<ChatRoom>::iterator iter_room;
	list<CLIENT_DATA>::iterator iter_user;
	iter_room = lpComp->ChatRoomList.begin();
	while (iter_room != lpComp->ChatRoomList.end())
	{
		if (iter_room->ChatRoomNum == RoomNum)
		{
			break;
		}
		else iter_room++;
	}
	if (iter_room == lpComp->ChatRoomList.end())
	{//방을 못찾았으면
		cout << "방을 찾지 못함" << endl;
		return false;
	}
	else if (iter_room->UserCount >= 3)
	{//방을 찾았지만 꽉찬경우
		cout << "방이 꽉참" << endl;
		return false;
	}
	else
	{
		iter_user = lpComp->Clients.begin();
		while (iter_user != lpComp->Clients.end())
		{
			if (iter_user->hClntSock == sock)
			{
				EnterCriticalSection(&cs);//cs
				iter_user->MyRoom = RoomNum;
				strcpy(iter_room->ClientsID[iter_room->UserCount++], iter_user->name);
				LeaveCriticalSection(&cs);//cs
				return true;
			}
			else iter_user++;
		}
		if (iter_user == lpComp->Clients.end())
		{
			cout << "접속죽인 유저가 아닙니다" << endl;
			return false;
		}
	}
	return false;
}
void ServerClass::Print_UserList()
{
	list<CLIENT_DATA>::iterator iter;
	iter = shareData->Clients.begin();
	while (iter != shareData->Clients.end())
	{
		cout << '[' << iter->name << ']';
		iter++;
	}
	cout << endl;

}
void ServerClass::Print_RoomList()
{
	list<ChatRoom>::iterator iter;
	iter = shareData->ChatRoomList.begin();
	cout << shareData->ChatRoomList.size() << "개의 대기방" << endl;
	while (iter != shareData->ChatRoomList.end())
	{
		cout << '[' << iter->chatRoomName << ']'<<endl;
		iter++;
	}
}