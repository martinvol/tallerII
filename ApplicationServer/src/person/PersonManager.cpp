#include "PersonManager.h"
#include "../Exceptions/UserAlreadyExistsException.h"

PersonManager::PersonManager() {
    db = new DBWrapper();
    DBWrapper::ResponseCode status = db->openDb();
    if (status == DBWrapper::ResponseCode::ERROR) {
        throw exception();
    }

}

PersonManager::~PersonManager() {
    db->deleteDB();
    db->destroyDB();

}

Person* PersonManager::getPersonById(int id) {
    //FIXME: sacar una vez que este la base de datos
    switch (id) {
        case 1:
            return this->getFakePerson1();
        case 2:
            return this->getFakePerson2();
    }
    throw UserNotFoundException(id);
}

//FIXME: sacar una vez que este la base de datos
Person* PersonManager::getFakePerson1() {
    Person* person = new Person();
    person->setId(1);
    person->setFirstName("John");
    person->setLastName("Doe");
    person->setEmail("John@doe.com");
    person->setDateOfBirth("01/01/1990");
    person->setCity("Buenos Aires");
    person->setSummary("Hi, this is John Doe");
    person->setLocation(-58.368368, -34.617589);

    WorkHistory* workHistory1 = new WorkHistory();
    workHistory1->setCompany("IBM");
    workHistory1->setPositionTitle("JavaScript Developer");
    workHistory1->setFromDate("10/2012");
    workHistory1->setToDate("11/2014");
    person->addWorkHistory(workHistory1);

    WorkHistory* workHistory2 = new WorkHistory();
    workHistory2->setCompany("Amazon");
    workHistory2->setPositionTitle("Project Manager");
    workHistory2->setFromDate("12/2014");
    workHistory2->setToDate("");
    person->addWorkHistory(workHistory2);

    Skill* skill1 = new Skill();
    skill1->setName("JavaScript");
    skill1->setDescription("JavaScrip programming language");
    skill1->setCategory("software");
    person->addSkill(skill1);

    Skill* skill2 = new Skill();
    skill2->setName("PM");
    skill2->setDescription("Project Management");
    skill2->setCategory("management");
    person->addSkill(skill2);

    return person;
}

//FIXME: sacar una vez que este la base de datos
Person* PersonManager::getFakePerson2() {
    Person* person = new Person();
    person->setId(2);
    person->setFirstName("Jane");
    person->setLastName("Doe");
    person->setEmail("Jane@doe.com");
    person->setDateOfBirth("01/11/1991");
    person->setCity("Madrid");
    person->setSummary("Hi, this is Jane Doe");
    person->setLocation(-58.368368, -34.617589);

    WorkHistory* workHistory1 = new WorkHistory();
    workHistory1->setCompany("Microsoft");
    workHistory1->setPositionTitle("QA");
    workHistory1->setFromDate("10/2013");
    workHistory1->setToDate("11/2015");
    person->addWorkHistory(workHistory1);

    WorkHistory* workHistory2 = new WorkHistory();
    workHistory2->setCompany("E-bay");
    workHistory2->setPositionTitle("Php Developer");
    workHistory2->setFromDate("12/2015");
    workHistory2->setToDate("");
    person->addWorkHistory(workHistory2);

    Skill* skill1 = new Skill();
    skill1->setName("Php");
    skill1->setDescription("Php programming language");
    skill1->setCategory("software");
    person->addSkill(skill1);

    Skill* skill2 = new Skill();
    skill2->setName("QA");
    skill2->setDescription("Software Quality Assurance");
    skill2->setCategory("software");
    person->addSkill(skill2);

    return person;
}

long PersonManager::savePerson(Json::Value person_json) {
    std::string user_mail, user_information, output;
    long user_id;
    Json::FastWriter fastWriter;

    if (!person_json.isMember("email")) throw InvalidRequestException("Missing email");
    //if (!person_json.isMember("password")) throw InvalidRequestException("Missing password");

    user_mail=  person_json["email"].asString();
    user_id = person_json["id"].asLargestInt();

    if (user_id == 0) {

        if (db->existsKey("user_" + user_mail, &user_information )) {
            //Ya existe un usuario con dicho mail
            throw  UserAlreadyExistsException();
        } else {

            uniqueId++;
            output = fastWriter.write(person_json);
            db->puTKey("user_" + user_mail, &output);
            db->puTKey("user_" + std::to_string(uniqueId), &user_mail);
            return uniqueId;
            //db->Put(leveldb::WriteOptions(), "user_" + user_mail, output);
            //db->Put(leveldb::WriteOptions(), "user_" + std::to_string(uniqueId), user_mail );
        }
    } else {
        //The person already exists in the system and it wants to refresh his information
        output = fastWriter.write(person_json);
        db->Put(leveldb::WriteOptions(), "user_" + user_mail, output);
        return uniqueId;
    }
}


void PersonManager::deletePerson(long id) {

    std::string user_mail, user_information;

    if (!db->existsKey("user_"+std::to_string(id), &user_mail)) {
        throw UserNotFoundException(UserNotFoundException::Message::id);
    }

    db->deleteKey("user_" + std::to_string(id));

    if (!db->existsKey("user_" + user_mail, &user_information)) {
        throw  UserNotFoundException(UserNotFoundException::Message::mail);
    }

    db->deleteKey("user_" + user_mail);
}


Person* PersonManager::getPersonById(long id) {

    std::string  user_mail, user;
    Json::Reader reader;
    Json::Value json_user;

    if (db->existsKey("user_" + std::to_string(id), &user_mail)) {
        try {
            return getPersonByMail(&user_mail);
        } catch (UserNotFoundException& exception1) {
            std::exception();
        }

        reader.parse( user.c_str(), json_user );
        return new Person(json_user);

    } else {
        //No se encontro el usuario
        throw UserNotFoundException(UserNotFoundException::Message::id);
    }

}

Person* PersonManager::getPersonByMail(std::string* user_mail) {
    std::string result;
    Json::Value json_user;
    Json::Reader reader;

    if (!db->existsKey("user_" + *user_mail, &result)) {
        throw UserNotFoundException(UserNotFoundException::Message::mail);
    }

    reader.parse( result.c_str(), json_user );
    return new Person(json_user);
}

//TODO: SEE IF IT WORKS
vector<Person*> PersonManager::getPersonFriendsById(long id) {
    std::string  user_mail, user, friend_user;
    Json::Reader reader;
    Json::Value json_friends;
    std::string friends_result;
    vector<Person*> friends;



    if (!db->existsKey("user_" + std::to_string(id), &user_mail)) {
        throw UserNotFoundException(UserNotFoundException::Message::id);
    }

    if (!db->existsKey("user_" + user_mail, &user)) {
        throw std::exception();
    }

    db->getKey( "user_friends_" + std::to_string(id), &friends_result);
    //Friends result has de id of every friend of the user_id
    reader.parse(friends_result.c_str(), json_friends);
    const Json::Value jfriends = json_friends["friends"];
    for (int index = 0; index < jfriends.size(); index++) {
        std::string friend_id = jfriends[index].asString();

        friends.push_back(this->getPersonById(std::stol(friend_id)));
    }
    return friends;
}

/*
bool PersonManager::userExists(long id, std::string* result) {
    std::string user_id = "user_" + std::to_string(id);
    return !(db->Get(leveldb::ReadOptions(), user_id, result)).IsNotFound();
}


bool PersonManager::userExists(std::string* user_mail, std::string* result) {
    std::string user_id = "user_" + *user_mail;
    return !(db->Get(leveldb::ReadOptions(), user_id, result)).IsNotFound();
}
*/
