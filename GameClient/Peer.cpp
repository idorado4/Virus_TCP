#include <iostream>
#include <thread>
#include <SFML/Network.hpp>
#include <MyNetwork.h>
#include <InputMemoryStream.h>
#include <OutputMemoryStream.h>

struct Player {
	std::string IP;
	uint16_t PORT;
};



struct Room
{
	std::string name;
	bool hasPassword;
	int maxPlayers;
	int currentPlayers;
	//Los clientes conectados
	std::vector<Player> clients;

};

enum Header { CREATE = 0, SHOW, ROOMS, SELECTEDROOM, ACKJOIN, CHAT };


void ManageConnections(MyNetwork::Selector& _selector, std::vector<MyNetwork::Socket*>& connections);
void GameManager(MyNetwork::Socket* _sock, std::vector<MyNetwork::Socket*>* _clientes);
void SendMessage(std::vector<MyNetwork::Socket*>* _clientes);
void CreateRoom(MyNetwork::Socket* sock, std::vector<MyNetwork::Socket*>& connections);
void SearchForRoom(MyNetwork::Socket* sock);
void SelectRoom(InputMemoryStream& ims, MyNetwork::Socket* _sock);
bool JoinRoom(InputMemoryStream& ims, MyNetwork::Socket* sockToBss, std::vector<MyNetwork::Socket*> connections, MyNetwork::Selector& _selector, MyNetwork::Listener* listener);
void CreateOrSearchRoom(MyNetwork::Socket* sock, std::vector<MyNetwork::Socket*>& connections);


int main() {

	std::cout << "Conectando al servidor" << std::endl;
	MyNetwork::Socket* sock = new MyNetwork::Socket();
	//conecta con BSS
	MyNetwork::Status status = sock->Connect("localhost", 50000);
	if (status == MyNetwork::Status::DONE) {
		std::cout << "¡Conexion con el servidor establecida!" << std::endl;
		std::cout << std::endl;
	}
	else {

		std::cout << "ERROR al establecer conexion" << std::endl;
		std::cout << std::endl;
	}

	MyNetwork::Selector selector;
	selector.Add(sock);
	std::vector<MyNetwork::Socket*> conexiones;
	conexiones.push_back(std::move(sock));



	while (true)
	{
		CreateOrSearchRoom(sock, conexiones);
		ManageConnections(selector, conexiones);
	}



	return 0;
}

void CreateOrSearchRoom(MyNetwork::Socket* sock, std::vector<MyNetwork::Socket*>& connections) {
	//CEAR o BUSCAR partida?
	std::cout << "Pulsa 1 para crear partida" << std::endl;
	std::cout << "Pulsa 2 para buscar partida" << std::endl;
	std::string str;
	std::cin >> str;
	bool correctComand = false;

	while (!correctComand) {
		if (str == "1") { //crear partida
			correctComand = true;
			CreateRoom(sock, connections); //Cuando acaba esto, parar el bucle del main

		}
		else if (str == "2") { //buscar partida
			correctComand = true;
			SearchForRoom(sock);
		}
		else {

			std::cout << "Comando incorrecto" << std::endl;
			std::cout << "Pulsa 1 para crear partida" << std::endl;
			std::cout << "Pulsa 2 para buscar partida" << std::endl;

			std::cin >> str;
		}
	}
}

//me he conectado ya al server
//me da la info de los demas
//me desconecto del server
//me conecto con los demas
void ManageConnections(MyNetwork::Selector& _selector, std::vector<MyNetwork::Socket*>& connections) {

	MyNetwork::Status status;

	bool connectedToBss = true;

	MyNetwork::Listener* listener = new MyNetwork::Listener();

	while (connectedToBss)
	{
		// Make the selector wait for data on any socket
		if (_selector.Wait())
		{
			for (size_t i = 0; i < connections.size(); i++) {

				MyNetwork::Socket* connection = connections.at(i);

				if (_selector.IsReady(connection)) {
					InputMemoryStream* ims;
					char buffer[1000];
					size_t br = 0;
					status = connection->Receive(&ims, buffer, 1000, br);
					if (status != MyNetwork::Status::DONE) {
						_selector.Remove(connection);
						connections.erase(connections.begin() + i);
						connection->Disconnect();
						delete connection;
						std::cout << "Elimino el socket que se ha desconectado\n";
						i--;
						continue;
					}
					std::cout << "Paquete recibido" << std::endl;

					int header;
					ims->Read(&header);

					switch (header)
					{
					case ROOMS:
						SelectRoom(*ims, connection);
						break;
					case ACKJOIN:
						connectedToBss = JoinRoom(*ims, connection, connections, _selector, listener);
						break;
					case CHAT:
						std::cout << "CHAT: " << ims->ReadString() << std::endl;
						break;
					default:
						break;
					}
				}
				else //listener
				{
					MyNetwork::Socket* connection = new MyNetwork::Socket();
					if (listener->Accept(connection) == MyNetwork::Status::DONE) {
						std::cout << "Se ha establecido conexion!" << std::endl;

						connections.push_back(std::move(connection));
						//Add the new client to the selector so that we will
						//be notified when he sends something
						_selector.Add(connection);
					}
				}
			}
		}

	}
}

