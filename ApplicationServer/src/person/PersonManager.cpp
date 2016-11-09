#include "PersonManager.h"
#include "../Exceptions/UserAlreadyExistsException.h"
#include "../Exceptions/BadImplementationException.h"

#include <chrono>
#include <memory>
#include <algorithm>
#include <regex>
#include <set>

#define USER_MAIL_ID "user:mail_"
#define USER_UUID_ID "user:uuid_"
#define USER_PASSWORD "user:password_"
#define USER_NAME_ID "user:name_"
#define USER_SKILL "user:skill_"

PersonManager::PersonManager(DBWrapper *db) {
    this->db = db;
}

PersonManager::~PersonManager() {}

void PersonManager::updateName(string user_newName, string user_oldName, string user_mail) {
    if (user_oldName.compare(user_newName) != 0) {
        db->deleteKey(USER_NAME_ID + user_oldName + "_" + user_mail);
        db->puTKey(USER_NAME_ID + user_newName + "_" + user_mail, &user_mail);
    }
}

void PersonManager::updateSkills(vector<Skill *> new_skills, vector<Skill *> old_skills, string user_mail) {

    for (int i = 0; i < new_skills.size(); i++) {
        bool skill_exists = false;
        std::string user_newSkill = new_skills[i]->getName();
        std::transform(user_newSkill.begin(), user_newSkill.end(), user_newSkill.begin(), ::tolower);
        for (int i2 = 0; i2 < old_skills.size(); i2++) {
            if (old_skills[i2]->getName().compare(user_newSkill) == 0) {
                skill_exists = true;
                break;
            }
        }
        if (!skill_exists) saveSkill(user_newSkill, user_mail);
    }
    for (int j = 0; j < old_skills.size(); j++) {

        bool skill_exists = false;
        for (int j2 = 0; j2 < new_skills.size(); j2 ++) {
            std::string skill_name = new_skills[j]->getName();
            if (old_skills[j]->getName().compare(skill_name) == 0) {
                skill_exists = true;
                break;
            }
        }
        if (!skill_exists) deleteUserFromSkill(old_skills[j]->getName(), user_mail);

    }

}

long PersonManager::updateUser(Json::Value juser_new_information) {
    //The person already exists in the system and it wants to refresh his information
    Person* new_user = new Person(juser_new_information);
    std::string user_mail = new_user->getEmail();

    Person* old_user = getUserByMail(&user_mail);

    //Actualizar nombre completo de usuario;
    updateName(new_user->getFullName(), old_user->getFullName(), user_mail);
    updateSkills(new_user->getSkills(), old_user->getSkills(), user_mail);

    //Actualizar skill
    updateSkills(new_user->getSkills(), old_user->getSkills(), user_mail);

    //No olvidar el numero de recomendaciones
    new_user->setTotalRecommendations(old_user->getTotalOfRecommendations());
    Json::FastWriter fastWriter;
    std::string person_string = fastWriter.write(new_user->serializeMe());
    db->puTKey(USER_MAIL_ID + user_mail, &person_string);
    delete new_user;
    return new_user->getId();
}
long PersonManager::savePerson(Json::Value juser_new_information, long forceID) {
    std::string user_mail, user_password, user_name, user_information, person_string;

    Json::FastWriter fastWriter;
    long uniqueId;

    user_mail=  juser_new_information["email"].asString();
    std::transform(user_mail.begin(), user_mail.end(), user_mail.begin(), ::tolower);
    user_name = juser_new_information["first_name"].asString() + "-" + juser_new_information["last_name"].asString();
    std::transform(user_name.begin(), user_name.end(), user_name.begin(), ::tolower);
    user_password = juser_new_information["password"].asString();


    if (db->existsKey(USER_MAIL_ID + user_mail, &user_information )) {
        //Ya existe un usuario con dicho mail
        throw  UserAlreadyExistsException();
    } else {

        if (forceID != -1) {
            //Quiero forzar a que el usuario se guarde con un id que se pase por parámetro
            uniqueId = forceID;
        } else {
            uniqueId = generateID();
        }
        juser_new_information["id"] = uniqueId;
        juser_new_information["tot_recommendations"] = 0;
        std::string password = "password";
        juser_new_information.removeMember(password.c_str());
        person_string = fastWriter.write(juser_new_information);
        std::transform(person_string.begin(), person_string.end(), person_string.begin(), ::tolower);
        db->puTKey(USER_MAIL_ID + user_mail, &person_string);
        db->puTKey(USER_NAME_ID + user_name + "_" + user_mail, &user_mail);
        db->puTKey(USER_UUID_ID + std::to_string(uniqueId), &user_mail);
        db->puTKey(USER_PASSWORD + user_mail, &user_password);

        vector<Skill*>* skills = new vector<Skill*>();
        const Json::Value jSkillVector = juser_new_information["skills"];
        for (int index2 = 0; index2 < jSkillVector.size(); index2++) {
            Skill* skill = new Skill(jSkillVector[index2]);
            skills->push_back(skill);
        }
        saveSkills(*skills, user_mail);

        delete skills;
        return uniqueId;

    }
}

