#include <iostream>
#include <Windows.h>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <time.h>
#include <io.h>
#include <sys/stat.h>

#pragma comment(lib,"ws2_32.lib")

using namespace std;

#define __DEBUG__CLIENT__MAIN
#define __DEBUG__CLIENT__INITIP
#define __DEBUG__CLIENT__CONNECT
#define __DEBUG__CLIENT__COMM
#define __DEBUG__CLIENT__SEND

//#define SOCKET_BUF 153600
#define SOCKET_BUF 65535
#define FILE_BUF 60000000
#define RESEND_TIME_LIMIT 3
#define RECEIVE_WAIT_TIME 5.0
#define FILE_NAME_LEN 100
#define SUFFIX_NAME_LEN 10
#define FAIL_FILE_NUM 100
#define IP_ADDRESS_LENGTH 20
#define PORT_LENGTH 10

//char ConstEmlPath[FILE_NAME_LEN] = "D:\\eml\\";
//char ConstCnfPath[FILE_NAME_LEN] = "D:\\config.txt";
char ConstEmlPath[FILE_NAME_LEN] = "\\eml\\";
char ConstCnfPath[FILE_NAME_LEN] = "\\config.txt";
char ConstEmlSuffix[SUFFIX_NAME_LEN] = "*.eml";

char IpAddress[IP_ADDRESS_LENGTH];
char X_UUID[FILE_NAME_LEN];
int PORT;

char SendBuffer[FILE_BUF];

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

int GetFileLen(char *FileName){
	int iresult;
	struct _stat buf;
	iresult = _stat(FileName, &buf);
	if(iresult == 0){
		return buf.st_size;
	}
	return -1;
}

int HttpString(char *FileName, char *HttpHead, char *SendBuffer, int &FileLen){
	FILE * PFile = NULL;
	char FileLenString[FILE_NAME_LEN];
	char InBuffer[SOCKET_BUF];
	int HeadLen = 0;

	memset(HttpHead, 0x00, sizeof HttpHead);
	memset(FileLenString, 0x00, sizeof FileLenString);
	memset(InBuffer, 0x00, sizeof InBuffer);
	memset(SendBuffer, 0x00, sizeof SendBuffer);
	FileLen = 0;

	PFile = fopen(FileName, "r");
	if(PFile == NULL){
		return -1;
	}
	else{

		while(fgets(InBuffer, SOCKET_BUF, PFile)){
			strcat(SendBuffer, InBuffer);
		}
		FileLen = strlen(SendBuffer);

		sprintf(FileLenString, "%d", FileLen);

		strcat(HttpHead, "POST /email HTTP/1.1\r\n");
		strcat(HttpHead, "Host: ");
		strcat(HttpHead, IpAddress);
		strcat(HttpHead, "\r\n");
		strcat(HttpHead, "Accept: */*\r\n");
		strcat(HttpHead, "X-UUID: ");
		strcat(HttpHead, X_UUID);
		strcat(HttpHead, "\r\n");
		strcat(HttpHead, "Content-Length: ");
		strcat(HttpHead, FileLenString);
		strcat(HttpHead, "\r\n");
		strcat(HttpHead, "\r\n");

		HeadLen = strlen(HttpHead);
		memmove(SendBuffer + HeadLen, SendBuffer, FileLen);
		memcpy(SendBuffer, HttpHead, HeadLen);

		fclose(PFile);

		return 0;
	}
	return 0;
}

