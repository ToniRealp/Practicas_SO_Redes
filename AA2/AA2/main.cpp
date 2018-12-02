#include <iostream>
#include <cstdlib>
#include <stdlib.h>
#include <SFML/Graphics.hpp>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>
#include <vector>
#include <string.h>
#include <queue>
#include <time.h>
#include <fcntl.h>
#include "GraficoSFML.h"
#include "../../rapidxml/rapidxml.hpp"
#include "../../rapidxml/rapidxml_print.hpp"
#include "../../rapidxml/rapidxml_utils.hpp"

#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/sem.h>


#define WINDOW_H 800
#define WINDOW_V 600
#define TITLE "Mi super practica 1.1"
#define MAX 1000

GraficoSFML graficos;

//Buffers
char buffer[1000];

//Cola de los clientes que llegan
std::queue<sf::Color> clientes;
sf::Color mesas[NUM_MESAS];
//contador
int contadorMesas[NUM_MESAS] = {0,0,0};

//Reserva de memoria compartida para el color de las mesas y el contador de clientes servidos
int id = shmget(IPC_PRIVATE,sizeof(mesas),IPC_CREAT|0600);
sf::Color* mesasP = (sf::Color*)shmat(id,NULL,0);
int id2 = shmget(IPC_PRIVATE,sizeof(int),IPC_CREAT|0600);
int* contadorClientes = (int*)shmat(id2,NULL,0);

//Semaforos
int semID = semget(IPC_PRIVATE,1,IPC_CREAT|0600);

union semun
{
    int val;
    //Se utiliza junto con cmd = SETVAL
    struct semid_ds* buf;
    //Con cmd = IPC_STAT, IPC_SET
    unsigned short* array;
    //Con cmd = GETALL, SETALL
};

//Declaracion de los semaforos
union semun arg;
sembuf opSemWait;
sembuf opSemSignal;

//Contador para llevar el numero de clientes enviados y asi evitar la llegada de clientes fantasma
int clientesEnviados;

//Función para saber si la posición del ratón colisiona con alguna mesa o un dispensador
void Colisiones(sf::Vector2f&, std::vector<sf::Vector2f>, bool,pid_t);

//Métodos de signals
void timerUpdate(int);
void sendClient(int);
void tableCounters(int);
//Funciones para printear el color de las mesas y asi debugar
void PrintColor(sf::Color _color)
{

        if(_color == sf::Color::Red)
            std::cout << "Cliente  es de color rojo" << std::endl;
        else if(_color == sf::Color::Yellow)
            std::cout << "Cliente es de color amarillo" << std::endl;
        else if(_color == sf::Color::Green)
            std::cout << "Cliente es de color verde" << std::endl;
        else if(_color == sf::Color::Black)
            std::cout << "Cliente es de color negro" << std::endl;
        else
            std::cout << "Cliente no tiene color: ERROR" << std::endl;

}
void PrintMesas()
{
    for(int i=0;i<NUM_MESAS;i++){
        if(mesasP[i] == sf::Color::Red)
            std::cout << "Mesa"<< i << "Roja" << std::endl;
        else if(mesasP[i] == sf::Color::Yellow)
            std::cout << "Mesa"<< i <<  "amarilla" << std::endl;
        else if(mesasP[i] == sf::Color::Green)
            std::cout << "Mesa"<< i <<  "verde" << std::endl;
        else if(mesasP[i] == sf::Color::Black)
            std::cout << "Mesa"<< i <<  "negra" << std::endl;
        else if(mesasP[i] == sf::Color::White)
            std::cout << "Mesa"<< i <<  "blanca" << std::endl;
        else
            std::cout << "Cliente no tiene color: ERROR" << std::endl;
        }
}


