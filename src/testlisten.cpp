#include <stdio.h>
#include "NSocket.h"

#define BUFLEN 4096

class QueryNPServer : public NSocket
{

    void onSocketReady()
    {
        printf("[%d] QueryNPServers: Conected to %s:%s\n", get_socknum(), getIP(), getPort());
        char *tos="\\status\\final\\";
        printf("[%d] Sending [%s]\n", get_socknum(), tos);
		sysSend(tos,strlen(tos));
    };

    bool onDataReceived(const char *data, const int datalen)
    {
        printf("[%d] Received %d\n", get_socknum(), datalen);
        return false;
    };

    void onDisconnected()
    {
        printf("[%d] We are disconnected\n", get_socknum());
    };

    int num_retry;

    void onReceiveTimeOut()
    {
        if ( num_retry < 3 ) {
            num_retry++;
            char *tos="\\status\\final\\";
            printf("[%d] Retry %d\n", get_socknum(), num_retry);
            sysSend(tos,strlen(tos));
        } else {
            printf("[%d] We TimeOut\n", get_socknum());
            disconnect();
        }
    };

public:
    QueryNPServer(const char * tohost, const char *toport) : num_retry(0)
    {
        printf("[%d] QueryNPServer(%s:%s)\n", get_socknum(), tohost,toport);
        setTimeOut(4000);
        connectUDP(tohost,toport);

    };

    ~QueryNPServer()
    {
        printf("[%d] ~QueryNPServer()\n", get_socknum());

    };
};


class QueryNetpanzerMasterserver : public NSocket
{
//    char ibuffer[BUFLEN];
//    char * xbuffer;
//    unsigned int xbufpos;

    typedef enum { STATE_NONE, PARSE_MASTERSERVERS, PARSE_SERVERS } STATE;
    STATE state;


    void onHostNotFoundError()
	{
		printf("Man we couldn't find the host\n");
	};

	void onConnectError()
	{
		printf("Man we couldn't connect\n");
	};

	void onSocketReady()
	{
		char *tos="\\list\\gamename\\master\\final\\list\\gamename\\netpanzer\\final\\";
		printf("Connected to NetPanzer Masterserver (%s:%s)\n", getIP(), getPort());
		sysSend(tos,strlen(tos));
	};

	bool onDataReceived(const char * data, const int datalen)
	{
	    static char xbuffer[BUFLEN];
	    static char * xbufpos = xbuffer;
	    char * curpos;
	    char * finalpointer = 0;
	    char * serverip;
	    char * serverport;

	    printf("DATA RECEIVED (%d)\n",datalen);
        memcpy(xbufpos, data, datalen);
        xbufpos+=datalen;
        *xbufpos=0;
        printf("[%s]\n", xbuffer);
      while ((finalpointer=strstr(xbuffer, "\\final\\")) != NULL ) {
        if (!finalpointer)
            return true; // still need more data

	    switch (state) {
            case STATE_NONE:
                printf("Error we shouldn't be here\n");
                return false;
            case PARSE_MASTERSERVERS:
                //printf("Parsing Masterservers\n");
                curpos=xbuffer+1;
                while ( curpos < finalpointer ) {
                    if (strncmp(curpos,"ip\\",3)) {
                        printf("Wrong ip answer from Masterserver(%s)\n",curpos);
                        return false;
                    }
                    curpos+=3;
                    serverip=curpos;
                    while ( *curpos != '\\' ) curpos++; // XXX WARNING EOB
                    *curpos++=0;

                    if (strncmp(curpos,"port\\",5)) {
                        printf("Wrong port answer from Masterserver(%s)\n",curpos);
                        return false;
                    }
                    curpos+=5;
                    serverport=curpos;
                    while( *curpos != '\\' ) curpos++; //XXX WARNING EOB
                    *curpos++=0;
                    //printf("Masterserver FOUND: %s:%s\n", serverip, serverport);
                }

                state = PARSE_SERVERS;
                curpos+=6; // final
                if ( curpos < xbufpos ) {
                    memcpy(xbuffer,curpos,xbufpos-curpos);
                    xbufpos = (char *)(xbufpos - curpos);
                } else {
                    xbufpos=xbuffer;
                }
                break;
            case PARSE_SERVERS:
                //printf("Parsing Servers\n");
                curpos=xbuffer+1;
                while ( curpos < finalpointer ) {
                    if (strncmp(curpos,"ip\\",3)) {
                        printf("Wrong ip answer from Masterserver(%s)\n",curpos);
                        return false;
                    }
                    curpos+=3;
                    serverip=curpos;
                    while ( *curpos != '\\' ) curpos++; // XXX WARNING EOB
                    *curpos++=0;

                    if (strncmp(curpos,"port\\",5)) {
                        printf("Wrong port answer from Masterserver(%s)\n",curpos);
                        return false;
                    }
                    curpos+=5;
                    serverport=curpos;
                    while( *curpos != '\\' ) curpos++; //XXX WARNING EOB
                    *curpos++=0;
                    //printf("Server FOUND: %s:%s\n", serverip, serverport);
                    new QueryNPServer(serverip, serverport);
                    //break; // WE WILL DO ONLY ONE, FOR THE MOMENT
                }

                return false; // end, we disconnect
	    }
      }
        return true;
	};
public:
    QueryNetpanzerMasterserver() : state(PARSE_MASTERSERVERS)
    {
       // printf("bufpos=%d\n",xbufpos);
        printf("QueryNetpanzerMasterserver()\n");
        setTimeOut(4000);
        connectTCP("81.169.185.36","28900");
    };

    ~QueryNetpanzerMasterserver()
    {
        printf("~QueryNetpanzerMasterserver()\n");
    };

};




int
main (int argc, char **argv)
{
//	TestListen test;
//	TestNSocket test;
    QueryNetpanzerMasterserver *test;

    test = new QueryNetpanzerMasterserver();

    NSocket::NSocket_mainloop();

    delete(test);

//	while (test.isWorking()) {
//	    printf("pass %d\n", pass++);
//		NSocket::handleEvents();
//	}

	return 0;
}




