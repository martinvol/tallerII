#include <string>
#include <iostream>
#include <thread>
#include "gtest/gtest.h"
#include "../Mongoose/mongoose.h"
#include "../api/WebHandler.h"
#include "../logger/Logger.h"
#include "../tools/DbBuilder.h"

static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;
static int s_sig_num = 0;

bool validBody(struct mbuf body, struct mbuf bodyToSend) {
    bool validBody = true;
    int offset = 7;
    const char* bodyPointer = body.buf + body.len - 1;
    const char* bodyToSendPointer = bodyToSend.buf + bodyToSend.len - 1;
    bodyToSendPointer = bodyToSendPointer - offset;
    for(int i = 0; i < body.len; i++) {
        if (*bodyPointer != *bodyToSendPointer) {
            validBody = false;
            break;
        }
        bodyPointer--;
        bodyToSendPointer--;
    }
    return validBody;
}

struct mbuf processMessage(struct mg_connection *nc, struct http_message *httpMessage, WebHandler *webHandler, Response* response) {
    response = webHandler->handleRequest(httpMessage);
    struct mbuf body;
    body.buf = (char*) response->getBody();
    body.len = response->getBodyLength();
    mg_printf(nc, response->getHeader());
    if (response->hasBinaryContent) {
        mg_send_http_chunk(nc, body.buf, body.len);
    } else {
        mg_printf_http_chunk(nc, body.buf, body.len);
    }
    mg_send_http_chunk(nc, "", 0);
    return body;
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_REQUEST) {
        WebHandler *webHandler = new WebHandler();
        Response* response = new Response();
        struct http_message *httpMessage = (struct http_message *) ev_data;
        struct mbuf body = processMessage(nc, httpMessage, webHandler, response);
        if (!validBody(body, nc->send_mbuf)) {
            Logger::getInstance()->info("Rearmando body de la respuesta");
            mbuf_remove(&nc->send_mbuf, nc->send_mbuf.len);
            processMessage(nc, httpMessage, webHandler, response);
        }
        delete response;
        delete webHandler;
    }
}

int runAppServer() {
    struct mg_mgr mgr;
    struct mg_connection *nc;
    struct mg_bind_opts bind_opts;
    const char *err_str;
    mg_mgr_init(&mgr, NULL);
    memset(&bind_opts, 0, sizeof(bind_opts));
    bind_opts.error_string = &err_str;
    nc = mg_bind_opt(&mgr, s_http_port, ev_handler, bind_opts);
    mg_set_protocol_http_websocket(nc);
    s_http_server_opts.enable_directory_listing = "yes";
    DbBuilder* dbb = new DbBuilder();
    dbb->loadUsers();
    delete dbb;
    while (s_sig_num == 0) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);
}

TEST(Testing, Api) {
    std::thread t1(runAppServer);
    system("resttest.py http://127.0.0.1:8000 ../ApplicationServer/tests/testing_allusers.yaml");
    system("resttest.py http://127.0.0.1:8000 ../ApplicationServer/tests/testing_contacts.yaml");
    system("resttest.py http://127.0.0.1:8000 ../ApplicationServer/tests/testing_messages.yaml");
    system("resttest.py http://127.0.0.1:8000 ../ApplicationServer/tests/testing_notificationtoken.yaml");
    system("resttest.py http://127.0.0.1:8000 ../ApplicationServer/tests/testing_recommendations.yaml");
    system("resttest.py http://127.0.0.1:8000 ../ApplicationServer/tests/testing_users.yaml");
    s_sig_num = 1;
    t1.join();
}
