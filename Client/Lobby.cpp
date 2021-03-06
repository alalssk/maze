#include "Lobby.h"

HANDLE Lobby::hLobbyEventForRequest;

Lobby::Lobby()
{
}


Lobby::~Lobby()
{
}
const int Lobby::LobbyMain()
{
	sock = user->getSocket();
	hLobbyEventForRequest = CreateEvent(NULL, FALSE, FALSE, NULL);
	gride.ClearXY();
	PrintLobbyListBox();
	gride.GrideBox(46, 6 + (Linfo.GetLobbyTxtNum() * 3), 1, 6);
	Sleep(50);
	req_GetWaitingRoom();
	Sleep(50);
	PrintWaitingRoomList();
	PrintLobbyTxt();
	while (1)
	{
		key = gride.getKeyDirectionCheck();
		Linfo.SetLobbyFlag(key);

		if (Linfo.GetLobbyFlag() == false)
		{
			if (key == SPACE || key == ENTER)
			{//메뉴선택확인
				if (Linfo.GetLobbyTxtNum() == 0) //방만들기
				{
					if (req_CreateRoom())return CREATE_ROOM;
					else
					{
						for (int i = 0; i < 3; i++)
						{
							gride.DrawXY(40, 15, "###########방생성 실패#############");
							Sleep(500);
							gride.DrawXY(40, 15, "                                   ");
							Sleep(500);
						}
						continue;
					}
				}
				else if (Linfo.GetLobbyTxtNum() == 1) //logoout
				{
					if (req_LogoutClient())
					{
						return LOGOUT_CODE;
					}
					else
					{
						for (int i = 0; i < 3; i++)
						{
							gride.DrawXY(40, 15, "###########로그아웃 실패#############");
							Sleep(500);
							gride.DrawXY(40, 15, "                                     ");
							Sleep(500);
						}
						continue;
					}
				}
				else if (Linfo.GetLobbyTxtNum() == 2)// exit
				{
					if (req_ExitClient())
					{
						return EXIT_CODE;
					}
					else
					{
						for (int i = 0; i < 3; i++)
						{
							gride.DrawXY(40, 15, "###########게임종료 실패#############");
							Sleep(500);
							gride.DrawXY(40, 15, "                                     ");
							Sleep(500);
						}
						continue;
					}
				}
			}
			else
			{//메뉴선택(방향키)
				Linfo.SetLobbyTxtNum(key);
				initRoomListCheck();
				AllClearPrintLobbyTxtBox();
				gride.GrideBox(46, 6 + (Linfo.GetLobbyTxtNum() * 3), 1, 6);
				PrintLobbyTxt();
			}
		}
		else
		{//방선택 부분

			//RoomInfoListMain();
			if (key == SPACE || key == ENTER)
			{	//WaitingRoomList[LobbyListPointNum] -> 선택 방이름
				//send는 방번호를 send해야한다
				//Linfo.WaitingRoomList[]에는 "No.2>>[방이름]" 이런식으로 들어가있으니 방 번호만 빼서 send
				char * tmp;
				int iRoomNum;
				tmp = strtok(Linfo.WaitingRoomList[Linfo.GetLobbyListPointNumber()], ".");
				tmp = strtok(NULL, ">");
				iRoomNum = atoi(tmp);

				if (req_EnterWaitingRoom(iRoomNum))//방입장성공
				{
					return JOIN_ROOM;
				}
				else
				{
					for (int i = 0; i < 3; i++)
					{
						gride.DrawXY(40, 15, "###########방입장 실패#############");
						Sleep(500);
						gride.DrawXY(40, 15, "                                     ");
						Sleep(500);
					}
					continue;
				}
				//Linfo.WaitingRoomList[Linfo.GetLobbyListPointNumber()];
			}
			else
			{
				// RoomExit();
				Linfo.SetLobbyListPointNumber(key);
				initRoomListCheck();
				PrintWaitingRoomList();
				PrintLobbyListCheck(Linfo.GetLobbyListPointNumber());

				AllClearPrintLobbyTxtBox();
				PrintLobbyTxt();
			}
		}
	}
}
void Lobby::PrintLobbyListBox()
{
	int i = 0;
	gride.DrawXY(16, 3, " 대 기 방 목 록");
	gride.GrideBox(5, 4, 14, 18);
}
void Lobby::AllClearPrintLobbyTxtBox()
{
	for (int i = 0; i < 12; i++)
	{
		gride.DrawXY(46, 6 + i, "                ");
	}
}
void Lobby::PrintLobbyTxt()
{
	for (int i = 0; i < LOBBY_TXT_NUM_MAX; i++)
	{
		gride.gotoxy(48, 7 + i * 3);
		switch (i)
		{
		case 0:
			cout << "방 만 들 기"; break;
		case 1:
			cout << "로 그 아 웃"; break;
		case 2:
			cout << "게 임 종 료"; break;
		}
	}
}

