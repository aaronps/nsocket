#include "NSocket.h"
#include "NTimer.h"


#include <string.h>
#include <stdio.h>

std::list<NSocket *> NSocketManager::_solist;
std::list<NSocket *> NSocketManager::_so_to_add;
std::list<NSocket *> NSocketManager::_so_to_remove;

NSocket::NSocket( NSocketListener *lis) :
        has_socket(false), is_connecting(false), wantstowrite(false),
        wantstoexcept(false), from_ssaddr(0),
        read_timeout(0), write_timeout(0)
{
	static int sn=0;
	socknum=sn++;
	ipaddress[0]=0;
	port[0]=0;
}

// ~NSocket(): close the socket if it is open
NSocket::~NSocket()
{
	sysClose(); // Should we notify?
}

// disconnect(): close the socket and alert.
void
NSocket::disconnect()
{
    sysClose();
    onDisconnected();
}

/*
 * sysAccept
 */
void
NSocket::sysAccept()
{
	char new_ipaddress[NI_MAXHOST];
	char new_port[NI_MAXSERV];
	NSocket * newcon;
	SOCKET new_fd;
	sockaddr_storage new_ssaddr;
	socklen_t slen = sizeof(new_ssaddr);
	new_fd = accept(fd,(struct sockaddr *) &new_ssaddr, &slen);
	if ( new_fd == INVALID_SOCKET )
    {
		error=GET_NET_ERROR();
		switch (error)
        {
        // Notify on BAD errors, the rest are ignorable
        // Note that this errors are NSocket errors
#ifdef _WIN32
            // for windows
            case WSAENOTSOCK:
            case WSAEOPNOTSUPP:
            case WSAEINVAL:
            case WSAEFAULT:
            case WSANOTINITIALISED:
#else
            // for others
            case EBADF:
            case ENOTSOCK:
            case EOPNOTSUPP:
            case EINVAL: // not in macosx, should test if this compiles in it
            case EFAULT:
#endif
                notifyError(); // on this errors this socket should be closed
		}
		return; // nothing more to do
	}

	sockaddr_to_ip(&new_ssaddr,slen,new_ipaddress,new_port);

	newcon=onNewConnection(new_ipaddress, new_port);
	if (newcon)
    {
        newcon->fd = new_fd;
        newcon->has_socket = true;
        strcpy(newcon->ipaddress, new_ipaddress);
        strcpy(newcon->port, new_port);
        NSocketManager::add_to_list(newcon);
        newcon->onSocketReady();
    }
    else
    {
		closesocket(new_fd);
	}
}

void
NSocket::sysClose()
{
    if (has_socket)
    {
        has_socket=false;
        closesocket(fd);
    }
}

bool
NSocket::setNonBlock()
{
#ifdef _WIN32
	unsigned long mode = 1;
	sys_result = ioctlsocket(fd, FIONBIO, &mode);
#else
	sys_result = fcntl(fd, F_SETFL, O_NONBLOCK);
#endif
	if (sys_result == SOCKET_ERROR)
    {
		error=GET_NET_ERROR();
		return notifyError();
	}
	return true;
}

bool
NSocket::setReuseAddr()
{
	int v=1;
	sys_result = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, SETSOCKOPT_TYPE &v, sizeof(v));
	if (sys_result == SOCKET_ERROR)
    {
		error=GET_NET_ERROR();
		notifyError();
	}
	return true;
}

bool
NSocket::sysRecv()
{
    int read_retrycount = 3; // +1 (first time)
read_again:
    if (from_ssaddr)
    { // this must be a udp socket
        fromlen=sizeof(from_ssaddr);
        sys_result = recvfrom(fd,recvbuffer,NSBUFLEN,0,(struct sockaddr *) from_ssaddr,&fromlen);
    }
    else
    {
        sys_result = recv(fd,recvbuffer,NSBUFLEN,0);
    }
	if (sys_result == SOCKET_ERROR)
    {
		error=GET_NET_ERROR();
        switch (error)
        {
            case NET_ERROR_EINTR:
                if (read_retrycount--)
                    goto read_again;
            case NET_ERROR_EAGAIN:
                return true;
            case NET_ERROR_ENOTCONN:
                onInternalError("recv: Tryed to receive on an unconected socket (and should be connected)");
                break;
            case NET_ERROR_ENOTSOCK:
                onInternalError("recv: The socket is not a socket");
                break;
            case NET_ERROR_EFAULT:
                onInternalError("recv: The internal buffer pointer is out of range");
                break;
            case NET_ERROR_EINVAL:
                onInternalError("recv: Invalid parameters passed");
                break;
            case NET_ERROR_ECONNREFUSED:
                onConnectError();
                sysClose();
                break;
            default:
                printf("[%d] Receive error %s\n", socknum, strerror(error));
                onReceiveError();
		}
		return false;
	}
	if (sys_result == 0)
    {
		onDisconnected();
		return false;
	}
	if (!onDataReceived(recvbuffer, sys_result))
    {
        disconnect();
        return false;
    }
    else
    {
        return true;
    }
//	return true;
}

