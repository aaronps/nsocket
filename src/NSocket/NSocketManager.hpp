/* 
 * File:   NSocketManager.hpp
 * Author: krom
 *
 * Created on October 19, 2010, 8:02 AM
 */

#ifndef NSOCKETMANAGER_HPP
#define	NSOCKETMANAGER_HPP

#include <list>

#include <cstdio>

#include <sys/time.h>

class NSocket;
class NSocketListener;

class NSocketManager
{
public:
    static NSocket* ConnectUDP( NSocketListener* listener,
                                const char * to_host,
                                const char * to_port);


    static void NSocket_mainloop()
    {
        handleEvents(1000);
        while (!_solist.empty())
            handleEvents(1000);
        if ( _solist.empty() ) {
            printf("solist is empty\n");
        }
    }

    static void handleEvents()
    {
        internal_handleEvents(NULL);
    }

    static void handleEvents(unsigned int utimeout)
    {
        struct timeval tv;
        tv.tv_sec=0;
        tv.tv_usec=utimeout;
        internal_handleEvents(&tv);
    }

private:
    static void add_to_list(NSocket * n)
    {
        _so_to_add.push_front(n);
    }

    static void remove_from_list(NSocket * n)
    {
        _so_to_remove.push_front(n);
    }


    static void internal_handleEvents(struct timeval *timeout);
    static void prepareSocketList();
//    static void checkTimeouts();

    static std::list<NSocket *> _solist;
    // Next two lists to avoid add/remove when in loop
    static std::list<NSocket *> _so_to_add;
    static std::list<NSocket *> _so_to_remove;

};

#endif	/* NSOCKETMANAGER_HPP */

