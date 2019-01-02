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

void PrintSFML(sf::TcpSocket*, std::string);
void RecieveOnPlay(sf::TcpSocket *);

enum GameObject {NONE, OBSTACLE, PLAYER, POKEMON};
enum ID {LOGIN, SIGNUP, MAPS, DISCONNECT, SPAWN, CATCH, VALIDSPAWN, INVENTORY, COINS};
enum GameState {SELECTMAP, PLAY};

struct PokemonsInSpawns{
    std::string name;
    sf::Vector2f position;
};

std::vector<std::vector<GameObject>> grid(GameObject::NONE);
sf::Vector2f playerPosition;
std::vector<PokemonsInSpawns> spawns;
bool playing = false;

int main(){
    //Inicializamos el socket para la conexión con el servidor
    sf::TcpSocket* socket = new sf::TcpSocket;
    sf::Socket::Status status = socket->connect("79.154.246.236", 50000, sf::seconds(5.f));

    //Comprobación de errores
    if (status != sf::Socket::Done)
    {
      std::cout<<"ERROR: Server unreachable." << std::endl;
      socket->disconnect();
      return(0);
    }

    //Variables necesarias para el inicio de sesión y/o creación de nuevo usuario
    sf::Packet packet;

    bool signUp = false;
    bool validName = false;

    int coins;
    std::vector<std::string> pokemonNames;
    std::string username, password;

    do
    {
		///Login / Sign Up

        std::cout<<"Username:"<<std::endl;
        std::cin>>username;

        if(username=="nuevo" || username=="new")
        {
            ///Sign Up
			//Se pregunta el Username
			signUp = true;
            std::cout<<"New Username:"<<std::endl;
            std::cin>>username;
			//Se comprueba que no exista otro jugador con ese mismo Username
            bool pwdMatch = false;

            do
            {
                ///Asignación de la contraseña
                //Se pregunta la contraseña
                std::string confirmPassword;
                std::cout<< "Password:"<<std::endl;
                std::cin>>password;
                //Confirmación de la contraseña
                std::cout<< "Confirm Password:"<<std::endl;
                std::cin>>confirmPassword;

                if(password==confirmPassword)
                    pwdMatch=true;

            }while(!pwdMatch);

            packet.clear();
            packet << SIGNUP <<username<<password;
        }
        else
        {
            signUp = false;
            std::cout<<"Password:"<<std::endl;
            std::cin>>password;

            packet.clear();
            packet << LOGIN << username << password;
        }

        if(socket->send(packet)!= sf::Socket::Status::Done){
            std::cout<<"ERROR: Server unreachable."<<std::endl;
            break;
        }

        socket->receive(packet);
        packet>>validName;

        if(!validName)
        {
            if(signUp)
                std::cout<<"This name already exists. Please, write a different one."<<std::endl;
            else
                std::cout<<"Username or Password are incorrect."<<std::endl;
        }
        else
        {
            std::system("clear");
            int pokemonNumber;
            packet>>coins>>pokemonNumber;
            std::cout << std::endl <<"Coins: " << coins << std::endl << std::endl << "Pokemons: " << std::endl;
            for(;pokemonNumber>0;pokemonNumber--){
                std::string pokemon;
                packet>>pokemon;
                std::cout<<pokemon<<std::endl;
                pokemonNames.push_back(pokemon);
            }
            std::cout << std::endl;
        }

    }while(!validName);

    GameState gameState = SELECTMAP;
    std::string structure;

    do
    {
        if(gameState == SELECTMAP)
        {
            structure = "";
            int id;
            int numMaps;
            socket->receive(packet);
            packet>>id >> numMaps;

            for(;numMaps>0;numMaps--)
            {
                std::string name;
                std::string description;
                packet>>name>>description;
                std::cout<<name<<std::endl<<description<<std::endl<<std::endl;
            }

            bool mapSelected = false;
            do
            {
                std::cout<<"Insert the name of the map you want to play:"<<std::endl;
                std::string mapName;
                std::cin>>mapName;

                packet.clear();
                packet << mapName;
                socket->send(packet);
                if(mapName != "exit")
                {
                    socket->receive(packet);
                    packet>>mapSelected;
                }
                else
                    return;

            }while(!mapSelected);

            packet >> structure;

            playing = true;
            gameState = PLAY;

            grid.resize(20,std::vector<GameObject>(20));

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

            packet.clear();
            int numberOfSpawns = spawns.size();
            packet << numberOfSpawns;
            socket->send(packet);
        }
        else if(gameState == PLAY)
        {
            std::system("clear");
            std::cout << "Playing..." << std::endl;
            std::thread recieve(RecieveOnPlay, socket);
            PrintSFML(socket, structure);

            recieve.join();
            std::system("clear");
            gameState = SELECTMAP;
        }
    }while(true);

    socket->disconnect();
}