int main()
{
    srand(time(NULL));
    //Inicializacion de las mesas en la memoria compartida
    mesasP[0]=PEDIDO_VACIO;
    mesasP[1]=PEDIDO_VACIO;
    mesasP[2]=PEDIDO_VACIO;
    //Inicializacion del contador de clientes en memoria compartida
    *contadorClientes = graficos.numClientesRestantes;
    //Inicializacion valor del semafor
    arg.val=1;
    semctl(semID,0,SETVAL,arg);
    //Inicializacion wait
    opSemWait.sem_num = 0;
    opSemWait.sem_op = -1;
    opSemWait.sem_flg = SEM_UNDO;
    //Inicializacion signal
    opSemSignal.sem_num = 0;
    opSemSignal.sem_op = 1;

    pid_t pid1 = fork();

    if(pid1==0)
    {
        /// Hijo Control de Llegada Clientes
        clientesEnviados=0;
        //Se lee el fichero xml y se vuelcan sus datos en la cola de clientes
        int fd = open("res/clientes.xml",O_RDONLY);
        read(fd,buffer,MAX);
        rapidxml::xml_document<> doc;
        doc.parse<0>(&buffer[0]);
        rapidxml::xml_node<> *pRoot=doc.first_node();
        for(rapidxml::xml_node<> *pNodeI = pRoot->first_node("pedido");pNodeI; pNodeI=pNodeI->next_sibling())
        {
            clientes.push(sf::Color(atoi(pNodeI->first_attribute("R")->value()),atoi(pNodeI->first_attribute("G")->value()),atoi(pNodeI->first_attribute("B")->value())));
        }

        //Signal que envia al próximo cliente de la cola al ser llamado
        signal (SIGALRM,sendClient);
        alarm(5);
        while(true){
            pause();
        };

        exit(0);
    }
    else
    {
        pid_t pid2 = fork();
        if(pid2==0)
        {
            ///Hijo Control de Vaciado de Mesas
            //Signal que actualiza el contador para las mesas servidas cada segundo
            signal(SIGALRM,tableCounters);
            alarm(1);
            while(true){
            pause();
            };
            exit(0);
        }
        else
        {
            ///Padre Control del Pintado, Eventos y Lógica Básica
            sf::RenderWindow window(sf::VideoMode(WINDOW_H,WINDOW_V), TITLE);

            sf::Vector2f mousePos;
            bool clicked;
            bool lastClicked;

            //Signal que decrementa el tiempo restante cada segundo
            signal(SIGALRM,timerUpdate);

            alarm(1);

            while(window.isOpen() && graficos.tiempoRestante > 0 && *contadorClientes > 0)
            {
                /// Control de eventos
                sf::Event event;
                while(window.pollEvent(event))
                {
                    if (event.type == sf::Event::Closed)
                    {
                        window.close();
                    }
                    if (event.type == sf::Event::KeyPressed)
                    {
                        if (event.key.code == sf::Keyboard::Escape)
                        {
                            window.close();
                        }
                    }
                    if (event.type == sf::Event::MouseButtonPressed)
                    {
                        if (event.mouseButton.button == sf::Mouse::Left)
                        {
                          mousePos=sf::Vector2f(event.mouseButton.x, event.mouseButton.y);
                          clicked = true;
                        }
                    }
                }

                window.clear();

                /// Lógica Básica
                //Actualiza la interfaz grafica al estado de las mesas
                for(int i = 0; i<3;i++){
                    graficos.PonPedido(i,mesasP[i]);
                    if(mesasP[i]!=PEDIDO_VACIO)
                        graficos.OcupaTaburete(i);
                    if(mesasP[i]==PEDIDO_VACIO)
                        graficos.VaciaTaburete(i);
                }
                //Actualiza el contador de clientes de la interfaz
                graficos.UpdateClients(*contadorClientes);

                // Lógica Movimiento jugador
                graficos.MueveJugador(mousePos);

                if(clicked&&!lastClicked)
                {
                    //Detección y reacción (coger comida) de colisiones con los dispensadores
                    Colisiones(graficos.posicionJugador, graficos.returnDispensadores(), false, pid2);
                    //Detección y reacción (soltar comida) de colisiones con las mesas
                    Colisiones(graficos.posicionJugador, graficos.returnMesas(), true, pid2);
                }

                lastClicked=clicked;
                clicked=false;

                /// Pintado

                for(size_t i = 0; i < graficos.aTextosADibujar.size(); i++)
                {
                    window.draw(graficos.aTextosADibujar[i]);
                }

                for(size_t i = 0; i < graficos.aObjetosADibujar.size(); i++)
                {
                    window.draw(graficos.aObjetosADibujar[i]);
                }

                for (size_t i =0 ; i< NUM_MESAS; i++)
                {
                    window.draw(graficos.aTaburetesADibujar[i]);
                    window.draw(graficos.aPedidosADibujar[i]);
                }

                window.draw(graficos.jugador);
                window.draw(graficos.manoIzquierda);
                window.draw(graficos.manoDerecha);

                window.display();
            }
            //kill a los procesos hijos
            shmdt(mesasP);
            shmdt(contadorClientes);
            shmctl(id,IPC_RMID, NULL);
            shmctl(id2,IPC_RMID, NULL);
            semctl(semID,0,IPC_RMID,arg);
            kill(pid1,SIGKILL);
            kill(pid2,SIGKILL);

            window.close();
        }
    }
    return 0;
}

//Cada segundo decrementa el valor del tiempo restante
void timerUpdate(int param)
{
    graficos.UpdateTimer();
    alarm(1);
}

