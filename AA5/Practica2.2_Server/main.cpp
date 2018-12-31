#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <jdbc/mysql_connection.h>
#include <jdbc/mysql_driver.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <iostream>
#include <vector>
#include <thread>
#include <ctime>
#include <chrono>
#include <string>

enum ID {LOGIN, SIGNUP, MAPS, DISCONNECT, SPAWN, CATCH};
enum GAMESTATE {MENU, PLAYING};
std::vector<sf::TcpSocket*> sockets;

struct PokeInfo
{
    int id;
    std::string name;
    int rate;
    int minCoins, maxCoins;
    PokeInfo(int _id, std::string _name, int _rate, int _minCoins, int _maxCoins):id(_id),name(_name),rate(_rate),minCoins(_minCoins), maxCoins(_maxCoins){}
};

void pokemonGen(sf::TcpSocket* socket, int spawnTime, std::vector<PokeInfo> pokeInfo, std::vector<int>* pokemonSpawn, bool* playing){

    int totalRatio = 0;
    for(int i=0; i<pokeInfo.size();i++)
    {
        totalRatio+=pokeInfo[i].rate;
    }

    auto start = std::chrono::high_resolution_clock::now();
    double duration=0;
    int pokemonId;

    while(*playing)
    {
        auto endfinal = std::chrono::high_resolution_clock::now();
        duration=std::chrono::duration_cast<std::chrono::seconds>(endfinal-start).count();

        if(duration*10>=spawnTime)
        {
            start = std::chrono::high_resolution_clock::now();
            bool found = false;
            do{
                int pos = rand()%(pokemonSpawn->size());
                if(pokemonSpawn->at(pos)==0){
                    int spawnId = rand()%(totalRatio+1);
                    int counter = 0;
                    for(int i=0; i<pokeInfo.size();i++)
                    {
                        counter+=pokeInfo[i].rate;
                        if(spawnId<=counter)
                        {
                            pokemonSpawn->at(pos)=i;
                            sf::Packet packet;
                            packet<<SPAWN<<pos<<pokeInfo[i].name;
                            if(!*playing)
                                return;
                            if(socket->send(packet)!=sf::Socket::Status::Done)
                                return;
                            found=true;
                            std::cout<<"pokemon spawned"<<std::endl;
                            break;
                        }
                    }
                }
            }while(!found);
        }
    }
}


