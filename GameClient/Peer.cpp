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

enum Header { CREATE = 0, SHOW, ROOMS, SELECTEDROOM, COUNT };


void ManageConnections(MyNetwork::Socket* _sock, MyNetwork::Selector& _selector);
void GameManager(MyNetwork::Socket* _sock, std::vector<MyNetwork::Socket*>* _clientes);
void SendMessage(std::vector<MyNetwork::Socket*>* _clientes);
void CreateRoom(MyNetwork::Socket* sock);
void SearchForRoom(MyNetwork::Socket* sock);
int main() {

	std::cout << "Conectando al servidor" << std::endl;
	MyNetwork::Socket sock;
	//conecta con BSS
	MyNetwork::Status status = sock.Connect("localhost", 50000);
	if (status == MyNetwork::Status::DONE) {
		std::cout << "¡Conexion con el servidor establecida!" << std::endl;
		std::cout << std::endl;
	}
	else {

		std::cout << "ERROR al establecer conexion" << std::endl;
		std::cout << std::endl;
	}
	//CEAR o BUSCAR partida?
	std::cout << "Pulsa 1 para crear partida" << std::endl;
	std::cout << "Pulsa 2 para buscar partida" << std::endl;
	std::string str;
	std::cin >> str;
	bool correctComand = false;

	while (!correctComand) {
		if (str == "1") { //crear partida
			correctComand = true;
			CreateRoom(&sock);

		}
		else if (str == "2") { //buscar partida
			correctComand = true;
			SearchForRoom(&sock);
		}
		else {

			std::cout << "Comando incorrecto" << std::endl;
			std::cout << "Pulsa 1 para crear partida" << std::endl;
			std::cout << "Pulsa 2 para buscar partida" << std::endl;

			std::cin >> str;

		}
	}

	MyNetwork::Selector selector;
	selector.Add(&sock);
	std::vector<MyNetwork::Socket*> _clientes;

	/*std::thread tMessage(SendMessage, &_clientes);
	tMessage.detach();*/

	ManageConnections(&sock, selector);

	return 0;
}


void GameManager(MyNetwork::Socket* _sock, std::vector<MyNetwork::Socket*>* _clientes) {

	//GESTION PARTIDA

	////Creamos un contenedor rececptor de datos
	//InputMemoryStream* ims = nullptr;

	////Recibimos 
	//size_t br = 0;
	//char buffer[1000];
	//status = _sock->Receive(&ims, buffer, 1000, br);

	//if (status != MyNetwork::Status::DONE) {
	//	std::cout << "Error al recibir el mensaje" << std::endl;
	//}

	//int currentClients = 0;
	//ims->Read(&currentClients);

	//std::vector<Player> players;

	////Hacemos conexion con los peers recibidos del server
	//for (int i = 0; i < currentClients; i++)
	//{
	//	std::string IP = "";
	//	IP = ims->ReadString();

	//	uint16_t port = 0;
	//	ims->Read(&port);

	//	std::cout << "Recibido: " << IP << " " << port << std::endl;
	//	Player newPlayer = { IP, port };
	//	players.push_back(newPlayer);
	//}

	//uint16_t localPort = _sock->GetLocalPort();

	////Desconectas del servidor
	//_sock->Disconnect();


	MyNetwork::Listener listener;
	/*
	status = listener.Listen(localPort);
	if (status != sf::Socket::Status::Done) {
		std::cout << "Error al escuchar por el puerto " << localPort << std::endl;
		char exit;
		std::cin >> exit;
		return;
	}*/

	//Creamos el selector
	MyNetwork::Selector selector;

	//Add the listener to the selector para gestionarlo despues
	selector.Add(&listener);

	////Conectas con los nuevos clientes
	//for (int i = 0; i < players.size(); i++)
	//{
	//	MyNetwork::Socket* client = new MyNetwork::Socket();
	//	status = client->Connect(players[i].IP, players[i].PORT);
	//	if (status != MyNetwork::Status::DONE) {
	//		std::cout << "Error al conectar el cliente: " << players[i].IP << " " << players[i].PORT << std::endl;
	//		char exit;
	//		std::cin >> exit;
	//		return;
	//	}
	//	selector.Add(client);
	//	_clientes->push_back(std::move(client));
	//}

	//// Endless loop that waits for new connections
	//while (true)
	//{
	//	// Make the selector wait for data on any socket
	//	if (selector.Wait())
	//	{
	//		if (selector.IsReady(&listener)) {
	//			// The listener is ready: there is a pending connection
	//			MyNetwork::Socket* client = new MyNetwork::Socket;
	//			if (listener.Accept(client) == MyNetwork::Status::DONE)
	//			{
	//				// Add the new client to the clients list
	//				std::cout << "Llega el cliente con IP: " << client->GetRemoteAdress() << std::endl;
	//				std::cout << "Llega el cliente con puerto: " << client->GetRemotePort() << std::endl;
	//				_clientes->push_back(std::move(client));
	//				// Add the new client to the selector so that we will
	//				// be notified when he sends something
	//				selector.Add(client);
	//			}
	//			else
	//			{
	//				// Error, we won't get a new connection, delete the socket
	//				std::cout << "Error al recoger conexión nueva\n";
	//				delete client;
	//			}
	//		}
	//		else { //ha llegado un mensaje
	//			for (size_t i = 0; i < _clientes->size(); i++)
	//			{
	//				MyNetwork::Socket* client = _clientes->at(i);
	//				if (selector.IsReady(client))
	//				{

	//					InputMemoryStream* ims;

	//					// The client has sent some data, we can receive it
	//					size_t br = 0;
	//					char buffer[1000];
	//					status = client->Receive(&ims, buffer, 1000, br);
	//					if (status == sf::Socket::Done)
	//					{
	//						std::string strRec;
	//						strRec = ims->ReadString();
	//						std::cout << "He recibido " << strRec << " del puerto " << client->GetRemotePort() << std::endl;
	//					}
	//					else if (status == sf::Socket::Disconnected)
	//					{
	//						selector.Remove(client);
	//						_clientes->erase(_clientes->begin() + i);
	//						client->Disconnect();
	//						delete client;
	//						std::cout << "Elimino el socket que se ha desconectado\n";
	//						i--;
	//					}
	//					else
	//					{
	//						std::cout << "Error al recibir de " << client->GetRemotePort() << std::endl;
	//					}

	//				}
	//			}
	//		}
	//	}
	//}
}

