#include <Windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <io.h>
#include <sys/stat.h>
#include <direct.h>

#pragma comment(lib,"ws2_32.lib")

//#define __DEBUG__CLIENT__MAIN
//#define __DEBUG__CLIENT__INITIP
//#define __DEBUG__CLIENT__INITRE
//#define __DEBUG__CLIENT__CONNECT
//#define __DEBUG__CLIENT__COMM
//#define __DEBUG__CLIENT__SEND

#define SOCKET_BUF 65535
#define FILE_BUF 60000000
#define RESEND_TIME_LIMIT 3
#define RECEIVE_WAIT_TIME 5.0
#define FILE_NAME_LEN 1000
#define SUFFIX_NAME_LEN 10
#define FAIL_FILE_NUM 100
#define IP_ADDRESS_LENGTH 20
#define USER_LENGTH 50
#define PORT_LENGTH 10
#define EML_NUM 2000

char ConstEmlPath[FILE_NAME_LEN] = "\\eml\\";
char ConstCnfPath[FILE_NAME_LEN] = "\\config.txt";
char ConstREPath[FILE_NAME_LEN] = "\\recveml.txt";
char ConstEmlSuffix[SUFFIX_NAME_LEN] = "*.eml";
char NowPath[FILE_NAME_LEN], NowFile[FILE_NAME_LEN];

char IpAddress[IP_ADDRESS_LENGTH];
char X_UUID[FILE_NAME_LEN];
int PORT;
char Username[USER_LENGTH];
char Password[USER_LENGTH];
char AppendPath[FILE_NAME_LEN];

char SendBuffer[FILE_BUF];

char RecvEml[EML_NUM][FILE_NAME_LEN];
int RecvEmlNum;

char FFPstr[FILE_NAME_LEN];

int SetCookieFlag;
char SessionId[SOCKET_BUF];

int StringCmp(const void *a, const void *b);
int FindFilePos(char *str, int &pos);
int InitIp();
int InitRecvEml();
int HttpString(char *FileName, char *HttpHead, char *SendBuffer, int &FileLen);
int SendSocket(SOCKET CientSocket, char *SendBuffer, clock_t &StartTime);
int JudegeRecvBuf(char *ReceiveBuffer);
int DoReceiveState(int state, char *ReceiveBuffer);
int Communication(char *FileName, SOCKET ClientSocket, char FailFileName[][FILE_NAME_LEN], int &FailTime);
int Connet(char *FileName, char FailFileName[][FILE_NAME_LEN], int &FailTime);
int SetCookie();
int GetCookie(SOCKET ClientSocket);
int CookieString();
int GetSessionId(char *ReceiveBuffer);

int GetSessionId(char *ReceiveBuffer){
	int i = 0, j = 0;
	int len = strlen(ReceiveBuffer);

	memset(SessionId, 0x00, sizeof SessionId);

	for(i = 0; i < len - 9; i ++){
		if((ReceiveBuffer[i] == 'S' || ReceiveBuffer[i] == 's')
			&& (ReceiveBuffer[i + 1] == 'E' || ReceiveBuffer[i + 1] == 'e')
			&& (ReceiveBuffer[i + 2] == 'S' || ReceiveBuffer[i + 2] == 's')
			&& (ReceiveBuffer[i + 3] == 'S' || ReceiveBuffer[i + 3] == 's')
			&& (ReceiveBuffer[i + 4] == 'I' || ReceiveBuffer[i + 4] == 'i')
			&& (ReceiveBuffer[i + 5] == 'O' || ReceiveBuffer[i + 5] == 'o')
			&& (ReceiveBuffer[i + 6] == 'N' || ReceiveBuffer[i + 6] == 'n')
			&& (ReceiveBuffer[i + 7] == 'I' || ReceiveBuffer[i + 7] == 'i')
			&& (ReceiveBuffer[i + 8] == 'D' || ReceiveBuffer[i + 8] == 'd')){
			for(int j = 0; i + j < len; j ++){
				SessionId[j] = ReceiveBuffer[i + j];
				if(ReceiveBuffer[i + j] == ';'){
					SessionId[j] = 0;
					break;
				}
			}
			break;
		}
	}
	return 1;
}

int CookieString(){
	char HttpHead[SOCKET_BUF];
	char FileLenString[FILE_NAME_LEN];
	int FileLen, HeadLen;

	memset(HttpHead, 0x00, sizeof HttpHead);
	memset(SendBuffer, 0x00, sizeof SendBuffer);
	memset(FileLenString, 0x00, sizeof FileLenString);
	FileLen = 0;
	HeadLen = 0;

	strcat(SendBuffer, "username=");
	strcat(SendBuffer, Username);
	strcat(SendBuffer, "&password=");
	strcat(SendBuffer, Password);
	FileLen = strlen(SendBuffer);

	sprintf(FileLenString, "%d", FileLen);

	strcat(HttpHead, "POST /login HTTP/1.1\r\n");
	strcat(HttpHead, "Host: ");
	strcat(HttpHead, IpAddress);
	strcat(HttpHead, "\r\n");
	strcat(HttpHead, "Accept: */*\r\n");
	strcat(HttpHead, "X-UUID: ");
	strcat(HttpHead, X_UUID);
	strcat(HttpHead, "\r\n");
	strcat(HttpHead, "Content-Type: application/x-www-form-urlencoded\r\n");
	strcat(HttpHead, "Content-Length: ");
	strcat(HttpHead, FileLenString);
	strcat(HttpHead, "\r\n");
	strcat(HttpHead, "\r\n");

	HeadLen = strlen(HttpHead);
	memmove(SendBuffer + HeadLen, SendBuffer, FileLen);
	memcpy(SendBuffer, HttpHead, HeadLen);

	return 0;
}

