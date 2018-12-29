#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <jdbc/mysql_connection.h>
#include <jdbc/mysql_driver.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <iostream>
#include <vector>
#include <thread>

enum ID {LOGIN,SIGNUP,MAPS};


void connection(sf::TcpSocket* socket){

    ///Se inicializa la conexión a la BD PracticaRedesAA4
    sql::Driver* driver = sql::mysql::get_driver_instance();
    sql::Connection* conn = driver->connect("tcp://127.0.0.1","root","linux123");
    conn->setSchema("PracticaRedesAA4");

    sql::Statement* stmt =conn->createStatement();
    sql::ResultSet* resultSet;

    std::string username, password, playerID;
    int id, coins;

    sf::Packet packet;

    bool validName = false;
    do{
        packet.clear();

        std::cout<<"esperando username"<<std::endl;
        if(socket->receive(packet)!=sf::Socket::Done)
        {
            std::cout<<"el cliente se ha desconectado"<<std::endl;
            break;
        }
        else
        {
            packet>>id>>username>>password;
            std::cout<<"datos de login: "<<username<<" "<<password<<std::endl;
        }

        resultSet = stmt->executeQuery("SELECT * FROM Player WHERE Player.Username = '"+ username + "'" "AND Player.Password = '"+ password + "'");
            //Si existe algun usuario con esa combinación de Username y Password se inicia sesión en esa cuenta

        std::vector<std::string>pokemonNames;
        int pokemonCounter=0;

        if(resultSet->next())
        {
            if(id==LOGIN){
                validName=true;
                playerID = resultSet->getString("ID");
                coins=resultSet->getInt("Coins");
                resultSet = stmt->executeQuery("SELECT * FROM Pokemons WHERE ID IN (SELECT PokemonID FROM PokemonXplayer WHERE PlayerID ="+playerID+")");

                if(resultSet->next()){

                    do{
                        pokemonCounter++;
                        pokemonNames.push_back(resultSet->getString("Name"));

                    }while(resultSet->next());
                }
            }
        }
        else
        {
            if(id==SIGNUP){
                validName=true;
                stmt->executeUpdate("INSERT INTO Player(Player.Username, Player.Password, Player.Coins) VALUES ('"+username+"','"+password+"', 0)");
                resultSet = stmt->executeQuery("SELECT * FROM Player WHERE Player.Username = '"+ username + "'" "AND Player.Password = '"+ password + "'");
                if(resultSet->next())
                     playerID = resultSet->getString("ID");
                coins = 0;
            }
        }
        packet.clear();
        packet<<validName;
        if(validName){
            packet<<coins<<pokemonCounter;
        }

        for(auto x: pokemonNames)
            packet<<x;

        if(socket->send(packet)!= sf::Socket::Done)
        {
            std::cout<<"cliente se ha desconectado"<<std::endl;
            break;
        }

    }while(!validName);

	//Se accede a la BD para sacar la información de los mapas, que son printeados en pantalla (nombre y descripción)
    packet.clear();
    std::string mapName;
    int numMapas;

    resultSet = stmt->executeQuery("SELECT COUNT(*) FROM Mapas");
    if(resultSet->next())
        numMapas=resultSet->getInt(1);
    resultSet = stmt->executeQuery("SELECT * FROM Mapas");
    packet<<MAPS<<numMapas;

    while(resultSet->next()){
             packet<<resultSet->getString("Name") << resultSet->getString("Description");
        }
    socket->send(packet);
	//Se pregunta al usuario que mapa quiere jugar. Debe escribir el nombre del mapa
    bool mapSelected = false;
    do {
        if(socket->receive(packet)!=sf::Socket::Status::Done){
            std::cout<<"el cliente se ha desconectado"<<std::endl;
            break;
        }
        packet >> mapName;
        resultSet->beforeFirst();
        while(resultSet->next()){
			//Se comprueba que el mapa escrito exista en la BD
            if(mapName==resultSet->getString("Name")){
                mapSelected = true;
                std::cout<<mapName<<std::endl;
                packet.clear();
                packet<<true;
                socket->send(packet);
                //logInSec = time(0);
                break;
            }
        }
    }while(!mapSelected);
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
            std::thread conn(&connection, incoming);
            conn.detach();
        }
    }
    dispatcher.close();
    std::cout<<"ejecucion terminada";
}
