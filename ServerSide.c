#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <dirent.h>

#define SIZE 1024
#define SERVER_IP "192.168.1.106"
#define PASSWORD "1"

int sendData(int s, char* buffer);
int recieveData(int s, char* buffer);
int getCode(char* str, int length);
void runExecutable(char* dataBuffer, char* path);
void deleteAFile(char* dataBuffer, char* path);
void getFilesInAFolder(char* dataBuffer, char* path);
void checkForErrors(int s, char* buffer, int code);
void cleanBuffer(char* buffer);
void addIPAddresses(char* str, char* clientIP);
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

			// first option, send the ip addresses
			if (code == 405001)
			{
				cleanBuffer(strSend);
				strcpy(strSend, "500");
				addIPAddresses(strSend, inet_ntoa(clientService.sin_addr)); // add ip adresses to the send buffer

				sendData(new_socket, strSend);

				printf("The IP addresses sent\n");
			}

			// second option, open command prompt
			if (code == 405002)
			{
				getFilesInAFolder(dataBuffer, strRecv + 6); // adding the data into the files buffer

				cleanBuffer(strSend);
				strcpy(strSend, "500");
				strcat(strSend, dataBuffer);

				sendData(new_socket, strSend);

				printf("files list sent, path: %s\n", strRecv + 6);
			}

			// third option, delete a file
			if (code == 405003)
			{
				deleteAFile(dataBuffer, strRecv + 6); // deleting the file with given path

				cleanBuffer(strSend);
				strcpy(strSend, "500");
				strcat(strSend, dataBuffer);

				sendData(new_socket, strSend);

				printf("delete file request sent, path: %s\n", strRecv + 6);
			}

			if (code == 405004)
			{
				runExecutable(dataBuffer, strRecv + 6); // running an executable file with given path

				cleanBuffer(strSend);
				strcpy(strSend, "500");
				strcat(strSend, dataBuffer);

				sendData(new_socket, strSend);

				printf("run file request sent, path: %s\n", strRecv + 6);
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
	strcat(str, "1. Get IP of both of us\n");
	strcat(str, "2. List files in a directory\n");
	strcat(str, "3. Delete a file\n");
	strcat(str, "4. Run a file\n");
}

void addIPAddresses(char* str, char* clientIP)
{
	strcat(str, "Server IP: ");
	strcat(str, SERVER_IP);
	strcat(str, "\n");
	strcat(str, "Client IP: ");
	strcat(str, clientIP);
	strcat(str, "\n");
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

void deleteAFile(char* dataBuffer, char* path)
{
	char fullCommand[200] = { '\0' };
	int result;

	strcpy(fullCommand, "del ");
	strcat(fullCommand, path);

	result = system(fullCommand);

	if (result == -1)
		strcpy(dataBuffer, "Error, file not deleted");
	else
		strcpy(dataBuffer, "File deleted successfully");
}

void runExecutable(char* dataBuffer, char* path)
{
	char fullCommand[200] = { '\0' };
	int result;

	strcpy(fullCommand, "start ");
	strcat(fullCommand, path);

	result = system(fullCommand);

	if (result == -1)
		strcpy(dataBuffer, "Error, file is not running");
	else
		strcpy(dataBuffer, "File is running successfully");
}