void connection(sf::TcpSocket* socket)
{
    ///Se inicializa la conexión a la BD PracticaRedesAA4
    sql::Driver* driver = sql::mysql::get_driver_instance();
    sql::Connection* conn = driver->connect("tcp://127.0.0.1","root","linux123");
    conn->setSchema("PracticaRedesAA4");

    sql::Statement* stmt =conn->createStatement();
    sql::ResultSet* resultSet;

    std::string username, password, playerID;
    int id, coins;

    time_t logInSec, logOutSec;

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
        //Si existe algun usuario con esa combinación de Username y Password se inicia sesión en esa cuenta

        std::vector<std::string>pokemonNames;
        int pokemonCounter=0;

        if(id==LOGIN)
        {
            resultSet = stmt->executeQuery("SELECT * FROM Player WHERE Player.Username = '"+ username + "'" "AND Player.Password = '"+ password + "'");
            if(resultSet->next())
            {
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
        else if(id==SIGNUP)
        {
            resultSet = stmt->executeQuery("SELECT * FROM Player WHERE Player.Username = '"+ username + "'");

            if(!resultSet->next())
            {
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

        if(validName)
        {
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
    std::string mapStructure;
    int numMapas;

    resultSet = stmt->executeQuery("SELECT COUNT(*) FROM Mapas");

    if(resultSet->next())
        numMapas=resultSet->getInt(1);

    resultSet = stmt->executeQuery("SELECT * FROM Mapas");
    packet<<MAPS<<numMapas;

    while(resultSet->next())
        packet<<resultSet->getString("Name") << resultSet->getString("Description");

    socket->send(packet);
	//Se pregunta al usuario que mapa quiere jugar. Debe escribir el nombre del mapa
    bool mapSelected = false;
    int spawnTime;

    do {
        std::cout<<"Waiting for map selection"<<std::endl;

        if(socket->receive(packet)!=sf::Socket::Status::Done)
        {
            std::cout<<"el cliente se ha desconectado"<<std::endl;
            break;
        }

        packet >> mapName;
        resultSet->beforeFirst();

        while(resultSet->next())
        {
			//Se comprueba que el mapa escrito exista en la BD
            if(mapName==resultSet->getString("Name"))
            {
                std::cout << "Map: "<<mapName <<"Selected"<< std::endl;
                mapStructure = resultSet->getString("Structure");
                spawnTime = resultSet->getInt("SpawnTime");
                mapSelected=true;
                logInSec = time(0);
                break;
            }
        }

        packet.clear();
        packet<<mapSelected<<mapStructure;
        socket->send(packet);

    }while(!mapSelected);

    int numberOfSpawns;
    socket->receive(packet);
    packet>>numberOfSpawns;
    std::vector<int> pokemonSpawn(numberOfSpawns,0);
    std::vector<PokeInfo> pokemonInfo;

    resultSet = stmt->executeQuery("SELECT * FROM Pokemons");

    while(resultSet->next())
        pokemonInfo.push_back(PokeInfo(resultSet->getInt("ID"),resultSet->getString("Name"),resultSet->getInt("Ratio"),resultSet->getInt("MinCoins"),resultSet->getInt("MaxCoins")));

    bool playing = true;
    bool inGame=true;
    std::thread PokemonGeneration(&pokemonGen, socket, spawnTime, pokemonInfo, &pokemonSpawn, &playing);

    //resultSet->close();
    GAMESTATE gameState = PLAYING;

    do{

        if(gameState==MENU)
        {
            std::cout<<"in map selection"<<std::endl;
            packet.clear();
            std::string mapName;
            std::string mapStructure;
            int numMapas;

            resultSet = stmt->executeQuery("SELECT COUNT(*) FROM Mapas");

            if(resultSet->next())
                numMapas=resultSet->getInt(1);

            resultSet = stmt->executeQuery("SELECT * FROM Mapas");
            packet<<MAPS<<numMapas;

            while(resultSet->next())
                packet<<resultSet->getString("Name") << resultSet->getString("Description");

            socket->send(packet);
            //Se pregunta al usuario que mapa quiere jugar. Debe escribir el nombre del mapa
            bool mapSelected = false;
            int spawnTime;

            do {
                std::cout<<"Waiting for map selection"<<std::endl;

                if(socket->receive(packet)!=sf::Socket::Status::Done)
                {
                    std::cout<<"el cliente se ha desconectado"<<std::endl;
                    break;
                }

                packet >> mapName;
                resultSet->beforeFirst();

                while(resultSet->next())
                {
                    //Se comprueba que el mapa escrito exista en la BD
                    if(mapName==resultSet->getString("Name"))
                    {
                        std::cout << "Map: "<<mapName <<"Selected"<< std::endl;
                        mapStructure = resultSet->getString("Structure");
                        spawnTime = resultSet->getInt("SpawnTime");
                        mapSelected=true;
                        logInSec = time(0);
                        break;
                    }
                }

                packet.clear();
                packet<<mapSelected<<mapStructure;
                socket->send(packet);

            }while(!mapSelected);

            gameState=PLAYING;
            std::thread PokemonGeneration(&pokemonGen, socket, spawnTime, pokemonInfo, &pokemonSpawn, &playing);
        }


        if(gameState==PLAYING)
        {
            if(socket->receive(packet)!=sf::Socket::Status::Done){
                break;
                id=DISCONNECT;
            }
            else
                packet>>id;
                std::cout<<"id: "<<id<<std::endl;

            if(id == DISCONNECT)
            {
                 for(int it = 0;it<sockets.size();it++)
                {
                    if(sockets[it]==socket)
                    {
                        sockets.erase(sockets.begin()+it);
                        break;
                    }
                }

                tm* logInTime, * logOutTime;
                char logInBuf [22];
                char logOutBuf [22];

                //La funcion strftime nos permite formatear la string con la fecha y hora para que se ajuste al formato de datetime de la bd
                logInTime=localtime(&logInSec);
                strftime(logInBuf,22,"%F %X\0",logInTime);
                std::string logInStr(logInBuf);

                time_t logOutSec = time(0);

                logOutTime=localtime(&logOutSec);
                strftime(logOutBuf,22,"%F %X\0",logOutTime);
                std::string logOutStr(logOutBuf);

                stmt->executeUpdate("INSERT INTO Sesion(PlayerID, LogInTime, LogOutTime) VALUES ("+playerID+",'"+logInStr+"','"+logOutStr+"')");

                std::cout<<"El cliente "<<username<<" se ha desconectado"<<std::endl;

                playing=false;
                PokemonGeneration.join();
                std::cout<<"server joined"<<std::endl;
                gameState = MENU;
                packet.clear();
                packet<<DISCONNECT;
                socket->send(packet);

            }
            else if(id==CATCH)
            {
                int pos;
                packet>>pos;
                std::string plID(playerID);
                std::string pokID = std::to_string(pokemonInfo[pokemonSpawn[pos]].id);
                std::cout<<pokemonInfo[pokemonSpawn[pos]].name<<std::endl;
                pokemonSpawn[pos]=0;
                stmt->executeUpdate("INSERT INTO PokemonXplayer(PlayerID, PokemonID) VALUES('"+plID+"','"+pokID+"')");

            }
        }


    }while(inGame);

}

int main()
{
    ///Se inicializan los sockets
    sf::TcpListener dispatcher;
    sf::Socket::Status status = dispatcher.listen(50000);


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
