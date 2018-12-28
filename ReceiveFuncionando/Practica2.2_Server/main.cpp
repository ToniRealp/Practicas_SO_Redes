#include <iostream>
#include <unistd.h>
#include <thread>
#include <SFML/Network.hpp>
#include <jdbc/mysql_connection.h>
#include <jdbc/mysql_driver.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>

void receive(sf::TcpSocket* socket){
    std::size_t received;
    while(true){
        char* data = new char[100];
        if(socket->receive(data, 100, received)!=sf::Socket::Done){
            std::cout<<"el cliente se ha desconectado"<<std::endl;
            break;
        }
        std::cout<<data<<std::endl;
        if(data=="exit"){
            std::cout<<"connexion finalizada correctamente"<<std::endl;
            break;
        }
    }
}
void connection(sf::TcpSocket* socket){
    std::cout<<socket->isBlocking()<<std::endl;
    ///Se inicializa la conexión a la BD PracticaRedesAA4
    sql::Driver* driver = sql::mysql::get_driver_instance();
    sql::Connection* conn = driver->connect("tcp://127.0.0.1","root","linux123");
    conn->setSchema("PracticaRedesAA4");

    sql::Statement* stmt =conn->createStatement();
    sql::ResultSet* resultSet;


    std::string username, password, test;
    bool validName = false;
    do{
        sf::Packet packet;
        std::cout<<"esperando packet"<<std::endl;
        if(socket->receive(packet)!=sf::Socket::Done)
        {
            std::cout<<"el cliente se ha desconectado"<<std::endl;
            break;
        }
        else
        {
            packet>>username>>password;
            std::cout<<"packet recibido"<<std::endl;
            std::cout<<username<<"  "<<password<<std::endl;
        }


        if(username!="nuevo")
        {
            resultSet = stmt->executeQuery("SELECT * FROM Player WHERE Player.Username = '"+ username + "'" "AND Player.Password = '"+ password + "'");
                //Si existe algun usuario con esa combinación de Username y Password se inicia sesión en esa cuenta
            if(resultSet->next())
            {
                validName=true;
            }
            else
            {
                validName=false;
            }

             packet<<validName;
             socket->send(packet);
        }

    }while(validName);
}

int main()
{
    ///Se inicializan los sockets
    sf::TcpListener listener;
    sf::Socket::Status status = listener.listen(50000);
    std::vector<sf::TcpSocket*> sockets;

    if(status != sf::Socket::Done)
    {
        std::cout<<"error en la creacion del listener";
        listener.close();
        return 0;
    }

    while(1){
        sf::TcpSocket* incoming = new sf::TcpSocket;
        if(listener.accept(*incoming) != sf::Socket::Done)
        {
            std::cout<<"error en la connexion";
        }
        else{
            std::cout<<"Conectado"<<std::endl;
            sockets.push_back(incoming);
            std::Thread conn(&connection, incoming);
            conn.detach();
        }
    }
    listener.close();
    return 0;
}