void SelectRoom(InputMemoryStream& ims, MyNetwork::Socket* _sock) {
	std::vector<Room> roomList;
	int numRooms;
	ims.Read(&numRooms);
	for (int i = 0; i < numRooms; i++)
	{
		Room newRoom;
		newRoom.name = ims.ReadString();
		ims.Read(&newRoom.hasPassword);
		ims.Read(&newRoom.currentPlayers);
		ims.Read(&newRoom.maxPlayers);
		roomList.push_back(newRoom);
	}

	std::cout << std::endl;
	std::cout << "----- LISTA DE SALAS ------" << std::endl;
	std::cout << std::endl;
	for (int i = 0; i < roomList.size(); i++)
	{
		std::cout << "SALA: " << roomList[i].name << std::endl;

		std::string hasPassword = roomList[i].hasPassword ? "true" : "false";
		std::cout << "Contraseña:" << hasPassword << std::endl;
		std::cout << "Jugadores: " << roomList[i].currentPlayers << "/" << roomList[i].maxPlayers << std::endl;
		std::cout << std::endl;
	}

	std::cout << "Escribe el nombre de la sala al que quieres unirte:" << std::endl;
	std::string tempString;
	Room selectedRoom;
	bool correctSelectedRoom = false;
	while (!correctSelectedRoom) {

		std::cin >> tempString;
		//recorro buscando si el nombre de la sala es correcto
		for (int i = 0; i < roomList.size(); i++)
		{
			if (roomList[i].name == tempString) {
				correctSelectedRoom = true;
				selectedRoom = roomList[i];
			}
			std::cout << "Sala " << roomList[i].name << " encontrada" << std::endl;
		}

		if (!correctSelectedRoom) {
			std::cout << "Esta sala no existe" << std::endl;
			std::cout << "Escribe el nombre de la sala al que quieres unirte:" << std::endl;
		}
	}

	if (selectedRoom.hasPassword) {
		std::cout << "Contrasenya de la sala: " << std::endl;
		std::cin >> tempString;
	}

	//Enviamos la selectedRoom al BSS
	OutputMemoryStream oms;
	int headerType = 3;
	oms.Write(headerType);
	oms.WriteString(selectedRoom.name);
	oms.WriteString(tempString);
	_sock->Send(&oms);

}

bool JoinRoom(InputMemoryStream& ims, MyNetwork::Socket* sockToBss, std::vector<MyNetwork::Socket*> connections, MyNetwork::Selector& _selector, MyNetwork::Listener* listener) {

	//Si recibo -1 es que no he conseguido entrar en la partida (por la contraseña)
	int numPlayers;
	ims.Read(&numPlayers);

	if (numPlayers == -1) {
		std::cout << "No has podido unirte a la partida. (Password incorrecta)" << std::endl;
		return false;
	}
	std::cout << "Password correcta!!" << std::endl;
	//establecemos conexion con los demas clientes de la sala
	for (int i = 0; i < numPlayers; i++)
	{
		std::string IPReceived;
		IPReceived = ims.ReadString();
		uint16_t portReceived;
		ims.Read(&portReceived);
		MyNetwork::Socket* newClient = new MyNetwork::Socket();
		newClient->Connect(IPReceived, portReceived);
		connections.push_back(std::move(newClient));
		_selector.Add(newClient);
	}

	//Me guardo el puerto del socket porqe ponemos a escuchar al listener por este
	uint16_t listenerPort = sockToBss->GetLocalPort();
	//elimino mi sock del selector
	_selector.Remove(sockToBss);
	//Me desconecto del server
	sockToBss->Disconnect();
	//libero memoria
	for (size_t i = 0; i < connections.size(); i++) {
		MyNetwork::Socket* client = connections.at(i);
		connections.erase(connections.begin() + i);
		break;
	}
	delete sockToBss;
	std::cout << "Elimino el socket conectado al server BSS\n";

	//Empiezo a escuchar por el puerto del sock descontado (listener)
	listener->Listen(listenerPort);
	//añado listener al selector
	_selector.Add(listener);

	//abro el chat
	std::thread tMessage(SendMessage, &connections);
	tMessage.detach();
	return true;
}

void SendMessage(std::vector<MyNetwork::Socket*>* _clientes)
{
	while (true) {
		std::cout << "Introduce mensaje" << std::endl;
		//OMS SEND PACKET
		OutputMemoryStream oms;
		oms.Write(5);
		std::string message;
		std::cin >> message;
		oms.WriteString(message);
		for (int i = 0; i < _clientes->size(); i++) {
			_clientes->at(i)->Send(&oms);
			std::cout << "mensaje mandado" << std::endl;
		}
	}
}

