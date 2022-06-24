#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <poll.h>
#include <iostream>
using namespace std;


//constexpr short HOST_PORT = 4545;
//const short NET_PORT = htons(HOST_PORT);
//const char* CHAT_PORT = to_string(NET_PORT).c_str();

#define STDIN 0
#define PORT 40351

constexpr int BUFFER_SIZE = 500;

// WARNING when Debug: #port is host order but ip is net order ? MAYBE WRONG
const char CHAT_PORT[] = "40351";
const char* map[] = { "127.0.0.1", "10.0.0.1", "10.0.0.2", "10.0.0.3", "10.0.0.4" };

void client();
int main() {
	cout << "###### chatroom initiating ######" << endl;
	cout << "Send Example: Broadcast: [msg]" << endl;
	//cout << "Broadcast: " << flush;

	client();
	//should not return 

	cout << "NOTE: Some how client returned" << endl;
	sleep(4);
	return 0;
}

void input(string& msg) {
	cin >> msg;
	cout << "Broadcast: " << flush;
}

in_addr_t ifa_addr;
uint32_t ifa_netmask;
void ifconfig() {
		struct ifaddrs* ifAddrStruct = nullptr;
		struct ifaddrs* ifa = nullptr;
		
		cout << "preforming ifconfig..." << endl;
		if (getifaddrs(&ifAddrStruct)) {
			cout << stderr << "ifconfig" << endl;
			cout << errno << strerror(errno) << endl;
			cout << "retry in 5 sec" << endl;
			return ifconfig();
		}

		for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
			if (!(ifa->ifa_addr)) continue;
			if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
				in_addr* tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
				auto addr = ntohl(tmpAddrPtr->s_addr);
				if ((addr & 0xff000000) == 0x0a000000) {
					ifa_addr = addr;
					ifa_netmask = ntohl(reinterpret_cast<sockaddr_in*>(ifa->ifa_netmask)->sin_addr.s_addr);
					break;
				}
			}
			//else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
			// // is a valid IP6 Address
			//	tmpAddrPtr = &((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr;
			//	char addressBuffer[INET6_ADDRSTRLEN];
			//	inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
			//	printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
			//}
		}
		if (ifAddrStruct != NULL) freeifaddrs(ifAddrStruct);
		cout << "IP:" << hex << ifa_addr << endl;
		cout << "netmask:" << hex << ifa_netmask << endl;
		cout << "Broadcast: " << flush;
}
// setup to listen() 
int listener_setup() {
	addrinfo this_hint;
	memset(&this_hint, 0, sizeof(addrinfo));

	this_hint.ai_family = AF_INET;
	this_hint.ai_socktype = SOCK_DGRAM;
	this_hint.ai_flags = AI_PASSIVE;

	addrinfo* myaddr;

	try {
		int err;
		err = getaddrinfo(nullptr, CHAT_PORT, &this_hint, &myaddr);
		if (err) throw "getaddrinfo";

		int sockfd = socket(myaddr->ai_family, myaddr->ai_socktype, myaddr->ai_protocol);
		if (sockfd < 0) throw "socket";

		err = bind(sockfd, myaddr->ai_addr, myaddr->ai_addrlen);
		if (err) throw "bind";

		//this_ai_addr.s_addr = reinterpret_cast<sockaddr_in*>(myaddr->ai_addr)->sin_addr.s_addr;
		return sockfd;
	}
	catch (const char* e) {
		cout << stderr << "Exception in listener setup: " << e << endl;
		cout << errno << strerror(errno) << endl;
		cout << "retry after 10 sec" << endl;
		sleep(10);
		return listener_setup();
	}
}

//setup to connect()
int sender_setup() {
	//addrinfo  reqinfo;
	//memset(&reqinfo, 0, sizeof(addrinfo));

	//reqinfo.ai_family = AF_INET;
	//reqinfo.ai_socktype = SOCK_DGRAM;
	//reqinfo.
	
	//addrinfo* dstinfo = nullptr;

	try {
		int errorcode;
		//errorcode = getaddrinfo(INADDR_BROADCAST, CHAT_PORT, &reqinfo, &dstinfo);
		//if (errorcode) throw "getaddrinfo";

		int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sockfd < 0) throw "socket";

		int yes = 1;
		errorcode = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
		if (errorcode) throw "setopt";

		return sockfd;

	}
	catch (const char* e) {
		cout << stderr << "Exception: sender setup: " << e << endl;
		cout << errno << strerror(errno) << endl;
		cout << "retry in 10 sec" << endl;
		sleep(10);
		return sender_setup();
	}
}

