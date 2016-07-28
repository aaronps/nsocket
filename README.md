# nsocket
Unfinished simple event driven socket library (c++)

_Currently didn't check if it compiles._

This is a small library for using sockets I began to do long time ago (2006). The code was originally hosted on googlecode, after it shutdown I didn't move it until the last moment, so the real history is lost.

The code was going through changes on the API and was called nsocket2 internally, there were no mor changes on this code, but I later began again and made nsocket3, which will upload to github sortly.

The code includes a mini dns query program, which I used for tests. The idea behind the mini dns was to query servers directly as to be asynchronous... I think didn't finish it.

The code includes some comments which seemed interesting at the time, and reading them now, still seems interesting, but unknown if still applies:

```c++
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
```

There is also some code that seems smart at that time but it isn't so now, for example, in Windows, winsock was initialized on a global static variable, which can cause a problem on some projects due to the "static initialization order is undefined"...

## History before first commit on googlecode
_copied from googlecode archive_

| Author  | Date         | Commit | Message |
|---------|--------------|--------|---------|
| aaronps | Oct 18, 2010 |   19   | commit test |
| aaronps | Oct 18, 2010 |   18   | commit test |
| aaronps | Oct 18, 2010 |   17   | commit test |
| aaronps | Oct 5, 2008  |   16   | - Ignore private project folder |
| aaronps | Oct 5, 2008  |   15   | - Added netbeans project - Start modifications of the class |
| aaronps | Apr 20, 2007 |   14   | Created wiki page through web user interface. |
| aaronps | Mar 8, 2007  |   13   | broken commit to switch develpment to linux |
| aaronps | Nov 22, 2006 |   12   | Added NSocket_facade.h to make NSocket.h more readable |
| aaronps | Nov 21, 2006 |   11   | some changes i did some time ago and cant remember clearly :) |
| aaronps | Nov 14, 2006 |   10   | minidsnq in windows do a test to discover the nameservers from the registry |
| aaronps | Nov 13, 2006 |    9   | fixed compiling on windows and fixed minidnsq.cpp |
| aaronps | Nov 13, 2006 |    8   | some fixes and added minidnsq.cpp a test file |
| aaronps | Nov 10, 2006 |    7   | added more timeouts, fixed some problems, still some remaining |
| aaronps | Nov 9, 2006  |    6   | fixed windows compiling |
| aaronps | Nov 9, 2006  |    5   | Fixed the loop on windows, still some problems to fix |
| aaronps | Nov 9, 2006  |    4   | Added simple timeout |
| aaronps | Nov 9, 2006  |    3   | Use std::list again, compile in linux, add empty file NTimer.h for later |
| aaronps | Nov 1, 2006  |    2   | first version |
|         | Nov 1, 2006  |    1   | Initial directory structure. |