int GetCookie(SOCKET ClientSocket){
	char RecvBuffer[SOCKET_BUF];
	int ReceiveBufferIsEmpty = 1;
	int CookieStringRes = -1;
	int SendRes = -1, ReceiveRes = -1;
	int HttpState = 0;
	int SendLen = 0;

	unsigned long ul = 1;
	int Ret = 0;

	CookieStringRes = CookieString();
	if(CookieStringRes == -1){
		return -1;
	}

	SendLen = (int)strlen(SendBuffer);
	SendRes = send(ClientSocket, SendBuffer, SendLen, 0);
	if(SendRes == SOCKET_ERROR){
		return -1;
	}

	memset(RecvBuffer, 0x00, sizeof RecvBuffer);
	ReceiveBufferIsEmpty = true;
	
	while(true){
		ReceiveRes = recv(ClientSocket, RecvBuffer, SOCKET_BUF, 0);
		if(ReceiveRes == 0 || ReceiveRes == SOCKET_ERROR){
			break;
		}

		if((int)strlen(RecvBuffer) != 0){
			ReceiveBufferIsEmpty = 0;
		}

		if(ReceiveBufferIsEmpty == false){
			ReceiveRes = JudegeRecvBuf(RecvBuffer);
			if(ReceiveRes != -1){
				HttpState = ReceiveRes;
			}
		}
	}
	ReceiveRes = DoReceiveState(HttpState, RecvBuffer);
	return 1;
}

int SetCookie(){
	WSADATA  Ws;
	SOCKET ClientSocket;
	struct sockaddr_in ServerAddr;
	int AddrLen = 0;
	HANDLE hThread = NULL;
	int Ret = 0;
	
	if(WSAStartup(MAKEWORD(2,2), &Ws) != 0){
		return -1;
	}

	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(ClientSocket == INVALID_SOCKET){
		return -1;
	}

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = inet_addr(IpAddress);
	ServerAddr.sin_port = htons(PORT);
	memset(ServerAddr.sin_zero, 0x00, 8);

	Ret = connect(ClientSocket,(struct sockaddr*)&ServerAddr, sizeof(ServerAddr));
	if(Ret == SOCKET_ERROR){
		return -1;
	}

	Ret = GetCookie(ClientSocket);
	if(Ret == -1){
		return -1;
	}

	closesocket(ClientSocket);
	WSACleanup();
	return 0;
}

int StringCmp(const void *a, const void *b){
	int result = 0;
	result = strcmp((char *)a, (char *)b);
	return result;
}

int FindFilePos(char *file, int &pos){
	int result = 0;
	int i = 0;

	memset(FFPstr, 0x00, sizeof FFPstr);
	strcpy(FFPstr, file);

	for(i = 0; i < strlen(FFPstr); i ++){
		if(FFPstr[i] >= 'A' && FFPstr[i] <= 'Z'){
			FFPstr[i] = FFPstr[i] - 'A' + 'a';
		}
	}

	for(int i = 0; i < RecvEmlNum; i ++){
		if(strcmp(RecvEml[pos], FFPstr) == 0){
			result = 1;
			break;
		}
	}

	/*
	for(;pos < RecvEmlNum; pos ++){
		if(strcmp(RecvEml[pos], FFPstr) == 0){
			result = 1;
			break;
		}
		else if(strcmp(RecvEml[pos], FFPstr) == -1){
			continue;
		}
		else{
			result = 0;
			break;
		}
	}
	*/

	return result;
}

int InitIp(){
	FILE * PFile = NULL;
	char TempPath[FILE_NAME_LEN];
	memset(TempPath, 0x00, sizeof TempPath);

	strcat(TempPath, NowPath);
	strcat(TempPath, ConstCnfPath);

#ifdef __DEBUG__CLIENT__INITIP
	printf("Now Path: %s\n", TempPath);
#endif

	PFile = fopen(TempPath, "r");
	if(PFile == NULL){
		return -1;
	}
	else{
		memset(IpAddress, 0x00, sizeof IpAddress);

		fscanf(PFile, "%s", IpAddress);
		fscanf(PFile, "%d", &PORT);
		fscanf(PFile, "%s", Username);
		fscanf(PFile, "%s", Password);
		fscanf(PFile, "%s", AppendPath);

#ifdef __DEBUG__CLIENT__INITIP
		printf("IP_ADDRESS: %s\n", IpAddress);
		printf("PORT: %d\n", PORT);
#endif

		fclose(PFile);
		return 0;
	}
}