int SendSocket(SOCKET CientSocket, char *SendBuffer, clock_t &StartTime){
	int SendRes = 0;
	int SendLen = 0;

	SendLen = (int)strlen(SendBuffer);
	SendRes = send(CientSocket, SendBuffer, SendLen, 0);
	if(SendRes == SOCKET_ERROR){

#ifdef __DEBUG__CLIENT__SEND
		printf("Send Info Error:: %d\n", GetLastError());
		system("pause");
#endif

		return -1;
	}

	StartTime = clock();

	return SendRes;

	/*
	PFile = fopen(FileName, "r");
	if(PFile == NULL){
		return -1;
	}
	else{

		
		HttpHeadStringLen = strlen(HttpHead);
		if(!feof(PFile)){
			//fgets(SendBuffer, SOCKET_BUF - HttpHeadStringLen, PFile);
			if(FileLen <= SOCKET_BUF - HttpHeadStringLen){
				fread(SendBuffer + HttpHeadStringLen, 1, FileLen, PFile);
				SendLen = FileLen + HttpHeadStringLen;
				SendRes = send(CientSocket, SendBuffer, SendLen, 0);

				FileLen = 0;
			}
			else{
				fread(SendBuffer + HttpHeadStringLen, 1, SOCKET_BUF - HttpHeadStringLen, PFile);
				SendLen = SOCKET_BUF;
				SendRes = send(CientSocket, SendBuffer, SendLen, 0);

				FileLen -= (SOCKET_BUF - HttpHeadStringLen);
			}

#ifdef __DEBUG__CLIENT__SEND
				printf("%s", SendBuffer);
#endif

			//SendRes = send(CientSocket, SendBuffer, (int)strlen(SendBuffer), 0);
			if(SendRes == SOCKET_ERROR){

#ifdef __DEBUG__CLIENT__SEND
				printf("Send Info Error:: %d\n", GetLastError());
#endif
				return -1;
			}

			while(!feof(PFile)){
				memset(SendBuffer, 0x00, sizeof SendBuffer);
				if(FileLen <= SOCKET_BUF){
					FileLen = 44832;

					fread(SendBuffer, 1, FileLen, PFile);
					SendLen = FileLen;
					SendRes = send(CientSocket, SendBuffer, SendLen, 0);

#ifdef __DEBUG__CLIENT__SEND
					printf("%s", SendBuffer);
#endif
					if(SendRes == SOCKET_ERROR){

#ifdef __DEBUG__CLIENT__SEND
						printf("Send Info Error:: %d\n", GetLastError());
						system("pause");
#endif

						return -1;
					}
					
					FileLen = 0;
					break;
				}
				else{
					fread(SendBuffer, 1, SOCKET_BUF, PFile);
					SendLen = SOCKET_BUF;
					SendRes = send(CientSocket, SendBuffer, SendLen, 0);

#ifdef __DEBUG__CLIENT__SEND
					printf("%s", SendBuffer);
#endif

					if(SendRes == SOCKET_ERROR){

#ifdef __DEBUG__CLIENT__SEND
						printf("Send Info Error:: %d\n", GetLastError());
						system("pause");
#endif

						return -1;
					}

					FileLen -= SOCKET_BUF;
				}
			}
		}
		else{
			return -1;
		}
		
		fclose(PFile);

		StartTime = clock();

		return SendRes;
	}
	*/
}

int JudegeRecvBuf(char *ReceiveBuffer){
	int Result = -1;
	int HttpState = 0;
	if(ReceiveBuffer[0] == 'H' && ReceiveBuffer[1] == 'T'
		&& ReceiveBuffer[2] == 'T' && ReceiveBuffer[3] == 'P'){

		HttpState = (ReceiveBuffer[9] - '0') * 100 + (ReceiveBuffer[10] - '0') * 10
			+ (ReceiveBuffer[11] - '0');

		Result = HttpState;
	}
	return Result;
}

int DoReceiveState(int state){
	int Ret = -1;

	switch(state){
	case 201:
		Ret = 1;
		break;
	case 400:
		
		break;
	case 409:

		break;
	case 500:

		break;
	}
	return Ret;
}

