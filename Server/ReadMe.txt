========================================================================
    CONSOLE APPLICATION : Maze Project Server, Client
========================================================================
클라 상태 플래그 :  static StateFlag = 0 //로그인화면(0), 로비(1), 대기방(2), 게임(3)
할거

----------------------------
서버에서 클라로 보낼패킷을 판단하는 방식이 아닌
모든 패킷을 클라로 보내 클라가 판단하는 방식으로 만들어짐
문제점>> 접속 클라수에 비례해서 클라 받는 패킷량이 많아짐
서버 성능이 좋다면 서버에서 보낼패킷을 판단하는방법이 좋아보임

----------------------------
		=====세션관련=====
**** 클라 접속 ID 중복 처리 *****.....
**** 클라 접속 LOGOUT 처리 *****.....
****클라에서 ID 비번 입력 안하고 그냥 접속 종료할 경우 서버에서의 클라 카운트가 -1이 됨.....ok

**** user_tbl 에 접속 카운트도 추가하자!.....ok
**** 지금 컴플리션키가 sock인데 이거를 클라이언트데이터구조체(sock, clientAddr)로 바꿔보자 IP 출력을 위해서 필요하거든
	->이거 안되는거같으니까 accpet 부분에서 공유데이터의 clients에 push한다음 로그인 실패하면 list에서 삭제하는 식으로...해볼까?(좀 나중에하자)
**** ID에 특수문자 입력 금지!@#$%^&*()_+=-~`/.,<>?[]{}\|


1. 클라에서 serverConnect 완성하기.....ok
2. 클라에서 ID, PW 서버로 전송 후 Insert(완료후 클라로 완료패킷 전송).....ok
3. insert 구현후 ServerDB.h의 Check_Password() 함수 구현(실패시 클라로 실패패킷 전송....ex: id중복, 비번틀림 등).....ok
4. Lobby 구현
	(1)로비 틀 구현													.....ok
	(2)방 생성 프로세스 구현											.....ok
		(2.1) 대기방 구조 설계(DB설계는 어떻게???)						.....
		(2.2) [클라 to 서버] 방생성 요청 패킷 전송						.....ok
		(2.3) [서버] 방생성 패킷받으면(mode:CREATE_ROOM) 방생성 처리	.....ok
		(2.4) [서버 to 클라] (2.3)완료후 방생성 완료 패킷 전송			.....ok
	(3)방 인원수 0되면 자동으로 대기방 삭제해줘야함						.....ok
	(4)클라-대기방 리스트를 실시간으로 받아와 출력						.....ok
	(5)클라-대기방 입장 구현											.....ok
5. WaitingRoomClass 구현
	(1) 대기방 틀 구현												.....ok
	(2) 대기방 프로세스 설계
		(2.1) 대기방 Join 구현
		(2.2) 
		(2.?) 대기방 나가기 구현

6. PlayGameClass 구현

