#ifndef APPLICATIONSERVER_HANDLER_H
#define APPLICATIONSERVER_HANDLER_H

#include <cstring>
#include "../../Mongoose/mongoose.h"
#include "../../jsoncpp/json/json.h"
#include "../../jsoncpp/json/json-forwards.h"
#include "../Response.h"
#include "../../Exceptions/InvalidRequestException.h"

using namespace std;

class Handler {
public:
    Handler();
    virtual ~Handler();

    Response* handleRequest(http_message* httpMessage, string url);
    virtual Response* handlePostRequest(http_message* httpMessage) = 0;
    virtual Response* handleGetRequest(http_message* httpMessage, string url) = 0;
    virtual Response* handleDeleteRequest(http_message* httpMessage, string url) = 0;
    virtual Response* handlePutRequest(http_message* httpMessage, string url) = 0;

protected:
    Response* getNotImplementedResponse();
    Response* getSuccessfulResponse();
    Response* getBadRequestResponse(string errorDetails);
    Response* getNotFoundResponse(string errorDetails);
    Json::Value parseBody(string body);
    vector<string> parseUrl(string url);
    int getUserIdFromUrl(string url);

private:
    int isEqual(const struct mg_str *s1, const struct mg_str *s2);
    string getSubUrl(string url);
};

#endif //APPLICATIONSERVER_HANDLER_H