void Lobby::initRoomListCheck()
{
	for (int i = 0; i < 14; i++)
	{
		gride.DrawXY(7, 5 + i, "   ");
	}
}
void Lobby::setUserInfo(UserInfo *user)
{
	this->user = user;
}
void Lobby::setSock(SOCKET sock)
{
	this->sock = sock;
}
bool Lobby::req_GetWaitingRoom()//방요청(시작시)
{
	send(sock, "@r", 2, 0);
	switch (WaitForSingleObject(hLobbyEventForRequest, 5000))
	{
	case WAIT_TIMEOUT:
		return false; break;
	case WAIT_OBJECT_0:
		return true; break;
	default:
		return false; break;
	}
}
bool Lobby::req_CreateRoom()
{
	send(sock, "@R", 2, 0);
	switch (WaitForSingleObject(hLobbyEventForRequest, 5000))
	{
	case WAIT_TIMEOUT:
		return false; break;
	case WAIT_OBJECT_0:
		return true; break;
	default:
		return false; break;
	}
}
bool Lobby::req_LogoutClient()
{
	send(sock, "@L", 2, 0);
	switch (WaitForSingleObject(hLobbyEventForRequest, 5000))
	{
	case WAIT_TIMEOUT:
		return false; break;
	case WAIT_OBJECT_0:
		return true; break;
	default:
		return false; break;
	}
}
bool Lobby::req_ExitClient()
{
	send(sock, "@G", 2, 0);
	switch (WaitForSingleObject(hLobbyEventForRequest, 5000))
	{
	case WAIT_TIMEOUT:
		return false; break;
	case WAIT_OBJECT_0:
		return true; break;
	default:
		return false; break;
	}
}
bool Lobby::req_EnterWaitingRoom(int RoomNum)
{
	char sendstr[10];//@G_9999
	sprintf(sendstr, "@J_%d", RoomNum);
	send(sock, sendstr, strlen(sendstr) + 1, 0);

	switch (WaitForSingleObject(hLobbyEventForRequest, 5000))
	{
	case WAIT_TIMEOUT:
		return false; break;
	case WAIT_OBJECT_0:
		return true; break;
	default:
		return false; break;
	}
}
void Lobby::PrintWaitingRoomList()
{
	initPrintWaitingRoomList();
	gride.gotoxy(0, 0); Sleep(100);
	for (int i = 0; i < Linfo.WaitingRoomListNum; i++)
	{
		gride.gotoxy(7 + 3, 5 + i); cout << Linfo.WaitingRoomList[i];
	}
}
void Lobby::GetWaitingRoomList(char *buf)
{//[방개수]_[No.[방번호] 방이름]_[방이름]_[방이름]
	//WaitingRoomCount
	char *tmp;
	int iRoomCount;

	memset(Linfo.WaitingRoomList, 0, sizeof(Linfo.WaitingRoomList));//2차원배열(tmp[max][max]) 초기화하는 법 확실하게 테스트 하자
	//memset(tmp,0,sizeof(tmp)) 해도 2차원 다 초기화 된다.
	if (buf[0] == '0')
	{//방이없음
		Linfo.WaitingRoomListNum = 0;
	}
	else
	{//방이 있는경우
		tmp = strtok(buf, "_");
		iRoomCount = atoi(tmp);
		Linfo.WaitingRoomListNum = iRoomCount;
		for (int i = 0; i < Linfo.WaitingRoomListNum; i++)
		{
			tmp = strtok(NULL, "_");
			strcpy(Linfo.WaitingRoomList[i], tmp);
		}
	}
	Linfo.initLobbyListPointNumber();
	PrintWaitingRoomList();
}

void Lobby::initPrintWaitingRoomList()
{
	for (int i = 0; i < 14; i++)
	{
		gride.DrawXY(7 + 3, 5 + i, "                              ");
	}
}

void Lobby::PrintLobbyListCheck(int WaitingRoomListPointNumber)
{
	gride.DrawXY(7, 5 + WaitingRoomListPointNumber, "☞ ");
}