vector<long> * PersonManager::getAllUsersIds() {
    // Get a database iterator
    std::vector<long>* ids = new vector<long>();

    leveldb::Slice startSlice = USER_UUID_ID;
    leveldb::Slice endSlice = USER_MAIL_ID;

    shared_ptr<leveldb::Iterator> iterator(db->newIterator());

    for(iterator->Seek(startSlice); iterator->Valid() && (iterator->key()).ToString().compare(endSlice.ToString()) ; iterator->Next())
    {
        // Read the record
        if( !iterator->value().empty() )
        {
            ids->push_back(std::stol(iterator->key().ToString().replace(0,strlen(USER_UUID_ID),"")));
        }
    }
    return ids;
}

vector<Person *> * PersonManager::searchByName(std::string user_searchName) {
    std::vector<Person*>* users_result = new vector<Person*>();
    leveldb::Slice startSlice = USER_NAME_ID;
    leveldb::Slice endSlice = USER_UUID_ID;
    std::string user_search = user_searchName;
    std::transform(user_search.begin(), user_search.end(), user_search.begin(), ::tolower);
    std::regex e ("(.*)("+user_search+")(.*)");
    std::string suser;
    Json::Value juser;
    shared_ptr<leveldb::Iterator> iterator(db->newIterator());
    for(iterator->Seek(startSlice); iterator->Valid() && (iterator->key()).ToString().compare(endSlice.ToString()) ; iterator->Next())
    {
        // Read the record
        if( !iterator->value().empty() )
        {
            if(regex_match(iterator->key().ToString(), e)) {
                //busqueda del usuario
                std::string user_mail_id = USER_MAIL_ID + iterator->value().ToString();
                if (!db->existsKey(user_mail_id, &suser)) {
                    throw BadImplementationException();
                }

                juser = this->getStringAsJson(suser);
                Person* user = new Person(juser);
                users_result->push_back(user);
            }

        }
    }

    return users_result;

}


void PersonManager::saveSkills(std::vector<Skill *> user_newSkills, string user_mail) {

    for (int index2 = 0; index2 < user_newSkills.size(); index2++) {
        std::string skill_name = user_newSkills[index2]->getName();
        std::transform(skill_name.begin(), skill_name.end(), skill_name.begin(), ::tolower);
        saveSkill(skill_name, user_mail);
    }
}

vector<Person *> * PersonManager::searchBySkill(std::string skill) {
    std::string users_withSkill, user_information;
    std::vector<Person*>* users_result = new vector<Person*>();
    std::transform(skill.begin(), skill.end(), skill.begin(), ::tolower);
    if (!db->existsKey(USER_SKILL + skill, &users_withSkill)) {
         return users_result;
    }

    size_t last = 1; size_t next = 0;
    std::string delimiter = ",";
    while ((next = users_withSkill.find(delimiter, last)) != string::npos) {
        std::string user_mail = users_withSkill.substr(last, next-last);

        if (!db->existsKey(USER_MAIL_ID + user_mail, &user_information)) {
            //El usuario se borro
            //Aprovecho y lo elimino de acá también.
            deleteUserFromSkill(skill, user_mail);

        } else {
            Json::Value juser_information = getStringAsJson(user_information);
            Person* user = new Person(juser_information);
            users_result->push_back(user);
        }



        last = next + 1;
    }



    return users_result;

}



