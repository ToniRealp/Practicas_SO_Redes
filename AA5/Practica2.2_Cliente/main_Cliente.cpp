#include <iostream>
#include <unistd.h>
#include <SFML/Network.hpp>
#include <thread>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <wait.h>
#include <stdio.h>
#include <iostream>
#include <SFML/Graphics.hpp>


#define CELL_SIZE 32
#define RADIUS_AVATAR 12.5f
#define OFFSET_AVATAR 5

void PrintSFML(sf::TcpSocket*, std::string, std::string);
void RecieveOnPlay(sf::TcpSocket *);
void Disconnect(sf::TcpSocket*);

enum GameObject {NONE, OBSTACLE, PLAYER, POKEMON};
enum ID {LOGIN, SIGNUP, MAPS, DISCONNECT, SPAWN, CATCH, VALIDSPAWN, INVENTORY, COINS, RECOLLECT};
enum GameState {SELECTMAP, PLAY};

struct PokemonsInSpawns
{
    std::string name;
    sf::Vector2f position;
};

bool playing = false;
bool inGame=false;
bool canRecolect=false;
std::vector<std::vector<GameObject>> grid(GameObject::NONE);
std::vector<PokemonsInSpawns> spawns;
sf::Vector2f playerPosition;
int recollectCoins;

