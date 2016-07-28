#ifndef _NTIMER_H_
#define _NTIMER_H_

#ifdef _WIN32
  #include <windows.h>
#else
  #include <sys/time.h>
  #include <time.h>
#endif

class NTimer {
public:
    static unsigned long getMiliseconds(void)
    {
#ifdef _WIN32
        return GetTickCount();
#else
        struct timeval tv;
        gettimeofday(&tv,NULL); // no need for checking errors errors, man
        return (tv.tv_sec*1000)+(tv.tv_usec/1000);
#endif
    };
};

class NTimeOut {
public:
    NTimeOut(unsigned long to) : timeout(to), starttime(NTimer::getMiliseconds()){};

    ~NTimeOut(){};

    void reset() { if (timeout) starttime=NTimer::getMiliseconds(); };

    void setTimeOut(unsigned long to) { timeout=to; };

    bool isTimeOut()
    {
        if ( timeout ) {
            unsigned long ms = NTimer::getMiliseconds();
            unsigned long dif;
            if ( ms < starttime ) // for overflow
                dif = ms + ((-1) - starttime);
            else
                dif=ms-starttime;

            if ( dif > timeout )
                return true;
        }
        return false;
    };

protected:
    unsigned long timeout;
    unsigned long starttime;
};

#endif // _NTIMER_H_