int
NSocket::sysSend(const char *b, unsigned int s, const struct sockaddr_storage *to_ssaddr, int tolen)
{
	if (s < 1)
    {
        return s; // XXX WE SHOULD LAUNCH onSoftwareError();
    }
    
	if (to_ssaddr)
    { // MUST BE A UDP
        sys_result = sendto(fd,b,s,0,(const struct sockaddr *)to_ssaddr,tolen);
	}
    else
    {
        sys_result = send(fd,b,s,0);
	}
    
	if (sys_result == SOCKET_ERROR)
    {
		error=GET_NET_ERROR();
		printf("[%d] sysSend Error: %s\n", socknum, strerror(error));
		onSendError(); // XXX check for errors
		return -1;
	}
	return sys_result;
}

void
NSocket::sockaddr_to_ip(const sockaddr_storage * ss, const int sslen, char *ip, char *po)
{
#ifdef getnameinfo
    sys_result = getnameinfo((const sockaddr *) address->ai_addr,
				 address->ai_addrlen,
				 ipaddress, NI_MAXHOST,
				 port, NI_MAXSERV,
				 NI_NUMERICHOST|NI_NUMERICSERV);

	if (sys_result) {} // unfinished
#error "still not finished this part"
#else
//    if (sslen == sizeof(struct sockaddr_in) ) {
    if ( ss->ss_family == AF_INET )
    {
        char * tmp_ip;
        tmp_ip=inet_ntoa(((struct sockaddr_in *)ss)->sin_addr);
        strncpy(ip, tmp_ip, NI_MAXHOST-1);
        ipaddress[NI_MAXHOST-1]=0;
        sprintf(po,"%u",ntohs(((struct sockaddr_in*)ss)->sin_port));
    }
    else if ( ss->ss_family == AF_INET6 )
    {
        printf("not finished for ipv6\n");
        ip[0] = 0;
        po[0] = 0;
    }
    else
    {
        ip[0] = 0;
        po[0] = 0;
    }
#endif
}