int main(){
    //Inicializamos el socket para la conexión con el servidor
    sf::TcpSocket* socket = new sf::TcpSocket;
    sf::Socket::Status status = socket->connect("79.154.246.236", 50000, sf::seconds(30.f));

    //Comprobación de errores en la conexión cliente-servidor
    if (status != sf::Socket::Done)
    {
        Disconnect(socket);
        return(0);
    }

    //Variables necesarias para el inicio de sesión o creación de nuevo usuario
    sf::Packet packet;

    bool signUp = false;
    bool validName = false;

    std::string username, password;

    int coins;

    do
    {   //Se pregunta el nombre de usuario
        std::cout << "Username:" << std::endl;
        std::cin >> username;

        //Si es "nuevo" o "new" se pasa a la creación de un nuevo usuario
        if(username == "nuevo" || username == "new")
        {
			signUp = true;
			//Nuevo nombre de usuario
            std::cout << "New Username:" << std::endl;
            std::cin >> username;

            bool pwdMatch = false;
            do
            {
                //Se pregunta la contraseña
                std::string confirmPassword;
                std::cout<< "Password:"<<std::endl;
                std::cin>>password;
                //Confirmación de la contraseña
                std::cout<< "Confirm Password:"<<std::endl;
                std::cin>>confirmPassword;
                //Si las contraseñas coinciden
                if(password==confirmPassword)
                    pwdMatch=true;
            }
            while(!pwdMatch);

            packet.clear();
            packet << SIGNUP << username << password;
        }
        //Si el usuario no es "nuevo" o "new"
        else
        {
            signUp = false;
            std::cout << "Password:" << std::endl;
            std::cin >> password;

            packet.clear();
            packet << LOGIN << username << password;
        }
        //Se envía al servidor la información recibida y comprueba que el servidor y el cliente sigan conectados.
        if(socket->send(packet)!= sf::Socket::Status::Done)
        {
            Disconnect(socket);
            return(0);
        }
        //Si el servidor y el cliente siguen conectados se recibe si la información enviada es correcta o no
        if(socket->receive(packet) != sf::Socket::Status::Done)
        {
            Disconnect(socket);
            return(0);
        }
        packet >> validName;
        //Si la combinación usuario-contraseña es incorrecta
        if(!validName)
        {   //El nombre utilizado para darse de alta ya existe
            if(signUp)
                std::cout << "This name already exists. Please, write a different one." << std::endl;
            //El usuario o la contraseña son incorrectos
            else
                std::cout << "Username or Password are incorrect." << std::endl;
        }
        //Si la combinación usuario-contraseña es correcta
        else
        {
            //Se recogen los datos del usuario (monedas y pokemons) y se muestran por consola
            int pokemonNumber;
            packet >> coins >> pokemonNumber;

            std::system("clear");
            std::cout << "Session opened: " << username << std::endl << std::endl << "Coins: " << coins << std::endl << std::endl << "Pokemons: " << std::endl;

            for(; pokemonNumber>0; pokemonNumber--)
            {
                std::string pokemon;
                packet >> pokemon;
                std::cout << pokemon << std::endl;
            }
            std::cout << std::endl;
        }

    }while(!validName);

    //Variables necesarias para el flujo de juego (selección de nivel y juego)
    inGame = true;
    GameState gameState = SELECTMAP;
    std::string structure;
    std::system("clear");
    do
    {
        std::string mapName;
        //Si el jugador está eligiendo el mapa
        if(gameState == SELECTMAP)
        {
            structure = "";
            int id, numMaps;
            //Si el servidor y el cliente siguen conectados se recibe el paquete que contiene los mapas y sus descripciones y se muestran por consola
            if(socket->receive(packet) != sf::Socket::Status::Done)
            {
                Disconnect(socket);
                return(0);
            }

            packet >> id >> numMaps;

            for(;numMaps>0;numMaps--)
            {
                std::string name, description;
                packet >> name >> description;
                std::cout << name << std::endl << description << std::endl << std::endl;
            }

            bool mapSelected = false;

            //Mientras que no se seleccione un mapa válido
            do
            {   //Se pregunta el nombre del mapa al que se quiere jugar
                std::cout << "Insert the name of the map you want to play:" << std::endl;
                std::cin >> mapName;

                packet.clear();
                packet << mapName;
                //Si el servidor y el cliente siguen conectados se envia el paquete que contiene el nombre introducido por pantalla
                if(socket->send(packet) != sf::Socket::Status::Done)
                {
                    Disconnect(socket);
                    return(0);
                }
                //Si el nombre introducido ha sido diferente a "exit"
                if(mapName != "exit")
                {   //Si el servidor y el cliente siguen conectados se recibe si la información enviada es correcta o no
                    if(socket->receive(packet) != sf::Socket::Status::Done)
                    {
                        Disconnect(socket);
                        return(0);
                    }
                    packet >> mapSelected;
                }
                //Si el nombre introducido ha sido "exit" el programa termina
                else
                    return (0);

            }while(!mapSelected);
            //Si el mapa seleccionado existe se recoge la estructura del mapa
            packet >> structure;

            playing = true;
            gameState = PLAY;

            grid.resize(20,std::vector<GameObject>(20));

            //Se inicializa el tablero, las posiciones de los spawns y la posicion inicial del jugador
            for (int i =0; i<20; i++)
            {
                for(int j = 0; j<20; j++)
                {
                    if (structure[i*20+j]=='O')
                            grid[i][j]=GameObject::OBSTACLE;
                    else if(structure[i*20+j]=='S')
                        spawns.push_back({"", sf::Vector2f(i,j)});
                    else if (structure[i*20+j]=='I')
                    {
                        grid[i][j]=GameObject::PLAYER;
                        playerPosition.x = j;
                        playerPosition.y = i;
                    }
                }
            }

            int numberOfSpawns = spawns.size();
            packet.clear();
            packet << numberOfSpawns;
            //Si el servidor y el cliente siguen conectados se envia el número de spawns que hay en ese mapa

            if(socket->send(packet) != sf::Socket::Status::Done)
            {
                Disconnect(socket);
                return(0);
            }
        }
        //Si el jugador ha elegido el mapa
        else if(gameState == PLAY)
        {
            std::system("clear");
            std::cout << "Playing..." << std::endl;
            //Se inicia el thread de recepción de packets para que la ejecución principal no se detenga
            std::thread recieve(RecieveOnPlay, socket);
            //Se llama a la función que se encarga de la ventana de juego
            PrintSFML(socket, structure, mapName);
            //Cuando el thread ha acabado (el jugador sale de la ventana de juego) vuelve al menú de selección de mapa
            recieve.join();
            std::system("clear");
            gameState = SELECTMAP;
        }
    }while(inGame);

    socket->disconnect();
}

void Disconnect(sf::TcpSocket* socket)
{
    std::cout << "Lost connection with server." << std::endl;
    socket->disconnect();
}

sf::Vector2f BoardToWindows(sf::Vector2f _position)
{
    return sf::Vector2f(_position.x*CELL_SIZE+OFFSET_AVATAR, _position.y*CELL_SIZE+OFFSET_AVATAR);
}

