#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <jdbc/mysql_connection.h>
#include <jdbc/mysql_driver.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <iostream>
#include <vector>

void connection(sf::TcpSocket* socket){

    ///Se inicializa la conexión a la BD PracticaRedesAA4
    sql::Driver* driver = sql::mysql::get_driver_instance();
    sql::Connection* conn = driver->connect("tcp://127.0.0.1","root","linux123");
    conn->setSchema("PracticaRedesAA4");

    sql::Statement* stmt =conn->createStatement();
    sql::ResultSet* resultSet;

    sf::Packet packet;
    std::string username, password;
    bool validName = false;
    do{

        socket->receive(packet);
        packet>>username>>password;

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
    sf::TcpListener dispatcher;
    sf::Socket::Status status = dispatcher.listen(50000);
    std::vector<sf::TcpSocket*> sockets;

    if(status != sf::Socket::Done)
    {
        std::cout<<"error en la creacion del listener";
        return 0;
    }

    while(1){
        sf::TcpSocket* incoming = new sf::TcpSocket;
        if(dispatcher.accept(*incoming) != sf::Socket::Done)
        {
            std::cout<<"error en la connexion";
        }
        else{
            std::cout<<"Conectado"<<std::endl;
            sockets.push_back(incoming);
            sf::Thread conn(&connection, incoming);
            conn.launch();
        }
    }
    dispatcher.close();
    std::cout<<"ejecucion terminada";
}
