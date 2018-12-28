#include <iostream>
#include <unistd.h>
#include <SFML/Network.hpp>
#include <thread>

void send(sf::TcpSocket* socket){

    bool validName = false;

    std::string username, password;
    do{
		///Login / Sign In
		//Se pregunta el Username
        sf::Packet packet;
        std::cout<<"Username:"<<std::endl;
        std::cin>>username;
        if(username=="nuevo"){
            packet << username;
        }else{
            std::cout<<"Contraseña:"<<std::endl;
            std::cin>>password;
            packet << username << password;
        }

        socket->send(packet);
        socket->receive(packet);
        packet>>validName;
        std::cout<<validName;
    }while(!validName);
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