void CreateRoom(MyNetwork::Socket* sock, std::vector<MyNetwork::Socket*>& connections) {

	OutputMemoryStream oms;
	int headerType = 0;
	oms.Write(headerType);
	std::cout << "Introduce el nombre de la sala: " << std::endl;
	std::string tempString;
	std::cin >> tempString;
	oms.WriteString(tempString);
	std::cout << "Quieres contrasenya? Y/N" << std::endl;
	std::cin >> tempString;
	bool correctComandBoolPasword = false;
	while (!correctComandBoolPasword) {
		if (tempString == "Y" || tempString == "y") {
			correctComandBoolPasword = true;
			std::cout << "Introduce la contraseña de la sala: " << std::endl;
			std::cin >> tempString;
			oms.WriteString(tempString);
		}
		else if (tempString == "N" || tempString == "n") {
			correctComandBoolPasword = true;
			tempString = "";
			oms.WriteString(tempString);
		}
		else {
			std::cout << "comando incorrecto" << std::endl;
			std::cout << "Quieres contrasenya? Y/N" << std::endl;
			std::cin >> tempString;
		}
	}
	std::cout << "Introduce el maximo de jugadores de la sala: (de 2 a 4)" << std::endl;
	bool correctPlayerNum = false;
	int tempInt;
	while (!correctPlayerNum) {
		std::cin >> tempInt;
		if (tempInt >= 2 && tempInt <= 4) {
			oms.Write(tempInt);
			correctPlayerNum = true;
		}
		else {
			std::cout << "numero de jugadores maximos incorrecto" << std::endl;
			std::cout << "Introduce el maximo de jugadores de la sala: (de 2 a 4)" << std::endl;

		}
	}

	sock->Send(&oms);
	std::cout << "MENSAJE ENVIADO" << std::endl;

	sock->Disconnect();

	//abro el chat
	std::thread tMessage(SendMessage, &connections);
	tMessage.detach();

	//TODO tiene que ponerse a escuchar por el listener
}

void SearchForRoom(MyNetwork::Socket* sock) {
	OutputMemoryStream oms;
	int headerType = 1;
	oms.Write(headerType);
	std::cout << "Filtrar partidas? Y/N" << std::endl;
	std::string tempString;
	std::cin >> tempString;
	bool correctComand = false;
	bool correctComandNumPlayers = false;
	bool correctComandPassword = false;

	while (!correctComand)
	{
		if (tempString == "Y" || tempString == "y") {
			correctComand = true;
			//envio bool de que quiero filtrar partidas
			oms.Write(true);


			std::cout << "Filtrar por numero de players? Y/N" << std::endl;
			std::cin >> tempString;
			while (!correctComandNumPlayers)
			{
				if (tempString == "Y" || tempString == "y") {
					correctComandNumPlayers = true;
					std::cout << "Introduce el maximo de jugadores de la sala: (de 2 a 4)" << std::endl;
					bool correctPlayerNum = false;
					int tempInt;
					while (!correctPlayerNum) {
						std::cin >> tempInt;
						if (tempInt >= 2 && tempInt <= 4) {
							oms.Write(tempInt);
							correctPlayerNum = true;
						}
						else {
							std::cout << "numero de jugadores maximos incorrecto" << std::endl;
							std::cout << "Introduce el maximo de jugadores de la sala: (de 2 a 4)" << std::endl;

						}
					}
				}
				else if (tempString == "N" || tempString == "n") {
					oms.Write(-1);
					correctComandNumPlayers = true;
				}
				else {
					std::cout << "comando incorrecto" << std::endl;
					std::cout << "Filtrar partidas? Y/N" << std::endl;
					std::cin >> tempString;
				}
			}

			std::cout << "Filtrar por partidas con contraseña? Y/N" << std::endl;
			std::cin >> tempString;
			while (!correctComandPassword) {
				if (tempString == "Y" || tempString == "y") {
					correctComandPassword = true;
					oms.Write(true);
				}
				else if (tempString == "N" || tempString == "n") {
					correctComandPassword = true;
					oms.Write(false);
				}
				else {
					std::cout << "comando incorrecto" << std::endl;
					std::cout << "Filtrar por partidas con contraseña? Y/N" << std::endl;
					std::cin >> tempString;
				}
			}
		}
		else if (tempString == "N" || tempString == "n") {
			correctComand = true;
			//envio bool de que NO quiero filtrar partidas
			oms.Write(false);

		}
		else {
			std::cout << "comando incorrecto" << std::endl;
			std::cout << "Filtrar partidas? Y/N" << std::endl;
			std::cin >> tempString;
		}
	}

	std::cout << "Envio los filtros al server\n";
	sock->Send(&oms);
}