sf::Vector2f BoardToWindows(sf::Vector2f _position)
{
    return sf::Vector2f(_position.x*CELL_SIZE+OFFSET_AVATAR, _position.y*CELL_SIZE+OFFSET_AVATAR);
}

void RecieveOnPlay(sf::TcpSocket *socket)
{
    while(playing)
    {
        sf::Packet packet;
        int id;

        if(socket->receive(packet)!= sf::Socket::Status::Done)
            break;

        packet >> id;

        if(id == SPAWN)
        {
            int spawnPos;
            std::string pokemonName;
            packet>>spawnPos>>pokemonName;
            grid[spawns[spawnPos].position.x][spawns[spawnPos].position.y]=POKEMON;
            spawns[spawnPos].name = pokemonName;
        }
        else if(id == DISCONNECT)
            return;
        else if(id == INVENTORY)
        {
            int numOfPokemons;
            packet>>numOfPokemons;

            std::cout << std::endl << std::endl <<"Inventory: " << std::endl << std::endl;
            for(;numOfPokemons>0;numOfPokemons--){
                std::string pokemon;
                packet>>pokemon;
                std::cout<<pokemon<<std::endl;
            }
            std::cout << std::endl;
        }
        else if(id == COINS)
        {
            int numOfCoins;
            packet>>numOfCoins;
            std::cout << "Coins: " << numOfCoins <<std::endl;
        }
    }
}

