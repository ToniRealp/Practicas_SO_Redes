#include "GraficoSFML.h"

GraficoSFML::GraficoSFML():posicionJugador(sf::Vector2f(385.f,285.f)),jugador(30.f)
{
    //ctor

    tiempoRestante = 180;
    numClientesRestantes = 5;
    font.loadFromFile("courier.ttf");
    aOrigenMesas.push_back(sf::Vector2f(120,85));
    aOrigenMesas.push_back(sf::Vector2f(120,255));
    aOrigenMesas.push_back(sf::Vector2f(120,425));

    aOrigenDispensadores.push_back(sf::Vector2f(650, 85));
    aOrigenDispensadores.push_back(sf::Vector2f(650, 255));
    aOrigenDispensadores.push_back(sf::Vector2f(650, 425));

    InitContadorClientes();
    InitContadorTiempo();
    InitTaburetes();
    InitPedidos();
    InitObjetosFijos();
    InitPlayer();
}

void GraficoSFML::InitContadorTiempo()
{
    sf::Text txtCounterTime;
    txtCounterTime.setFont(font);
    txtCounterTime.setPosition(10,10);
    txtCounterTime.setFillColor(sf::Color::White);
    txtCounterTime.setCharacterSize(14);
    txtCounterTime.setString(std::to_string(tiempoRestante)+" seg.");
    aTextosADibujar.push_back(txtCounterTime);
}

void GraficoSFML::InitContadorClientes()
{
    sf::Text txtCounterClients;
    txtCounterClients.setFont(font);
    txtCounterClients.setPosition(120,10);
    txtCounterClients.setFillColor(sf::Color::White);
    txtCounterClients.setCharacterSize(14);
    txtCounterClients.setString(std::to_string(numClientesRestantes)+" clientes");
    aTextosADibujar.push_back(txtCounterClients);
}

void GraficoSFML::InitTaburetes()
{
    sf::CircleShape shape(30.f);
    shape.setFillColor(TABURETE_VACIO);
    shape.setPosition(sf::Vector2f(10,120));
    aTaburetesADibujar[0] = shape;
    shape.setPosition(sf::Vector2f(10,290));
    aTaburetesADibujar[1] = shape;
    shape.setPosition(sf::Vector2f(10,460));
    aTaburetesADibujar[2] = shape;

}

void GraficoSFML::InitPedidos()
{
    sf::RectangleShape shape;
    shape.setSize(sf::Vector2f(25,25));
    shape.setFillColor(PEDIDO_VACIO);

    shape.setPosition(sf::Vector2f(80,135));
    aPedidosADibujar[0] = shape;

    shape.setPosition(sf::Vector2f(80,305));
    aPedidosADibujar[1] = shape;

    shape.setPosition(sf::Vector2f(80,475));
    aPedidosADibujar[2] = shape;

}

void GraficoSFML::InitPlayer()
{
    jugador.setFillColor(sf::Color::Cyan);
    jugador.setPosition(posicionJugador);

    manoIzquierda.setSize(sf::Vector2f(25,25));
    manoIzquierda.setFillColor(PEDIDO_VACIO);
    manoIzquierda.setPosition(sf::Vector2f(posicionJugador.x-25,posicionJugador.y));

    manoDerecha.setSize(sf::Vector2f(25,25));
    manoDerecha.setFillColor(PEDIDO_VACIO);
    manoDerecha.setPosition(sf::Vector2f(posicionJugador.x+25+35,posicionJugador.y));

}

void GraficoSFML::InitObjetosFijos()
{
    sf::RectangleShape mesaShape;
    mesaShape.setSize(MESA_SIZE);
    mesaShape.setFillColor(sf::Color(150,150,150));

    for(size_t i = 0; i<NUM_MESAS; i++)
    {
        mesaShape.setPosition(aOrigenMesas[i]);
        aObjetosADibujar.push_back(mesaShape);
    }

    sf::RectangleShape dispensadorShape;
    dispensadorShape.setSize(DISPENSADOR_SIZE);
    dispensadorShape.setFillColor(sf::Color(150,150,150));

    for(size_t i = 0; i < aOrigenDispensadores.size(); i++)
    {
        dispensadorShape.setPosition(aOrigenDispensadores[i]);
        aObjetosADibujar.push_back(dispensadorShape);
    }

    sf::RectangleShape comidaDispensadorShape;
    comidaDispensadorShape.setSize(sf::Vector2f(25,25));
    comidaDispensadorShape.setFillColor(COMIDA_VERDE);
    comidaDispensadorShape.setPosition(sf::Vector2f(687,135));
    aObjetosADibujar.push_back(comidaDispensadorShape);
    comidaDispensadorShape.setFillColor(COMIDA_AMARILLA);
    comidaDispensadorShape.setPosition(sf::Vector2f(687,305));
    aObjetosADibujar.push_back(comidaDispensadorShape);
    comidaDispensadorShape.setFillColor(COMIDA_ROJA);
    comidaDispensadorShape.setPosition(sf::Vector2f(687,475));
    aObjetosADibujar.push_back(comidaDispensadorShape);


}