/****************************************************************************
*
* for support of ipv6: linux, mac osx, windows (xp+, dont compile in older)
*   getaddrinfo, freeaddrinfo, getnameinfo
*
* if we dont want ipv6 support we can do 2 things:
*   1- write our own get/freeaddrinfo and getnameinfo
*   2- use totally different code with gethostbyname, etc...
*
*
* what we will do is, on non windows, use the ipv6_able functions
* and on windows, if the user wants ipv6 support will need to define some
* variable (like _ENABLE_IPV6?) or will only be able to use ipv4 and we will
* use old functions (gethostbyname, inet_addr, ...)
*
* XXX TODO think a way to do asyn DNS
*
****************************************************************************/
void
NSocket::sysResolve(const char *hostname, const char *p, SOCKET_TYPE stype)
{
#ifndef _WIN32
	struct addrinfo hints;
	struct addrinfo *address;
	struct addrinfo *firstaddress;

	memset(&hints, 0, sizeof(hints));
	//hints.ai_family = AF_UNSPEC;
	hints.ai_family = AF_INET;
	if (stype == TCP || stype == LISTEN_TCP)
    {
		hints.ai_socktype = SOCK_STREAM;
	}
    else
    {
		hints.ai_socktype = SOCK_DGRAM;
	}

	if (stype == LISTEN_TCP || stype == LISTEN_UDP)
    {
		hints.ai_flags = AI_PASSIVE;
	}

	sys_result = getaddrinfo(hostname, p, &hints, &firstaddress);
	if (sys_result)
    {
		error=sys_result; // this is correct. DON'T CHANGE
		if (error == NET_ERROR_NONAME)
        {
			onHostNotFoundError();
		}
		return; // XXX check for getaddrinfo errors
	}

	for (address=firstaddress; address; address=address->ai_next)
    {
		fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
		if (fd == INVALID_SOCKET)
        {
			error=GET_NET_ERROR();
			continue; // use next address if cant create socket.
		}

		has_socket=true;

		if (!setNonBlock())
        {
			sysClose();
			break; // XXX just close? no error?
		}

		if (stype == TCP || stype == UDP)
        {
			sys_result = connect(fd, address->ai_addr, address->ai_addrlen);
			if (sys_result == SOCKET_ERROR)
            {
				error=GET_NET_ERROR();
				if (error != NET_ERROR_CONINPROGRESS)
                {
					sysClose();
					onConnectError();
					continue; // try next address if cant connect
				}
				write_timeout.reset();
				is_connecting=true;
				add_to_list(this);
				break;
			}
			add_to_list(this); // XXX may add an element when we are in the loop
			break; // finished the loop here
		}
        else
        { // wants to bind
			if (!setReuseAddr())
            {
				sysClose();
				break; // XXX what can we do?
			}

			sys_result = bind(fd, address->ai_addr, address->ai_addrlen);
			if (sys_result == SOCKET_ERROR)
            {
				error=GET_NET_ERROR();
				sysClose();
				onBindError();
				break; // XXX what can we do?
			}

			if (stype == LISTEN_TCP)
            {
				sys_result = listen(fd, 10);
				if (sys_result == SOCKET_ERROR)
                {
					error=GET_NET_ERROR();
					sysClose();
					onListenError();
					break; // XXX what can we do?
				}
				add_to_list(this); // XXX should put some onListening()?
			}
			break;
		}
	}

	if (!has_socket)
    {
		printf("dont has socket %d\n", sys_result);
		freeaddrinfo(firstaddress);
		return; // XXX put some errors somewhere, but we already has, dont we?
	}

	socket_type=stype;

    // convert sockaddr to ipaddress and port char *

	memset(ipaddress, 0, NI_MAXHOST);
	memset(port, 0, NI_MAXSERV);
	sys_result = getnameinfo((const sockaddr *) address->ai_addr,
				 address->ai_addrlen,
				 ipaddress, NI_MAXHOST,
				 port, NI_MAXSERV,
				 NI_NUMERICHOST|NI_NUMERICSERV);

	if (sys_result)
    { // XXX maybe check for some errors?
		int hlen;
		hlen=strlen(hostname);
		memcpy(ipaddress,hostname,(hlen<NI_MAXHOST)?hlen:NI_MAXHOST-1);
		hlen=strlen(p);
		memcpy(port,p,(hlen<NI_MAXSERV)?hlen:NI_MAXSERV-1);
	}
	freeaddrinfo(firstaddress); // XXX maybe here maybe up and copy sockaddr_in?
	if ( (stype == TCP || stype == UDP) && !is_connecting ) // if we are here is because we are connected
    {
        onSocketReady();
    }
#else
// for windows, no resolver
    struct sockaddr_in sa;
    sa.sin_addr.s_addr=inet_addr(hostname);
    if ( sa.sin_addr.s_addr == 0xffffffff)
    {
      printf("bad name\n");
      return;
    }

    sa.sin_port=htons(53);
    if (!sa.sin_port)
    {
      printf("bad port\n");
      return;
    }

    sa.sin_family=AF_INET;

    int socktype;
    if (stype == TCP || stype == LISTEN_TCP)
    {
		socktype = SOCK_STREAM;
	}
    else
    {
		socktype = SOCK_DGRAM;
	}

	fd = socket(AF_INET, socktype, 0);
	if ( fd == INVALID_SOCKET )
    {
	    printf("invalid socket\n");
	    return;
	}

    has_socket=true;

    if (!setNonBlock())
    {
        sysClose();
        return; // XXX just close? no error?
    }

    if (stype == TCP || stype == UDP)
    {
        sys_result = connect(fd, (const sockaddr *)&sa, sizeof(sa));
        if (sys_result == SOCKET_ERROR)
        {
            error=GET_NET_ERROR();
            if (error != NET_ERROR_CONINPROGRESS)
            {
                sysClose();
                onConnectError();
                return; // try next address if cant connect
            }
            write_timeout.reset();
            is_connecting=true;
            NSocketManager::add_to_list(this);
            return;
        }
        NSocketManager::add_to_list(this); // XXX may add an element when we are in the loop
    }
    else
    { // wants to bind
        if (!setReuseAddr())
        {
            sysClose();
            return; // XXX what can we do?
        }

        sys_result = bind(fd, (const struct sockaddr *)&sa, sizeof(sa));
        if (sys_result == SOCKET_ERROR)
        {
            error=GET_NET_ERROR();
            sysClose();
            onBindError();
            return; // XXX what can we do?
        }

        if (stype == LISTEN_TCP)
        {
            sys_result = listen(fd, 10);
            if (sys_result == SOCKET_ERROR)
            {
                error=GET_NET_ERROR();
                sysClose();
                onListenError();
                return; // XXX what can we do?
            }
            NSocketManager::add_to_list(this); // XXX should put some onListening()?
        }
    }

	if ( (stype == TCP || stype == UDP) && !is_connecting ) // if we are here is because we are connected
        onSocketReady();


//  #error "Not ready for this compiler"
#endif

}

