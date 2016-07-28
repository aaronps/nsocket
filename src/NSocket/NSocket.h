#ifndef _NSOCKET_H_
#define _NSOCKET_H_

#include "NSocket_facade.h"

//using namespace std;

class NSocket;
//struct NSocket;

class NSocketListener
{
private:
    friend class NSocket;
    virtual void onSocketReady( NSocket * so) = 0;
    virtual void onDisconected( NSocket * so) = 0;
    virtual void onSocketError( NSocket * so) = 0;
    virtual void onDataReceived( NSocket * so, const char * data, const int datalen){};
    virtual void onNewConnection (NSocket * so, const char * ipaddress, const char * port){};
};

// These are flags
// if ! TCP then it is UDP, think.
typedef enum { TCP = 1, LISTEN = 2 } SOCKET_FLAGS;

class NSocketX
{
public:
    NSocket( NSocketListener *lis );
    ~NSocket();

    inline const char * getIP()
    {
        return (const char *)&ipaddress;
    }
    
    inline const char * getPort()
    {
        return (const char *)&port;
    }

// let me think, we have ( * means supported here)
//    *-Listening TCP sessions (servers)
//    *-Connected TCP sessions (normal)
//      Connected TCP sessions from specific port (not normal, some say (ms) better dont do it)
//    * Listening UDP sessions (servers and normal) receive/send from/to any address
//    *-Connected UDP sessions (normal, what about server?) receive/send from/to only one address
//      Connected UDP sessions from specific port to especific server

    void connectTCP (const char * to_host, const char * to_port);
    void listenTCP(const char * listip, const char * lisport);
    void connectUDP (const char * to_host, const char * to_port);
    void listenUDP(const char * listip, const char * lisport);

    
protected:

    int sysSend(const char *b, unsigned int s, const struct sockaddr_storage *to_ssaddr=0, int tolen=0);
    void disconnect();


    
private:
    friend class NSocketManager;

    
    NSocketListener *listener;

    void notifyError() {}
    
    SOCKET fd;
    struct sockaddr_storage ssaddr;

    char ipaddress[NI_MAXHOST];
    char port[NI_MAXSERV];

    char recvbuffer[NSBUFLEN];

    bool has_socket;
    bool is_connecting;
    bool wantstowrite;
    bool wantstoexcept;

    SOCKET_FLAGS socket_type;

    struct sockaddr_storage *from_ssaddr;
    socklen_t fromlen;

    int sys_result;
    int error;

    virtual bool onDataReceived(const char * data, const int datalen);
    virtual void onDataSent() {};
    virtual void onSocketReady() {};
    virtual NSocket * onNewConnection(const char * new_ipaddres, const char * new_port);
    virtual void onReceiveTimeOut();
    virtual void onSendTimeOut();
    virtual void onConnectTimeOut();

    void onAbleToWrite() {};
    virtual void onDisconnected();

    void sysAccept();

    void sysClose();

    bool sysRecv();

    bool setNonBlock();

    bool setReuseAddr();


    void sysResolve(const char *hostname, const char *p, SOCKET_FLAGS stype);

    NSocket(const NSocket &n);
    NSocket& operator=(const NSocket &n);

    void sockaddr_to_ip(const sockaddr_storage * ss, const int sslen, char *ip, char *po);


};

#endif
