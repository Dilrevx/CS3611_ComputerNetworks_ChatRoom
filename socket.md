## Section 2
`socket`: a way to speak to other programs using standard Unix file descriptors.

> A file descriptor is simply an integer associated with an open file. But >(and here’s the catch), that file can be a network connection, a FIFO, a pipe, a terminal, a real on-the-disk file, or just about anything else. 

### Internet Sockets

Introduced 2 types:
* Stream Sockets - *SOCK_STREAM*
* Datagram Sockets - *SOCK_DGRAM* : connectionless usually
* Raw Sockets are also powerful

`Stream sockets` : reliable two-way connected communication streams. 
* TCP
* i.e. : HTTP, telnet

`Datagram sockets` : UDP<br>
&emsp;&emsp; i.e. tftp(trivial file transfer protocol), a little brother to FTP; <br>dhcpcd (a DHCP client),games, audio, video


### IP fundamentals
`loopback address` : <br>&emsp;&emsp;
IPv4 - 127.0.0.1 4 bytes ; IPv6 - : :1 16 bytes

`IPv4-compatibility` :: ffff : 255.255.255.255

`subnets`: network portion, host portion -- netmask

`port` : 16 bits. Below 1024 should be reserved

`Host Byte Order`
* `Big-Endian`: 0xb34f -> 0xb3 + 0x4f 
* `Little-Endian`: 0xb34f -> 0x4f + 0xb3

`Network Byte Order` : Big Endian
*  htons,htonl

`Private Networks` 192.168.x.x , 10.x.x.x


### Programming 
#### Data Structures
`Socket Descriptor`: int

`addrinfo` Overall infos, with addr: Net Form

`sockaddr` sock addr, 2 elements: Family and buffer

`sockaddr_in` casted from above, with readable fields.

`sockaddr_in6`

Relationship: Same size, addrinfo do NOT know its ip version. After getaddrinfo it knows, and reinterpret_cast to corresbounding type. To
extract some specific field

`sockaddr_storage` may contain ipv4 & ipv6


#### Functions

    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    
    global errorno

`inet_pton` printable to network

`getaddrinfo` set up structures  
    
    // node: hostname or IP, service: "http" or #port
    int getaddrinfo(const char *node, const char *service, 
                    const struct addrinfo *hints, struct addrinfo **res);
    // hints->ai_flags = AI_PASSIVE: fill my IP, return 0 or -1

`socket` get file descriptor 

    // domain: PF_INET(6), type: SOCK_{STREAM,DGRAM}, protocol: 0 or getprotobyname()
    int socket(int domain, int type, int protocol);
    //return -1 or positive int

`bind` associate socket with a port

    // sockfd: sock file descriptor, my_addr: my info, addrlen: IP len(ip: self, manually write if multiple)
    int bind (int sockfd, struct sockaddr* my_addr, int addrlen);
    // return -1 or 0

`connect` connect to a remote host

    // use dst socket to connect dst server
    int connect(int sockfd, struct sockaddr* serv_addr, int addrlen);
    // return 0, -1

`listen` wait for connected listen -> accept

    // backlog: #connections allowed on the incoming queue
    // incoming connections wait in queue until accept
    int listen(int sockfd, int backlog);
    // return 0 -1

`accept`
> Someone connect your machine on a port you are listening on. 
> <br>
> Connections queue up waiting to be accepted. 
> <br>
> Call accept() and get the pending connection. 
> <br>
> Returns a brand new socket file descriptor for this single connection
> 
> The original one is still listening for more new connections. The newly created one is for send() and recv().

    // sockfd: listening sock, addr: local created sock_addrstorage with addrlen
    int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen); 
    return >0 , -1

`send & recv` For stream sockets or connected datagram sockets. NOT sock_dgram

    // flags=0, returns #bytes send out
    int send(int sockfd, const void* msg, int len, int flags);
    // return -1 or #send

    // flags=0, buf: buffer. return #bytes read in
    // len need be init
    int recv(int sockfd, void *buf, int len, int flags);
    // return #recv , -1 , 0 for closed

`sendto & recvfrom` unconnected SOCK_DGRAM. **NO LISTEN()**

    // to: dstIP
    int sendto(int sockfd, const void *msg, int len, unsigned int flags,
         const struct sockaddr *to, socklen_t tolen);

    // from: be filled. from: sockaddr_storage
    int recvfrom(int sockfd, void *buf, int len, unsigned int flags,
        struct sockaddr *from, int *fromlen); 

`close, shutdown`

    // cut off connection in both way
    close(int sockfd);

    // doesn't close file descriptor, but change its connectivity  
    int shutdown(int sockfd, int how);

`getpeername`

    int getpeername(int sockfd, struct sockaddr *addr, int *addrlen); 
inet_ntop, getnameinfo, gethostbyaddr

`gethostname`

    // IP
    int gethostname(char *hostname, size_t size); 

### Advanced Techniques
`Blocking` *accept, recv* stuck the program till some data arrive, we don't want this!

        fcntl(sockfd, F_SETFL, O_NONBLOCK);

`poll()` monitor multiple sockets at once and handle the ones that have data
> ask the operating system to do all the dirty work for us, let us know when some data is ready to read on which sockets. Meanwhile, our process can go to sleep, saving system resources.

    // arr: a array of struct pollfds that we want to monitor, and what event we wanna monitor for

    #include<poll.h>
    // stuck at poll() till one ready
    int poll(struct pollfd fds[], nfds_t nfds, int timeout);
* POLLIN, POLLOUT

`select`

~~Serial~~ Encode

`Encapsulate` First send #bytes.

`broadcast` 

    // -1 for error
    int setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast)
* Send to subnet: $network\_number | (! netmask)$
* Global: INADDR_BROADCAST

`0.0.0.0` every interface can access socket


>listen() merely sets up the listening socket's backlog and opens the bound port, so clients can start connecting to the socket. That opening is a very quick operation, there is no need to worry about it blocking.
>
>A 3-way handshake is not performed by listen(). It is performed by the kernel when a client tries to connect to the opened port and gets placed into the listening socket's backlog. Each new client connection performs its own 3-way handshake.
>
>Once a client connection is fully handshaked, that connection is made available for accept() to extract it from the backlog. accept() blocks (or, if you use a non-blocking listening socket, accept() succeeds) only when a new client connection is available for subsequent communication.
>
>You call listen() only 1 time, to open the listening port, that is all it does. Then you have to call accept() for each client that you want to communicate with. That is why accept() blocks and listen() does not.

>Considering the kernel source code, listen() is mainly used for initializing accept queue and syn queue, which are useless when using UDP.

>If the AI_PASSIVE flag is specified in hints.ai_flags, and node is
NULL, then the returned socket addresses will be suitable for
bind(2)ing a socket that will accept(2) connections. The returned
socket address will contain the "wildcard address" (INADDR_ANY for IPv4
addresses, IN6ADDR_ANY_INIT for IPv6 address). The wildcard address is
used by applications (typically servers) that intend to accept connec‐
tions on any of the hosts's network addresses