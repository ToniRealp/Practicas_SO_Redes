#include <iostream>
#include <string>
#include <ctime>
#include <jdbc/mysql_connection.h>
#include <jdbc/mysql_driver.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>

int main(){
try{
    sql::Driver* driver = sql::mysql::get_driver_instance();
    sql::Connection* conn = driver->connect("tcp://83.59.240.38","root","linux123");
    conn->setSchema("PracticaRedesAA4");

    sql::Statement* stmt =conn->createStatement();
    sql::ResultSet* user;
    sql::ResultSet* pokemons;
    tm* logInTime, * logOutTime;
    char logInBuf [22];
    char logOutBuf [22];
    std::string userName, password;
    bool validName = false;

    do{
        std::cout<<"Username:"<<std::endl;
        std::cin>>userName;
        if(userName!="nuevo"){
        ///login usuario
            std::cout<< "Password:"<<std::endl;
            std::cin>>password;
            user = stmt->executeQuery("SELECT * FROM Player WHERE Player.Username = '"+ userName + "'" "AND Player.Password = '"+ password + "'");
            if(user->next()){
                validName=true;
                std::cout<<std::endl<<"Monedas: "<<std::endl<<user->getString("Coins")<<std::endl<<std::endl;
                pokemons = stmt->executeQuery("SELECT * FROM Pokemons WHERE ID IN (SELECT PokemonID FROM PokemonXplayer WHERE PlayerID ="+user->getString("ID")+")");
                std::cout<<"Pokemons: "<<std::endl;
                if(pokemons->next()){
                     do{
                     std::cout<<"   "<<pokemons->getString("Name")<<std::endl;
                     }while(pokemons->next());
                }else{
                    std::cout<< "You have no pokemons"<<std::endl;
                }
            }else
            std::cout<<"Your username or password is incorrect"<<std::endl;
        }else{
            ///Sign in
            std::cout<<"New Username:"<<std::endl;
            std::cin>>userName;
            user = stmt->executeQuery("SELECT count(*) FROM Player where Player.Username = '" + userName +"'");
            user->next();
            if(user->getInt(1)==0){
            bool pwdMatch=false;
                do{
                    std::string confirmPassword;
                    std::cout<< "New Password:"<<std::endl;
                    std::cin>>password;
                    std::cout<< "Confirm Password:"<<std::endl;
                    std::cin>>confirmPassword;
                    if(password==confirmPassword){
                        stmt->executeUpdate("INSERT INTO Player(Player.Username, Player.Password, Player.Coins) VALUES ('"+userName+"','"+password+"', 0)");
                        validName=true;
                        pwdMatch=true;
                    }
                }while(!pwdMatch);
            }else
                std::cout<<"Username already exists"<<std::endl;

        }
    }while(!validName);

    std::cout<<std::endl<<std::endl<<std::endl;

    bool mapSelected = false;
    std::string mapName;
    sql::ResultSet* maps = stmt->executeQuery("SELECT * FROM Mapas");

    while(maps->next()){
            std::cout <<maps->getString("Name") << std::endl << maps->getString("Description") << std::endl;
        }

    do {
        std::cout << "Choose a map:"<<std::endl;
        std::cin >> mapName;
        maps->beforeFirst();
        while(maps->next()){
            if(mapName==maps->getString("Name")){
                mapSelected = true;
                std::cout<<"Map "<<mapName<<" selected";
                time_t now = time(0);
                logInTime=localtime(&now);
                break;
            }
        }
    }while(!mapSelected);


    std::cout << "\033[2J\033[1;1H";
    std::cout<<"You are playing: "<<mapName<<std::endl;
    bool isPlaying =true;
    std::string exit;
    while(isPlaying){



        std::cin>>exit;
        if(exit=="exit"){
            strftime(logInBuf,22,"%F %X\0",logInTime);
            std::string logInStr(logInBuf);

            time_t now = time(0);
            logOutTime=localtime(&now);
            strftime(logOutBuf,22,"%F %X\0",logOutTime);
            std::string logOutStr(logOutBuf);

            stmt->executeUpdate("INSERT INTO Sesion(PlayerID, LogInTime, LogOutTime) VALUES ('"+user->getString("ID")+"','"+logInStr+"','"+logOutStr+"')");
            isPlaying=false;
        }
    }



    user->close();
    delete(user);
    stmt->close();
    delete(stmt);
    conn->close();
    delete(conn);

    }catch(sql::SQLException& e){
        std::cout<<"salto excepcion\n";
    }
    return 0;
}