void RecieveOnPlay(sf::TcpSocket *socket)
{   //Mientras esté la ventana de juego abierta reciben packets
    while(playing)
    {
        sf::Packet packet;
        int id;

        if(socket->receive(packet)!= sf::Socket::Status::Done)
            break;

        packet >> id;
        //Si se recibe un packet con id SPAWN
        if(id == SPAWN)
        {
            int spawnPos;
            std::string pokemonName;
            //Se recogen los datos de spawn de un pokemon (posición del vector de spawns y nombre del pokemon)
            packet >> spawnPos >> pokemonName;
            //Se coloca el pokemon en la grid para que se muestre en pantalla
            grid[spawns[spawnPos].position.x][spawns[spawnPos].position.y]=POKEMON;
            //Se pone el nombre del pokemon que hay en ese spawn en la posición donde está
            spawns[spawnPos].name = pokemonName;
        }
        //Si se recibe un packet con id DISCONNECT
        else if(id == DISCONNECT)
            return;
        //Si se recibe un packet con id INVENTORY
        else if(id == INVENTORY)
        {
            int numOfPokemons;
            //Se recogen el numero de pokemons y los nombres del packet y se muestran en consola
            packet >> numOfPokemons;
            std::cout << std::endl << std::endl << "Inventory: " << std::endl << std::endl;
            for(;numOfPokemons>0;numOfPokemons--){
                std::string pokemon;
                packet>>pokemon;
                std::cout<<pokemon<<std::endl;
            }
            std::cout << std::endl;
        }
        //Si se recibe un packet con id COINS
        else if(id == COINS)
        {
            int numOfCoins;
            //Se recoge el número de monedas del packet y se muestra por consola
            packet >> numOfCoins;
            std::cout << "Coins: " << numOfCoins <<std::endl;
        }
        //Si se recibe un packet con id RECOLLECT
        else if(id == RECOLLECT)
        {
            //Se recoge el número de monedas para recoger y se muestra por pantalla
            canRecolect = true;
            packet >> recollectCoins;
            std::cout << "You can recollect " << recollectCoins << " coins" << std::endl;
        }
    }
}

