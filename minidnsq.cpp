#include <stdio.h>
#include <string.h>
#include "NSocket.h"
//#include <stdint.h>

#define QBUFLEN 512
#define DNSPACKETLEN 512
#define DNSHEADLEN 12

class DNSQuery
{
private:
    unsigned short id;
    unsigned short flags;
    unsigned short qdcount;
    unsigned short ancount;
    unsigned short nscount;
    unsigned short arcount;
    unsigned char qdata[DNSPACKETLEN - DNSHEADLEN];
    int qlength;

public:
    DNSQuery(const char * name) :
        id(1111), flags(0), qdcount(0x100), ancount(0), nscount(0), arcount(0)
    {
        const char * pn = name;
        unsigned char * pqdata = (unsigned char *)&qdata;
        unsigned char * pcount = (unsigned char *)&qdata;
        id = NTimer::getMiliseconds()&0xffff;

        pqdata++;
        *pcount=0;
        while (*pn) {
            if ( *pn != '.' ) {
                *pqdata++=*pn;
                (*pcount)++;
            } else {
                pcount=pqdata++;
                *pcount=0;
            }
            pn++;
        }
        *pqdata++=0;
        *pqdata++=0; // A
        *pqdata++=1;
        *pqdata++=0; // IN
        *pqdata++=1;
        qlength=pqdata-((unsigned char *)&qdata)+DNSHEADLEN;

    };

    int length() { return qlength; };
    const char * getQuery() { return (const char *)this; };

    ~DNSQuery(){};

};

typedef enum {
        A       = 1,
        NS      = 2,
        MD      = 3,
        MF      = 4,
        CNAME   = 5,
        SOA     = 6,
        MB      = 7,
        MG      = 8,
        MR      = 9,
        NULLRR  = 10,
        WKS     = 11,
        PTR     = 12,
        HINFO   = 13,
        MINFO   = 14,
        MX      = 15,
        TXT     = 16
        } DNSRRTYPE;

typedef enum {
        CLASSIN = 1,
        CLASSCS = 2,
        CLASSCH = 3,
        CLASSHS = 4
        } DNSCLASS;

//typedef enum {
//        IN      = 1,
//        CS      = 2,
//        CH      = 3,
//        HS      = 4
//    } DNSCLASSTYPE;


class DNSRequest : public NSocket
{
private:

    DNSQuery query;

    virtual void onSocketReady()
    {
        printf("onSocketReady()\n");
        const char * data=query.getQuery();
        int n;
        for (n = 0; n<query.length(); n++)
            printf("%02x ", data[n]&0xff);
        printf("\n");

        sysSend(query.getQuery(), query.length());
    };

