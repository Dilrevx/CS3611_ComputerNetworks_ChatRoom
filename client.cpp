#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
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
#define COUTERROR cout<<errno<<strerror(errno)<<endl

constexpr int BUFFER_SIZE = 800;

// WARNING when Debug: #port is host order but ip is net order
const char CHAT_PORT[] = "4545";
const char* map[] = { "127.0.0.1", "10.0.0.1", "10.0.0.2", "10.0.0.3", "10.0.0.4" };

void client();
int main() {
	cout << "###### chatroom initiating ######" << endl;
	cout << "Send Example: To [dst hostname]: [msg]" << endl;
	cout << "To: " << flush;
	
	client();
	//should not return 

	cout << "NOTE: Somehow client returned" << endl;
	sleep(4);
	return 0;
}

void input(int& dst_id, string& msg) {
	string dst;
	cin >> dst >> msg;

	while (dst.size() != 3 || dst[0] != 'h' || dst[1] <= '0' || dst[2] != ':' || dst[1] >= '5') {
		cout << stderr << ":\thostname " << dst << " ilegal" << endl;
		cout << "Note: To is given, ':' needed" << endl;
		cout << "and there's no white space bewteen 'h1' and ':'" << endl;
		cout << "To: " << flush;
		cin >> dst >> msg;
	}
	dst_id = dst[1] - '0';
	cout << "To: " << flush;
}

// setup to listen() 
int listener_setup() {
	addrinfo this_hint;
	memset(&this_hint, 0, sizeof(addrinfo));

	this_hint.ai_family = AF_UNSPEC;
	this_hint.ai_socktype = SOCK_STREAM;
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

		err = listen(sockfd, 10);
		if (err) throw "listen";

		return sockfd;
	}
	catch (const char* e) {
		cout << stderr << "Exception in listener setup: " << e << endl;
		COUTERROR;
		cout << "retry after 10 sec" << endl;
		sleep(10);
		return listener_setup();
	}
}

//setup to connect()
int sender_setup(int dstHost) {
	const char* dstIP = map[dstHost];

	addrinfo  reqinfo;
	memset(&reqinfo, 0, sizeof(addrinfo));

	reqinfo.ai_family = AF_UNSPEC;
	reqinfo.ai_socktype = SOCK_STREAM;
	addrinfo* dstinfo = nullptr;

	try {
		int errorcode;
		errorcode = getaddrinfo(dstIP, CHAT_PORT, &reqinfo, &dstinfo);
		if (errorcode) throw "getaddrinfo";

		int sockfd = socket(dstinfo->ai_family, dstinfo->ai_socktype, dstinfo->ai_protocol);
		if (sockfd < 0) throw "socket";

		errorcode = connect(sockfd, dstinfo->ai_addr, dstinfo->ai_addrlen);
		if (errorcode) throw "connect";

		return sockfd;

	}
	catch (const char* e) {
		cout << stderr << "Exception: sender setup: " << e << endl;
		COUTERROR;
		cout << "retry in 10 sec" << endl;
		sleep(10);
		return sender_setup(dstHost);
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
	int cnt = 0;
	try {
		while (cnt < total) {
			int tmp = send(sockfd, data.c_str() + cnt, total - cnt, 0);
			if (tmp == -1) throw "send";
			cnt += tmp;
		}
	}
	catch (const char* e) {
		cout << stderr << "Exception send_data: " << e << endl;
		COUTERROR;
		cout << "cnt = " << cnt << endl;
		exit(1);
	}

}

void loop(PollList& pollList);

void client() {
	int listen_fd = listener_setup();
	PollList pollList(listen_fd, POLLIN);

	pollList.push_back(STDIN, POLLIN);

	while (true) loop(pollList);
}

int getHostID(sockaddr_storage& storage, socklen_t& len) {
	void* addr = nullptr;
	if (storage.ss_family == AF_INET) {
		addr = &((reinterpret_cast<sockaddr_in&>(storage)).sin_addr);
	}
	else if (storage.ss_family == AF_INET6) {
		addr = &((reinterpret_cast<sockaddr_in6&>(storage)).sin6_addr);
	}

	char buf[BUFFER_SIZE];
	inet_ntop(storage.ss_family, addr, buf, BUFFER_SIZE);

	string tmp(buf);
	int i = tmp.size() - 1;
	while (!tmp[i]) i--;
	return tmp[i] - '0';
}
void loop(PollList& pollList) {
	pollfd* pList = pollList.c_array();
	int	poll_cnt;

	poll_cnt = poll(pList, pollList.size(), -1);
	
	if (poll_cnt < 0) {
		cout << stderr << "poll error" << endl;
		COUTERROR;
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
				sockaddr_storage saddr_tmp;
				memset(&saddr_tmp, 0, sizeof(sockaddr_storage));
				socklen_t sa_tmp_len = sizeof(sockaddr_storage);

				int recvSock = accept(pfd.fd, reinterpret_cast<sockaddr*>(&saddr_tmp), &sa_tmp_len);
				if (recvSock < 0) {
					cout << stderr << "poll accept err" << i << endl;
					COUTERROR;
					continue;
				}

				char buffer[BUFFER_SIZE] = {};
				int recv_cnt = recv(recvSock, buffer, BUFFER_SIZE, 0);

				if (recv_cnt > 0) {
					cout << buffer << ' ';
					cout << "From h" << getHostID(saddr_tmp, sa_tmp_len) << endl;
					cout << "To: " << flush;
				}
				else if (!recv_cnt) close(recvSock);
				else {
					cout << stderr << "recv";
					COUTERROR;
					continue;
				}
			}
			// stdin
			//  NOTE: cout will be blocked till next cin. 
			else if (i == 1) {
				int dst_id;
				string data;
				input(dst_id, data);
				int send_sock = sender_setup(dst_id);
				send_data(send_sock, data);
				close(send_sock);
			}
		}
	}
}