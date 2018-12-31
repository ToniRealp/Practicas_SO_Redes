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


#define MAX 100
#define LADO_CASILLA 32
#define RADIO_AVATAR 12.5f
#define OFFSET_AVATAR 5

void DibujaSFML(sf::TcpSocket*, std::string);
void Recieve(sf::TcpSocket *);

enum GameObject {NONE, OBSTACLE, PLAYER, POKEMON};
enum ID {LOGIN, SIGNUP, MAPS, DISCONNECT, SPAWN, CATCH};
enum GameState {SELECTMAP, PLAY};

struct PokemonsInSpawns{
    std::string name;
    sf::Vector2f position;
};

std::vector<std::vector<GameObject>> grid(GameObject::NONE);
sf::Vector2f position;
std::vector<PokemonsInSpawns> spawns;
bool playing = false;

int main(){
    sf::TcpSocket* socket = new sf::TcpSocket;
    sf::Socket::Status status = socket->connect("79.154.246.236", 50000, sf::seconds(5.f));

    if (status != sf::Socket::Done)
    {
      std::cout<<"no se ha podido establecer la conexion";
      socket->disconnect();
    }
    bool signUp = false;
    bool validName = false;
    int coins;
    std::vector<std::string> pokemonNames;
    sf::Packet packet;

    std::string username, password;
    do{
		///Login / Sign In
		//Se pregunta el Username
        packet.clear();

        std::cout<<"Username:"<<std::endl;
        std::cin>>username;
        if(username=="nuevo"){
             ///Sign in
			//Se pregunta el Username
			signUp = true;
            std::cout<<"New Username:"<<std::endl;
            std::cin>>username;
			//Se comprueba que no exista otro jugador con ese mismo Username
            bool pwdMatch = false;
                do{
					///Asignación de la contraseña
					//Se pregunta la contraseña
                    std::string confirmPassword;
                    std::cout<< "New Password:"<<std::endl;
                    std::cin>>password;
					//Confirmación de la contraseña
                    std::cout<< "Confirm Password:"<<std::endl;
                    std::cin>>confirmPassword;
					//Si las dos contraseñas coinciden se crea un nuevo jugador con ese Username y esa Password con 0 monedas. Si no coinciden se vuelven a preguntar las contraseñas
                    if(password==confirmPassword){
                        pwdMatch=true;
                    }
                }while(!pwdMatch);
                packet << SIGNUP <<username<<password;
        }else{
            signUp = false;
            std::cout<<"Contraseña:"<<std::endl;
            std::cin>>password;
            packet << LOGIN << username << password;
        }

        if(socket->send(packet)!= sf::Socket::Status::Done){
            std::cout<<"Server unreachable"<<std::endl;
            break;
        }

        socket->receive(packet);
        packet>>validName;

        if(!validName)
        {
            if(signUp)
                std::cout<<"Este nombre ya existe. Por favor, introduce otro diferente"<<std::endl;
            else
                std::cout<<"Los datos introducidos son icorrectos"<<std::endl;
        }
        else{
            int pokemonNumber;
            packet>>coins>>pokemonNumber;
            std::cout << std::endl <<"Monedas: " << std::endl<< coins << std::endl << std::endl << "Pokemons: " << std::endl;
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

    do{
        if(gameState == SELECTMAP)
        {
            structure = "";
            int id;
            int numMapas;
            socket->receive(packet);
            packet>>id >> numMapas;
            std::cout << "packet recieved" << std::endl;

            for(;numMapas>0;numMapas--)
            {
                std::string nombre;
                std::string descripcion;
                packet>>nombre>>descripcion;
                std::cout<<nombre<<std::endl<<descripcion<<std::endl<<std::endl;
            }

            bool mapSelected = false;
            do{
                std::cout<<"Introduce el nombre del mapa al que desees jugar:"<<std::endl;
                std::string mapName;
                std::cin>>mapName;
                packet.clear();
                packet<<mapName;
                socket->send(packet);
                socket->receive(packet);
                packet>>mapSelected;

            }while(!mapSelected);

            packet>>structure;
            playing = true;
            gameState = PLAY;

            grid.resize(20,std::vector<GameObject>(20));

            for (int i =0; i<20; i++)
            {
                for(int j = 0; j<20; j++)
                {
                    if (structure[i*20+j]=='O'){
                            grid[i][j]=GameObject::OBSTACLE;
                    }
                    else if (structure[i*20+j]=='I'){
                        grid[i][j]=GameObject::PLAYER;
                        position.x = j;
                        position.y = i;
                    }
                    else if(structure[i*20+j]=='S'){
                        spawns.push_back({"", sf::Vector2f(i,j)});
                    }
                }
            }

            for(int i = 0; i < spawns.size(); i++)
                std::cout << spawns[i].position.x << " - " << spawns[i].position.y << std::endl;

            packet.clear();
            int numberOfSpawns = spawns.size();
            packet<<numberOfSpawns;
            socket->send(packet);
        }
        else if(gameState == PLAY)
        {
            std::cout << "playing" << std::endl;
            std::thread recieve(Recieve, socket);
            DibujaSFML(socket, structure);

            recieve.join();
            gameState = SELECTMAP;
        }
    }while(true);

    socket->disconnect();
}




/**
 * Si guardamos las posiciones de los objectos con valores del 0 al 7,
 * esta función las transforma a posición de ventana (pixel), que va del 0 al 512
 */
sf::Vector2f BoardToWindows(sf::Vector2f _position)
{
    return sf::Vector2f(_position.x*LADO_CASILLA+OFFSET_AVATAR, _position.y*LADO_CASILLA+OFFSET_AVATAR);
}

void Recieve(sf::TcpSocket *socket)
{
    while(playing)
    {
        sf::Packet packet;
        int id;
        int spawnPos;
        std::string pokemonName;
        if(socket->receive(packet)!= sf::Socket::Status::Done)
            break;
        packet>>id>>spawnPos>>pokemonName;
        if(id==SPAWN){
            grid[spawns[spawnPos].position.x][spawns[spawnPos].position.y]=POKEMON;
            spawns[spawnPos].name = pokemonName;
        }
        else if(id == DISCONNECT)
            return;
    }
}

void DibujaSFML(sf::TcpSocket *socket, std::string structure)
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
                window.close();
                packet.clear();
                packet<<DISCONNECT;
                socket->send(packet);
                playing = false;
            }
            else if(event.type == sf::Event::KeyPressed){
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
                {
                    window.close();
                    packet.clear();
                    packet<<DISCONNECT;
                    socket->send(packet);
                    playing = false;
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
                {
                    if(grid[position.y][position.x - 1] != GameObject::OBSTACLE){
                        grid[position.y][position.x]=GameObject::NONE;
                        position.x--;
                        if(grid[position.y][position.x] == GameObject::POKEMON)
                        {
                            for(int i = 0; i < spawns.size(); i++)
                            {
                                if(spawns[i].position.x == position.y && spawns[i].position.y == position.x)
                                {
                                    packet.clear();
                                    packet<<CATCH<<i;
                                    socket->send(packet);
                                    std::cout << "You catched a: "<<spawns[i].name << std::endl;
                                }
                            }
                        }
                        grid[position.y][position.x]=GameObject::PLAYER;
                    }
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
                {
                    if(grid[position.y][position.x + 1] != GameObject::OBSTACLE)
                    {
                        grid[position.y][position.x]=GameObject::NONE;
                        position.x++;
                        if(grid[position.y][position.x] == GameObject::POKEMON)
                        {
                            for(int i = 0; i < spawns.size(); i++)
                            {
                                if(spawns[i].position.x == position.y && spawns[i].position.y == position.x)
                                {
                                    packet.clear();
                                    packet<<CATCH<<i;
                                    socket->send(packet);
                                    std::cout << "You catched a: "<<spawns[i].name << std::endl;
                                }
                            }
                        }
                        grid[position.y][position.x]=GameObject::PLAYER;
                    }
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
                {
                    if(grid[position.y - 1][position.x] != GameObject::OBSTACLE)
                    {
                        grid[position.y][position.x]=GameObject::NONE;
                        position.y--;
                        if(grid[position.y][position.x] == GameObject::POKEMON)
                        {
                            for(int i = 0; i < spawns.size(); i++)
                            {
                                if(spawns[i].position.x == position.y && spawns[i].position.y == position.x)
                                {
                                    packet.clear();
                                    packet<<CATCH<<i;
                                    socket->send(packet);
                                    std::cout << "You catched a: "<<spawns[i].name << std::endl;
                                }
                            }
                        }
                        grid[position.y][position.x]=GameObject::PLAYER;
                    }
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
                {
                    if(grid[position.y + 1][position.x] != GameObject::OBSTACLE)
                    {
                        grid[position.y][position.x]=GameObject::NONE;
                        position.y++;
                        if(grid[position.y][position.x] == GameObject::POKEMON)
                        {
                            for(int i = 0; i < spawns.size(); i++)
                            {
                                if(spawns[i].position.x == position.y && spawns[i].position.y == position.x)
                                {
                                    packet.clear();
                                    packet<<CATCH<<i;
                                    socket->send(packet);
                                    std::cout << "You catched a: "<<spawns[i].name << std::endl;
                                }
                            }
                        }
                        grid[position.y][position.x]=GameObject::PLAYER;
                    }
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
                sf::RectangleShape rectBlanco(sf::Vector2f(LADO_CASILLA,LADO_CASILLA));
                rectBlanco.setFillColor(sf::Color::White);
                if (grid[i][j]==GameObject::OBSTACLE){
                        rectBlanco.setPosition(sf::Vector2f(j*LADO_CASILLA, i*LADO_CASILLA));
                        window.draw(rectBlanco);
                }
                else if (grid[i][j]==GameObject::PLAYER){
                    sf::CircleShape shape(RADIO_AVATAR);
                    shape.setFillColor(sf::Color::Blue);
                    sf::Vector2f posicionDibujar = BoardToWindows(sf::Vector2f(j,i));
                    shape.setPosition(posicionDibujar);
                    window.draw(shape);
                }
                else if (grid[i][j]==GameObject::POKEMON){
                    sf::CircleShape shape(RADIO_AVATAR);
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
