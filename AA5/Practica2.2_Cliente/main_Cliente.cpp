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

enum GameObject {NONE, OBSTACLE, PLAYER, POKEMON};
enum ID {LOGIN, SIGNUP, MAPS, DISCONNECT};

int main(){
    sf::TcpSocket* socket = new sf::TcpSocket;
    sf::Socket::Status status = socket->connect("79.154.246.236", 50000, sf::seconds(5.f));

    if (status != sf::Socket::Done)
    {
      std::cout<<"no se ha podido establecer la conexion";
      socket->disconnect();
    }

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
            std::cout<<"Los datos introducidos son icorrectos"<<std::endl;
        else{
            int pokemonNumber;
            packet>>coins>>pokemonNumber;
            for(;pokemonNumber>0;pokemonNumber--){
                std::string pokemon;
                packet>>pokemon;
                std::cout<<pokemon<<std::endl;
                pokemonNames.push_back(pokemon);
            }
        }

    }while(!validName);

    int id;
    int numMapas;
    socket->receive(packet);
    packet>>id >> numMapas;

    for(;numMapas>0;numMapas--)
    {
        std::string nombre;
        std::string descripcion;
        packet>>nombre>>descripcion;
        std::cout<<nombre<<":     "<<descripcion<<std::endl<<std::endl<<std::endl<<std::endl;
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

    std::string structure;
    packet>>structure;
    std::cout << structure << std::endl;

    DibujaSFML(socket, structure);

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

void DibujaSFML(sf::TcpSocket *socket, std::string structure)
{
    sf::RenderWindow window(sf::VideoMode(640,640), "Interfaz cuadraditos");
    std::vector<std::vector<GameObject>> grid(GameObject::NONE);
    grid.resize(20,std::vector<GameObject>(20));

    for (int i =0; i<20; i++)
    {
        for(int j = 0; j<20; j++)
        {
            sf::RectangleShape rectBlanco(sf::Vector2f(LADO_CASILLA,LADO_CASILLA));
            rectBlanco.setFillColor(sf::Color::White);
            if (structure[i*20+j]=='O'){
                    grid[i][j]=GameObject::OBSTACLE;
            }
            else if (structure[i*20+j]=='I'){
                grid[i][j]=GameObject::PLAYER;
            }
        }
    }

    while(window.isOpen())
    {
        sf::Event event;

        //Este primer WHILE es para controlar los eventos del mouse
        while(window.pollEvent(event))
        {
            switch(event.type)
            {
                case sf::Event::Closed:
                    window.close();
                    sf::Packet packet;
                    packet<<DISCONNECT;
                    socket->send(packet);
                    break;
                case sf::Event::KeyPressed:
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
                    {
                        window.close();
                    }
                    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
                    {

                    }
                    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
                    {

                    }
                    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
                    {

                    }
                    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
                    {

                    }
                    break;
                default:
                    break;

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
            }
        }

        //TODO: Para pintar un círculo que podría simular el personaje
        window.display();
    }

}
