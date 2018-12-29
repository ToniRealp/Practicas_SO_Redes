#include <iostream>
#include <unistd.h>
#include <SFML/Network.hpp>
#include <thread>

enum ID {LOGIN, SIGNUP};

void send(sf::TcpSocket* socket){

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

int main(){
    sf::TcpSocket socket;
    sf::Socket::Status status = socket.connect("79.154.246.236", 50000, sf::seconds(5.f));

    if (status != sf::Socket::Done)
    {
      std::cout<<"no se ha podido establecer la conexion";
      socket.disconnect();
    }

    std::thread sendThread(send,&socket);

    sendThread.join();
    socket.disconnect();
}
