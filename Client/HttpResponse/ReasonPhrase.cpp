#include "HttpResponse.hpp"

void HttpResponse::getReasonPhraseInfo() {
    switch (_statusCode) {
    case (100):
        _reasonPhrase = "Continue";
        break;
    case (101):
        _reasonPhrase = "Switching Protocols";
    }
}
void HttpResponse::getReasonPhraseSuccess() {
    switch (_statusCode) {
    case (200):
        _reasonPhrase = "OK";
        break;
    case (201):
        _reasonPhrase = "Created";
        break;
    case (202):
        _reasonPhrase = "Accepted";
        break;
    case (203):
        _reasonPhrase = "Non-Authoritative Information";
        break;
    case (204):
        _reasonPhrase = "No Content";
        break;
    case (205):
        _reasonPhrase = "Reset Content";
        break;
    case (206):
        _reasonPhrase = "Partial Content";
    }
}
void HttpResponse::getReasonPhraseRedir() {
    switch (_statusCode) {
    case (300):
        _reasonPhrase = "Multiple Choices";
        break;
    case (301):
        _reasonPhrase = "Moved Permanently";
        break;
    case (302):
        _reasonPhrase = "Found";
        break;
    case (303):
        _reasonPhrase = "See Other";
        break;
    case (304):
        _reasonPhrase = "Not Modified";
        break;
    case (305):
        _reasonPhrase = "Use Proxy";
        break;
    case (307):
        _reasonPhrase = "Temporary Redirect";
    }
}
void HttpResponse::getReasonPhraseClientErr() {
    switch (_statusCode) {
    case (400):
        _reasonPhrase = "Bad Request";
        break;
    case (401):
        _reasonPhrase = "Unauthorized";
        break;
    case (402):
        _reasonPhrase = "Payment Required";
        break;
    case (403):
        _reasonPhrase = "Forbidden";
        break;
    case (404):
        _reasonPhrase = "Not Found";
        break;
    case (405):
        _reasonPhrase = "Method Not Allowed";
        break;
    case (406):
        _reasonPhrase = "Not Acceptable";
        break;
    case (407):
        _reasonPhrase = "Proxy Authentication Required";
        break;
    case (408):
        _reasonPhrase = "Request Time-out";
        break;
    case (409):
        _reasonPhrase = "Conflict";
        break;
    case (410):
        _reasonPhrase = "Gone";
        break;
    case (411):
        _reasonPhrase = "Length Required";
        break;
    case (412):
        _reasonPhrase = "Precondition Failed";
        break;
    case (413):
        _reasonPhrase = "Request Entity Too Large";
        break;
    case (414):
        _reasonPhrase = "Request-URI Too Large";
        break;
    case (415):
        _reasonPhrase = "Unsupported Media Type";
        break;
    case (416):
        _reasonPhrase = "Requested range not satisfiable";
        break;
    case (417):
        _reasonPhrase = "Expectation Failed";
    case (431):
        _reasonPhrase = "Request Header Fields Too Large";
    break;
    }
}

void HttpResponse::getReasonPhraseServerErr() {
    switch (_statusCode) {
    case (500):
        _reasonPhrase = "Internal Server Error";
        break;
    case (501):
        _reasonPhrase = "Not Implemented";
        break;
    case (502):
        _reasonPhrase = "Bad Gateway";
        break;
    case (503):
        _reasonPhrase = "Service Unavailable";
        break;
    case (504):
        _reasonPhrase = "Gateway Time-out";
        break;
    case (505):
        _reasonPhrase = "HTTP Version Not Supported";
    }
}

void HttpResponse::getReasonPhrase() {
    switch (_responseClass) {
    case INFO:
        getReasonPhraseInfo();
        break;
    case SUCCESS:
        getReasonPhraseSuccess();
        break;
    case REDIR:
        getReasonPhraseRedir();
        break;
    case CLIENT_ERR:
        getReasonPhraseClientErr();
        break;
    case SERVER_ERR:
        getReasonPhraseServerErr();
    }
}
