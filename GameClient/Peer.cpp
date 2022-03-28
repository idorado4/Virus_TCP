#include <iostream>
#include <thread>
#include <SFML/Network.hpp>
#include <MyNetwork.h>
#include <InputMemoryStream.h>
#include <OutputMemoryStream.h>


bool end;


enum Type { ORGANO, VIRUS, MEDICINA, TRATAMIENTO };

enum Color { VERDE, ROJO, AZUL, AMARILLO, COMODIN };

struct Card {
	Type type;
	Color color;
	int ID;
};
struct Peer {
	std::string IP;
	uint16_t PORT;
};

struct Player {
	Card* hand[3];
	bool turn;
	std::vector<Card*> table;
	int ID;
};

//Se usa para la gestión de salas
struct Room
{
	std::string name;
	bool hasPassword;
	int maxPlayers;
	int currentPlayers;
};

//La partida como tal
struct Game {
	int currentPlayers;
	int maxPlayers;
	int seed;
	//Informacion de la partida
	int localId;
	std::vector<Card*>* deck;
	std::vector<Card*> discardPile; // = new std::vector<Card*>;
	std::vector<Player> players;
};


enum Header { CREATE = 0, SHOW, ROOMS, SELECTEDROOM, ACKJOIN, CHAT, ACKCREATE };


void CreateRoom(MyNetwork::Socket*& sock, std::vector<MyNetwork::Socket*>*& connections, MyNetwork::Listener*& listener, MyNetwork::Selector*& selector);
void ManageConnections(MyNetwork::Selector*& selector, MyNetwork::Socket*& sockToBss, std::vector<MyNetwork::Socket*>*& connections, MyNetwork::Listener*& listener, Game*& game);
void CreateOrSearchRoom(MyNetwork::Socket*& sockToBss, std::vector<MyNetwork::Socket*>*& connections, MyNetwork::Listener*& listener, MyNetwork::Selector*& selector);
void SearchForRoom(MyNetwork::Socket*& sock);
bool SelectRoom(InputMemoryStream*& ims, MyNetwork::Socket*& _sock);
bool AcknowledgeJoin(InputMemoryStream*& ims, MyNetwork::Socket*& sockToBss, std::vector<MyNetwork::Socket*>*& connections, MyNetwork::Selector*& selector, MyNetwork::Listener*& listener, Game*& game);
bool AcknowledgeCreate(InputMemoryStream*& ims, MyNetwork::Socket*& sock, MyNetwork::Listener*& listener, MyNetwork::Selector*& selector, std::vector<MyNetwork::Socket*>*& connections, Game*& game);

void SendMessage(std::vector<MyNetwork::Socket*>* conexiones);
void Shuffle(Game*& game);
void Deal(Game*& game);


int main() {
	end = false;

	std::cout << "Conectando al servidor" << std::endl;
	MyNetwork::Socket* sock = new MyNetwork::Socket();
	//conecta con BSS
	MyNetwork::Status status = sock->Connect("localhost", 50000);
	if (status == MyNetwork::Status::DONE) {
		std::cout << "¡Conexion con el servidor establecida!" << std::endl;
		std::cout << std::endl;
	}
	else {

		std::cout << "ERROR al establecer conexion. Cierra el cliente." << std::endl;
		char temp;
		std::cin >> temp;
	}

	MyNetwork::Selector* selector = new MyNetwork::Selector();
	selector->Add(sock);
	std::vector<MyNetwork::Socket*>* conexiones = new std::vector<MyNetwork::Socket*>();

	MyNetwork::Listener* listener = new MyNetwork::Listener();

	Game* game;

	while (!end)
	{
		CreateOrSearchRoom(sock, conexiones, listener, selector);
		ManageConnections(selector, sock, conexiones, listener, game);
	}

	return 0;
}

