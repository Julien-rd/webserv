#pragma once

#define ROOT_FOLDER "Pages"
#define ERR 1
#define SUCCESS 0
#define TIMEOUT 20
#define MEGABYTE 1048576
#define KEEP 0
#define CLOSE 1

enum e_closeReason {
    CLOSE_CLEAN,          // normal keep-alive end, client done, no error to report
    CLOSE_CLIENT_ERROR,   // bad request, etc. -> send error page first
    CLOSE_SERVER_ERROR,   // 500-type situation -> send error page first
    CLOSE_TRANSPORT_FAIL  // recv/send failed -> socket is dead, don't even try to send
};