int InitRecvEml(){
	FILE * PFile = NULL;
	char TempPath[FILE_NAME_LEN];
	int i;

	memset(TempPath, 0x00, sizeof TempPath);
	strcat(TempPath, NowPath);
	strcat(TempPath, ConstREPath);
	PFile = fopen(TempPath, "r");
	if(PFile == NULL){
		PFile = fopen(TempPath, "w");
		fclose(PFile);
	}
	else{
		RecvEmlNum = 0;
		while(fgets(RecvEml[RecvEmlNum], FILE_NAME_LEN, PFile)){
			for(i = 0; i < strlen(RecvEml[RecvEmlNum]); i ++){
				if(RecvEml[RecvEmlNum][i] >= 'A' && RecvEml[RecvEmlNum][i] <= 'Z'){
					RecvEml[RecvEmlNum][i] = RecvEml[RecvEmlNum][i] - 'A' + 'a';
				}
			}
			RecvEml[RecvEmlNum][i - 1] = 0;
			RecvEmlNum ++;
		}

		qsort(RecvEml, RecvEmlNum, sizeof RecvEml[0], StringCmp);

#ifdef __DEBUG__CLIENT__INITRE
		for(i = 0; i < RecvEmlNum; i ++){
			printf("%s", RecvEml[i]);
		}
#endif

	}
	return 0;
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

		strcat(HttpHead, "POST /email/path/");
		strcat(HttpHead, AppendPath);
		strcat(HttpHead, " HTTP/1.1\r\n");
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
		strcat(HttpHead, "Cookie: ");
		strcat(HttpHead, SessionId);
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

int DoReceiveState(int state, char *ReceiveBuffer){
	FILE *PFile;
	int Ret = -1;
	char TempPath[FILE_NAME_LEN];

	switch(state){
	case 200:
		break;
	case 201:
		memset(TempPath, 0x00, sizeof TempPath);
		strcat(TempPath, NowPath);
		strcat(TempPath, ConstREPath);
		PFile = fopen(TempPath, "a+");
		fprintf(PFile, "%s\n", NowFile);
		fclose(PFile);
		Ret = 1;
		break;
	case 302:
		GetSessionId(ReceiveBuffer);
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

			HttpRes = DoReceiveState(ReceiveRes, RecvBuffer);

			//---------------------------------------------------------------------
			/*
			ReceiveRes = recv(ClientSocket, RecvBuffer, SOCKET_BUF, 0);
			if(ReceiveRes == 0 || ReceiveRes == SOCKET_ERROR){
#ifdef __DEBUG__CLIENT__COMM
				printf("Communication:: SOCKET break\n");
#endif
				break;
			}
			*/

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

	char *TempPath;

	int FindPos = 0;

	memset(FailFileName, 0x00, sizeof FailFileName);
	memset(FileName, 0x00, sizeof FileName);
	memset(tempFindFileClass, 0x00, sizeof tempFindFileClass);
	memset(NowPath, 0x00, sizeof NowPath);
	memset(NowFile, 0x00, sizeof NowFile);
	
	TempPath = getcwd(NULL, 0);
	strcpy(NowPath, TempPath);
	free(TempPath);
	result = InitIp();
	if(result == -1){

#ifdef __DEBUG__CLIENT__MAIN
		printf("OPEN CONFIG FILE FAIL\n");
		system("pause");
#endif

		return -1;
	}

	result = InitRecvEml();
	if(result == -1){

#ifdef __DEBUG__CLIENT__MAIN
		printf("OPEN RECV EML FILE FAIL\n");
		system("pause");
#endif

		return -1;
	}

	strcat(tempFindFileClass, NowPath);
	strcat(tempFindFileClass, ConstEmlPath);
	strcat(tempFindFileClass, ConstEmlSuffix);

	FILE *PP;
	PP = fopen("copy.txt", "w");
	fclose(PP);

	SetCookieFlag = 0;

	if((fHandle = _findfirst(tempFindFileClass, &FindFile)) == -1L){

#ifdef __DEBUG__CLIENT__MAIN
		printf("当前目录下没有eml文件\n");
#endif

	}
	else{
		do{

			PP = fopen("copy.txt", "a+");
			strcpy(NowFile, FindFile.name);
			fprintf(PP, "%s\n", NowFile);
			fclose(PP);


#ifdef __DEBUG__CLIENT__MAIN
			//printf("找到文件:%s\n", FindFile.name);
			//system("pause");
#endif

			result = FindFilePos(FindFile.name, FindPos);
			if(result == 1){
				continue;
			}

			if(SetCookieFlag == 0){
				result = SetCookie();
				if(result == 0){
					SetCookieFlag = 1;
				}
				else{
					continue;
				}
			}

			memset(NowFile, 0x00, sizeof NowFile);
			strcpy(NowFile, FindFile.name);

			memset(FileName, 0x00, sizeof FileName);
			strcat(FileName, NowPath);
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