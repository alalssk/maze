#include "WaitingRoom.h"
HANDLE WaitingRoom::hWaitingRoomEventForRequest;

WaitingRoom::WaitingRoom()
{
}


WaitingRoom::~WaitingRoom()
{
}
void WaitingRoom::AllClear()
{
	gride.ClearXY();
	gride.gotoxy(1, 1);
}
void WaitingRoom::PrintStartGameMsg()
{
	gride.DrawXY(12, 15, "+--------------------------------------------------+");
	gride.DrawXY(12, 16, "|           Press any key for start game           |");
	gride.DrawXY(12, 17, "+--------------------------------------------------+");
}
void WaitingRoom::initWaitingRoom()
{
	WRinfo.initWaitingRoomInfo();
}
void WaitingRoom::setUserInfo(UserInfo *input_user)
{
	user = input_user;
}
void WaitingRoom::req_SendMsg(char* msg)
{//=================== "/[방번호]_[ID]_[내용]
	char SendMsg[1024] = "";
	sprintf(SendMsg, "/%d_%s_%s", user->wData.RoomNum, user->getID(), msg);
	send(sock, SendMsg, strlen(SendMsg) + 1, 0);
}
int WaitingRoom::WatingRoomMain()
{
	hWaitingRoomEventForRequest = CreateEvent(NULL, FALSE, FALSE, NULL);
	gride.ClearXY();
	Sleep(500);
	sock = user->getSocket();
	//user.setUserID("alalssk");/*###임시 채팅로그 만드는중*/
	gride.DrawXY(5, 3, " No. ");
	gride.gotoxy(5 + 5, 3); cout << user->wData.RoomNum;
	gride.DrawXY(5 + 15, 3, "Name: ");
	gride.gotoxy(5 + 15 + 6, 3); cout << user->wData.RoomName;
	gride.GrideBox(5, 4, 3, 18);//PrintUserListBox
	PrintUserList();
	gride.GrideBox(5, 9, 20, 18);//PrintChatBox
	//PrintChat()
	PrintButton();
	char inputstr[MAXCHAR];
	int inputstrSz = 0;
	memset(inputstr, 0, sizeof(inputstr));

	while (!(user->IsCurrentClientMode(GameState::GAMEPLAY)))//(user->getClientMode() != 3))
	{
		key = gride.getKeyDirectionCheck();
		WRinfo.SetWaitingRoomFlag(key);//왼쪽 오른쪽 구분(메뉴선택, 채팅)
		if (WRinfo.GetWaitingRoomFlag() == false)
		{//메뉴선택부분
			if (key == ENTER || key == SPACE)
			{	//종료부분 방만 나가는걸로 게임종료는 로비랑 로그인 화면에서만
				//@E_[방번호]_[ID]
				if (WRinfo.GetWaitingRoomTxtNum() == 2)//exit room
				{
					sprintf(inputstr, "@E_%d_%s", user->wData.RoomNum, user->getID());
					send(sock, inputstr, strlen(inputstr) + 1, 0);
					memset(inputstr, 0, sizeof(inputstr)); inputstrSz = 0;
					if (WaitForSingleObject(hWaitingRoomEventForRequest, 5000) == WAIT_OBJECT_0)
					{
						//방정보 변경 >> 방 나가는거니까 아얘 방정보 초기화시켜버림
						user->ExitWaitingRoom();
						break;
					}
					else
					{
						continue;
					}
				}
				else if (WRinfo.GetWaitingRoomTxtNum() == 1)//게임시작버튼
				{
					if (strcmp(user->wData.UserName[0], user->getID()) == 0) // 방장만 누를수 있음
					{
						char StartMsg[3 + 4 + 13] = "";
						sprintf(StartMsg, "$R_%d", user->wData.RoomNum);//$R_방번호 전송 >> 해당 방 게임시작 요청

						send(sock, StartMsg, strlen(StartMsg) + 1, 0);

						key = gride.getKeyDirectionCheck();//방장만 break나 리턴하면 다른 방장이 아닌애들이랑 다른상태가 되니까 
					}
					else {
						//방장만 시작할수 있어욤 출력
					}
				}
			}
			else
			{
				WRinfo.SetWaitingRoomTxtNum(key);
				PrintButton();
			}
		}
		else
		{//채팅부분 구현
			if (key == ENTER)
			{//서버연결후 고칠부분
				//
				//ChattingSendToServer(string chat) >> 이 함수 안에 InputChatLog("alalssk", inputstr); 이함수를 넣음
				//여기선 채팅을 버서로 보내기만하고 inputstr이랑 sz초기화해준다
				//채팅 출력과  채팅창 초기화는 Recv스레드에서 할것임
				//=================================
				req_SendMsg(inputstr);
				//InputChatLog(user->getID(), inputstr);
				memset(inputstr, 0, sizeof(inputstr)); inputstrSz = 0;
				//initChatListBox();//채팅창 초기화
				//PrintChatLogList();//출력
				gride.DrawXY(7, 28, "                                   "); 
				gride.gotoxy(7, 28);
			}
			else
			{
				//initChatListBox();
				if (key != LEFT &&key != RIGHT && key != UP && key != DOWN &&key != ENTER){ inputstr[inputstrSz++] = key; }
				gride.DrawXY(7, 28, inputstr);
			}
			//채팅방 생성시 채팅출력용 스레드 만들어서 채팅list에 입력이있으면(타클라로부터(recv) 채팅출력
		}
	}//end while
	return 0;
}

void WaitingRoom::PrintUserList()
{
	Sleep(500);
	for (int i = 0; i < MAX_USERNUM; i++)
	{//임시 초기화
		gride.DrawXY(7, 5 + i, "                                     ");
	}
	for (int i = 0; i < MAX_USERNUM; i++)//MAX user 3
	{
		if (i < user->wData.ConnectUserNum)
		{
			gride.DrawXY(7, 5 + i, user->wData.UserName[i]);
		}
		else {
			gride.DrawXY(7, 5 + i, "                              ");
		}
	}
}
void WaitingRoom::PrintButton()
{
	AllClearPrintLobbyTxtBox();
	gride.GrideBox(50, 4 * WRinfo.GetWaitingRoomTxtNum(), 1, 7);// 4 * ButtonNumber(1,2)
	gride.DrawXY(56, 5, "Start !");//or Ready !!
	gride.DrawXY(56, 9, "Exit !!");
}
void WaitingRoom::AllClearPrintLobbyTxtBox()
{
	for (int i = 0; i < 8; i++)
	{
		gride.DrawXY(50, 4 + i, "                    ");
	}
}
bool WaitingRoom::InputChatLog(string name, string chat)
{
	ChatLog.push_back(pair<string, string>(name, chat));
	return true;//예외처리 추가하셈
}
void WaitingRoom::PrintChatLogList()
{
	//7,28 부터 y값 --
	for (int i = 1; i <= 17; i++)
	{
		if (ChatLog.size() < i)break;
		gride.gotoxy(7, 27 - i);
		cout << ChatLog[ChatLog.size() - i].first << ": " << ChatLog[ChatLog.size() - i].second;
	}
}
void WaitingRoom::initChatListBox()
{
	for (int i = 0; i < 18; i++)
	{
		gride.DrawXY(7, 11 + i, "                                    ");
	}
}