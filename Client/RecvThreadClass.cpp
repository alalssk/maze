#include "RecvThreadClass.h"

bool RecvThreadClass::threadOn = false;//임시
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
void RecvThreadClass::setUserInfo(UserInfo* input_user)
{
	tData.user = input_user;
}
unsigned WINAPI RecvThreadClass::RecvMsg(void * arg)   // read thread main
{
	ThreadData tData = *((ThreadData*)arg);
	char recvMsg[BUF_SIZE] = "";
	char recvID[13] = "";
	char *tmp_tok;
	int itmp_RoomNum;
	int strLen;
	//ClientMode >= 1(lobby상태) 일때 넘어가도록 이벤트 처리 WaitSingleObject(hEvent,INFINITE);
	//그럼 로그아웃할때 스레드도 종료시켜줘야겠지

	while (!ExitFlag && !LogoutFlag)
	{
		strLen = recv(tData.sock, recvMsg, BUF_SIZE - 1, 0);
		//이 부분에 send스레드에서 종료플레그가 켜지면 이벤트 처리를....
		//방법2. 소캣을 넌블로킹으로 만들기 (ioctlsocket)
		if (strLen == -1)
			return -1;

		if (recvMsg[0] == '!' && tData.user->IsCurrentClientMode(GameState::LOBBY))
		{
			//방정보 받아옴==> !"방개수"_"No.[방번호]>> [방이름]"_...
			//!0 이면 방이없다는 말임
			//일단 서버에서 ! 요고만 보내고 방정보 함수가 제대로 동작하는지 체크하자
			if (recvMsg[1] == '0')tData.lobby->GetWaitingRoomList("0");
			else tData.lobby->GetWaitingRoomList(recvMsg + 1);

			memset(recvMsg, 0, sizeof(recvMsg));
		}
		else if (recvMsg[0] == '/')
		{	//채팅메시지 전용
			//방번호는 굳이 받을필요없음 방에있는애들한테만 메시지를 보내니까
			//"/1_[보낸놈ID]_[내용]"
			tmp_tok = strtok(recvMsg, "_");
			tmp_tok = strtok(NULL, "_");
			strcpy(recvID, tmp_tok);
			tmp_tok = strtok(NULL, "_");
			tData.wRoom->InputChatLog(recvID, tmp_tok);
			tData.wRoom->initChatListBox();//채팅창 초기화
			tData.wRoom->PrintChatLogList();//출력

		}
		else if (recvMsg[0] == '$')
		{//============ 방장이 서버로 보낸 "$R_방번호" 고대로 받음 >> 이건 모든 클라에 뿌리는 패킷임
			if (recvMsg[1] == 'R')
			{
				if (tData.user->wData.RoomNum == atoi(recvMsg + 3))// "$R_방번호" 이 대기방 방에 접속한 애들은 게임플레이를 준비하라
				{
					tData.user->setClientMode(GameState::GAMEPLAY);
					tData.wRoom->initWaitingRoom();
					// *** 이 초기화를 안해주면 게임 시작할때 방장이 아닌 다른 클라에서 WRinfo의 WaitingRoomTxtNum이 2인상태로 키를 누르기 때문에 종료요청을 보내게됨.
					tData.wRoom->PrintStartGameMsg();
				}
				
			}
			else if (recvMsg[1] == 'S' && 
				tData.user->IsCurrentClientMode(GameState::GAMEPLAY))// $S관련 패킷은 클라모드가 GamePlay(3)인 상태에만 처리한다.
			{//$S1_[ID] 해당 아이디 게임준비상태
				if (recvMsg[2] == '1')
				{
					for (int i = 0; i < tData.user->wData.ConnectUserNum; i++)
					{
						if (strcmp(tData.user->wData.UserName[i], recvMsg + 4) == 0)
						{
							tData.user->wData.UserState[i] = true;
						}
					}
				}

			}
		}
		else if (recvMsg[0] == '@')	//리퀘스트(req) 방만들기, 종료(로그아웃)요청완료 등 메시지 받는곳
		{							//방생성 실패 성공(@R0, @R1), 종료요청완료(@E1), 로그아웃
			if (recvMsg[1] == 'R')
			{
				if (recvMsg[2] == '1' && tData.user->IsCurrentClientMode(GameState::LOBBY))
				{
					////////////////
					//만들었으니까 방정보를 받아와야겠지
					//@R1_[방번호]_[방제목]
					//받는거 확인
					if (!tData.user->getRoomState())//방이없는경우(RoomState==false)
					{
						tData.user->setWaitingRoomData(recvMsg + 4); //[방번호]_[방제목]
						tData.user->setClientMode(GameState::WAIT_ROOM);
						SetEvent(tData.lobby->hLobbyEventForRequest);
					}
					else
					{
						cout << "[방생성 실패] 이미 방에 접속중입니다" << endl;
					}

				}
			}
			else if (recvMsg[1] == 'r')//방요청(시작시)
			{
				if (recvMsg[2] == '1')
				{

					SetEvent(tData.lobby->hLobbyEventForRequest);
				}
			}
			else if (recvMsg[1] == 'L')//로그아웃요청완료(@L1)
			{
				if (recvMsg[2] == '1')
				{
					SetEvent(tData.lobby->hLobbyEventForRequest);
					LogoutFlag = true;
				}
			}
			else if (recvMsg[1] == 'G')//종료요청완료(@G1)
			{
				if (recvMsg[2] == '1')
				{
					SetEvent(tData.lobby->hLobbyEventForRequest);
					ExitFlag = true;
				}
			}
			else if (recvMsg[1] == 'E')//방나가기요청 완료
			{
				if (recvMsg[2] == '1')
				{
					if (tData.user->IsCurrentClientMode(GameState::WAIT_ROOM))//대기방상태일때만
					{
						tData.user->setClientMode(GameState::LOBBY);
						SetEvent(tData.wRoom->hWaitingRoomEventForRequest);
					}
				}
			}
			else if (recvMsg[1] == 'J')//방입장요청성공
			{
				if (recvMsg[2] == '1')
				{
					if (tData.user->IsCurrentClientMode(GameState::LOBBY))//로비상태일떄만
					{
						tData.user->setWaitingRoomData(recvMsg + 4);
						tData.user->setClientMode(GameState::WAIT_ROOM);
						SetEvent(tData.lobby->hLobbyEventForRequest);
					}
				}
			}
			else if (recvMsg[1] == 'U')
			{//@U1_[이름]-승수_[이름-승수]
				if (recvMsg[2] == '1' && tData.user->IsCurrentClientMode(GameState::WAIT_ROOM))
				{
					tData.user->setWaitingRoomUserList(recvMsg + 4);
					tData.user->setRoomUserKey();//RoomUserKey 세팅
					tData.wRoom->PrintUserList();
				}
			}
		}
		else if (recvMsg[0] == 'P' && tData.user->IsCurrentClientMode(GameState::GAMEPLAY))
		{//P유저키_방향키
			tData.gPlay->RecvPlayerPosition(recvMsg + 1);
		}
		else if (recvMsg[0] == 'Q' && tData.user->IsCurrentClientMode(GameState::GAMEPLAY))
		{//"Q유저키" 이게 세번(클라당 한번씩) 오면 게임 종료
			int iUserKey;
			iUserKey = atoi(recvMsg + 1);
			tData.user->wData.Rating[iUserKey-1] = ++tData.user->wData.EndUserNum;
			if (tData.user->wData.EndUserNum == tData.user->wData.ConnectUserNum)
			{
				tData.user->setClientMode(GameState::WAIT_ROOM);
				

			}
		}
		memset(recvMsg, 0, sizeof(recvMsg));

	}
	cout << "exitRecvMsgThread" << endl;
	return 0;
}