//me he conectado ya al server
//me da la info de los demas
//me desconecto del server
//me conecto con los demas
void ManageConnections(MyNetwork::Socket* _sock, MyNetwork::Selector& _selector) {



	MyNetwork::Status status;
	std::vector<Room> roomList;

	bool hasReceived = false;
	while (!hasReceived)
	{
		// Make the selector wait for data on any socket
		if (_selector.Wait())
		{
			if (_selector.IsReady(_sock)) {
				InputMemoryStream* ims;
				char buffer[1000];
				size_t br = 0;
				status = _sock->Receive(&ims, buffer, 1000, br);
				if (status == MyNetwork::Status::DONE) {
					std::cout << "Paquete recibido" << std::endl;

				}
				else {
					std::cout << "ERROR al recibir el paquete" << std::endl;

				}
				int header;
				ims->Read(&header);

				switch (header)
				{
				case ROOMS:
					hasReceived = true;
					int numRooms;
					ims->Read(&numRooms);
					for (int i = 0; i < numRooms; i++)
					{
						Room newRoom;
						newRoom.name = ims->ReadString();
						ims->Read(&newRoom.hasPassword);
						ims->Read(&newRoom.currentPlayers);
						ims->Read(&newRoom.maxPlayers);
						roomList.push_back(newRoom);
					}


					break;
				default:
					break;
				}



			}
		}
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

	while (true)
	{

	}

}
void SendMessage(std::vector<MyNetwork::Socket*>* _clientes)
{
	while (true) {
		//OMS SEND PACKET
		OutputMemoryStream oms;
		std::string message;
		std::cin >> message;
		oms.WriteString(message);
		for (int i = 0; i < _clientes->size(); i++) {
			_clientes->at(i)->Send(&oms);
		}
	}
}

void CreateRoom(MyNetwork::Socket* sock) {

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
			std::cout << "Filtrar por partidas con contraseña? Y/N" << std::endl;
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

	//sock->Disconnect();


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

	sock->Send(&oms);
	//bool hasReceivedRooms = false;
	//while (!hasReceivedRooms)
	//{
	//	if (selecctor->Wait()) {
	//		if (selecctor->IsReady(sock)) {
	//			hasReceivedRooms = true;
	//			InputMemoryStream* ims;
	//			char buffer[1000];
	//			size_t br = 0;
	//			sock->Receive(&ims, buffer, 1000, br);
	//			std::cout << "Paquete recibido" << std::endl;
	//			int header;
	//			ims->Read(&header);


	//		}
	//	}
	//}

}