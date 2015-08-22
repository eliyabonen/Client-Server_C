#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#define SIZE 4000
#define SERVER_IP "127.0.0.1"
#define MAX_OPTIONS 3 // the zero counts, so it is one more actually
#define MAX_DIGITS_FOR_OPTION 3

int sendData(int s, char* buffer);
int recieveData(int s, char* buffer);
int getCode(char* str, int length);
void getDigits(int digits[], int choice);
void setFormattedOption(char* buffer, int choice);
void checkForErrors(int s, char* buffer, int code);
void cleanBuffer(char* buffer);
void closeConnection(int s);

int main(void)
{
	WSADATA info;
	struct sockaddr_in clientService;
	int err, s, cResult, sendResult, recvResult, choice = -1;
	char strSend[SIZE] = { '\0' };
	char strRecv[SIZE] = { '\0' };
	char szPassword[50] = { '\0' };
	char path[SIZE - 6] = { '\0' };

	// Configuration of the socket type
	err = WSAStartup(MAKEWORD(1, 1), &info);

	if (err != 0)
	{
		printf("WSAStartup failed, ERROR: %d\n", err);
		return 1;
	}

	// Creating the socket
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (s == INVALID_SOCKET)
		printf("Failed at creating the socket, ERROR: %d\n", WSAGetLastError());
	else
		printf("Socket function succeeded\n");

	// Configuration of the socket
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(SERVER_IP);
	clientService.sin_port = htons(7777);

	// Connection request
	cResult = connect(s, (struct sockaddr_in*) &clientService, sizeof(clientService));

	if (cResult == SOCKET_ERROR)
	{
		printf("Failed at connect function, ERROR: %d\n", WSAGetLastError());
		cResult = closesocket(s);

		// checking if the closing of the socket succeeded
		if (cResult == SOCKET_ERROR)
			printf("Failed at closesocket function, ERROR: %d\n", WSAGetLastError());
		WSACleanup();

		return 1;
	}

	printf("Acquired connection with %s\n", SERVER_IP);

	// Recieving 200
	cleanBuffer(strRecv);
	recieveData(s, strRecv);
	checkForErrors(s, strRecv, 200);

	// Sending 205
	cleanBuffer(strSend);
	strcpy(strSend, "205");
	sendData(s, strSend);

	printf("Connection confiremed!\n");

	// Validating the password(100)
	printf("Please enter the password: ");
	gets(szPassword);

	// send the password
	cleanBuffer(strSend);
	strcpy(strSend, "100");
	strcat(strSend, szPassword);
	sendData(s, strSend);

	// recieve the server answer
	cleanBuffer(strRecv);
	recieveData(s, strRecv);
	checkForErrors(s, strRecv, 101);

	while (choice != 0) // while the client didn't choose to quit
	{
		// Recieving the menu
		cleanBuffer(strRecv);
		recieveData(s, strRecv);
		checkForErrors(s, strRecv, 400);

		printf("%s\n", strRecv + 3);

		// make sure the client entered a valid choice
		do
		{
			printf("Enter your choice: ");
			scanf("%d", &choice);

			if (choice > MAX_OPTIONS || choice < 0)
				printf("What the hell is this option?\n");

		} while (choice > MAX_OPTIONS || choice < 0);

		// adding the command code here because i want to add something afterwards
		cleanBuffer(strSend);
		setFormattedOption(strSend, choice);

		// adding the content of the command
		if (choice == 1 || choice == 2)
		{
			flushall();

			printf("Enter the path(\\) or command: ");
			gets(path);

			strcat(strSend, path);
		}

		// Sending the option(405)
		sendData(s, strSend);

		if (choice == 0)
		{
			closeConnection(s);
			break;
		}

		// Recieving the data(500)
		cleanBuffer(strRecv);
		recieveData(s, strRecv);
		checkForErrors(s, strRecv, 500);

		printf("Data from server:\n");
		printf("%s\n", strRecv + 3);
	}

	getch();
	return 0;
}

void cleanBuffer(char* buffer)
{
	int i;

	for (i = 0; i < SIZE; i++)
		buffer[i] = '\0';
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

int getCode(char* str, int length)
{
	int i;
	char szCode[10] = { '\0' };

	for (i = 0; i < length; i++)
		szCode[i] = str[i];

	return atoi(szCode);
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

void setFormattedOption(char* buffer, int choice)
{
	int digits[MAX_DIGITS_FOR_OPTION] = { 0 };
	int i;

	strcpy(buffer, "405");
	getDigits(digits, choice); // get the digits of the choice to the array

	// set the digits from integer to string in the buffer
	for (i = 3; i < MAX_DIGITS_FOR_OPTION+3; i++)
		buffer[i] = digits[i - 3] + 48;
}

void getDigits(int digits[], int choice)
{
	int i = 0;
	
	while (choice != 0)
	{
		digits[MAX_DIGITS_FOR_OPTION - i - 1] = choice % 10;

		choice /= 10;
		i++;
	}
}
