#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <jdbc/mysql_connection.h>
#include <jdbc/mysql_driver.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <iostream>
#include <vector>

void send(sf::TcpSocket* socket, std::string text){
    while(true){
        std::string data(text);
        if(socket->send(data.c_str(), 100)!=sf::Socket::Done){
            std::cout<<"error en el envio del paquete, el servidor se ha desconectado";
            break;
        }
        if(data=="exit")
            break;
    }
}

std::string receive(sf::TcpSocket* socket){
    std::size_t _size;
    char* data = new char[100];
    if(socket->receive(data, 100, _size)!=sf::Socket::Done){
        std::cout<<"el cliente se ha desconectado"<<std::endl;
    }
    data[_size+1]='\0';
    return std::string(data);
}

void connection(sf::TcpSocket* incoming){

    ///Se inicializa la conexión a la BD PracticaRedesAA4
    sql::Driver* driver = sql::mysql::get_driver_instance();
    sql::Connection* conn = driver->connect("tcp://127.0.0.1","root","linux123");
    conn->setSchema("PracticaRedesAA4");

    sql::Statement* stmt =conn->createStatement();

    sql::ResultSet* resultSet;

    do{
        std::cout<<receive(incoming);
    }while()

    incoming->disconnect();
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
