#pragma once

#include <cstddef>

#define ROOT_FOLDER "Pages"
#define ERR 1
#define TIMEOUT 20
#define MEGABYTE 1048576
#define KEEP 0
#define CLOSE 1
#define BUFFER_SIZE 4096

const size_t MAX_METHOD_LEN = 6;
const size_t MAX_URI_LEN = 8192;
const size_t MAX_HTTP_LEN = 8;
const size_t MAX_FIELD_LEN = 8192;
const size_t MAX_HEADERS = 100;
const size_t MAX_HEADER_SUM = 32768;
const size_t HEADER_SLACK = 16384;
const size_t MAX_CHUNK_LINE = 64;
const size_t MAX_TRAILER_SUM = 8192;

enum e_closeReason {
    CLOSE_CLEAN,          // normal keep-alive end, client done, no error to report
    CLOSE_CLIENT_ERROR,   // bad request, etc. -> send error page first
    CLOSE_SERVER_ERROR,   // 500-type situation -> send error page first
    CLOSE_TRANSPORT_FAIL  // recv/send failed -> socket is dead, don't even try to send
};
