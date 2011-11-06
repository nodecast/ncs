/****************************************************************************
**   ncs is the backend's server of nodecast
**   Copyright (C) 2010-2011  Frédéric Logier <frederic@logier.org>
**
**   https://github.com/nodecast/ncs
**
**   This program is free software: you can redistribute it and/or modify
**   it under the terms of the GNU Affero General Public License as
**   published by the Free Software Foundation, either version 3 of the
**   License, or (at your option) any later version.
**
**   This program is distributed in the hope that it will be useful,
**   but WITHOUT ANY WARRANTY; without even the implied warranty of
**   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**   GNU Affero General Public License for more details.
**
**   You should have received a copy of the GNU Affero General Public License
**   along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/


#include "http_api.h"


Http_api::Http_api(QxtAbstractWebSessionManager * sm, Nosql &a, QObject * parent): QxtWebSlotService(sm,parent), nosql_(a)
{
    z_message = new zmq::message_t(2);
    m_context = new zmq::context_t(1);
    z_push_api = new zmq::socket_t(*m_context, ZMQ_PUSH);
    z_push_api->bind("tcp://*:5555");
}


Http_api::~Http_api()
{}


bool Http_api::checkAuth(QString header, BSONObjBuilder &payload_builder)
{
    QString b64 = header.section("Basic ", 1 ,1);
    qDebug() << "auth : " << b64;

    QByteArray tmp_auth_decode = QByteArray::fromBase64(b64.toAscii());
    QString auth_decode =  tmp_auth_decode.data();
    QStringList arr_auth =  auth_decode.split(":");

    QString email = arr_auth.at(0);
    QString key = arr_auth.at(1);

    qDebug() << "email : " << email << " key : " << key;

    bo auth = BSON("email" << email.toStdString() << "authentication_token" << key.toStdString());


    bo user = nosql_.Find("users", auth);
    if (user.nFields() == 0)
    {
        qDebug() << "auth failed !";
        return false;
    }

    be login = user.getField("login");
    std::cout << "user : " << login << std::endl;

    payload_builder.append("email", email.toStdString());
    return true;
}




/********** CREATE HOST ************/
void Http_api::host(QxtWebRequestEvent* event, QString action)
{
    qDebug() << "action : " << action << " headers : " << event->headers;
    QString bodyMessage;

    //QString uuid = QUuid::createUuid().toString();

    QUuid uuid = QUuid::createUuid();
    QUuid pub_uuid = QUuid::createUuid();
    QString str_uuid = uuid.toString().mid(1,36);
    QString str_pub_uuid = pub_uuid.toString().mid(1,36);

    //QString uuid = QUuid::createUuid().toString().mid(1,36).toUpper();

    BSONObjBuilder payload_builder;
    payload_builder.append("action", "dispatcher.create");
    payload_builder.append("uuid", str_uuid.toStdString());
    payload_builder.append("pub_uuid", str_pub_uuid.toStdString());

    //std::cout << "payload builder : " << payload_builder.obj() << std::cout;

    bool res = checkAuth(event->headers.value("Authorization"), payload_builder);

    if (!res)
    {
        bodyMessage = buildResponse("error", "auth");
    }
    else
    {
        QxtWebContent *myContent = event->content;

        qDebug() << "Bytes to read: " << myContent->unreadBytes();
        myContent->waitForAllContent();

        QByteArray requestContent = myContent->readAll();
        //qDebug() << "Content: ";
        //qDebug() << requestContent;

        //bo gfs_file_struct;
        bo gfs_file_struct = nosql_.WriteFile(requestContent.data());


        if (gfs_file_struct.nFields() == 0)
        {
            qDebug() << "write on gridFS failed !";
        }
        else
        {
            std::cout << "writefile : " << gfs_file_struct << std::endl;
            //std::cout << "writefile id : " << gfs_file_struct.getField("_id") << " date : " << gfs_file_struct.getField("uploadDate") << std::endl;

        be uploaded_at = gfs_file_struct.getField("uploadDate");

        std::cout << "uploaded : " << uploaded_at << std::endl;


        payload_builder.append("created_at", uploaded_at.date());
        payload_builder.append(gfs_file_struct.getField("_id"));


        bo payload = payload_builder.obj();

        /****** PUSH API PAYLOAD *******/
        qDebug() << "PUSH API PAYLOAD";
        z_message->rebuild(payload.objsize());
        memcpy(z_message->data(), (char*)payload.objdata(), payload.objsize());
        z_push_api->send(*z_message, ZMQ_NOBLOCK);
        /************************/

        }
        bodyMessage = buildResponse("create", str_uuid);
    }
    qDebug() << bodyMessage;


    postEvent(new QxtWebPageEvent(event->sessionID,
                                 event->requestID,
                                 bodyMessage.toUtf8()));
}


/********** UPDATE HOST ************/
void Http_api::host(QxtWebRequestEvent* event, QString action, QString uuid)
{
    qDebug() << "action : " << action << " uuid : " << uuid << " headers : " << event->headers;

/*
     QHash<QString, QString>::const_iterator i = event->headers.constBegin();

     while (i != event->headers.constEnd()) {
         qDebug() << i.key() << ": " << i.value();
         ++i;
     }
*/
    QString bodyMessage;


    BSONObjBuilder payload_builder;
    payload_builder.append("action", "dispatcher.update");
    payload_builder.append("uuid", uuid.toStdString());

    bool res = checkAuth(event->headers.value("Authorization"), payload_builder);

    if (!res)
    {
        bodyMessage = buildResponse("error", "auth");
    }
    else
    {      
        QxtWebContent *myContent = event->content;

        qDebug() << "Bytes to read: " << myContent->unreadBytes();
        myContent->waitForAllContent();

        QByteArray requestContent = QByteArray::fromBase64(myContent->readAll());
        //qDebug() << "Content: ";
        //qDebug() << requestContent;

        //bo gfs_file_struct;
        bo gfs_file_struct = nosql_.WriteFile(requestContent.data());


        if (gfs_file_struct.nFields() == 0)
        {
            qDebug() << "write on gridFS failed !";
        }
        else
        {
            std::cout << "writefile : " << gfs_file_struct << std::endl;
            //std::cout << "writefile id : " << gfs_file_struct.getField("_id") << " date : " << gfs_file_struct.getField("uploadDate") << std::endl;

        be uploaded_at = gfs_file_struct.getField("uploadDate");

        std::cout << "uploaded : " << uploaded_at << std::endl;


        payload_builder.append("created_at", uploaded_at.date());
        payload_builder.append(gfs_file_struct.getField("_id"));


        bo payload = payload_builder.obj();

        /****** PUSH API PAYLOAD *******/
        qDebug() << "PUSH HTTP PAYLOAD";
        z_message->rebuild(payload.objsize());
        memcpy(z_message->data(), (char*)payload.objdata(), payload.objsize());
        z_push_api->send(*z_message, ZMQ_NOBLOCK);
        /************************/

        }
        bodyMessage = buildResponse("update", "proceed");
    }
    qDebug() << bodyMessage;

    postEvent(new QxtWebPageEvent(event->sessionID,
                                 event->requestID,
                                 bodyMessage.toUtf8()));
}


QString Http_api::buildResponse(QString action, QString status)
{
    QString body;
    if (action == "update")
    {
        body.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?><host><status>" + status + "</status></host>");
    }
    else if (action == "create")
    {
        body.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?><host><uuid>" + status + "</uuid></host>");
    }
    else if (action == "error")
    {
        body.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?><host><error>" + status + "</error></host>");
    }

    return body;
}
