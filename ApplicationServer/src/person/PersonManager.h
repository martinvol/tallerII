#ifndef PROJECT_PERSONMANAGER_H
#define PROJECT_PERSONMANAGER_H

#include <leveldb/db.h>
#include "Person.h"
#include "../Exceptions/UserNotFoundException.h"

class PersonManager {

public:
    PersonManager();
    virtual ~PersonManager();

    Person* getPersonById(int id);
    void savePerson(Json::Value person_json);

private:
    leveldb::DB* db;
    Person* getFakePerson1();
    Person* getFakePerson2();

    long uniqueId;

    Person *getPersonById(long id);
};

#endif //PROJECT_PERSONMANAGER_H
