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

enum ID {LOGIN, SIGNUP, MAPS, DISCONNECT, SPAWN, CATCH, VALIDSPAWN, INVENTORY, COINS, RECOLLECT};
enum GAMESTATE {MENU, PLAYING};

struct PokeInfo
{
    int id;
    std::string name;
    int rate;
    int minCoins, maxCoins;
    PokeInfo(int _id, std::string _name, int _rate, int _minCoins, int _maxCoins):id(_id),name(_name),rate(_rate),minCoins(_minCoins), maxCoins(_maxCoins){}
};

//Esta funcion nos permite convertir una string en time_T
time_t String2time_t(const std::string& strDateTime){
    tm t;
    strptime(strDateTime.c_str(), "%F %T", &t);
    return mktime(&t);
}
//Esta funcion es llamada por un thread el cual se encarga de la generacion de monedas cada X segundos
void coinGeneration(sf::TcpSocket *socket, time_t *lastRecollection, int *numPokemons, bool *canRecolect, bool *inGame)
{
    std::cout<<"coinGeneration launched"<<std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    double duration=0;

    sf::Packet packet;
    time_t remainigTime;
    tm* lastRecollectionTm=localtime(lastRecollection);
    tm* actualRecollectionTm;

    while(*inGame)
    {
        auto endfinal = std::chrono::high_resolution_clock::now();
        duration=std::chrono::duration_cast<std::chrono::seconds>(endfinal-start).count();
        //Cada 5 segundos comprovamos si se han de generar las monedas (para no hacerlo en cada frame)
        if(duration>=5)
        {
            start=std::chrono::high_resolution_clock::now();
            remainigTime=time(0)-*lastRecollection;
            actualRecollectionTm=localtime(&remainigTime);
            //Se generan monedas si la diferencia entre el timpo actual y la ultima vez que se recogieron es mas grande de 1 min
            if(actualRecollectionTm->tm_sec>=60 && *canRecolect)
            {
                *canRecolect=false;
                sf::Packet packet;
                packet<<RECOLLECT<<10**numPokemons;
                socket->send(packet);
                std::cout<<"Coins generated"<<std::endl;
            }
        }
    }
    std::cout<<"Coin thread exit correctly"<<std::endl;
}
//Esta funcion el llamada por el thread que se encarga de generar los pokemons
void pokemonGeneration(sf::TcpSocket* socket, int spawnTime, std::vector<PokeInfo> pokeInfo, std::vector<int> *pokemonSpawn, bool *playing){

    std::cout<<"pokemonGeneration launched"<<std::endl;
    int totalRatio = 0;
    for(int i=0; i<pokeInfo.size();i++)
    {
        totalRatio+=pokeInfo[i].rate;
    }

    auto start = std::chrono::high_resolution_clock::now();
    double duration=0;
    int pokemonId;
    sf::Packet packet;

    while(*playing)
    {
        auto endfinal = std::chrono::high_resolution_clock::now();
        duration=std::chrono::duration_cast<std::chrono::seconds>(endfinal-start).count();
        //Segun el tiempo indicado en la base de datos se generaran con mas o menos frecuencia
        if(duration>=spawnTime)
        {
            start = std::chrono::high_resolution_clock::now();
            bool found = false;
            do{//se busca una posicion random dentro del vector de spawns para generar el pokemon
                int pos = rand()%(pokemonSpawn->size());

                if(pokemonSpawn->at(pos)==0){
                //se genera un numero random dentro del total de probabilidades de spawn de cada pokemon
                    int spawnId = rand()%(totalRatio+1);
                    int counter = 0;
                    for(int i=0; i<pokeInfo.size();i++)
                    {
                        counter+=pokeInfo[i].rate;
                        //El pokemon al que le corresponda el random dentro del total de posibilidades es spawneado
                        if(spawnId<=counter)
                        {
                            pokemonSpawn->at(pos)=i;
                            packet.clear();
                            packet<<SPAWN<<pos<<pokeInfo[i].name;
                            if(!*playing)
                                return;
                            if(socket->send(packet)!=sf::Socket::Status::Done)
                                return;
                            found=true;
                            std::cout<<"Pokemon spawned"<<std::endl;
                            break;
                        }
                    }
                }
            }while(!found);
        }
    }
    std::cout<<"spawn thread exit correctly"<<std::endl;
}


