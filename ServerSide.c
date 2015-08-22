// downloads:
//http://www.upf.co.il/file/955018246.html
//http://www.f2h.co.il/tp3vh0cfm3m

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <dirent.h>

#define SIZE 4000
#define PASSWORD "1"

const char* FILE_NAME = "output.txt";

int sendData(int s, char* buffer);
int recieveData(int s, char* buffer);
int getCode(char* str, int length);
int getSizeOfFile(FILE* file);
void getCommandOutput(char* dataBuffer, char* command);
void getFilesInAFolder(char* dataBuffer, char* path);
void checkForErrors(int s, char* buffer, int code);
void cleanBuffer(char* buffer);
void buildMenu(char* str);
void closeConnection(int s);

int main(void)
{
	WSADATA info;
	struct sockaddr_in serverService, clientService;
	int err, s, new_socket, cResult, sizeStruct, sendResult, recvResult, code;
	char strSend[SIZE] = { '\0' };
	char strRecv[SIZE] = { '\0' };
	char dataBuffer[SIZE - 6] = { '\0' };

	// Configuration of the socket type
	err = WSAStartup(MAKEWORD(1, 1), &info);

	if (err != 0)
	{
		printf("WSAStartup failed, ERROR: %d\n", err);
		return 1;
	}

	printf("WSA Initialized\n");

	// Creating the socket
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (s == INVALID_SOCKET)
		printf("Failed at creating the socket, ERROR: %d\n", WSAGetLastError());
	else
		printf("Socket function succeeded\n");

	// Configuration of the socket
	serverService.sin_family = AF_INET;
	serverService.sin_addr.s_addr = INADDR_ANY;
	serverService.sin_port = htons(7777);

	// Binding
	if (bind(s, (struct sockaddr *)&serverService, sizeof(serverService)) == SOCKET_ERROR)
		printf("Bind failed with error code : %d", WSAGetLastError());

	printf("Bind is done\n");

	while (1)
	{
		// Listening
		printf("Wainting for connection on port 7777...\n");
		listen(s, 1);

		// Accepting a connection
		sizeStruct = sizeof(struct sockaddr_in);
		new_socket = accept(s, (struct sockaddr_in*)&clientService, &sizeStruct);

		if (new_socket == INVALID_SOCKET)
			printf("accept failed with error code : %d", WSAGetLastError());

		printf("Acquired connection with %s\n", inet_ntoa(clientService.sin_addr));

		// Sending 200
		cleanBuffer(strSend);
		strcpy(strSend, "200");
		sendData(new_socket, strSend);

		// Recieving 205
		cleanBuffer(strRecv);
		recieveData(new_socket, strRecv);
		checkForErrors(new_socket, strRecv, 205);

		printf("Connection Confiremed!\n");

		// Valinading the password(101, 102)
		cleanBuffer(strRecv);
		recieveData(new_socket, strRecv);
		checkForErrors(new_socket, strRecv, 100);

		if (strcmp(strRecv + 3, PASSWORD) == 0)
		{
			// send password confirmation
			cleanBuffer(strSend);
			strcpy(strSend, "101");
			sendData(new_socket, strSend);

			printf("The client has entered the password correctly!\n");
		}
		else
		{
			// send password error
			cleanBuffer(strSend);
			strcpy(strSend, "102");
			sendData(new_socket, strSend);

			printf("The client has entered the wrong password!\n");
			closeConnection(new_socket);

			getch();
			return 1;
		}

		cleanBuffer(strRecv); // clean the buffer for the check in the while loop

		while (getCode(strRecv, 6) != 405000) // while the user didn't choose to exit
		{
			// sending the menu(400)
			cleanBuffer(strSend);
			strcpy(strSend, "400");
			buildMenu(strSend);

			sendData(new_socket, strSend);
			printf("The menu sent\n");

			// recieving which command to operate
			cleanBuffer(strRecv);
			recieveData(new_socket, strRecv);
			checkForErrors(new_socket, strRecv, 405);

			code = getCode(strRecv, 6); // get the code for multiple checks

			// first option, list files in folder
			if (code == 405001)
			{
				getFilesInAFolder(dataBuffer, strRecv + 6); // adding the data into the files buffer

				cleanBuffer(strSend);
				strcpy(strSend, "500");
				strcat(strSend, dataBuffer);

				sendData(new_socket, strSend);

				printf("files list sent, path: %s\n", strRecv + 6);
			}

			// second option, open command prompt
			if (code == 405002)
			{
				getCommandOutput(dataBuffer, strRecv + 6);

				cleanBuffer(strSend);
				strcpy(strSend, "500");
				strcat(strSend, dataBuffer);

				sendData(new_socket, strSend);
			}
		}
	}

	getch();
	return 0;
}