============================================================================================================

	0524 일단 지금 ServerClass의 SendWaitingRoomList함수에 문제가있음 .... iter++안해줘서 생긴 문제였음 `ㅅ`v
	
	지금 제일 중요한 문제가 recv가 곂친다(?) 라는 건데 어떻게 해결할지 생각좀 해보자
	이미 recv쓰레드(계속 날라오는?(비동기) 채팅, 채팅방리스트 패킷 받는애임)에서 이미 recv가 대기하고있는 상태인데....
	클라에서 방생성(CreateRoom) 요청 패킷을 send한다음 서버에서 날라오는 처리패킷을 recv해야하는데 위에 먼저 대기하고있는 recv쓰레드의 recv에서 이 처리패킷을 리시브하는게 문제임. 어떻게 해결해야 할까?생각생각
	일단 대처로 서버에서 요청패킷을 받은다음 처리패킷을 보내기전에 쓰레기패킷을 send해서 recv쓰레드의 recv함수가 받게하고 그 다음 send에 처리패킷을 보내게 해놨음.
	ㄴ 요청관련 함수에서는 send만 하고 (recv하지않음) recv스레드에서 관련 요청패킷 처리하도록 구현함(이벤트핸들 사용)

	또 다른 문제는 connectToServer클래스에서 생성한lobby 객체인데 지금 이 객체로만 방정보를 받고있어서 출력이 잘 안대고있음.
	이 부분 어케 바꿀지도 생각



	★★★★★★★지금 출력이 이상한 부분에 나오는 경우가 있는데 send recv랑 출력이랑 곂쳐서 이상하게 출력이되는거같다 cs를 만들거나(>>이건해봤는데 안댐) 화면초가회에 조금더 신경써보자
	ㄴSleep로 해결함



	내일0526은 그거 해야댄다 방생성 이후(WaitingRoom )구현

	로비 매인이 끝나도 로비메인의 스레드가 작동하는가 확인해보기
	ㄴ Recv스레드를 LoginMain에서 실행하던가 메인함수에서 실행한다음 스레드 데이터에 모든 클래스의 객체를 담는다.
	>> 클라 구조 변경 
		- Lobby 클래스에 있던 recv쓰레드를 별도로 만든 ThreadClass에 넣고 쓰레드 인자(ThreadData)에 다른 클래스들의 객체포인터+Socket를 담음

	0526 virtual 소멸자는 상속관계가 아닐떄는 쓸필요없음 
	ex) ~LoginMain 이 호출되면 LoginMain클래스와 상속관계가 아닌 LoginMainInfo, ConnectToServer의 소멸자도 같이호출됨 



	0528 
	>> 클라에 UserInfo Class를 활용하기위해 구조를 바꿈
		- 로그인, 로비, 대기방, 게임 클래스 객체(추가로 ThreadClass)에 UserInfo 객체의 포인터를 담음
	>> 방만들었을떄 방정보를 서버로부터 받고 userInfo에 저장하는 것 까지 해놨음
	이제 해야할 것은 CreateRoom 완료 패킷을 받으면(이미 받은 상태) WaitingRoomMain으로 넘어가는 것 구현 할 차례임.
	ㄴ Client.cpp 의 CREATE_ROOM 부분

	0529
	>> 이벤트핸들을 각 세션? 마다 만들어주었음 
		- 하나의 이벤트 핸들로 하는 것 과의 차이점이 뭘까?
	>> 방 나가기 구현 ..... ok
	>> Lobby -> 방 접속 구현(방만들기 아님)....ok

	**** Client 방에 접속중일떄는 방정보 패킷을 받아도 출력하지 않게 변경....ok
	**** Client 방 만든 다음 나갔다가 방 다시 만들면 방생성 안됨.......ok
	->변경된 방정보 저장&출력 할때 LobbyInfo의 LobbyListPointNum도 0으로 초기화 해주자 
	안그럼 방정보 변경되고나서 방향키 변경없이 엔터키를 입력하면 엉뚱한 방에 접속됨.

	내일할일
	>>채팅 구현 전에 방에 접속중인 사람 리스트 출력하기
		- Join으로 유저가 방에 입장하거나 나갈때 방정보를 어케 받고 지우고 할건지 생각....
	>> 채팅보내고 받는 부분 구현
		- 채팅전송 -> 서버에서 저장 -> 완료패킷(채팅전송한 클라에만)전송 -> 서버는 완료패킷 전송후 해당 채팅을 모든 클라에 뿌림

	0530
	***** 클라 종료시 서버에서 종료처리가 제대로 안되는 현상이 있음. ......일단? 해결(0531)
	

	******	@U~~~패킷 받는 부분에있는 함수들(대기방유저리스트 출력용) 이 호출되면 이상하게 방 만들고 나갔다가 다시 만들때 방 만들기가 실패한다 이유는...?
			-  [클라]의 RoomState 관련해서 문제있었음. changeRoomState() -> setRoomState(bool)로 변경 ..... ok

	0531
	**** 대기방의 유저리스트가 제대로 출력안대는 현상
		- server에서 방유저 정보가 바뀔 때 소캣배열도 바꿔줘야하는데 ID배열만 바꿔서 생긴 문제였음.......ok
		
	>> recv쓰레드의 recv함수 전에 버퍼 초기화 해줌 (recv버퍼가 곂치는 문제가 있었음)

	>>채팅 구현
		- server로 패킷"/[방번호]_[보낸놈ID]_[내용]" 보내는거 까지 해놨음


	0601 
	>> 대기방 채팅구현 어느정도 완료했지만 몇가지 문제점이 있음
		- 채팅 후 방 나갔을때 채팅 로그가 초기화 안되서 다른방에 입장 후 채팅을 하면 이전 방에서 한 채팅 내용이 출력됨.
		- 채팅 로그 저장방식을 어떻게 할지 아직 생각으 안해놨음.
			-> ChatRoom DB를 아직 만들지도 않았음. 어떠케 만들까?
			-> 서버에서 ChatRoom리스트 마다 채팅 로그를 저장하는변수를 만들고 해당 채팅방이 없어질 때 DB에 저장하는 방식
			
	************** 아무 버튼이나 누르세요 메시지 추가해서 넘어가도록


	0602

	>> winCount부터 나누자 .......ok

	>> 그 다음 GamePlay 에서 UserState를 이용해 동기화

	>> 대기방에 들어와 있는 어떠한 유저가 나갈 경우 user데이타도 변경 해주어야함


	>> 게임 시작 동기화
			방장이 $R_방번호 >> 게임시작요청 보냄
			$R_[방번호]관련 패킷을 받은 서버는 해당 내용을 모든 클라에 Send함
			클라의 Recv엥서 $R_[방번호] 를 받으면 자신의 방 번호와 비교하여 GamePlayMain으로 넘어온다.
			GamePlayMain 넘어오면 자신이 게임 시작 준비가 되었다는 상태를 서버에 전송한다. >> $S방번호_ID
			서버는 $S방번호_ID를 받으면 해당 방을 찾고 그 방에있는 클라들에게 $1_준비된ID 를 send함
						

	>> 대기방 상태 ----> 게임중 상태 구분할수있는 플래그 기능 추가 (서버 클라 둘다)
	>> 게임중이면 방에 들어올수 없게....서버의JoinRoom 


	0605
	
	>> 서버 SetStartRoom() 함수 작동 오류있음. splice 문제일것 같음



	순서 게임좌표출력 동기화 - 종료 동기화

	좌표출력 동기화 순서
		1. 클라 이동키 서버로 전송 "P방_유저키_방향키"											.....OK
		2. 서버는 게임방 탐색 후 해당 방(GamePlaylist)에 있는 유저한테만 "P유저키_방향키" 전송		.....OK
		3. 받은 유저키와 방향키를 판별하여 해당하는 유저표시를 이동시킨다.							.....OK
	자동이동->테스트전용 만들기
	
	종료 동기화
	>>게임 끝날때 wData의 Userstate도 초기화
	>> EndUserNum Recv스레드에서 얘가 3일때 종료 되는거로 되있음 지금 연결중인 유저로 바꾸는게 좋을듯.

	*****게임 시작할때 방장 클라이언트에서 크레쉬가 나는 현상이있는데 원인은 아직 모름


	====== 코드 정리 ======
	1. 클라부분
	grideXY
	UserInfo
	RecvThreadClass
	ConnectToServer
	LoginMain
	LoginMainInfo
	Lobby
	LobbyInfo
	WaitingRoom
	WaitingRoomInfo
	GamePlayClass
	GamePlayInfo

	2. 서버부분
	ServerDB
	ServerClass
	LogClass >> 아직 안씀


	****room DB 관련 문제
		- DeleteWaitingRoom() 구현 하기 전에 room_id 와 room_num 의 무결성? 위반 어케 바꿔야될지 생각.....room_id 연동 시켰음 room_tbl의 room_num 지우면 됨
		- 한글 입력관련(DB에 한글이 안드가네 뭐가 문제지)....그냥 다 영어로 바꾸겠음. >> DB문제는 아님 mysql_query()함수가 한글이 안대는듯 이 함수에 입력하기전까진 분명히 한글이 깨기지않음.
		- 


	0608


	***** GetTotalCreateRoomCount() 의 query 부분에서 max를 사용하면 user_tbl에 값이 없을때 프로그렘 실행시 에러남 >> 이 경우를 피하려면 count 쿼리를 쓰면 된다.

	***** room_tbl 의 auto_increment를 1로 초기화를 해야되는데 안대네?
		- "ALTER TABLE uer_tbl AUTO_INCREMENT = 1;" 이 쿼리를 실행해도 다음에 insert 할때 최대값으로 들어감

	>> 게임종료시 end처리만 해주면됨
		- sDB.EndPlayGame(gameNum)
		- (update) game_tbl.state ---> 'END' 로 변경
		- (update) 종료 시간 추가
		- 일단 위에껀 해놨음 (DB쪽) 이제 클라에서 종료 패킷 보내고 서버에서 받으면 ServerClass의 DeleteStartRoom()함수를 호출하면됨

	0609

	>> 게임이 시작될때 특정 클라에서 키를 누르면 게임이 종료가 되는 현상이있었음.
		- 방장이 아닌 다른 클라에서 WRinfo의 WaitingRoomTxtNum이 2인상태로 키를 누르기 때문에 종료요청을 보내게됨.
		- 떄문에 Recv쓰레드에서 게임요청 패킷을 서버로 부터 받으면 tData.wRoom->initWaitingRoom(); 를 해주고 초기화함. 
	>> 일단 게임종료시 디비에 END써주는거까지 해놨음. 이제 wincount, playcount 만 하면댐

	0612
	***** t13일 새벽 1시에 입력시간이 12일 15시 .... 이라고 입력됨 왜그럴까 --> getsystemtime() 는 그리니치 기준시 출력임 getlocaltime()를 써야함

	병행성....공부하자.....


	Send도 넌블락처리 해보자...

	컴플리션키 포인터로 바꾸고 IP주소 ID등 참조할수있게...

	1217
	ServerClass의 private static member 함수들을 -> public 맴버 함수로 변경
	(공유 구조체에 ServerClass* thisServerClass 추가, ServerClass 생성자에서 this 포인터 연결)
		--> ServerClass의 동작 함수들을 private으로 감추고 싶은데 좋은 방법을 생각해보자.
	DB객체를 static으로 두는게 맞는지 생각해보기 


	왜 이렇게 써논거지????????
	if (ConnectionCheck())
	{
		query_stat = mysql_query(connection, query.c_str());
	}
	else false;


	방번호 관련해서 DB 수정 다시해야됨!!!!.


	ALTER TABLE t2 ADD d TIMESTAMP; 로 수정하자
	----> 시간 따로 구할 필요없이 insert 할떄마다 자동으로 시간 찍힘
	createTime 은 타임스템프로 deleteTime는 시간 따로 구해서 대입

	**테이블 수정
	- room_tbl; 
	modify create_time timestamp;

	- user_connection_log_tbl; 
	modify time timestamp;
	add type char(20);

	- game_tbl;
	add state char(20)
