#include <string>
#include <iostream>
#include <thread>
#include <dirent.h>
#include "gtest/gtest.h"
#include "../Mongoose/mongoose.h"
#include "../api/WebHandler.h"
#include "../logger/Logger.h"
#include "../tools/DbBuilder.h"
#include "../tools/MainHelper.h"

static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;
static int s_sig_num = 0;
static string configFile = "../ApplicationServer/src/tests/config.js";

struct mbuf processMessage(struct mg_connection *nc, struct http_message *httpMessage, WebHandler *webHandler) {
    Response * response = webHandler->handleRequest(httpMessage);
    struct mbuf body;
    body.buf = (char*) response->getBody();
    body.len = response->getBodyLength();
    mg_printf(nc, "%s", response->getHeader());
    if (response->hasBinaryContent) {
        mg_send_http_chunk(nc, body.buf, body.len);
    } else {
        mg_printf_http_chunk(nc, body.buf, body.len);
    }
    mg_send_http_chunk(nc, "", 0);
    delete response;
    return body;
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_REQUEST) {
        WebHandler *webHandler = new WebHandler();
        struct http_message *httpMessage = (struct http_message *) ev_data;
        try {
            struct mbuf body = processMessage(nc, httpMessage, webHandler);
            MainHelper* mainHelper = new MainHelper();
            if (!mainHelper->validBody(body, nc->send_mbuf)) {
                mbuf_remove(&nc->send_mbuf, nc->send_mbuf.len);
                processMessage(nc, httpMessage, webHandler);
            }
            delete mainHelper;
        } catch (SpecialRequestException &e) {
            s_http_server_opts.document_root = "/usr/appServer/img";
            mg_serve_http(nc, (struct http_message *) ev_data, s_http_server_opts);
        }
        delete webHandler;
    }
}

void runAppServer() {
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
    while (s_sig_num == 0) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);
}

void loadDB() {
    DbBuilder* dbb = new DbBuilder();
    dbb->loadUsers();
    delete dbb;
}

void deleteDB(string dbName) {
    string removeDataBaseCommand = "rm -rf " + dbName;
    system(removeDataBaseCommand.c_str());
}

void reloadImages() {
    string command = "mkdir -p " + Config::getInstance()->get(Config::IMG_FOLDER);
    int result = system(command.c_str());
    if (result != 0) {
        ASSERT_TRUE(false) << "No se ha podido crear el directorio para cargar las imagenes de prueba";
    }
    command = "cp ../ApplicationServer/src/tests/img/* " + Config::getInstance()->get(Config::IMG_FOLDER);
    result = system(command.c_str());
    if (result != 0) {
        ASSERT_TRUE(false) << "No se han podido cargar las imagenes de prueba";
    }
}

void runTest(string test, string dbName, bool shouldReloadImages) {
    loadDB();
    if (shouldReloadImages) {
        reloadImages();
    }
    int result = system(test.c_str());
    if (result != 0) {
        ASSERT_TRUE(false) << "Fallo el test: " + test;
    }
    deleteDB(dbName);
}


TEST(Testing, Api) {

    if (FILE *file = fopen(configFile.c_str(), "r")) {
        fclose(file);
    } else {
        ASSERT_TRUE(false) << "No se pudo cargar el archivo de configuracion";
        return;
    }
    Config::getInstance()->load(configFile);
    string dbName = Config::getInstance()->get(Config::NAME_DB);
    if (dbName == "") {
        ASSERT_TRUE(false) << "Nombre de base de datos invalido";
        return;
    }
    std::thread t1(runAppServer);
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir("../ApplicationServer/src/tests/apitests")) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            string fileName(ent->d_name);
            if (fileName.find("testing_") != string::npos) {
                string test = "resttest.py http://127.0.0.1:8000 ../ApplicationServer/src/tests/apitests/" + fileName;
                bool shouldReloadImages = false;
                if (fileName.find("picture") != string::npos) {
                    shouldReloadImages = true;
                }
                runTest(test, dbName, shouldReloadImages);
            }
        }
        closedir (dir);
    }
    s_sig_num = 1;
    t1.join();
    delete Logger::getInstance();
    delete Config::getInstance();
}