void PersonManager::deletePerson(long id) {

    std::string user_mail, user_information;

    if (!db->existsKey(USER_UUID_ID+std::to_string(id), &user_mail)) {
        throw UserNotFoundException(id);
    }

    db->deleteKey(USER_UUID_ID + std::to_string(id));

    if (!db->existsKey(USER_MAIL_ID + user_mail, &user_information)) {
        throw  BadImplementationException();
    }

    Json::Value juser_information = getStringAsJson(user_information);
    std::string user_name = juser_information["first_name"].asString() + "-" + juser_information["last_name"].asString();
    std::transform(user_name.begin(), user_name.end(), user_name.begin(), ::tolower);
    db->deleteKey(USER_MAIL_ID + user_mail);
    if (!db->existsKey(USER_NAME_ID + user_name + "_" + user_mail, &user_information)) {
        throw BadImplementationException();
    }

    db->deleteKey(USER_NAME_ID + user_name + "_" + user_mail);
    if (!db->existsKey(USER_PASSWORD + user_mail, &user_information)) {
        throw BadImplementationException();
    }

    db->deleteKey(USER_PASSWORD + user_mail);
}


Person* PersonManager::getPersonById(long id) {

    std::string  user_mail;

    if (db->existsKey(USER_UUID_ID + std::to_string(id), &user_mail)) {
        return getUserByMail(&user_mail);
    } else {
        //No se encontro el usuario
        throw UserNotFoundException(id);
    }

}

Person* PersonManager::getUserByMail(std::string *user_mail) {
    std::string result;
    Json::Value json_user;

    if (!db->existsKey(USER_MAIL_ID + *user_mail, &result)) {
        throw UserNotFoundException(*user_mail);
    }

    json_user = this->getStringAsJson(result);
    std::string password = "password";
    json_user.removeMember(password.c_str()); //TODO a lo wacho
    return new Person(json_user);
}

vector<Contact*> PersonManager::getContactsByUserId(long id) {
    if (!this->userExists(id)) {
        throw UserNotFoundException(id);
    }
    RelationsManager *relationsManager = new RelationsManager(this->db);
    vector<Contact*> contacts = relationsManager->getContactsByUserId(id);
    delete relationsManager;
    return contacts;
}

void PersonManager::saveOrUpdateRelation(Json::Value relation) {
    long authorId = relation["author_id"].asLargestInt();
    long contactId = relation["contact_id"].asLargestInt();
    if (!this->userExists(authorId)) throw UserNotFoundException(authorId);
    if (!this->userExists(contactId)) throw UserNotFoundException(contactId);
    if (authorId == contactId) throw InvalidRequestException("Author user and contact user cannot be equals");

    string action = relation["action"].asString();

    RelationsManager* relationsManager = new RelationsManager(this->db);
    relationsManager->saveRelation(authorId, contactId, action);
    delete relationsManager;
}

void PersonManager::saveRecommendation(Json::Value recommendation) {
    long fromUserId = recommendation["from"].asLargestInt();
    long toUserId = recommendation["to"].asLargestInt();
    this->validateUsersOfRequest(fromUserId, toUserId);
    RecommendationsManager* recommendationsManager = new RecommendationsManager(this->db);
    recommendationsManager->addRecommendation(fromUserId, toUserId);
    delete recommendationsManager;
}

Json::Value PersonManager::getRecommendationsByUserId(long userId) {
    RecommendationsManager* recommendationsManager = new RecommendationsManager(this->db);
    Json::Value recommendations = recommendationsManager->getRecommendationsAsJson(userId);
    delete recommendationsManager;
    return recommendations;
}

void PersonManager::removeRecommendation(long fromUserId, long toUserId) {
    this->validateUsersOfRequest(fromUserId, toUserId);
    RecommendationsManager* recommendationsManager = new RecommendationsManager(this->db);
    recommendationsManager->removeRecommendation(fromUserId, toUserId);
    delete recommendationsManager;
}

string PersonManager::saveMessage(Json::Value request) {
    long fromUserId = request["from"].asLargestInt();
    long toUserId = request["to"].asLargestInt();
    string messageToSave = request["message"].asString();
    this->validateUsersOfRequest(fromUserId, toUserId);
    MessagesManager* messagesManager = new MessagesManager(this->db);
    string savedMessage = messagesManager->saveMessage(fromUserId, toUserId, messageToSave);
    delete messagesManager;
    return savedMessage;
}

