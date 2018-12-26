#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

void send(sf::TcpSocket* socket, std::string text){
    while(true){
        std::string data(text);
        if(socket->send(data.c_str(), 100)!=sf::Socket::Done){
            std::cout<<"error en el envio del paquete, el servidor se ha desconectado";
            break;
        }
        if(data=="exit")
            break;
    }
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
