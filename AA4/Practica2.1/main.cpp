#include <iostream>
#include <string>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/mysql_driver.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>

int main(){
try{
	///Se inicializa la conexión a la BD PracticaRedesAA4
    sql::Driver* driver = sql::mysql::get_driver_instance();
    sql::Connection* conn = driver->connect("tcp://127.0.0.1","root","linux123");
    conn->setSchema("PracticaRedesAA4");

    sql::Statement* stmt =conn->createStatement();

    sql::ResultSet* user;
    sql::ResultSet* pokemons;

    time_t logInSec, logOutSec;

    std::string userName, password;

    bool validName = false;

    do{
		///Login / Sign In
		//Se pregunta el Username
        std::cout<<"Username:"<<std::endl;
        std::cin>>userName;
		//Si el username es diferente a "nuevo" accedemos al Login (iniciar sesión)
        if(userName!="nuevo"){
			///Login
			//Se pregunta la contraseña
            std::cout<< "Password:"<<std::endl;
            std::cin>>password;
			//Se accede a la BD para comprobar si existe algun usario con esa combinación de Username y Password
            user = stmt->executeQuery("SELECT * FROM Player WHERE Player.Username = '"+ userName + "'" "AND Player.Password = '"+ password + "'");
			//Si existe algun usuario con esa combinación de Username y Password se inicia sesión en esa cuenta
            if(user->next()){
                validName=true;
				//Se printea la información de esa cuenta por pantalla
                std::cout<<std::endl<<"Monedas: "<<std::endl<<user->getString("Coins")<<std::endl<<std::endl;
				//Se accede a la BD para poder printear los pokemons que tiene la cuenta
                pokemons = stmt->executeQuery("SELECT * FROM Pokemons WHERE ID IN (SELECT PokemonID FROM PokemonXplayer WHERE PlayerID ="+user->getString("ID")+")");
                std::cout<<"Pokemons: "<<std::endl;
                if(pokemons->next()){
                     do{
                     std::cout<<"   "<<pokemons->getString("Name")<<std::endl;
                     }while(pokemons->next());
                }else{
					//Si no tiene pokemons
                    std::cout<< "You have no pokemons"<<std::endl;
                }
            }else //Si no existe ningun usuario con esa combinación de Username y Password vuelve a preguntar Username y Password
            std::cout<<"Your username or password is incorrect"<<std::endl;
        }else{
            ///Sign in
			//Se pregunta el Username
            std::cout<<"New Username:"<<std::endl;
            std::cin>>userName;
			//Se comprueba que no exista otro jugador con ese mismo Username
            user = stmt->executeQuery("SELECT count(*) FROM Player where Player.Username = '" + userName +"'");
            user->next();
			//Si no existe ningun otro jugador con ese mismo Username
            if(user->getInt(1)==0){
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
                        stmt->executeUpdate("INSERT INTO Player(Player.Username, Player.Password, Player.Coins) VALUES ('"+userName+"','"+password+"', 0)");
                        validName=true;
                        pwdMatch=true;
                    }
                }while(!pwdMatch);
            }else //Si existe otro jugador con ese mismo username se informa al usuario y se vuelve a iniciar el proceso
                std::cout<<"Username already exists"<<std::endl;
        }
    }while(!validName);

    std::cout<<std::endl<<std::endl<<std::endl;

	//Se accede a la BD para sacar la información de los mapas, que son printeados en pantalla (nombre y descripción)
    bool mapSelected = false;
    std::string mapName;
    sql::ResultSet* maps = stmt->executeQuery("SELECT * FROM Mapas");

    while(maps->next()){
            std::cout <<maps->getString("Name") << std::endl << maps->getString("Description") << std::endl;
        }

	//Se pregunta al usuario que mapa quiere jugar. Debe escribir el nombre del mapa
    do {
        std::cout << "Choose a map:"<<std::endl;
        std::cin >> mapName;
        maps->beforeFirst();
        while(maps->next()){
			//Se comprueba que el mapa escrito exista en la BD
            if(mapName==maps->getString("Name")){
                mapSelected = true;
                std::cout<<"Map "<<mapName<<" selected";
                logInSec = time(0);

                break;
            }
        }
    }while(!mapSelected);

	//Se limpia la consola
    std::cout << "\033[2J\033[1;1H";
	//Se informa que se está jugando en el mapa elejido (JUGANDO...)
    std::cout<<"You are playing: "<<mapName<<std::endl;
    bool isPlaying =true;
    std::string exit;

    while(isPlaying){

        std::cin>>exit;
        if(exit=="exit"){
        //Utilizamos el struct tm dentro de <time> que nos da toda la info necesaria para la fecha
            tm* logInTime, * logOutTime;
            char logInBuf [22];
            char logOutBuf [22];
        //La funcion strftime nos permite formatear la string con la fecha y hora para que se ajuste al formato de datetime de la bd
            logInTime=localtime(&logInSec);
            strftime(logInBuf,22,"%F %X\0",logInTime);
            std::string logInStr(logInBuf);

            time_t logOutSec = time(0);
            logOutTime=localtime(&logOutSec);
            strftime(logOutBuf,22,"%F %X\0",logOutTime);
            std::string logOutStr(logOutBuf);

            stmt->executeUpdate("INSERT INTO Sesion(PlayerID, LogInTime, LogOutTime) VALUES ("+user->getString("ID")+",'"+logInStr+"','"+logOutStr+"')");

        //Se calcula el tiempo de duracion de cada sesion y se suma al previo para llevar un contador de minutos jugados por jugador
            int secondsPlayed = user->getInt("SecondsPlayed");
            secondsPlayed+=difftime(logOutSec,logInSec);

            std::string temp = std::to_string(secondsPlayed);
            stmt->executeUpdate("UPDATE Player SET SecondsPlayed =" + temp + " WHERE Player.ID = "+user->getString("ID"));
            isPlaying=false;
        }
    }

	//Se libera memoria
	stmt->close();
	delete(stmt);
	conn->close();
	delete(conn);
    user->close();
    delete(user);
	pokemons->close();
	delete(pokemons);
	maps->close();
	delete(maps);

    }catch(sql::SQLException& e){
        std::cout<<"salto excepcion\n";
    }
    return 0;
}