    virtual bool onDataReceived(const char * data, const int datalen)
    {
        printf("received response from DNS %d bytes\n", datalen);
        int n;
        const char * pdata = data;
        for (n = 0; n<datalen; n++)
            printf("%02x ", data[n]&0xff);
        printf("\n");

        printf("Heading section\n");

        printf("ID = %04x\n", ntohs(*((unsigned int *)pdata)));
        pdata+=2;
        printf("QR = %s\n", (*pdata&0x80)?"Response":"Query");
        char * opcode;
        switch ((*pdata>>3)&0x0f) {
            case 0:
                opcode="QUERY";
                break;
            case 1:
                opcode="IQUERY";
                break;
            case 2:
                opcode="STATUS";
                break;
            default:
                opcode="RESERVED";
        }
        printf("OPCODE = %s\n", opcode);
        printf("AA = %s\n", (*pdata&0x04)?"AUTHORITATIVE":"NON AUTHORITATIVE");
        printf("TC = %s\n", (*pdata&0x02)?"TRUNCATED":"NON TRUNCATED");
        printf("RD = %s\n", (*pdata&0x01)?"DESIRED":"NON DESIRED");
        pdata++;
        printf("RA = %s\n", (*pdata&0x80)?"AVAILABLE":"NON AVAILABLE");
        char * rcode;
        switch (*pdata&0x0f) {
            case 0:
                rcode="OK";
                break;
            case 1:
                rcode="Format Error";
                break;
            case 2:
                rcode="Server Failure";
                break;
            case 3:
                rcode="Name Error";
                break;
            case 4:
                rcode="Not Implemented";
                break;
            case 5:
                rcode="Refused";
                break;
            default:
                rcode="Unknown rcode";
        }
        printf("RCODE = %s\n", rcode);
        pdata++;

        int qdcount = ntohs(*((unsigned short *)pdata));
        pdata+=2;
        int ancount = ntohs(*((unsigned short *)pdata));
        pdata+=2;
        int nscount = ntohs(*((unsigned short *)pdata));
        pdata+=2;
        int arcount = ntohs(*((unsigned short *)pdata));
        pdata+=2;

        printf("QDCOUNT = %d\n", qdcount);
        printf("ANCOUNT = %d\n", ancount);
        printf("NSCOUNT = %d\n", nscount);
        printf("ARCOUNT = %d\n", arcount);

        char name[256];
        int qtype;
        int qclass;
        while (qdcount--) {
            printf("---Question section\n");


            pdata+=decodeName((const unsigned char *)data, (const unsigned char *)pdata, name);
            qtype = ntohs(*((unsigned short *)pdata));
            pdata+=2;
            qclass = ntohs(*((unsigned short *)pdata));
            pdata+=2;
            printf("QNAME = '%s' ", name);
            printf("QTYPE = %d, ", qtype);
            printf("QCLASS = %d\n", qclass);
        }


        int type;
        int cclass;
        unsigned int ttl;
        int rdlength;
        bool ancountprint=false,nscountprint=false,arcountprint=false;

        while (1) {
            if (ancount) {
                if (!ancountprint)
                    printf(";; Answer section\n");
                ancountprint=true;
                ancount--;
            } else if (nscount) {
                if (!nscountprint)
                    printf(";; Nameserver section\n");
                nscountprint=true;
                nscount--;
            } else if (arcount) {
                if (!arcountprint)
                    printf(";; Additional section\n");
                arcountprint=true;
                arcount--;
            } else {
                printf(";; END\n");
                break;
            }

            pdata+=decodeName((const unsigned char *)data, (const unsigned char *)pdata, name);
            type = ntohs(*((unsigned short *)pdata));
            pdata+=2;
            cclass = ntohs(*((unsigned short *)pdata));
            pdata+=2;
            ttl = ntohl(*((unsigned long *)pdata));
            pdata+=4;
            rdlength = ntohs(*((unsigned char *)pdata));
            pdata+=2;

            printf("%s\t", name);
            printf("%s ", getRRTypeName(type));
            printf("%s ", getRRClassName(cclass));
            printf("%u ", ttl);
            printf("%d ", rdlength);

            switch (type) {
                case A:
                    printf("%s\n", inet_ntoa((in_addr)(*(in_addr*)pdata)));
                    pdata+=4;
                    break;
                case NS:
                    pdata+=decodeName((const unsigned char *)data, (const unsigned char *)pdata, name);
                    printf("%s\n", name);
                    break;
                default:
                    printf("UNKNOWN TYPE\n");
                    return false;
            }


        }

        printf("\n");
        return false;
    };

    int decodeName(const unsigned char * data, const unsigned char * name, char * dest)
    {
        const unsigned char * pname = name;
        bool inpointer=false;
        int usedbytes=0;
        *dest=0;
        unsigned char count;
        while (*pname) {
            if ( (*pname&0xc0) == 0xc0 ) {
//                printf("found compresed data, points to %d\n", (ntohs(*(unsigned short *)pname)&0x3fff));
                pname=data+(ntohs(*(unsigned short *)pname)&0x3fff);
                if (!inpointer)
                    usedbytes+=2;
                inpointer=true;
                continue;
            }
            count=*pname++;
//            printf("Count = %d\n", count);
            if (!inpointer)
                usedbytes+=count+1;;
            while (count--) {
//                printf("found char %c\n", *pname);
                *dest++=*pname++;
            }
            if (*pname) {
//                printf("add a dot\n");
                *dest++='.';
            } else {
                if (!inpointer) {
                    usedbytes++;
                }
            }
        }
        *dest=0;
//        printf("\n--used bytes=%d--\n", usedbytes);
        if (!usedbytes) {
            usedbytes=1;
        }
        return usedbytes;
    };

    const char * getRRTypeName(unsigned short t)
    {
        switch (t) {
            case A: return "A";
            case NS: return "NS";
            case MD: return "MD";
            case MF: return "MF";
            case CNAME: return "CNAME";
            case SOA: return "SOA";
            case MB: return "MB";
            case MG: return "MG";
            case MR: return "MR";
            case NULLRR: return "NULL";
            case WKS: return "WKS";
            case PTR: return "PTR";
            case HINFO: return "HINFO";
            case MINFO: return "MINFO";
            case MX: return "MX";
            case TXT: return "TXT";
        }
        return "UNKNOWN";
    };