void
NSocket::connectTCP(const char * to_host, const char * to_port)
{
    sysResolve(to_host, to_port, TCP);
}

void
NSocket::listenTCP(const char * listip, const char * lisport)
{
    sysResolve(listip, lisport, LISTEN_TCP);
}

void
NSocket::connectUDP(const char * to_host, const char * to_port)
{
    sysResolve(to_host, to_port, UDP);
}

void
NSocket::listenUDP(const char * listip, const char * lisport)
{
    sysResolve(listip, lisport, LISTEN_UDP);
}

/****************************************************************************
* internal_handleEvents
*
* Uses select, list of errors select can give in various systems
* int select(int n, fd_set * read, fd_set *write, fd_set *excep, timeval *tv)
*
* Linux:
*   EBADF       Invalid fd in one of the sets
* *-EINTR       Signal received (select didn't finish)
* *-EINVAL      n is negative
*   ENOMEM      select unable to alocate mem for internal tables
*
* Mac OSX:
*   EBADF       Invalid fd in on of the sets
* *-EINTR       Signal received (select didn't finish)
* *-EINVAL      tv is invalid (negative value)
*
* Windows:
*   WSANOTINITIALISED   winsock is not initialised
*   WSAEFAUL        unable to allocate resources, or bad pointers
*   WSAENETDOWN     the network subsystem has failed
* *-WSAEINTR        cancelled by WSACancelBlockingCall (like signal received)
* *-WSAEINVAL       tv not valid or the 3 fd_sets are NULL
*   WSAEINPROGRESS  other call in progress, or service provider in callback
*   WSAENOTSOCK     one of the fd_sets contains an entry that is not a socket
*
* Notes
*
* fd_sets may change, always reinit them
* tv may change or not (system depend) always reinit it
*
* Linux and Mac OSX:
*   n = highest_numbered_descriptor + 1;
*   returns:
*       0 - if nothing changed
*       -1 - if error
*       int - num descriptor changed
*
* Windows:
*   the 3 fd_sets cant be null
*
****************************************************************************/
void
NSocketManager::internal_handleEvents(struct timeval *timeout)
{
    checkTimeouts();
    prepareSocketList();
	if (_solist.empty())
    {
		return; // no more sockets
	}
    
	fd_set to_read;
	fd_set to_write;
	fd_set to_except;
	int s_res;
	int maxfd = 0; //note in windows not used
	NSocket *ns;
	std::list<NSocket *>::iterator listiter;

	FD_ZERO(&to_read);
	FD_ZERO(&to_write);
	FD_ZERO(&to_except);

	for (listiter=_solist.begin(); listiter!=_solist.end(); listiter++)
    {
	    ns=*listiter;
	    if (ns->is_connecting)
        {
            SETMAXFD(maxfd,ns->fd);
			FD_SET(ns->fd,&to_write);
			FD_SET(ns->fd,&to_except);
        }
        else
        { 
            SETMAXFD(maxfd,ns->fd);
            FD_SET(ns->fd,&to_read); // always read
            if (ns->wantstowrite)
            {
                FD_SET(ns->fd,&to_write);
            }
            if (ns->wantstoexcept)
            {
                FD_SET(ns->fd,&to_except);
            }
        }
	}
	s_res = select(++maxfd,&to_read,&to_write,&to_except,timeout);
	if (s_res == SOCKET_ERROR)
    {
	    s_res=GET_NET_ERROR();
        switch (s_res)
        {
            case NET_ERROR_EINTR:   // XXX maybe we could retry the select?
                return;             // or check if have many eintr?
            case NET_ERROR_EINVAL:
                printf("select: Invalid parameters passed"); // TODO: remove printf s
                break;
            default:
                printf("select: Some error happened\n");
                exit(1); // TODO: don't exit here
		}
		return; // TODO: return some error code
	}

	if ( !s_res )
    {
		return; // Nothing to do
	}

	for (listiter=_solist.begin(); listiter!=_solist.end(); listiter++ )
    {
	    ns=*listiter;
		if (ns->is_connecting)
        {
			if (FD_ISSET(ns->fd,&to_write))
            {
				ns->is_connecting = false;
				ns->wantstowrite = false;
				ns->onSocketReady();
                ns->write_timeout.reset();
			} // WARNING what if got write AND except?
			if (FD_ISSET(ns->fd,&to_except))
            {
				ns->sysClose();
				ns->is_connecting = false;
				ns->wantstoexcept = false;
				ns->onConnectError();
                remove_from_list(ns);
			}
			continue; // premature finish loop
		} 
		
		if (FD_ISSET(ns->fd,&to_read))
        {
			if (ns->socket_type == LISTEN_TCP)
            {
				ns->sysAccept();
			}
            else
            {
                if (!ns->sysRecv())
                { // XXX if ns was deleted... the pointer is not valid.
                // XXX remove here
                }
                ns->read_timeout.reset();
				continue;
            }
        }
		if (ns->wantstowrite && FD_ISSET(ns->fd,&to_write))
        {
			ns->onAbleToWrite();
		}
		if (ns->wantstoexcept && FD_ISSET(ns->fd,&to_except))
        {
			// SEE THE EXCEPTIONS (or oob in linux)
		}
	}
}