void PrintSFML(sf::TcpSocket *socket, std::string structure)
{
    sf::RenderWindow window(sf::VideoMode(640,640), "Interfaz cuadraditos");
    sf::Packet packet;

    while(window.isOpen())
    {
        sf::Event event;
        //Este primer WHILE es para controlar los eventos del mouse
        while(window.pollEvent(event))
        {
            if(event.type == sf::Event::Closed){
                for(auto &x : grid)
                {
                    for( auto &g : x)
                        g = NONE;
                }
                spawns.resize(0);
                window.close();
                packet.clear();
                packet<<DISCONNECT;
                socket->send(packet);
                playing = false;
            }
            else if(event.type == sf::Event::KeyPressed){
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
                {
                    for(auto &x : grid)
                    {
                        for( auto &g : x)
                            g = NONE;
                    }
                    spawns.resize(0);
                    window.close();
                    packet.clear();
                    packet<<DISCONNECT;
                    socket->send(packet);
                    playing = false;
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
                {
                    if(grid[playerPosition.y][playerPosition.x - 1] != GameObject::OBSTACLE){
                        grid[playerPosition.y][playerPosition.x]=GameObject::NONE;
                        playerPosition.x--;
                        if(grid[playerPosition.y][playerPosition.x] == GameObject::POKEMON)
                        {
                            for(int i = 0; i < spawns.size(); i++)
                            {
                                if(spawns[i].position.x == playerPosition.y && spawns[i].position.y == playerPosition.x)
                                {
                                    packet.clear();
                                    packet<<CATCH<<i;
                                    socket->send(packet);
                                    std::cout << "You catched a: "<<spawns[i].name << std::endl;
                                }
                            }
                        }
                        grid[playerPosition.y][playerPosition.x]=GameObject::PLAYER;
                    }
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
                {
                    if(grid[playerPosition.y][playerPosition.x + 1] != GameObject::OBSTACLE)
                    {
                        grid[playerPosition.y][playerPosition.x]=GameObject::NONE;
                        playerPosition.x++;
                        if(grid[playerPosition.y][playerPosition.x] == GameObject::POKEMON)
                        {
                            for(int i = 0; i < spawns.size(); i++)
                            {
                                if(spawns[i].position.x == playerPosition.y && spawns[i].position.y == playerPosition.x)
                                {
                                    packet.clear();
                                    packet<<CATCH<<i;
                                    socket->send(packet);
                                    std::cout << "You catched a: "<<spawns[i].name << std::endl;
                                }
                            }
                        }
                        grid[playerPosition.y][playerPosition.x]=GameObject::PLAYER;
                    }
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
                {
                    if(grid[playerPosition.y - 1][playerPosition.x] != GameObject::OBSTACLE)
                    {
                        grid[playerPosition.y][playerPosition.x]=GameObject::NONE;
                        playerPosition.y--;
                        if(grid[playerPosition.y][playerPosition.x] == GameObject::POKEMON)
                        {
                            for(int i = 0; i < spawns.size(); i++)
                            {
                                if(spawns[i].position.x == playerPosition.y && spawns[i].position.y == playerPosition.x)
                                {
                                    packet.clear();
                                    packet<<CATCH<<i;
                                    socket->send(packet);
                                    std::cout << "You catched a: "<<spawns[i].name << std::endl;
                                }
                            }
                        }
                        grid[playerPosition.y][playerPosition.x]=GameObject::PLAYER;
                    }
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
                {
                    if(grid[playerPosition.y + 1][playerPosition.x] != GameObject::OBSTACLE)
                    {
                        grid[playerPosition.y][playerPosition.x]=GameObject::NONE;
                        playerPosition.y++;
                        if(grid[playerPosition.y][playerPosition.x] == GameObject::POKEMON)
                        {
                            for(int i = 0; i < spawns.size(); i++)
                            {
                                if(spawns[i].position.x == playerPosition.y && spawns[i].position.y == playerPosition.x)
                                {
                                    packet.clear();
                                    packet<<CATCH<<i;
                                    socket->send(packet);
                                    std::cout << "You catched a: "<<spawns[i].name << std::endl;
                                }
                            }
                        }
                        grid[playerPosition.y][playerPosition.x]=GameObject::PLAYER;
                    }
                }
                else if( sf::Keyboard::isKeyPressed(sf::Keyboard::I))
                {
                    packet.clear();
                    packet << INVENTORY;
                    socket->send(packet);
                }
                else if( sf::Keyboard::isKeyPressed(sf::Keyboard::M))
                {
                    packet.clear();
                    packet << COINS;
                    socket->send(packet);
                }
            }
        }

        window.clear();

        //A partir de aquí es para pintar por pantalla
        //Este FOR es para el mapa --> Esto es para pintar casillas estilo tablero de ajedrez
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
                else if (grid[i][j]==GameObject::PLAYER){
                    sf::CircleShape shape(RADIUS_AVATAR);
                    shape.setFillColor(sf::Color::Blue);
                    sf::Vector2f posicionDibujar = BoardToWindows(sf::Vector2f(j,i));
                    shape.setPosition(posicionDibujar);
                    window.draw(shape);
                }
                else if (grid[i][j]==GameObject::POKEMON){
                    sf::CircleShape shape(RADIUS_AVATAR);
                    shape.setFillColor(sf::Color::Magenta);
                    sf::Vector2f posicionDibujar = BoardToWindows(sf::Vector2f(j,i));
                    shape.setPosition(posicionDibujar);
                    window.draw(shape);
                }
            }
        }

        //TODO: Para pintar un círculo que podría simular el personaje
        window.display();
    }

}