    const char * getRRClassName(unsigned short c)
    {
        switch (c) {
            case CLASSIN: return "IN";
            case CLASSCS: return "CS";
            case CLASSCH: return "CH";
            case CLASSHS: return "HS";
        }
        return "UNKNOWN";
    }


public:
    DNSRequest(const char * name) : query(name)
    {
        printf("DNSRequest(\"%s\")\n",name);
        setTimeOut(10000);
//        connectUDP("216.239.32.10","53");
//        connectUDP("202.99.104.68","53");
        connectUDP("202.99.96.68","53");
//        connectUDP("86.109.98.97","53");
    };

    virtual ~DNSRequest()
    {
        printf("~DNSRequest()\n");
    };

};


int
main (int argc, char **argv)
{
#if 1
    DNSRequest dnsreq("google.com");

    NSocket::NSocket_mainloop();
#else
    HKEY regkey;
    LONG result;
    result=RegOpenKeyEx(HKEY_LOCAL_MACHINE,TEXT("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"),0,KEY_READ, &regkey);
    if (result != ERROR_SUCCESS) {
        printf("Error opening key\n");
        return(1);
    }

    BYTE buf[2048];
    DWORD buflen=2048;
    DWORD type;
    result=RegQueryValueEx(regkey,TEXT("NameServer"),NULL,&type,buf,&buflen);
    if ( result != ERROR_SUCCESS ) {
        printf("Error quering value\n");
        RegCloseKey(regkey);
        RegCloseKey(HKEY_LOCAL_MACHINE);
        return 1;
    }
    if (buflen>1)
        printf("Value= '%s', size=%lu, type=%lu\n", buf,buflen,type);
    DWORD kindex;
    CHAR kclass[2048];
    DWORD kclasslength;

    HKEY enumkey;
    result=RegOpenKeyEx(regkey, TEXT("Interfaces"),0,KEY_READ, &enumkey);
    if (result != ERROR_SUCCESS) {
        printf("error opening Interfaces for enumeration\n");
        RegCloseKey(regkey);
        RegCloseKey(HKEY_LOCAL_MACHINE);
        return 1;
    }

    kindex=0;
    while (1) {
        buflen=1024;
        kclasslength=1024;
        result=RegEnumKeyEx(enumkey,kindex++,(CHAR *)buf,&buflen,NULL,kclass,&kclasslength,NULL);
        if (result == ERROR_NO_MORE_ITEMS) {
            printf("End of keys\n");
            break;
        } else if ( result == ERROR_SUCCESS) {
            //printf("SUBKEY= '%s', size= %lu, CLASS= '%s', size= %lu\n", buf, buflen, kclass, kclasslength);
            HKEY interfacekey;
            result=RegOpenKeyEx(enumkey, (const CHAR *)buf, 0, KEY_READ, &interfacekey);
            if ( result == ERROR_SUCCESS ) {
                buflen=1024;
                result=RegQueryValueEx(interfacekey,TEXT("NameServer"),NULL,&type,buf,&buflen);
                if ( result == ERROR_SUCCESS) {
                    if (buflen<=1)
                        continue;
                    printf("NameServer= '%s', size=%lu, type=%lu\n", buf,buflen,type);
                    #define NSADDRSIZE 256
                    unsigned int nsap;
                    unsigned int bpos;
                    char nsaddr[NSADDRSIZE];
                    for (nsap=0,bpos=0; bpos<buflen; bpos++) {
                        if (buf[bpos] != ' ' && buf[bpos] != ',') {
                            nsaddr[nsap++]=buf[bpos];
                            if (nsap>=NSADDRSIZE) {
                                printf("Name is longer than buffer\n");
                                nsap=0;
                                nsaddr[0]=0;
                            }
                        } else {
                            nsaddr[nsap]=0;
                            printf("NameServer Found: '%s'\n", nsaddr);
                            nsap=0;
                            nsaddr[0]=0;
                        }
                    }
                    if (nsaddr[0] != 0)
                        printf("NameServer Found: '%s'\n", nsaddr);
                }
                RegCloseKey(interfacekey);
            }
        } else {
            printf("Some error happened enumerating\n");
            break;
        }
    }
    RegCloseKey(enumkey);

    RegCloseKey(regkey);
    RegCloseKey(HKEY_LOCAL_MACHINE);
#endif
	return 0;
}





