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
#define SIZE_TABLERO 64
#define SIZE_FILA_TABLERO 8
#define LADO_CASILLA 64
#define RADIO_AVATAR 25.f
#define OFFSET_AVATAR 5

#define SIZE_TABLERO 64
#define LADO_CASILLA 64
#define RADIO_AVATAR 25.f
#define OFFSET_AVATAR 5

void DibujaSFML(sf::TcpSocket*);

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

    DibujaSFML(socket);

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

void DibujaSFML(sf::TcpSocket *socket)
{
    sf::Vector2f posicion(4.f, 4.f);
    sf::RenderWindow window(sf::VideoMode(512,512), "Interfaz cuadraditos");
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
                        posicion.x--;
                    }
                    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
                    {
                        posicion.x++;
                    }
                    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
                    {
                        posicion.y--;
                    }
                    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
                    {
                        posicion.y++;
                    }
                    break;
                default:
                    break;

            }
        }

        window.clear();

        //A partir de aquí es para pintar por pantalla
        //Este FOR es para el mapa --> Esto es para pintar casillas estilo tablero de ajedrez
        for (int i =0; i<8; i++)
        {
            for(int j = 0; j<8; j++)
            {
                sf::RectangleShape rectBlanco(sf::Vector2f(LADO_CASILLA,LADO_CASILLA));
                rectBlanco.setFillColor(sf::Color::White);
                if(i%2 == 0)
                {
                    //Empieza por el blanco
                    if (j%2 == 0)
                    {
                        rectBlanco.setPosition(sf::Vector2f(i*LADO_CASILLA, j*LADO_CASILLA));
                        window.draw(rectBlanco);
                    }
                }
                else
                {
                    //Empieza por el negro
                    if (j%2 == 1)
                    {
                        rectBlanco.setPosition(sf::Vector2f(i*LADO_CASILLA, j*LADO_CASILLA));
                        window.draw(rectBlanco);
                    }
                }
            }
        }

        //TODO: Para pintar un círculo que podría simular el personaje
        sf::CircleShape shape(RADIO_AVATAR);
        shape.setFillColor(sf::Color::Blue);
        sf::Vector2f posicionDibujar = BoardToWindows(posicion);
        shape.setPosition(posicionDibujar);
        window.draw(shape);
        window.display();
    }

}