int Communication(char *FileName, SOCKET ClientSocket, char FailFileName[][FILE_NAME_LEN], int &FailTime){
	char RecvBuffer[SOCKET_BUF];
	char HttpHead[SOCKET_BUF];
	int HttpStringRes = 0, SendRes = 0, ReceiveRes = 0, HttpRes = 0;
	clock_t StartTime, CurrentTime;
	int ReSendTime = 0;
	int FileLen = 0;
	int ReceiveBufferIsEmpty = 1;

	unsigned long ul = 1;
	int Ret = 0;

	HttpStringRes = HttpString(FileName, HttpHead, SendBuffer, FileLen);
	if(HttpStringRes == -1){

#ifdef __DEBUG__CLIENT__COMM
		printf("Communication Error:: HttpHead\n");
#endif

		return -1;
	}
#ifdef __DEBUG__CLIENT__COMM
	else{
		printf("%s\n", SendBuffer);
	}
#endif

	SendRes = SendSocket(ClientSocket, SendBuffer, StartTime);
	if(SendRes == -1){

#ifdef __DEBUG__CLIENT__COMM
		printf("Communication Error:: SendSocket\n");
#endif

		return -1;
	}

	//receive message
	memset(RecvBuffer, 0x00, sizeof RecvBuffer);

	ReceiveBufferIsEmpty = true;
	ReSendTime = 0;
	
	//while(true){
	for(int i = 0; i < RECEIVE_WAIT_TIME; i ++){
		if(ReSendTime > RESEND_TIME_LIMIT){
			strcat(FailFileName[FailTime ++], FileName);

#ifdef __DEBUG__CLIENT__COMM
			printf("Communication Error:: RESEND TIME LIMIT\n");
#endif
			return -1;
		}

		ReceiveRes = recv(ClientSocket, RecvBuffer, SOCKET_BUF, 0);
		if(ReceiveRes == 0 || ReceiveRes == SOCKET_ERROR){

#ifdef __DEBUG__CLIENT__COMM
			printf("Communication:: SOCKET break\n");
#endif

			//break;
			continue;
		}

		if((int)strlen(RecvBuffer) != 0){
			ReceiveBufferIsEmpty = 0;

#ifdef __DEBUG__CLIENT__COMM
			printf("Communication:: %s\n", RecvBuffer);
#endif
		}
		

		if(ReceiveBufferIsEmpty == false){
			ReceiveRes = JudegeRecvBuf(RecvBuffer);

			/*
			ReceiveRes = recv(ClientSocket, RecvBuffer, SOCKET_BUF, 0);
#ifdef __DEBUG__CLIENT__COMM
			printf("Communication:: %s\n", RecvBuffer);
#endif
			*/

			HttpRes = DoReceiveState(ReceiveRes);
			break;
		}

#ifdef __DEBUG__CLIENT__COMM
		printf("Communication Receive Result:: %d\n", ReceiveRes);
#endif

	}

	return 0;
}

int Connet(char *FileName, char FailFileName[][FILE_NAME_LEN], int &FailTime){
	WSADATA  Ws;
	SOCKET ClientSocket;
	struct sockaddr_in ServerAddr;
	int AddrLen = 0;
	HANDLE hThread = NULL;

	int Ret = 0;

	//Init Windows Socket
	if(WSAStartup(MAKEWORD(2,2), &Ws) != 0){

#ifdef __DEBUG__CLIENT__CONNECT
		printf("Init Windows Socket Failed:: %d\n", (int)GetLastError());
		system("pause");
#endif

		return -1;
	}

	//Create Socket
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(ClientSocket == INVALID_SOCKET){

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

	Ret = connect(ClientSocket,(struct sockaddr*)&ServerAddr, sizeof(ServerAddr));
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
	Ret = Communication(FileName, ClientSocket, FailFileName, FailTime);
	if(Ret == -1){

#ifdef __DEBUG__CLIENT__CONNECT
		printf("Communication Error\n");
#endif
		return -1;
	}

	closesocket(ClientSocket);
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

	//strcpy(IpAddress, "127.0.0.1");
	//PORT = 8008;

	if((fHandle = _findfirst(tempFindFileClass, &FindFile)) == -1L){

#ifdef __DEBUG__CLIENT__MAIN
		printf("当前目录下没有eml文件\n");
#endif

	}
	else{
		do{

#ifdef __DEBUG__CLIENT__MAIN
			printf("找到文件:%s\n", FindFile.name);
			system("pause");
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