bool GraficoSFML::TabureteVacio(int _posicion)
{
    sf::Color color = aTaburetesADibujar[_posicion].getFillColor();
    if (color == TABURETE_VACIO)
    {
        return true;
    }
    return false;
}

void GraficoSFML::OcupaTaburete(int _posicion)
{
    aTaburetesADibujar[_posicion].setFillColor(TABURETE_OCUPADO);
}

void GraficoSFML::VaciaTaburete(int _posicion)
{
    aTaburetesADibujar[_posicion].setFillColor(TABURETE_VACIO);
}

void GraficoSFML::VaciaPedido(int _posicion)
{
    aPedidosADibujar[_posicion].setFillColor(PEDIDO_VACIO);
}

void GraficoSFML::PonPedido(int _posicion, sf::Color _queComida)
{
    aPedidosADibujar[_posicion].setFillColor(_queComida);
}

void GraficoSFML::CogeComida(sf::Color _queComida)
{
    if (manoIzquierda.getFillColor() == PEDIDO_VACIO)
    {
        manoIzquierda.setFillColor(_queComida);
        return;
    }
    else if (manoDerecha.getFillColor() == PEDIDO_VACIO)
    {
        manoDerecha.setFillColor(_queComida);
        return;
    }
}

bool GraficoSFML::DejaComida(sf::Color _queComida)
{
    if (manoIzquierda.getFillColor() == _queComida)
    {
        manoIzquierda.setFillColor(PEDIDO_VACIO);
        return true;
    }
    else if (manoDerecha.getFillColor() == _queComida)
    {
        manoDerecha.setFillColor(PEDIDO_VACIO);
        return true;
    }
    return false;
}

 void GraficoSFML::MueveJugador(sf::Vector2f _posicion)
 {
    posicionJugador = _posicion;
    jugador.setFillColor(sf::Color::Cyan);
    jugador.setPosition(posicionJugador);

    manoIzquierda.setSize(sf::Vector2f(25,25));
    manoIzquierda.setPosition(sf::Vector2f(posicionJugador.x-25,posicionJugador.y));

    manoDerecha.setSize(sf::Vector2f(25,25));
    manoDerecha.setPosition(sf::Vector2f(posicionJugador.x+25+35,posicionJugador.y));
 }

 void GraficoSFML::UpdateTimer(){
    tiempoRestante--;
    sf::Text txtCounterTime;
    txtCounterTime.setFont(font);
    txtCounterTime.setPosition(10,10);
    txtCounterTime.setFillColor(sf::Color::White);
    txtCounterTime.setCharacterSize(14);
    txtCounterTime.setString(std::to_string(tiempoRestante)+" seg.");
    aTextosADibujar[1]=txtCounterTime;
}

void GraficoSFML::UpdateClients(int _contadorClientes){
    sf::Text txtCounterClients;
    txtCounterClients.setFont(font);
    txtCounterClients.setPosition(120,10);
    txtCounterClients.setFillColor(sf::Color::White);
    txtCounterClients.setCharacterSize(14);
    txtCounterClients.setString(std::to_string(_contadorClientes)+" clientes");
    aTextosADibujar[0]=txtCounterClients;
}

std::vector<sf::Vector2f> GraficoSFML::returnMesas()
{
    return GraficoSFML::aOrigenMesas;
}

std::vector<sf::Vector2f> GraficoSFML::returnDispensadores()
{
    return GraficoSFML::aOrigenDispensadores;
}


GraficoSFML::~GraficoSFML()
{
    //dtor
}
