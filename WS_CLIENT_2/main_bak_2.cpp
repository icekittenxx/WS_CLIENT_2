#include <iostream>
#include <Windows.h>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <time.h>
#include <io.h>
#include <sys/stat.h>

#pragma comment(lib,"ws2_32.lib")

using namespace std;

#define __DEBUG__CLIENT__MAIN
#define __DEBUG__CLIENT__INITIP
#define __DEBUG__CLIENT__CONNECT

//#define  PORT 8000
//#define  IP_ADDRESS "192.168.1.100"

#define SOCKET_BUF 153600
#define RESEND_TIME_LIMIT 3
#define RECEIVE_WAIT_TIME 5.0
#define FILE_NAME_LEN 100
#define SUFFIX_NAME_LEN 10
#define FAIL_FILE_NUM 100
#define IP_ADDRESS_LENGTH 20
#define PORT_LENGTH 10

char ConstEmlPath[FILE_NAME_LEN] = "D:\\eml\\";
char ConstEmlSuffix[SUFFIX_NAME_LEN] = "*.eml";
char ConstCnfPath[FILE_NAME_LEN] = "D:\\config.txt";

char IpAddress[IP_ADDRESS_LENGTH];
int PORT;

int InitIp(){
	FILE * PFile = NULL;
	PFile = fopen(ConstCnfPath, "r");
	if(PFile == NULL){
		return -1;
	}
	else{
		memset(IpAddress, 0x00, sizeof IpAddress);

		fscanf(PFile, "%s", IpAddress);
		fscanf(PFile, "%d", &PORT);

#ifdef __DEBUG__CLIENT__INITIP
		printf("IP_ADDRESS: %s\n", IpAddress);
		printf("PORT: %d\n", PORT);
#endif

		fclose(PFile);
		return 0;
	}
}

int HTTP_string(char *FileName, char *SendBuffer){
	ifstream Fin(FileName);
	
	char InBuffer[SOCKET_BUF];
	char Tempstring[SOCKET_BUF];
	char Len[SOCKET_BUF];
	int StrLen = 0;

	memset(InBuffer, 0x00, sizeof InBuffer);
	memset(Tempstring, 0x00, sizeof Tempstring);
	memset(Len, 0x00, sizeof Len);

	while(Fin.getline(InBuffer, sizeof(InBuffer))){
		strcat(Tempstring, InBuffer);
	}

	StrLen = strlen(Tempstring);
	sprintf(Len, "%d", StrLen);

	strcat(SendBuffer, "POST /email HTTP/1.1\r\n");
	strcat(SendBuffer, "Host: ");
	strcat(SendBuffer, IpAddress);
	strcat(SendBuffer, "\r\n");
	strcat(SendBuffer, "Accept: */*\r\n");
	strcat(SendBuffer, "Content-Length: ");
	strcat(SendBuffer, Len);
	strcat(SendBuffer, "\r\n");
	//strcat(SendBuffer, "Content-Type: aplication/json\r\n");
	strcat(SendBuffer, "\r\n");
	strcat(SendBuffer, Tempstring);

	Fin.close();
	return 0;
}

int JudegeRecvBuf(char *ReceiveBuffer){
	int Result = 0;
	int HttpState = 0;
	if(ReceiveBuffer[0] == 'H' && ReceiveBuffer[1] == 'T'
		&& ReceiveBuffer[2] == 'T' && ReceiveBuffer[3] == 'P'){
		HttpState = (ReceiveBuffer[9] - '0') * 100 + (ReceiveBuffer[10] - '0') * 10
			+ (ReceiveBuffer[11] - '0');
		if(HttpState != 201){
			Result = -1;
		}
	}
	return Result;
}

int SendSocket(SOCKET CientSocket, char *SendBuffer, clock_t &StartTime){
	int SendRes = 0;
	SendRes = send(CientSocket, SendBuffer, (int)strlen(SendBuffer), 0);

	if(SendRes == SOCKET_ERROR){
		cout<<"Send Info Error::"<<GetLastError()<<endl;
		//system("pause");
		return -1;
	}

	StartTime = clock();
	return SendRes;
}