void connection(sf::TcpSocket* socket)
{
    ///Se inicializa la conexión a la BD PracticaRedesAA4
    sql::Driver* driver = sql::mysql::get_driver_instance();
    sql::Connection* conn = driver->connect("tcp://127.0.0.1","root","linux123");
    conn->setSchema("PracticaRedesAA4");

    sql::Statement* stmt =conn->createStatement();
    sql::ResultSet* resultSet;

    std::string username, password, playerID, lastRecollectionStr;
    time_t lastRecollection;
    int id, coins, numPokemons;

    time_t logInSec, logOutSec;

    sf::Packet packet;

    bool validName = false;
    ///LOGIN / SIGN IN
    do{

        packet.clear();
        std::cout<<"Waiting for username"<<std::endl;

        if(socket->receive(packet)!=sf::Socket::Done)
        {
            std::cout<<"The client has disconnected"<<std::endl;
            break;
        }
        else
        {
            packet>>id>>username>>password;
            std::cout<<"Login credentials: "<<username<<" "<<password<<std::endl;
        }
        //Si existe algun usuario con esa combinación de Username y Password se inicia sesión en esa cuenta

        std::vector<std::string>pokemonNames;
        if(id==LOGIN)
        {
            resultSet = stmt->executeQuery("SELECT * FROM Player WHERE Player.Username = '"+ username + "'" "AND Player.Password = '"+ password + "'");
            if(resultSet->next())
            {
                validName=true;

                playerID = resultSet->getString("ID");
                coins=resultSet->getInt("Coins");
                lastRecollectionStr = resultSet->getString("LastRecollection");
                lastRecollection=String2time_t(lastRecollectionStr);
                resultSet = stmt->executeQuery("SELECT * FROM Pokemons WHERE ID IN (SELECT PokemonID FROM PokemonXplayer WHERE PlayerID ="+playerID+")");

                numPokemons=0;
                if(resultSet->next()){

                    do{
                        numPokemons++;
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
                {
                    playerID = resultSet->getString("ID");
                    lastRecollectionStr = resultSet->getString("LastRecollection");
                    lastRecollection=String2time_t(lastRecollectionStr);
                }
                coins = 0;
            }
        }

        packet.clear();
        packet<<validName;
        //Se envian el numero de monedas del jugador y todos sus pokemons
        if(validName)
        {
            packet<<coins<<numPokemons;
        }

        for(auto x: pokemonNames)
            packet<<x;

        if(socket->send(packet)!= sf::Socket::Done)
        {
            std::cout<<"Client has disconnected"<<std::endl;
            break;
        }

    }while(!validName);
    //Se genera un vector con la informacion de todos los pokemons en el juego para despues poder selecionar que pokemons spawnear
    std::vector<PokeInfo> pokemonInfo;

    resultSet = stmt->executeQuery("SELECT * FROM Pokemons");

    while(resultSet->next())
        pokemonInfo.push_back(PokeInfo(resultSet->getInt("ID"),resultSet->getString("Name"),resultSet->getInt("Ratio"),resultSet->getInt("MinCoins"),resultSet->getInt("MaxCoins")));

    std::vector<int> pokemonSpawn;
    bool playing, inGame, canRecollect;
    playing = inGame = canRecollect = true;
    bool validSpawn;
    std::thread *CoinGen, *PokemonGen;

    GAMESTATE gameState = MENU;

    do{

        if(gameState==MENU)
        {
            packet.clear();
            std::string mapName;
            std::string mapStructure;
            int numMapas;

            resultSet = stmt->executeQuery("SELECT COUNT(*) FROM Mapas");

            if(resultSet->next())
                numMapas=resultSet->getInt(1);
            //Se envian todos los mapas disponibles al cliente
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
                    std::cout<<"Client lost connection"<<std::endl;
                    inGame=false;
                    break;
                }

                packet >> mapName;
                resultSet->beforeFirst();
                //En el caso que recivamos exit salimos del bucle principal
                if(mapName=="exit")
                {
                    inGame=false;
                    break;
                }

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
                //En el caso que el nombre del mapa sea correcto se envia la estructura de esta al cliente para su pintado
                packet.clear();
                packet<<mapSelected<<mapStructure;
                socket->send(packet);


            }while(!mapSelected);

            if(inGame)
            {
                //Se llena el vector con todos los posibles spawns de los pokemons dependiendo del mapa
                int numberOfSpawns;
                socket->receive(packet);
                packet>>numberOfSpawns;
                pokemonSpawn.resize(numberOfSpawns);
                std::fill(pokemonSpawn.begin(),pokemonSpawn.end(),0);
                playing=true;
                PokemonGen=new std::thread(&pokemonGeneration, socket, spawnTime, pokemonInfo, &pokemonSpawn, &playing);
                CoinGen=new std::thread(&coinGeneration, socket, &lastRecollection, &numPokemons, &canRecollect, &inGame);
                gameState=PLAYING;
            }
        }

        if(gameState==PLAYING)
        {
            if(socket->receive(packet)!=sf::Socket::Status::Done){
                id=DISCONNECT;
                inGame=false;
            }
            else
                packet>>id;

            if(id == DISCONNECT)
            {
                //Si recibimos un id disconect se cierra la sesion del cliente y se guardan los tiempos
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

                std::cout<<"Cliente "<<username<<" has ended his sesion"<<std::endl;

                playing=false;
                //Se espera al join del thread de generacion de pokemons
                PokemonGen->join();
                gameState = MENU;
                packet.clear();
                packet<<DISCONNECT;
                socket->send(packet);

            }
            else if(id==CATCH)
            {
                //En el caso que el id sea catch se inserta el nuevo pokemon capturado a la lista de pokemons del cliente
                int pos;
                packet>>pos;
                std::string plID(playerID);
                std::string pokID = std::to_string(pokemonInfo[pokemonSpawn[pos]].id);
                std::string pokeCoins = std::to_string(coins + pokemonInfo[pokemonSpawn[pos]].minCoins+rand()%(pokemonInfo[pokemonSpawn[pos]].maxCoins-pokemonInfo[pokemonSpawn[pos]].minCoins));
                pokemonSpawn[pos]=0;
                stmt->executeUpdate("INSERT INTO PokemonXplayer(PlayerID, PokemonID) VALUES('"+plID+"','"+pokID+"')");
                stmt->executeUpdate("UPDATE Player SET Coins ="+pokeCoins+" WHERE ID ="+playerID);

            }

            else if(id==INVENTORY)
            {
                //En el caso que el id sea invertory se hace una query a la base de datos que nos devuelve todos los pokemons del cliente
                resultSet=stmt->executeQuery("SELECT Name FROM Pokemons WHERE ID IN (SELECT PokemonID FROM PokemonXplayer WHERE PlayerID ="+playerID+")");
                std::vector<std::string> pokemonNames;
                numPokemons=0;
                while(resultSet->next()){
                    numPokemons++;
                    pokemonNames.push_back(resultSet->getString(1));
                }
                packet.clear();
                packet<<INVENTORY<<numPokemons;
                for(auto x : pokemonNames)
                    packet<<x;
                socket->send(packet);
            }
            else if(id==COINS)
            {
                //En el caso el id sea coins se hace una query a la base de datos que devuelve las monedas del cliente
                resultSet=stmt->executeQuery("SELECT Coins FROM Player WHERE ID ="+playerID);
                resultSet->next();
                coins=resultSet->getInt(1);
                packet.clear();
                packet<<COINS<<coins;
                socket->send(packet);
            }
            else if(id==RECOLLECT)
            {
                //En el caso que el id sea recollect se anañaden las monedas al cliente y se actualiza la ultima vez que este ha recogido monedas
                int numCoins;
                packet>>numCoins;
                coins+=numCoins;
                std::string coinsStr = std::to_string(coins);
                char lastRecollectionBuf [22];
                lastRecollection=time(0);
                tm* lastRecollectionTm = localtime(&lastRecollection);
                strftime(lastRecollectionBuf,22,"%F %X\0",lastRecollectionTm);
                std::string str(lastRecollectionBuf);
                stmt->executeUpdate("UPDATE Player SET Coins = "+coinsStr+", LastRecollection = '"+str+"' WHERE ID ="+playerID);
                canRecollect=true;
            }
        }
    }while(inGame);

    //AL finalizar la conexion esperamos al join de coin generation y borramos la memoria dinamica asignada
    CoinGen->join();
    std::cout<<"Client: "<< username <<" has disconnected"<<std::endl;
    delete PokemonGen;
    delete CoinGen;
    resultSet->close();
    delete resultSet;
    stmt->close();
    delete stmt;
}

int main()
{
    ///Se inicializan los sockets
    sf::TcpListener dispatcher;
    sf::Socket::Status status = dispatcher.listen(50000);

    if(status != sf::Socket::Done)
    {
        std::cout<<"Error in listener creation";
        return 0;
    }

    while(1){
    //Cada vez que se accepta una connexion se genera un nuevo thread que la gestiona
        sf::TcpSocket* incoming = new sf::TcpSocket;
        if(dispatcher.accept(*incoming) != sf::Socket::Done)
        {
            std::cout<<"Error in socket creation";
        }
        else{
            std::cout<<"Connected"<<std::endl;
            std::thread conn(&connection, incoming);
            conn.detach();
        }
    }
    dispatcher.close();
    std::cout<<"Execution ended";
}