int sendData(int s, char* buffer)
{
	int sendResult = send(s, buffer, SIZE, 0);

	if (sendResult == SOCKET_ERROR)
		return SOCKET_ERROR;
}

int recieveData(int s, char* buffer)
{
	int recvResult = recv(s, buffer, SIZE, 0);

	if (recvResult == SOCKET_ERROR)
		return SOCKET_ERROR;
}

void cleanBuffer(char* buffer)
{
	int i;

	for (i = 0; i < SIZE; i++)
		buffer[i] = '\0';
}

void closeConnection(int s)
{
	int Result = closesocket(s);

	if (Result == SOCKET_ERROR)
		printf("Failed at closesocket function, ERROR: %d\n", WSAGetLastError());
	WSACleanup();

	printf("Connection closed successfully\n");

	getch();
}

int getCode(char* str, int length)
{
	int i;
	char szCode[10] = { '\0' };

	for (i = 0; i < length; i++)
		szCode[i] = str[i];

	return atoi(szCode);
}

void buildMenu(char* str)
{
	strcat(str, "0. Quit\n");
	strcat(str, "1. List files in a directory\n");
	strcat(str, "2. Run a command in command prompt\n");
}

void checkForErrors(int s, char* buffer, int code)
{
	char errorBuffer[100] = { '\0' };

	if (getCode(buffer, 3) != code)
	{
		printf("There was an error!\n");

		strcpy(errorBuffer, "600");
		strcat(errorBuffer, "There was an error!\n");
		sendData(s, errorBuffer);

		closeConnection(s);
		exit(1);
	}
}

void getFilesInAFolder(char* dataBuffer, char* path)
{
	DIR *dir;
	struct dirent *ent;
	int i = 0;

	dir = opendir(path);

	if (dir == NULL)
	{
		strcpy(dataBuffer, "No such directory");
		return;
	}

	while ((ent = readdir(dir)) != NULL)
	{
		if (i++ > 1) // skip the dots
		{
			strcat(dataBuffer, ent->d_name);
			strcat(dataBuffer, "\n");
		}
	}

	closedir(dir);
}

void getCommandOutput(char* dataBuffer, char* command)
{
	FILE* file;
	int sizeOfFile, result, ch, i = 0;
	char* fullCommand;

	// allocating memory for the full command
	fullCommand = (char*)malloc(sizeof(char) * (strlen(command) + strlen(FILE_NAME) + 4)); // 4 stands for " > " and null

	if (fullCommand == NULL)
	{
		strcpy(dataBuffer, "Error at allocating memory\n");
		printf("Error at allocating memory\n");
		return;
	}

	strcpy(fullCommand, command);
	strcat(fullCommand, " > ");
	strcat(fullCommand, FILE_NAME);

	printf("Executing the following command: %s\nLoading...\n\n", fullCommand);

	// sending the command to the system to take care of
	result = system(fullCommand);

	if (result == -1)
	{
		strcpy(dataBuffer, "Error at the command\n\n");
		printf("Error at the command\n");
		return;
	}

	// I don't need it anymore
	free(fullCommand);

	// opening the file
	file = fopen(FILE_NAME, "rt");

	if (file == NULL)
	{
		strcpy(dataBuffer, "Error at opening file");
		printf("Error at opening file\n");
		return;
	}

	// writing the data into the output string

	while ((ch = getc(file)) != EOF)
		dataBuffer[i++] = ch;

	dataBuffer[i] = '\0'; // set a null to the end

	fclose(file);
	printf("Data of the command was sent\n");
}

int getSizeOfFile(FILE* file)
{
	int size;

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);

	return size;
}