void CreateOrSearchRoom(MyNetwork::Socket*& sockToBss, std::vector<MyNetwork::Socket*>*& connections, MyNetwork::Listener*& listener, MyNetwork::Selector*& selector) {
	//CEAR o BUSCAR partida?
	std::cout << "Pulsa 1 para crear partida" << std::endl;
	std::cout << "Pulsa 2 para buscar partida" << std::endl;
	std::string str;
	std::cin >> str;
	bool correctComand = false;

	while (!correctComand) {
		if (str == "1") { //crear partida
			correctComand = true;
			CreateRoom(sockToBss, connections, listener, selector); //Cuando acaba esto, parar el bucle del main

		}
		else if (str == "2") { //buscar partida
			correctComand = true;
			SearchForRoom(sockToBss);
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
void ManageConnections(MyNetwork::Selector*& selector, MyNetwork::Socket*& sockToBss, std::vector<MyNetwork::Socket*>*& connections, MyNetwork::Listener*& listener, Game*& game) {
	
	MyNetwork::Status status;

	bool returnMenu = false;

	while (!returnMenu)
	{
		// Make the selector wait for data on any socket
		if (selector->Wait())
		{
			if (listener != nullptr && selector->IsReady(listener))
			{
				MyNetwork::Socket* connection = new MyNetwork::Socket();
				if (listener->Accept(connection) == MyNetwork::Status::DONE) {

					std::cout << "Se ha establecido conexion!" << std::endl;

					connections->push_back(std::move(connection));

					//Add the new client to the selector so that we will be notified when he sends something
					selector->Add(connection);

					//TODO GESTION DE LISTENER CON SALA COMPLETA
					if (game->currentPlayers == game->maxPlayers) {

						selector->Remove(listener);
						delete listener;
						listener = nullptr;
						Shuffle(game);
						Deal(game);
					}
				}
				else {
					delete connection;
				}
				continue;
			}
			else if (sockToBss != nullptr && selector->IsReady(sockToBss)) {
				InputMemoryStream* ims;
				char buffer[1000];
				size_t br = 0;
				status = sockToBss->Receive(&ims, buffer, 1000, br);
				if (status != MyNetwork::Status::DONE) {
					selector->Remove(sockToBss);
					sockToBss->Disconnect();
					delete sockToBss;
					std::cout << "Elimino el socket (al BSS) que se ha desconectado\n";
					continue;
				}
				std::cout << "Paquete recibido" << std::endl;

				int header;
				ims->Read(&header);

				switch (header)
				{
				case ROOMS:
					std::cout << "He recibido ROOMS\n";
					returnMenu = SelectRoom(ims, sockToBss);
					break;
				case ACKCREATE:
					std::cout << "He recibido ACKCREATE\n";
					returnMenu = !AcknowledgeCreate(ims, sockToBss, listener, selector, connections, game);
					break;
				case ACKJOIN:
					std::cout << "He recibido ACKJOIN\n";
					returnMenu = !AcknowledgeJoin(ims, sockToBss, connections, selector, listener, game);
					break;
				case CHAT:
					std::cout << "Ha entrado en el chat que no toca\n";

					break;
				default:
					break;
				}
			}
			else
			{
				for (size_t i = 0; i < connections->size(); i++) {

					MyNetwork::Socket* connection = connections->at(i);

					if (selector->IsReady(connection)) {
						InputMemoryStream* ims;
						char buffer[1000];
						size_t br = 0;
						status = connection->Receive(&ims, buffer, 1000, br);
						if (status != MyNetwork::Status::DONE) {
							selector->Remove(connection);
							connections->erase(connections->begin() + i);
							connection->Disconnect();
							delete connection;
							std::cout << "Elimino el socket (cliente) que se ha desconectado\n";
							i--;
							continue;
						}
						std::cout << "Paquete recibido" << std::endl;

						int header;
						ims->Read(&header);

						switch (header)
						{
						case CHAT:
							std::cout << "He recibido CHAT\n";
							std::cout << "CHAT: " << ims->ReadString() << std::endl;
							break;
						default:
							break;
						}
					}
				}
			}
		}
	}
}

bool AcknowledgeCreate(InputMemoryStream*& ims, MyNetwork::Socket*& sock, MyNetwork::Listener*& listener, MyNetwork::Selector*& selector, std::vector<MyNetwork::Socket*>*& connections, Game*& game) {

	bool ackCreate;
	ims->Read(&ackCreate);
	int maxPlayers;
	ims->Read(&maxPlayers);
	game = new Game();
	game->seed = sock->GetLocalPort();
	std::cout << std::endl;
	std::cout << "SEED: " << game->seed << std::endl;
	std::cout << std::endl;
	if (ackCreate) {
		std::cout << "Puedo crear una sala\n";

		uint16_t listenerPort = sock->GetLocalPort();
		sock->Disconnect();
		delete sock;
		sock = nullptr;
		std::cout << "Elimino el socket conectado al server BSS al crear una partida\n";

		//Empiezo a escuchar por el puerto del sock descontado (listener)
		listener->Listen(listenerPort);
		//añado listener al selector
		selector->Add(listener);

		//abro el chat
		std::thread tMessage(SendMessage, connections);
		tMessage.detach();
		game->currentPlayers = 1;
		game->maxPlayers = maxPlayers;

		Player newPlayer;
		newPlayer.ID = 1;
		game->players.push_back(newPlayer);

	}
	else std::cout << "No puedo crear una sala\n";

	return ackCreate;

}

bool SelectRoom(InputMemoryStream*& ims, MyNetwork::Socket*& _sock) {

	std::vector<Room> roomList;
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

	if (numRooms != 0) {
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
		else tempString = "";

		//Enviamos la selectedRoom al BSS
		OutputMemoryStream oms;
		int headerType = 3;
		oms.Write(headerType);
		oms.WriteString(selectedRoom.name);
		oms.WriteString(tempString);
		_sock->Send(&oms);
		return false;
	}
	else {
		std::cout << "NO HAY SALAS DSPONIBLES " << std::endl;
		return true;
	}
}

bool AcknowledgeJoin(InputMemoryStream*& ims, MyNetwork::Socket*& sockToBss, std::vector<MyNetwork::Socket*>*& connections, MyNetwork::Selector*& selector, MyNetwork::Listener*& listener, Game*& game) {
	game = new Game();
	//Si recibo -1 es que no he conseguido entrar en la partida (contraseña/sala llena)
	int numPlayers;
	ims->Read(&numPlayers);

	int maxPlayers;
	ims->Read(&maxPlayers);

	int seed;
	ims->Read(&seed);
	game->seed = seed;
	std::cout << std::endl;
	std::cout << "SEED: " << game->seed << std::endl;
	std::cout << std::endl;
	if (numPlayers == -1) {
		std::cout << "No has podido unirte a la partida." << std::endl;
		return false;
	}
	std::cout << "Password correcta!!" << std::endl;

	//establecemos conexion con los demas clientes de la sala
	for (int i = 0; i < numPlayers; i++)
	{
		std::string IPReceived;
		IPReceived = ims->ReadString();

		uint16_t portReceived;
		ims->Read(&portReceived);

		int IDReceived;
		ims->Read(&IDReceived);

		std::cout << "Recibido: IP->" << IPReceived << " PORT->" << portReceived << " ID->" << IDReceived << std::endl;

		MyNetwork::Socket* newClient = new MyNetwork::Socket();
		MyNetwork::Status status = newClient->Connect(IPReceived, portReceived);
		if (status == MyNetwork::Status::DONE) {
			std::cout << "Me conecto al cliente: IP->" << newClient->GetRemoteAdress() << " PORT->" << newClient->GetRemotePort() << std::endl;
		}
		else {
			std::cout << "Error al conectar con el cliente recibido" << std::endl;

		}
		connections->push_back(std::move(newClient));
		selector->Add(newClient);

		Player newPlayer;
		newPlayer.ID = IDReceived;
		game->players.push_back(newPlayer);


	}

	Player localPlayer;
	localPlayer.ID = game->players.size() + 1;
	game->localId = localPlayer.ID;
	game->players.push_back(localPlayer);


	std::cout << "Hay en el game:" << std::endl;
	for (int i = 0; i < game->players.size(); i++)
	{
		std::cout << "Player " << game->players[i].ID << std::endl;
	}

	//Me guardo el puerto del socket porqe ponemos a escuchar al listener por este
	uint16_t listenerPort = sockToBss->GetLocalPort();
	//Me desconecto del server
	sockToBss->Disconnect();
	//libero memoria
	delete sockToBss;
	sockToBss = nullptr;
	std::cout << "Elimino el socket conectado al server BSS cuando entro en una partida\n";

	//Sumamos uno que seríamos nosotros
	numPlayers++;

	//TODO GESTION DEL LISTENER CUANDO LA SALA ESTA LLENA
	if (numPlayers == maxPlayers) {

		game->currentPlayers = numPlayers;
		game->maxPlayers = maxPlayers;
		std::cout << "No abro el listener porque ya estamos todos" << std::endl;
		delete listener;
		listener = nullptr;
	}
	else {
		//Empiezo a escuchar por el puerto del sock descontado (listener)
		listener->Listen(listenerPort);
		//añado listener al selector
		selector->Add(listener);
		std::cout << "Empiezo a escuchar por el puerto " << listenerPort << std::endl;
	}

	//abro el chat
	std::thread tMessage(SendMessage, connections);
	tMessage.detach();
	return true;
}

void SendMessage(std::vector<MyNetwork::Socket*>* conexiones) {
	while (true) {
		std::cout << "Introduce mensaje" << std::endl;
		//OMS SEND PACKET
		OutputMemoryStream oms;
		oms.Write((int)Header::CHAT);
		std::string message;
		std::cin >> message;
		oms.WriteString(message);
		for (int i = 0; i < conexiones->size(); i++) {
			MyNetwork::Status status = conexiones->at(i)->Send(&oms);
			if (status != MyNetwork::Status::DONE) {
				std::cout << "mensaje no enviado" << std::endl;
			}
			else std::cout << "mensaje enviado" << std::endl;
		}
	}
}

void CreateRoom(MyNetwork::Socket*& sock, std::vector<MyNetwork::Socket*>*& connections, MyNetwork::Listener*& listener, MyNetwork::Selector*& selector) {

	OutputMemoryStream oms;

	oms.Write((int)Header::CREATE);

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
			std::cout << "Introduce el maximo de jugadores de la sala: (de 2 a 4)" << std::endl;
		}
	}

	MyNetwork::Status status = sock->Send(&oms);
	if (status != MyNetwork::Status::DONE)
		std::cout << "Error al enviar la informacion de crear la nueva sala" << std::endl;
	else std::cout << "MENSAJE ENVIADO" << std::endl;
}

void SearchForRoom(MyNetwork::Socket*& sock) {

	OutputMemoryStream oms;

	oms.Write((int)Header::SHOW);

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

void Shuffle(Game*& game) {

	std::vector<Card*>* deck = new std::vector<Card*>();
	//organos
	for (int i = 0; i < 21; i++) {
		if (i >= 0 && i < 5) {
			Card* newCard = new Card{ Type::ORGANO, Color::ROJO };
			deck->push_back(newCard);
		}
		else if (i >= 5 && i < 10) {
			Card* newCard = new Card{ Type::ORGANO, Color::VERDE };
			deck->push_back(newCard);
		}
		else if (i >= 10 && i < 15) {
			Card* newCard = new Card{ Type::ORGANO, Color::AZUL };
			deck->push_back(newCard);
		}
		else if (i >= 15 && i < 20) {
			Card* newCard = new Card{ Type::ORGANO, Color::AMARILLO };
			deck->push_back(newCard);
		}
		else {
			Card* newCard = new Card{ Type::ORGANO, Color::COMODIN };
			deck->push_back(newCard);
		}
	}

	//VIRUS
	for (int i = 0; i < 17; i++) {
		if (i >= 0 && i < 4) {
			Card* newCard = new Card{ Type::VIRUS, Color::ROJO };
			deck->push_back(newCard);
		}
		else if (i >= 4 && i < 8) {
			Card* newCard = new Card{ Type::VIRUS, Color::VERDE };
			deck->push_back(newCard);
		}
		else if (i >= 8 && i < 12) {
			Card* newCard = new Card{ Type::VIRUS, Color::AZUL };
			deck->push_back(newCard);
		}
		else if (i >= 12 && i < 16) {
			Card* newCard = new Card{ Type::VIRUS, Color::AMARILLO };
			deck->push_back(newCard);
		}
		else {
			Card* newCard = new Card{ Type::VIRUS, Color::COMODIN };
			deck->push_back(newCard);
		}
	}
	//MEDICINAS
	for (int i = 0; i < 20; i++) {
		if (i >= 0 && i < 4) {
			Card* newCard = new Card{ Type::VIRUS, Color::ROJO };
			deck->push_back(newCard);
		}
		else if (i >= 4 && i < 8) {
			Card* newCard = new Card{ Type::VIRUS, Color::VERDE };
			deck->push_back(newCard);
		}
		else if (i >= 8 && i < 12) {
			Card* newCard = new Card{ Type::VIRUS, Color::AZUL };
			deck->push_back(newCard);
		}
		else if (i >= 12 && i < 16) {
			Card* newCard = new Card{ Type::VIRUS, Color::AMARILLO };
			deck->push_back(newCard);
		}
		else {
			Card* newCard = new Card{ Type::VIRUS, Color::COMODIN };
			deck->push_back(newCard);
		}
	}
	srand(game->seed);


	for (int i = 0; i < deck->size(); i++) {
		int random = rand() % deck->size();
		std::swap(deck[i], deck[random]);
	}

	game->deck = deck;

}

void Deal(Game*& game) {

	int dealToPlayerNum = 0;
	int dealedCards = 0;
	for (int i = 0; i < game->currentPlayers; i++)
	{
		while (dealedCards < 3)
		{
			game->players.at(dealToPlayerNum).hand[dealedCards] = game->deck->at(0);
			game->deck->erase(game->deck->begin());
			dealedCards++;
		}
		dealToPlayerNum++;
		dealedCards = 0;
	}
}