void PrintSFML(sf::TcpSocket *socket, std::string structure, std::string mapName)
{
    //Creamos una ventana
    sf::RenderWindow window(sf::VideoMode(640,640), mapName);
    sf::Packet packet;

    while(window.isOpen())
    {
        sf::Event event;
        //Controlamos los eventos
        while(window.pollEvent(event))
        {
            if(event.type == sf::Event::Closed){
                    //Se reinicia la grid con todo NONE
                    for(auto &x : grid)
                    {
                        for( auto &g : x)
                            g = NONE;
                    }
                    //Se pone spawns a tamaño 0 para borrar los spawns del mapa recien cerrado y se cierra la ventana
                    spawns.resize(0);
                    window.close();
                    //Se envia al servidor un paquete de disconnect
                    packet.clear();
                    packet<<DISCONNECT;
                    socket->send(packet);
                    playing = false;
                }
                else if(event.type == sf::Event::KeyPressed)
                {
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
                    {
                        //Se reinicia la grid con todo NONE
                        for(auto &x : grid)
                        {
                            for( auto &g : x)
                                g = NONE;
                        }
                        //Se pone spawns a tamaño 0 para borrar los spawns del mapa recien cerrado y se cierra la ventana
                        spawns.resize(0);
                        window.close();
                        //Se envia al servidor un paquete de disconnect
                        packet.clear();
                        packet<<DISCONNECT;
                        socket->send(packet);
                    }
                    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
                    {
                        //Si en la posición objetivo no hay una pared se mueve a esa posicion
                        if(grid[playerPosition.y][playerPosition.x - 1] != GameObject::OBSTACLE){
                            grid[playerPosition.y][playerPosition.x]=GameObject::NONE;
                            playerPosition.x--;
                            grid[playerPosition.y][playerPosition.x]=GameObject::PLAYER;
                        }
                    }
                    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
                    {
                        //Si en la posición objetivo no hay una pared se mueve a esa posicion
                        if(grid[playerPosition.y][playerPosition.x + 1] != GameObject::OBSTACLE)
                        {
                            grid[playerPosition.y][playerPosition.x]=GameObject::NONE;
                            playerPosition.x++;
                            grid[playerPosition.y][playerPosition.x]=GameObject::PLAYER;
                        }
                    }
                    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
                    {
                        //Si en la posición objetivo no hay una pared se mueve a esa posicion
                        if(grid[playerPosition.y - 1][playerPosition.x] != GameObject::OBSTACLE)
                        {
                            grid[playerPosition.y][playerPosition.x]=GameObject::NONE;
                            playerPosition.y--;
                            grid[playerPosition.y][playerPosition.x]=GameObject::PLAYER;
                        }
                    }
                    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
                    {
                        //Si en la posición objetivo no hay una pared se mueve a esa posicion
                        if(grid[playerPosition.y + 1][playerPosition.x] != GameObject::OBSTACLE)
                        {
                            grid[playerPosition.y][playerPosition.x]=GameObject::NONE;
                            playerPosition.y++;
                            grid[playerPosition.y][playerPosition.x]=GameObject::PLAYER;
                        }
                    }
                    else if( sf::Keyboard::isKeyPressed(sf::Keyboard::I))
                    {
                        //Se envia al servidor un packet con la id INVENTORY
                        packet.clear();
                        packet << INVENTORY;
                        if(socket->send(packet)!=sf::Socket::Status::Done)
                        {
                            playing=false;
                            inGame=false;
                            break;
                        }
                    }
                    else if( sf::Keyboard::isKeyPressed(sf::Keyboard::M))
                    {
                        //Se envia al servidor un packet con la id COINS
                        packet.clear();
                        packet << COINS;
                        if(socket->send(packet)!=sf::Socket::Status::Done)
                        {
                            playing=false;
                            inGame=false;
                            break;
                        }
                    }
                    else if(sf::Keyboard::isKeyPressed(sf::Keyboard::R) && canRecolect)
                    {
                        //Se envia al servidor un packet con la id RECOLECT
                        canRecolect = false;
                        packet.clear();
                        packet << RECOLLECT << recollectCoins;
                        if(socket->send(packet)!=sf::Socket::Status::Done)
                        {
                            playing=false;
                            inGame=false;
                            Disconnect(socket);
                            break;
                        }
                        else
                        std::cout << "Coins recollected" << std::endl;
                    }
                }
            }

        //Si el jugador está encima de un spawn de pokemons y hay un pokemon en ese spawn lo caprura y envia un packet al servidor con la posición en el vector de ese spawn
        for(unsigned int i = 0; i < spawns.size(); i++)
        {
            if(spawns[i].position.x == playerPosition.y && spawns[i].position.y == playerPosition.x && spawns[i].name != "")
            {
                packet.clear();
                packet<<CATCH<<i;
                socket->send(packet);
                std::cout << "You catched a: "<<spawns[i].name << std::endl;
                spawns[i].name = "";
            }
        }

        window.clear();

        //Se pinta la pantalla con el juego
        for (int i =0; i<20; i++)
        {
            for(int j = 0; j<20; j++)
            {
                sf::RectangleShape rectBlanco(sf::Vector2f(CELL_SIZE,CELL_SIZE));
                rectBlanco.setFillColor(sf::Color::White);
                if (grid[i][j]==GameObject::OBSTACLE){
                        rectBlanco.setPosition(sf::Vector2f(j*CELL_SIZE, i*CELL_SIZE));
                        window.draw(rectBlanco);
                }
                else if(grid[i][j] == GameObject::POKEMON){
                    sf::CircleShape shape(RADIUS_AVATAR);
                    shape.setFillColor(sf::Color::Magenta);
                    sf::Vector2f posicionDibujar = BoardToWindows(sf::Vector2f(j,i));
                    shape.setPosition(posicionDibujar);
                    window.draw(shape);
                }
                else if (grid[i][j]==GameObject::PLAYER){
                    sf::CircleShape shape(RADIUS_AVATAR);
                    shape.setFillColor(sf::Color::Blue);
                    sf::Vector2f posicionDibujar = BoardToWindows(sf::Vector2f(j,i));
                    shape.setPosition(posicionDibujar);
                    window.draw(shape);
                }
            }
        }
        window.display();
    }
}
