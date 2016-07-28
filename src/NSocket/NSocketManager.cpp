
#include "NSocketManager.hpp"
#include "NSocket.h"

#include <cstdlib>

std::list<NSocket *> NSocketManager::_solist;
std::list<NSocket *> NSocketManager::_so_to_add;
std::list<NSocket *> NSocketManager::_so_to_remove;

NSocket*
NSocketManager::ConnectUDP( NSocketListener* listener,
                            const char * to_host,
                            const char * to_port)
{
    NSocket * res = new NSocket(listener);

    //sysResolve(to_host, to_port, UDP);
    return res;
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
//    checkTimeouts();
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
//                ns->write_timeout.reset();
            } // WARNING what if got write AND except?
            if (FD_ISSET(ns->fd,&to_except))
            {
                ns->sysClose();
                ns->is_connecting = false;
                ns->wantstoexcept = false;
                //ns->onConnectError();
                remove_from_list(ns);
            }
            continue; // premature finish loop
        }

        if (FD_ISSET(ns->fd,&to_read))
        {
            if (ns->socket_type&(LISTEN|TCP) == (LISTEN|TCP) )
            {
                ns->sysAccept();
            }
            else
            {
                if (!ns->sysRecv())
                { // XXX if ns was deleted... the pointer is not valid.
                // XXX remove here
                }
//                ns->read_timeout.reset();
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
        std::list<NSocket *>::const_iterator sta;
        for ( sta=_so_to_add.begin(); sta != _so_to_add.end(); sta++)
        {
            _solist.push_back(*sta);
        }
        _so_to_add.clear();
    }

    if ( ! _so_to_remove.empty() )
    {
        std::list<NSocket *>::const_iterator stre;
        for ( stre=_so_to_remove.begin(); stre != _so_to_remove.end(); stre++)
        {
            _solist.remove(*stre);
        }
        _so_to_remove.clear();
    }

    if ( ! _solist.empty() )
    {
        std::list<NSocket *>::iterator liter;
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

/*
void
NSocketManager::checkTimeouts()
{
    if ( ! _solist.empty() )
    {
        NSocket * ns;
        std::list<NSocket *>::iterator liter;
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
*/

