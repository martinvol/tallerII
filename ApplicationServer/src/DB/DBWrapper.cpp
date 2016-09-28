//
// Created by milena on 28/09/16.
//

#include "DBWrapper.h"

ResponseCode DBWrapper::openDb() {
    if (db != nullptr) {
        throw std::exception(); //No se debería abrir una base que ya está abierta.
    }

    leveldb::Options options;
    leveldb::Status s;

    options.create_if_missing = true;
    s = leveldb::DB::Open(options, "/tmp/appDB", &db);
    if (!s.ok) {
        return ERROR;
    }
    return OK;
}

DBWrapper::ResponseCode DBWrapper::getKey(std::string key, std::string *output) {
    if (!db) {
        //Primero se tiene que iniciar la base
        throw std::exception();
    }

    leveldb::Status s = db->Get(leveldb::ReadOptions(), key, output);

    if (!s.ok()) {
        return ERROR;
    }

    return OK;
}

DBWrapper::ResponseCode DBWrapper::puTKey(std::string key, std::string *output) {
    if (!db) {
        throw std::exception(); //Se debe primero iniciar la base
    }

    leveldb::Status s;

    s = db->Put(leveldb::WriteOptions(), key, *output );

    if (!s.ok()) {
        return ERROR;
    }

    return OK;
}

DBWrapper::ResponseCode DBWrapper::deleteKey(std::string key) {
    if (!db) {
        throw std::exception(); //Se debe primero iniciar la base
    }

    leveldb::Status s;
    s = db->Delete(leveldb::WriteOptions(), key);

    if (!s.ok()) {
        return ERROR;
    }

    return OK;
}

DBWrapper::ResponseCode DBWrapper::deleteDB() {
    if (!db) {
        return OK;
    }

    delete db;
}

DBWrapper::~DBWrapper() {
    if (db) {
        deleteDB();
    }
}



