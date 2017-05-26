#include "RecvThreadClass.h"

int RecvThreadClass::ClientMode = 0;
bool RecvThreadClass::threadOn = false;//�ӽ�
bool RecvThreadClass::ExitFlag = false;
bool RecvThreadClass::LogoutFlag = false;
RecvThreadClass::RecvThreadClass()
{
}


RecvThreadClass::~RecvThreadClass()
{
}
void RecvThreadClass::StartThread()
{

	if (!threadOn)
	{
		hRcvThread = (HANDLE)_beginthreadex(NULL, 0, &RecvMsg, (void*)&tData, 0, NULL);
		threadOn = true;

	}

}
unsigned WINAPI RecvThreadClass::RecvMsg(void * arg)   // read thread main
{
	ThreadData tData = *((ThreadData*)arg);
	char recvMsg[BUF_SIZE] = "";
	char *tmp_tok;
	int itmp_RoomNum;
	int strLen;
	//ClientMode >= 1(lobby����) �϶� �Ѿ���� �̺�Ʈ ó�� WaitSingleObject(hEvent,INFINITE);
	//�׷� �α׾ƿ��Ҷ� �����嵵 ���������߰���

	while (!ExitFlag && !LogoutFlag)
	{
		strLen = recv(tData.sock, recvMsg, BUF_SIZE - 1, 0);
		//�� �κп� send�����忡�� �����÷��װ� ������ �̺�Ʈ ó����....
		//���2. ��Ĺ�� �ͺ���ŷ���� ����� (ioctlsocket)
		if (strLen == -1)
			return -1;

		if (recvMsg[0] == '!')
		{
			//������ �޾ƿ�==> !"�氳��"_"No.[���ȣ]>> [���̸�]"_...
			//!0 �̸� ���̾��ٴ� ����
			//�ϴ� �������� ! ����� ������ ������ �Լ��� ����� �����ϴ��� üũ����
			if (recvMsg[1] == '0')tData.lobby->GetWaitionRoomList("0");
			else tData.lobby->GetWaitionRoomList(recvMsg + 1);
			memset(recvMsg, 0, sizeof(recvMsg));
		}
		else if (recvMsg[0] == '/')
		{//ä�ø޽��� ����

		}
		else if (recvMsg[0] == '@')	//������Ʈ(req) �游���, ����(�α׾ƿ�)��û�Ϸ� �� �޽��� �޴°�
		{							//����� ���� ����(@R0, @R1), �����û�Ϸ�(@E1), �α׾ƿ�
			if (recvMsg[1] == 'R')
			{
				if (recvMsg[2] == '1')
				{
					SetEvent(tData.lobby->hEventForRequest);
				}
			}
			else if (recvMsg[1] == 'r')//���û(���۽�)
			{
				if (recvMsg[2] == '1')
				{
					SetEvent(tData.lobby->hEventForRequest);
				}
			}
			else if (recvMsg[1] == 'L')//�α׾ƿ���û�Ϸ�(@L1)
			{
				if (recvMsg[2] == '1')
				{
					SetEvent(tData.lobby->hEventForRequest);
					LogoutFlag = true;
				}
			}
			else if (recvMsg[1] == 'G')//�����û�Ϸ�(@G1)
			{
				if (recvMsg[2] == '1')
				{
					SetEvent(tData.lobby->hEventForRequest);
					ExitFlag = true;
				}
			}
		}
		else {}
	}
	cout << "exitRecvMsgThread" << endl;
	return 0;
}