class PollList {
private:
	int currentSize, maxSize;
	pollfd* storage;
	void doubleSize() {
		pollfd* backUp = storage;
		storage = new pollfd[2 * maxSize]{};
		for (int i = 0; i < currentSize; i++) storage[i] = backUp[i];
		delete[] backUp;
		maxSize <<= 1;
	}
public:
	PollList(int init_fd, short _event) {
		currentSize = 1;
		maxSize = 1;
		storage = new pollfd[maxSize]{ {init_fd , _event} };
	}
	~PollList() {
		delete[] storage;
	}

	// element access
	const pollfd& operator[](int index) const {
		return storage[index];
	}
	pollfd& operator[](int index) {
		return storage[index];
	}
	pollfd& front() {
		return storage[0];
	}
	const pollfd& front() const {
		return storage[0];
	}
	pollfd& back() {
		return storage[currentSize - 1];
	}
	const pollfd& back() const {
		return storage[currentSize - 1];
	}
	pollfd* c_array() {
		return storage;
	}

	// Modifiers
	void push_back(int fd, short _event) {
		if (currentSize == maxSize) doubleSize();
		storage[currentSize].fd = fd;
		storage[currentSize].events = _event;
		currentSize++;
	}
	void pop_back() {
		currentSize--;
		storage[currentSize].fd = -1;
	}

	//Capacity
	int size() const {
		return currentSize;
	}
	void resize(int newSize) {
		while (newSize > maxSize) doubleSize();
		currentSize = newSize;
	}
};

void send_data(int sockfd, const string& data) {
	const int total = data.size();
	
	sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl((~ifa_netmask) | ifa_addr);
	saddr.sin_port = htons(PORT);

	try {
		int tmp = sendto(sockfd, data.c_str(), data.size(), 0,
			reinterpret_cast<sockaddr*>(&saddr), sizeof(sockaddr_in));
		if (tmp == -1) throw "send";
	}
	catch (const char* e) {
		cout << stderr << "Exception send_data: " << e << endl;
		cout << "errno:" << errno << strerror(errno) << endl;
		exit(1);
	}

}

bool non_this(const sockaddr_in* recv_addr) {
	in_addr_t addr = ntohl(recv_addr->sin_addr.s_addr);
	if (!addr || addr == 0x7f000001)
		cout << "Some bug may occur, recv from" << hex << addr << endl;
	return !(addr == ifa_addr || !addr || addr == 0x7f000001);
}

void recv_data(int fd) {
	sockaddr_in sender_addr;
	socklen_t addrlen = sizeof(sockaddr_in);

	char buffer[BUFFER_SIZE] = {};
	int recv_cnt = recvfrom(fd, buffer, BUFFER_SIZE, 0, 
		reinterpret_cast<sockaddr*>(&sender_addr), &addrlen);

	if (recv_cnt > 0 && non_this(&sender_addr)) {
		cout << buffer << endl;
		cout << "Broadcast: " << flush;
	}
	else if (recv_cnt < 0) {
		cout << stderr << "recv_data" << endl;
		cout << errno << strerror(errno) << endl;
		cout << "Broadcast: " << flush;
	}
}
void loop(PollList& pollList);

void client() {
	ifconfig();
	int listen_fd = listener_setup();
	PollList pollList(listen_fd, POLLIN);

	pollList.push_back(STDIN, POLLIN);

	while (true) loop(pollList);
}

void loop(PollList& pollList) {
	pollfd* pList = pollList.c_array();
	int	poll_cnt;

	poll_cnt = poll(pList, pollList.size(), -1);

	if (poll_cnt < 0) {
		cout << stderr << "poll error" << endl;
		cout << errno << strerror(errno) << endl;
		exit(1);
	}
	if (!poll_cnt) {
		cout << stderr << "WARNING: Poll returns but nothing happened" << endl;
		return;
	}

	// some event happen

	for (int i = 0; i < pollList.size(); i++) {
		pollfd& pfd = pollList[i];
		if (pfd.revents & POLLIN) {
			// listen
			if (i == 0) {
				recv_data(pfd.fd);
			}
			// stdin
			//  NOTE: cout will be blocked till next cin. 
			else if (i == 1) {
				string data;
				input(data);
				int send_sock = sender_setup();
				send_data(send_sock, data);
				close(send_sock);
			}
		}
	}
}