vector<Message*> PersonManager::getMessages(long fromUserId, long toUserId) {
    this->validateUsersOfRequest(fromUserId, toUserId);
    MessagesManager* messagesManager = new MessagesManager(this->db);
    vector<Message*> messages = messagesManager->getMessages(fromUserId, toUserId);
    delete messagesManager;
    return messages;
}

void PersonManager::validateUsersOfRequest(long fromUserId, long toUserId) {
    if (!this->userExists(fromUserId)) throw UserNotFoundException(fromUserId);
    if (!this->userExists(toUserId)) throw UserNotFoundException(toUserId);
    if (fromUserId == toUserId) throw InvalidRequestException("From user and to user cannot be equals");
}

long PersonManager::generateID() {
    std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::system_clock::now().time_since_epoch() );
    srand ((unsigned int)time(NULL));
    int rand = std::rand();
    return  labs(ms.count() << 3 + rand % 100);
}

bool PersonManager::userExists(long userId) {
    std::string result;
    return (db->existsKey(USER_UUID_ID + to_string(userId), &result));
}

string PersonManager::getNotificationTokenByUserId(long userId) {
    if (!this->userExists(userId)) throw UserNotFoundException(userId);
    NotificationTokenManager* notificationTokenManager = new NotificationTokenManager(this->db);
    string token = notificationTokenManager->getTokenByUserId(userId);
    delete notificationTokenManager;
    return token;
}

void PersonManager::setOrUpdateNotificationToken(Json::Value request, long userId) {
    if (!request.isMember(NotificationTokenManager::TOKEN_KEY)) throw InvalidRequestException("Missing token");
    if (!this->userExists(userId)) throw UserNotFoundException(userId);
    string token = request[NotificationTokenManager::TOKEN_KEY].asString();
    NotificationTokenManager* notificationTokenManager = new NotificationTokenManager(this->db);
    notificationTokenManager->setOrUpdateToken(userId, token);
    delete notificationTokenManager;
}

Json::Value PersonManager::getStringAsJson(std::string svalue) {
    Json::Reader reader;
    Json::Value jvalue;
    reader.parse(svalue.c_str(), jvalue);
    return  jvalue;
}

void PersonManager::saveSkill(string skill_name, string user_mail) {
    string users_mail;
    if (db->existsKey(USER_SKILL + skill_name, &users_mail)) {

        users_mail += user_mail;
        users_mail += ",";
    } else {
        users_mail = ","+user_mail +",";
    }
    db->puTKey(USER_SKILL + skill_name, &users_mail);

}

void PersonManager::deleteUserFromSkill(string skill_name, string user_mail) {
    std::string output;
    if (!db->existsKey(USER_SKILL + skill_name, &output)) {
        throw BadImplementationException();
    } else {
        regex reg("(.*)," + user_mail +"(.*)");
        output = regex_replace(output, reg, "$1$2");
        db->puTKey(USER_SKILL + skill_name, &output);
    }
}

vector<Person *> *PersonManager::searchByJobPosition(string job_position) {
    return nullptr;
}

vector<Person *> *PersonManager::searchByMail(string user_mail) {
    std::vector<Person*>* users_result = new vector<Person*>();
    leveldb::Slice startSlice = USER_MAIL_ID;
    leveldb::Slice endSlice = USER_NAME_ID;
    std::string user_search = user_mail;
    std::transform(user_search.begin(), user_search.end(), user_search.begin(), ::tolower);
    std::regex e ("(.*)("+user_search+")(.*)");
    Json::Value juser;
    shared_ptr<leveldb::Iterator> iterator(db->newIterator());
    for(iterator->Seek(startSlice); iterator->Valid() && (iterator->key()).ToString().compare(endSlice.ToString()) ; iterator->Next())
    {
        // Read the record
        if( !iterator->value().empty() )
        {
            if(regex_match(iterator->key().ToString(), e)) {
                //busqueda del usuario
                std::string user_information = iterator->value().ToString();
                juser = this->getStringAsJson(user_information);
                Person* user = new Person(juser);
                users_result->push_back(user);
            }

        }
    }

    return users_result;
}