int Connet(char *FileName, char FailFileName[][FILE_NAME_LEN], int &FailTime){
	strcpy(IpAddress, "127.0.0.1");

	WSADATA  Ws;
	SOCKET CientSocket;
	struct sockaddr_in ServerAddr;
	int AddrLen = 0;
	HANDLE hThread = NULL;

	char SendBuffer[SOCKET_BUF];
	char RecvBuffer[SOCKET_BUF];
	int HttpStringRes = 0, Ret = 0, SendRes = 0, ReceiveRes = 0;
	clock_t StartTime, CurrentTime;
	int ReSendTime = 0;

	//Init Windows Socket
	if(WSAStartup(MAKEWORD(2,2), &Ws) != 0){

#ifdef __DEBUG__CLIENT__CONNECT
		printf("Init Windows Socket Failed:: %d\n", (int)GetLastError());
		system("pause");
#endif

		return -1;
	}

	//Create Socket
	CientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(CientSocket == INVALID_SOCKET){

#ifdef __DEBUG__CLIENT__CONNECT
		printf("Create Socket Failed:: %d\n", (int)GetLastError());
		system("pause");
#endif

		return -1;
	}

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = inet_addr(IpAddress);
	ServerAddr.sin_port = htons(PORT);
	memset(ServerAddr.sin_zero, 0x00, 8);

	Ret = connect(CientSocket,(struct sockaddr*)&ServerAddr, sizeof(ServerAddr));
	if(Ret == SOCKET_ERROR){

#ifdef __DEBUG__CLIENT__CONNECT
		printf("Connect Error:: %d\n", GetLastError());
		system("pause");
#endif

		return -1;
	}
	else{

#ifdef __DEBUG__CLIENT__CONNECT
		printf("Connect Successfully!\n");
#endif

	}

	//Send message
	memset(SendBuffer, 0x00, sizeof SendBuffer);
	HttpStringRes = HTTP_string(FileName, SendBuffer);

	printf("%s\n", SendBuffer);

	SendRes = SendSocket(CientSocket, SendBuffer, StartTime);
	if(SendRes == -1){
		return -1;
	}

	//receive message
	memset(RecvBuffer, 0x00, sizeof RecvBuffer);

	bool ReceiveBufferIsEmpty = true;
	ReSendTime = 0;
	
	while(true){
		if(ReSendTime > RESEND_TIME_LIMIT){
			strcat(FailFileName[FailTime ++], FileName);
			return -1;
		}

		CurrentTime = clock();
		if(((double)(CurrentTime - StartTime)/CLOCKS_PER_SEC > RECEIVE_WAIT_TIME) && ReceiveBufferIsEmpty == true){
			//resend message
			SendRes = SendSocket(CientSocket, SendBuffer, StartTime);
			if(SendRes == -1){
				return -1;
			}
			
			ReSendTime ++;
		}
		
		ReceiveRes = recv(CientSocket, RecvBuffer, SOCKET_BUF, 0);
		if(ReceiveRes == 0 || ReceiveRes == SOCKET_ERROR){
			cout<<"SOCKET断开!"<<endl;
			system("pause");
			break;
		}
		if((int)strlen(RecvBuffer) != 0){
			ReceiveBufferIsEmpty = false;
		}
		cout<<"接收到客户信息为:"<<RecvBuffer<<endl;

		ReceiveRes = JudegeRecvBuf(RecvBuffer);
		if(ReceiveRes == -1){
			strcat(FailFileName[FailTime ++], FileName);
			//return -1;
		}
	}

	cout<<"发送成功"<<endl;
	//system("pause");
	closesocket(CientSocket);
	WSACleanup();
	return 0;
}

int main(){
	int result = 0;

	char FileName[FILE_NAME_LEN];
	char FailFileName[FAIL_FILE_NUM][FILE_NAME_LEN];
	char tempFindFileClass[FILE_NAME_LEN + SUFFIX_NAME_LEN];
	int FailTime = 0;

	struct _finddata_t FindFile;
	long fHandle;

	memset(FailFileName, 0x00, sizeof FailFileName);
	memset(FileName, 0x00, sizeof FileName);
	memset(tempFindFileClass, 0x00, sizeof tempFindFileClass);

	result = InitIp();
	if(result == -1){

#ifdef __DEBUG__CLIENT__MAIN
		printf("OPEN FILE FAIL\n");
		system("pause");
#endif

		return -1;
	}

	strcat(tempFindFileClass, ConstEmlPath);
	strcat(tempFindFileClass, ConstEmlSuffix);

	if((fHandle = _findfirst(tempFindFileClass, &FindFile)) == -1L){

#ifdef __DEBUG__CLIENT__MAIN
		printf("当前目录下没有eml文件\n");
#endif

	}
	else{
		do{

#ifdef __DEBUG__CLIENT__MAIN
			printf("找到文件:%s\n", FindFile.name);
#endif

			memset(FileName, 0x00, sizeof FileName);
			strcat(FileName, ConstEmlPath);
			strcat(FileName, FindFile.name);

#ifdef __DEBUG__CLIENT__MAIN
			printf("%s\n", FileName);
#endif

			result = Connet(FileName, FailFileName, FailTime);
		}while(_findnext(fHandle, &FindFile) == 0);
		_findclose(fHandle);
	}

#ifdef __DEBUG__CLIENT__MAIN
	for(int i = 0; i < FailTime; i ++){
		printf("%s\n", FileName[i]);
	}
#endif

#ifdef __DEBUG__CLIENT__MAIN
	system("pause");
#endif

	return 0;
}

/*
int getfilesize()
{
	int iresult;
	struct _stat buf;
	iresult = _stat("D:\\MediaID.bin",&buf);
	if(iresult == 0)
	{
		return buf.st_size;
	}
	return -1;
}

int main(){
	int result = getfilesize();
	cout << result << endl;
	system("pause");
	return 0;
}
*/