void
NSocketManager::prepareSocketList()
{
    if ( ! _so_to_add.empty() )
    {
        list<NSocket *>::const_iterator sta;
        for ( sta=_so_to_add.begin(); sta != _so_to_add.end(); sta++)
        {
            _solist.push_back(*sta);
        }
        _so_to_add.clear();
    }

    if ( ! _so_to_remove.empty() )
    {
        list<NSocket *>::const_iterator stre;
        for ( stre=_so_to_remove.begin(); stre != _so_to_remove.end(); stre++)
        {
            _solist.remove(*stre);
        }
        _so_to_remove.clear();
    }

    if ( ! _solist.empty() )
    {
        list<NSocket *>::iterator liter;
        NSocket *ns;
        for (liter = _solist.begin(); liter != _solist.end();)
        {
            ns = *liter;
            if ( ! ns->has_socket )
            {
                liter=_solist.erase(liter);
            }
            else 
            {
                liter++;
            }
        }
    }
}

void
NSocketManager::checkTimeouts()
{
    if ( ! _solist.empty() )
    {
        NSocket * ns;
        list<NSocket *>::iterator liter;
        for ( liter = _solist.begin(); liter != _solist.end(); liter++ )
        {
            ns = *liter;
            if ( ns->is_connecting && ns->write_timeout.isTimeOut() )
            {
                ns->onConnectTimeOut();
                remove_from_list(ns);
                ns->sysClose();
            }
            else
            {
                if (ns->read_timeout.isTimeOut())
                {
                    ns->onReceiveTimeOut();
                    ns->read_timeout.reset();
                }
                if (ns->wantstowrite && ns->write_timeout.isTimeOut())
                {
                    ns->onSendTimeOut();
                    ns->write_timeout.reset();
                }
            }
        }
    }
}

#ifdef _WIN32
class WinSockInit {
public:
    WinSockInit() {
        WSADATA wsaData;
        WORD wVers = MAKEWORD(2, 0);
        int rc = WSAStartup(wVers, &wsaData);
        if(rc != 0) {
	    fprintf(stderr, "Failed to initialize winsock: %d\n", rc);
	    exit(1);
        }
    }

    ~WinSockInit() {
	WSACleanup();
    }
};
WinSockInit _WinSockInit;
#endif
