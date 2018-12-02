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

#define WINDOW_H 800
#define WINDOW_V 600
#define TITLE "Mi super practica 1.1"
#define MAX 1000

GraficoSFML graficos;

//Pipes
int fdPipe[2];
int estado = pipe(fdPipe);
int fdPipeC[2];
int estadoC = pipe(fdPipeC);
//Buffers
sf::Color bufClientes;
int bufMesas;
char buffer[1000];

//Cola de los clientes que llegan
std::queue<sf::Color> clientes;
//Array para saber cuando una mesa debe ser desocupada (contador)
int mesas[NUM_MESAS]{-1,-1,-1};
//Cuantos clientes hay sentados (mesas ocupadas)
int clientesEnMesa = 0;
//Cola de ints para saber que mesa desocupar
std::queue<int> queCliente;

bool isSent=false;
bool initMesasCounter = true;
int contadorClientesServidos = graficos.numClientesRestantes;

//Función para saber si la posición del ratón colisiona con alguna mesa o un dispensador
void Colisiones(sf::Vector2f&, std::vector<sf::Vector2f>, bool,pid_t);

//Métodos de signals
void timerUpdate(int);
void sendClient(int);
void readClient(int);
void initCounter(int);
void tableCounters(int);
void updateClients(int);

int main()
{
    srand(time(NULL));

    pid_t pid1 = fork();

    if(pid1==0)
    {
        /// Hijo Control de Llegada Clientes

        close(fdPipe[0]);
        close(fdPipeC);

        //Signal que envia al próximo cliente de la cola al ser llamado
        signal (SIGUSR1,sendClient);

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

        while(true);

        exit(0);
    }
    else
    {
        pid_t pid2 = fork();
        if(pid2==0)
        {
            ///Hijo Control de Vaciado de Mesas
            close(fdPipe);
            close(fdPipeC[1]);

            //Signal que inicia un contador para la mesa servida
            signal(SIGUSR2,initCounter);
            //Signal que actualiza el contador para las mesas servidas cada segundo
            //y avisa al padre que ese cliente debe irse cuando ese contador llegue a 5 segundos
            signal(SIGALRM,tableCounters);

            while(true);
            exit(0);

        }
        else
        {
            ///Padre Control del Pintado, Eventos y Lógica Básica
            sf::RenderWindow window(sf::VideoMode(WINDOW_H,WINDOW_V), TITLE);

            sf::Vector2f mousePos;
            bool clicked;
            bool lastClicked;

            close(fdPipe[1]);
            close(fdPipeC[0]);

            //Signal que decrementa el tiempo restante cada segundo
            signal(SIGALRM,timerUpdate);
            //Signal que lee el próximo cliente de la cola y lo coloca en un taburete vació (si hay alguno)
            signal(SIGUSR1,readClient);
            //Signal que actualiza los clientes en las mesas
            signal(SIGUSR2,updateClients);

            alarm(1);

            while(window.isOpen() && graficos.tiempoRestante > 0 && contadorClientesServidos > 0)
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

                // Lógica Movimiento jugador
                graficos.MueveJugador(mousePos);

                if(clicked&&!lastClicked)
                {
                    //Detección y reacción (coger comida) de colisiones con los dispensadores
                    Colisiones(graficos.posicionJugador, graficos.returnDispensadores(), false, pid2);
                    //Detección y reacción (soltar comida) de colisiones con las mesas
                    Colisiones(graficos.posicionJugador, graficos.returnMesas(), true, pid2);
                }

                // Petición Clientes
                if (graficos.tiempoRestante % 5 == 0 && graficos.tiempoRestante != 180 && !isSent && clientesEnMesa<3 && contadorClientesServidos>0 )
                {
                    //Se llama a sendClient del Hijo Control Llegada Clientes
                    kill(pid1,SIGUSR1);

                    isSent = true;
                    clientesEnMesa++;
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
            kill(pid1,SIGKILL);
            kill(pid2,SIGKILL);

            window.close();
        }
    }
    return 0;
}

//Cada segundo decrementa el valor del tiempo restante
//Además pone isSent a false también en cada segundo, así sólo se envia un cliente cuando se solicita y no uno en cada frame de ese segundo
void timerUpdate(int param)
{
    graficos.UpdateTimer();
    isSent=false;
    alarm(1);
}

//Envia por Pipe el siguiente cliente de la cola de clientes y lo elimina después.
//Llama a readClient
void sendClient(int param)
{
    write(fdPipe[1],&clientes.front(),sizeof(sf::Color));
    clientes.pop();
    kill(getppid(),SIGUSR1);

}

//Recibe por Pipe el cliente enviado y lo coloca en un taburete vacio aleatorio
void readClient(int param)
{
    read(fdPipe[0],&bufClientes,sizeof(sf::Color));
    bool isPlaced=false;
    do
    {
        int random = rand()% NUM_MESAS;

        if (graficos.TabureteVacio(random))
        {
            graficos.OcupaTaburete(random);
            graficos.PonPedido(random,bufClientes);
            isPlaced=true;
        }
    }while(!isPlaced);
}

//Lee por Pipe qué mesa ha sido servida e inicializa el contador de tiempo para desocupar ese taburete
void initCounter(int param)
{
    read(fdPipeC[0],&bufMesas,sizeof(int));
    mesas[bufMesas]=0;
    if(initMesasCounter)
    {
        alarm(1);
        initMesasCounter=false;
    }
}

//Actualiza los contadores de tiempo de los clientes servidos
//Cuando llega a 5 segundos se para el contador y se llama a updateClients
void tableCounters(int param)
{
    for (int i=0; i<NUM_MESAS;i++)
    {
        if(mesas[i]!=-1)
            mesas[i]++;
        if(mesas[i]==5){
            mesas[i]=-1;
            kill(getppid(),SIGUSR2);
        }
    }
    alarm(1);
}

//Se desocupa ese taburete y se decrementan los clientes sentados
void updateClients(int param)
{
    clientesEnMesa--;
    graficos.UpdateClients();
    graficos.aTaburetesADibujar[queCliente.front()].setFillColor(TABURETE_VACIO);
    queCliente.pop();
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
                if(graficos.manoDerecha.getFillColor()==graficos.aPedidosADibujar[0].getFillColor())
                {
                    //Quitar comida de la mano
                    graficos.DejaComida(graficos.manoDerecha.getFillColor());
                    //Se hace un push a la cola de ints con la posición de la mesa 0
                    queCliente.push(0);
                    //Se decrementa el contador de Clientes Servidos
                    contadorClientesServidos--;
                    //Se elimina el pedido de la mesa mientras el cliente come
                    graficos.aPedidosADibujar[0].setFillColor(PEDIDO_VACIO);

                    bufMesas=0;
                    write(fdPipeC[1],&bufMesas,sizeof(int));
                    //Se llama a initCounter
                    kill(pid2,SIGUSR2);
                }
                else if (graficos.manoIzquierda.getFillColor()==graficos.aPedidosADibujar[0].getFillColor())
                {
                    graficos.DejaComida(graficos.manoIzquierda.getFillColor());
                    queCliente.push(0);
                    contadorClientesServidos--;
                    graficos.aPedidosADibujar[0].setFillColor(PEDIDO_VACIO);
                    bufMesas=0;
                    write(fdPipeC[1],&bufMesas,sizeof(int));
                    kill(pid2,SIGUSR2);
                }
            }
            else //Si se colisiona con el primer dispensador se coge comida verde
                graficos.CogeComida(COMIDA_VERDE);
        }

        else if((pPos.y > disp[1].y) && (pPos.y < disp[1].y + mySize.y))
        {
            if(isTable)
            {
                if(graficos.manoDerecha.getFillColor()==graficos.aPedidosADibujar[1].getFillColor()){
                    graficos.DejaComida(graficos.manoDerecha.getFillColor());
                    queCliente.push(1);
                    contadorClientesServidos--;
                    graficos.aPedidosADibujar[1].setFillColor(PEDIDO_VACIO);
                    bufMesas=1;
                    write(fdPipeC[1],&bufMesas,sizeof(int));
                    kill(pid2,SIGUSR2);
                    }
                else if (graficos.manoIzquierda.getFillColor()==graficos.aPedidosADibujar[1].getFillColor()){
                    graficos.DejaComida(graficos.manoIzquierda.getFillColor());
                    queCliente.push(1);
                    contadorClientesServidos--;
                    graficos.aPedidosADibujar[1].setFillColor(PEDIDO_VACIO);
                    bufMesas=1;
                    write(fdPipeC[1],&bufMesas,sizeof(int));
                    kill(pid2,SIGUSR2);
                    }
            }
            else
                graficos.CogeComida(COMIDA_AMARILLA);
        }
        else if((pPos.y > disp[2].y) && (pPos.y < disp[2].y + mySize.y))
        {
            if(isTable)
            {
                if(graficos.manoDerecha.getFillColor()==graficos.aPedidosADibujar[2].getFillColor()){
                    graficos.DejaComida(graficos.manoDerecha.getFillColor());
                    queCliente.push(2);
                    contadorClientesServidos--;
                    graficos.aPedidosADibujar[2].setFillColor(PEDIDO_VACIO);
                    bufMesas = 2;
                    write(fdPipeC[1],&bufMesas,sizeof(int));
                    kill(pid2,SIGUSR2);
                    }
                else if (graficos.manoIzquierda.getFillColor()==graficos.aPedidosADibujar[2].getFillColor()){
                    graficos.DejaComida(graficos.manoIzquierda.getFillColor());
                    queCliente.push(2);
                    contadorClientesServidos--;
                    graficos.aPedidosADibujar[2].setFillColor(PEDIDO_VACIO);
                    bufMesas = 2;
                    write(fdPipeC[1],&bufMesas,sizeof(int));
                    kill(pid2,SIGUSR2);
                    }
            }
            else
                graficos.CogeComida(COMIDA_ROJA);
        }
    }
}


//Código utilizado para generar clientes con pedidos aleatorios en el Hijo Control Llegada Clientes, no usado en la entrega

//        int random;
//        for (int i = 0; i<5; i++){
//            random=rand()%3;
//            std::cout<<random<<std::endl;
//            switch (random){
//                case 0:
//                    clientes.push(sf::Color(255,0,0,255));
//                    break;
//                case 1:
//                    clientes.push(sf::Color(255,255,0,255));
//                    break;
//                case 2:
//                    clientes.push(sf::Color(0,255,0,255));
//                    break;
//                default:
//                    break;
//            }
//        }


