#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

void send(sf::TcpSocket* socket){

    bool validName = false;
    sf::Packet packet;
    std::string username, password;
    do{
		///Login / Sign In
		//Se pregunta el Username
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
    sf::Socket::Status status = socket.connect("10.38.1.9", 50000, sf::seconds(15.f));
    sf::Thread sendThread(send,&socket);
    sf::Thread receiveThread(receive,&socket);
    if (status != sf::Socket::Done)
    {
      std::cout<<"no se ha podido establecer la conexion";
    }
    else{
        sendThread.launch();
        receiveThread.launch();
    }
    receiveThread.wait();
    socket.disconnect();
}
