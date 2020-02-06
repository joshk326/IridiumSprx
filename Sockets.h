#include <sys/sys_time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/timer.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netex/net.h>
#include <netex/errno.h>
#include <string.h>

#pragma comment(lib, "net_stub")
#pragma comment(lib, "netctl_stub")

#define SERVER_PORT htons(80)

int Socket;
struct hostent *Host;
struct sockaddr_in SocketAddress;
char bufferReturn[10000];
char RequestBuffer[2000];
char Key[32];

int WriteinConsole(const char * s)
{
	uint32_t len;
	system_call_4(403, 0, (uint64_t)s, std::strlen(s), (uint64_t)&len);
	return_to_user_prog(int);
}

void SleepMM(usecond_t time)
{
	sys_timer_usleep(time * 1000);
}

char* SocketRequest(char* URL, char* Key, char* Path = "")
{
	Host = gethostbyname(URL);
	SocketAddress.sin_addr.s_addr = *((unsigned long*)Host->h_addr);
	SocketAddress.sin_family = AF_INET;
	SocketAddress.sin_port = SERVER_PORT;
	Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(Socket, (struct sockaddr *)&SocketAddress, sizeof(SocketAddress)) != 0) {
		return "CONNECTION ERROR";
	}
	strcpy(RequestBuffer, "GET /");
	if (strlen(Path) > 0){
		strcat(RequestBuffer, Path);
	}
	strcat(RequestBuffer, Key);
	strcat(RequestBuffer, " HTTP/1.0\r\nHOST: ");
	strcat(RequestBuffer, URL);
	strcat(RequestBuffer, "\r\n\r\n");

	send(Socket, RequestBuffer, strlen(RequestBuffer), 0);

	while (recv(Socket, bufferReturn, 10000, 0) > 0)
	{
		return bufferReturn;
		SleepMM(1);
	}
	socketclose(Socket);
}

bool IsRequest(char* Key)
{
	char* penis = SocketRequest("www.jkmods.pe.hu", Key, "gtalicensething.php?key=");
	char* s = strstr(penis, "Key is valid");

	if (s != NULL)
	{
		return true;
	}
	else
	{
		return false;
	}
}

int ExitThisShit()
{
	system_call_1(41, 0);
	return_to_user_prog(int);
}

char* GetKey()
{
	int fd;
	int ret;
	uint64_t pos;
	uint64_t nread;

	cellMsgDialogProgressBarInc(0, 1);
	cellMsgDialogProgressBarSetMsg(0, "Loading Key...");
	ret = cellFsOpen("/dev_hdd0/tmp/Key.txt", 0, &fd, NULL, 0);
	if (!ret)
	{
		cellFsLseek(fd, 0, CELL_FS_SEEK_SET, &pos);
		ret = cellFsRead(fd, Key, sizeof(Key), &nread);
		if (!ret)
		{
			cellFsClose(fd);
		}
		else
		{
			cellMsgDialogClose(5.0);
			SleepMM(500);
			Dialog::msgdialog_mode = 2;
			Dialog::Show("Key failed to Read!");
			ExitThisShit();
		}
	}
	else
	{
		cellMsgDialogClose(5.0);
		SleepMM(500);
		Dialog::msgdialog_mode = 2;
		Dialog::Show("Key failed to load!");
		ExitThisShit();
	}
	return;
}