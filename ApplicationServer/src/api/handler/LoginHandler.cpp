//
// Created by milena on 02/10/16.
//

#include "LoginHandler.h"
#include "../../person/PersonManager.h"
#include "../../session/SessionManager.h"
#include "../../Exceptions/InvalidPasswordException.h"

const string LoginHandler::TOKEN_KEY = "token";
const string LoginHandler::USER_ID_KEY = "user_id";

Response *LoginHandler::handlePostRequest(http_message *httpMessage) {
    string user_password, user_mail;
    Json::Value body;
    SessionManager* sessionManager = nullptr;
    std::string user_token;


    try {
        body = this->parseBody(string(httpMessage->body.p));

        //Validations

        if (!body.isMember("email")) {
            throw InvalidRequestException("Mail member is missing");
        }
        if (!body.isMember("password")) {
            throw InvalidRequestException("Password member is missing");
        }
    } catch (InvalidRequestException &e) {
        Response* response = new Response();
        response->setBadRequestHeader();
        response->setErrorBody(e.getMessage());
        return response;
    }

    user_mail = body["email"].asString();
    user_password = body["password"].asString();

    try {
        sessionManager = new SessionManager(this->db);
        user_token = sessionManager->login(user_mail, user_password);

    } catch (InvalidPasswordException& e) {

        Response *response = new Response();
        response->setUnauthorizedHeader();
        response->setErrorBody(e.what());
        delete sessionManager;
        return response;
    } catch (UserNotFoundException& e) {
        Response *response = new Response();
        response->setNotFoundHeader();
        response->setErrorBody(e.getMessage(UserNotFoundException::Message::mail));
        //TODO no tiene sentido que repita logica porque los mensajes se manejan distintos para este tipo de excepción, MEjorar.
        delete sessionManager;
        return response;
    }

    Json::Value responseBody;
    responseBody[TOKEN_KEY] = user_token; //TODO: ASK FOR DTO.
    responseBody[USER_ID_KEY] = sessionManager->getUserId(user_token); //TODO: NO SIENTO QUE SEA NECESARIO ESTO
    Response* response = new Response();
    response->setSuccessfulHeader();
    response->setBody(responseBody.toStyledString());

    delete sessionManager;
    return response;

}

Response* LoginHandler::handleGetRequest(http_message* httpMessage, string url) {
    return this->getNotImplementedResponse();
}

Response* LoginHandler::handleDeleteRequest(http_message* httpMessage, string url) {
    return this->getNotImplementedResponse();
}

Response* LoginHandler::handlePutRequest(http_message* httpMessage, string url) {
    return this->getNotImplementedResponse();
}