//Handler del alarm envia clientes cada 5 segundos
void sendClient(int param)
{
    bool searchingSit=true;
    int i;
    while(searchingSit&&clientesEnviados<5){
        i=rand()%3;
        if(mesasP[i]==PEDIDO_VACIO){
            semop(semID,&opSemWait,1);
            mesasP[i]=clientes.front();
            semop(semID,&opSemSignal,1);
            searchingSit=false;
            clientesEnviados++;
        }
    }
    clientes.pop();
    alarm(5);
}

//Handler del alarm que cuenta el tiempo de comida de cada cliente y los elimina al acabar
void tableCounters(int param)
{
    for (int i=0; i<NUM_MESAS;i++)
    {
        if(mesasP[i]==PEDIDO_SERVIDO)
            contadorMesas[i]++;

        if(contadorMesas[i]==5){
            semop(semID,&opSemWait,1);
            mesasP[i]=PEDIDO_VACIO;
            semop(semID,&opSemSignal,1);
            contadorMesas[i]=0;
            *contadorClientes-=1;
        }
    }
    alarm(1);
}

//Colisiones Mouse-Dispensadores / Mouse-Mesas
//Recibe por parámetro la posicion del ratón, las posiciones de los objetos con los que colisiona,
//un bool que indica si es una mesa o un dispensador y el pid del Hijo Control Vaciado de Mesas
void Colisiones(sf::Vector2f &pPos, std::vector<sf::Vector2f> disp, bool isTable, pid_t pid2)
{
    //Dimensiones de los objetos con los que detectar colisiones
    sf::Vector2f mySize;
    isTable ? mySize = MESA_SIZE : mySize = DISPENSADOR_SIZE;

    if ((pPos.x > disp[0].x) && (pPos.x < disp[0].x + mySize.x))
    {
        if((pPos.y > disp[0].y) && (pPos.y < disp[0].y + mySize.y))
        {
            //Si se colisiona con las mesas
            if(isTable)
            {
                //Si la comida en la mano derecha es la misma que la mesa 0 (con la que ha colisionado)
                if(graficos.manoDerecha.getFillColor()==graficos.aPedidosADibujar[0].getFillColor()&&graficos.aPedidosADibujar[0].getFillColor()!=PEDIDO_VACIO)
                {
                    //Quitar comida de la mano
                    graficos.DejaComida(graficos.manoDerecha.getFillColor());
                    //la mesa correspondiente pasa a estado servido
                    mesasP[0]=PEDIDO_SERVIDO;

                }
                else if (graficos.manoIzquierda.getFillColor()==graficos.aPedidosADibujar[0].getFillColor()&&graficos.aPedidosADibujar[0].getFillColor()!=PEDIDO_VACIO)
                {
                    graficos.DejaComida(graficos.manoIzquierda.getFillColor());
                    mesasP[0]=PEDIDO_SERVIDO;
                }
            }
            else //Si se colisiona con el primer dispensador se coge comida verde
                graficos.CogeComida(COMIDA_VERDE);
        }

        else if((pPos.y > disp[1].y) && (pPos.y < disp[1].y + mySize.y))
        {
            if(isTable)
            {
                if(graficos.manoDerecha.getFillColor()==graficos.aPedidosADibujar[1].getFillColor()&&graficos.aPedidosADibujar[1].getFillColor()!=PEDIDO_VACIO){
                    graficos.DejaComida(graficos.manoDerecha.getFillColor());
                    mesasP[1]=PEDIDO_SERVIDO;
                    }
                else if (graficos.manoIzquierda.getFillColor()==graficos.aPedidosADibujar[1].getFillColor()&&graficos.aPedidosADibujar[1].getFillColor()!=PEDIDO_VACIO){
                    graficos.DejaComida(graficos.manoIzquierda.getFillColor());
                    mesasP[1]=PEDIDO_SERVIDO;;
                    }
            }
            else
                graficos.CogeComida(COMIDA_AMARILLA);
        }
        else if((pPos.y > disp[2].y) && (pPos.y < disp[2].y + mySize.y))
        {
            if(isTable)
            {
                if(graficos.manoDerecha.getFillColor()==graficos.aPedidosADibujar[2].getFillColor()&&graficos.aPedidosADibujar[2].getFillColor()!=PEDIDO_VACIO){
                    graficos.DejaComida(graficos.manoDerecha.getFillColor());
                    mesasP[2]=PEDIDO_SERVIDO;
                    }
                else if (graficos.manoIzquierda.getFillColor()==graficos.aPedidosADibujar[2].getFillColor()&&graficos.aPedidosADibujar[2].getFillColor()!=PEDIDO_VACIO){
                    graficos.DejaComida(graficos.manoIzquierda.getFillColor());
                    mesasP[2]=PEDIDO_SERVIDO;
                    }
            }
            else
                graficos.CogeComida(COMIDA_ROJA);
        }
    }
}
