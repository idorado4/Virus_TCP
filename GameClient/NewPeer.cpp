#include <iostream>
#include <thread>
#include <SFML/Network.hpp>
#include <MyNetwork.h>
#include <InputMemoryStream.h>
#include <OutputMemoryStream.h>
#include <ConsoleControl.h>

#include <future>


enum Type { ORGANO, VIRUS, MEDICINA, TRATAMIENTO };

enum Color { VERDE, ROJO, AZUL, AMARILLO, COMODIN };

struct Card {
	Type type;
	Color color;
	int ID;
};

struct Peer {
	std::string IP;
	unsigned short PORT;
};

struct Player {
	std::vector<Card> hand;
	bool turn;
	std::vector<Card> table;
	int ID;
	bool isReady = false;
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
	std::vector<Card> deck;
	std::vector<Card> discardPile; // = new std::vector<Card*>;
	std::vector<Player> players;
};


enum Header { CREATE = 0, SHOW, ROOMS, SELECTEDROOM, ACKJOIN, CHAT, ACKCREATE };

void CreateOrSearchRoom();
void CreateRoom();
void SearchForRoom();
void ManageServerCommands();
void WaitForOtherPlayers();
void ManageConnectedClients();

std::string GetLineFromCin();
void CheckCommand();

bool SelectRoom(InputMemoryStream* ims);
bool AcknowledgeJoin(InputMemoryStream* ims);
bool AcknowledgeCreate(InputMemoryStream* ims);

void Shuffle();
void Deal();
void SetInitialTurn();
std::string CardToString(Card card);

void GameManager();

MyNetwork::Socket sockToServer;
MyNetwork::Selector selector;
MyNetwork::Listener listener;

std::vector<MyNetwork::Socket*> socksToClients;

Game game;

bool end;
bool hasToListen;
bool endChatThread;

int currentTurnNumber;

int main() {
	end = false;
	currentTurnNumber = 0;
	std::cout << "Conectando al servidor" << std::endl;

	//conecta con BSS
	MyNetwork::Status status = sockToServer.Connect("localhost", 50000);
	if (status == MyNetwork::Status::DONE) {
		std::cout << "¡Conexion con el servidor establecida!" << std::endl;
		std::cout << std::endl;
	}
	else {
		std::cout << "ERROR al establecer conexion. Cierra el cliente." << std::endl;
		char temp;
		std::cin >> temp;
	}

	std::cout << sockToServer.GetLocalPort() << std::endl;


	selector.Add(&sockToServer);



	while (!end)
	{
		CreateOrSearchRoom();
		ManageServerCommands();
		WaitForOtherPlayers();
		std::thread tManageConnectedClients(ManageConnectedClients);
		tManageConnectedClients.detach();
		GameManager();
	}

	return 0;

}

void CreateOrSearchRoom()
{
	//CEAR o BUSCAR partida?
	std::cout << "Pulsa 1 para crear partida" << std::endl;
	std::cout << "Pulsa 2 para buscar partida" << std::endl;
	std::string str;
	std::cin >> str;
	bool correctComand = false;

	while (!correctComand) {
		if (str == "1") { //crear partida
			correctComand = true;
			CreateRoom(); //Cuando acaba esto, parar el bucle del main
		}
		else if (str == "2") { //buscar partida
			correctComand = true;
			SearchForRoom();
		}
		else {

			std::cout << "Comando incorrecto" << std::endl;
			std::cout << "Pulsa 1 para crear partida" << std::endl;
			std::cout << "Pulsa 2 para buscar partida" << std::endl;

			std::cin >> str;
		}
	}
}

void CreateRoom()
{
	std::cout << "CREATE ROOM" << std::endl;

	if (end) return;

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
			break;
		}
		else if (tempString == "N" || tempString == "n") {
			correctComandBoolPasword = true;
			tempString = "";
			oms.WriteString(tempString);
			break;
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
			break;
		}
		else {
			std::cout << "numero de jugadores maximos incorrecto" << std::endl;
			std::cout << "Introduce el maximo de jugadores de la sala: (de 2 a 4)" << std::endl;
		}
	}

	MyNetwork::Status status = sockToServer.Send(&oms);
	if (status != MyNetwork::Status::DONE)
		std::cout << "Error al enviar la informacion de crear la nueva sala" << std::endl;
	else std::cout << "MENSAJE ENVIADO" << std::endl;

}

void SearchForRoom()
{
	std::cout << "SEARCH FOR ROOM" << std::endl;
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
	MyNetwork::Status status = sockToServer.Send(&oms);
	if (status != MyNetwork::Status::DONE) {
		std::cout << "No se envió el paquete para buscar sala\n";
	}

}

void ManageServerCommands()
{
	if (end) return;

	MyNetwork::Status status;

	bool endManagement = false;

	while (!endManagement)
	{
		if (selector.Wait()) {

			if (selector.IsReady(&sockToServer)) {
				InputMemoryStream* ims;
				char buffer[1000];
				size_t br = 0;
				status = sockToServer.Receive(&ims, buffer, 1000, br);
				if (status != MyNetwork::Status::DONE) {
					sockToServer.Disconnect();
					selector.Remove(&sockToServer);
					std::cout << "Elimino el socket (al BSS) que se ha desconectado\n";
					endManagement = true;
					break;
				}
				std::cout << "Paquete recibido" << std::endl;

				int header;
				ims->Read(&header);

				switch (header)
				{
				case ROOMS:
					std::cout << "He recibido ROOMS\n";
					endManagement = SelectRoom(ims);
					break;
				case ACKCREATE:
					std::cout << "He recibido ACKCREATE\n";
					endManagement = AcknowledgeCreate(ims);
					break;
				case ACKJOIN:
					std::cout << "He recibido ACKJOIN\n";
					endManagement = AcknowledgeJoin(ims);
					break;
				default:
					break;
				}
			}
		}
	}
}

void WaitForOtherPlayers()
{
	std::cout << "WAIT FOR OTHER PLAYERS" << std::endl;
	if (end) return;
	if (!hasToListen) return;

	MyNetwork::Status status;

	bool endWait = false;

	while (!endWait)
	{
		std::cout << "WAIT FOR MORE PLAYERS" << std::endl;
		if (selector.Wait())
		{
			if (selector.IsReady(&listener))
			{
				MyNetwork::Socket* newClientConnected = new MyNetwork::Socket();
				if (listener.Accept(newClientConnected) == MyNetwork::Status::DONE) {

					std::cout << "Se ha establecido conexion con un nuevo cliente" << std::endl;
					std::cout << "PORT DEL NUEVO" << newClientConnected->GetRemotePort() << std::endl;

					socksToClients.push_back(std::move(newClientConnected));

					//Add the new client to the selector so that we will be notified when he sends something
					selector.Add(newClientConnected);
					game.currentPlayers++;
					Player newPlayer;
					newPlayer.ID = game.currentPlayers;
					game.players.push_back(newPlayer);
					std::cout << "Game players " << game.currentPlayers << "/" << game.maxPlayers << std::endl;

					//TODO GESTION DE LISTENER CON SALA COMPLETA
					if (game.currentPlayers == game.maxPlayers) {

						std::cout << "Ya estamos todos!! ^^ " << std::endl;
						std::thread tChat(CheckCommand);
						tChat.detach();
						selector.Remove(&listener);
						Shuffle();
						Deal();
						return;
					}
				}
				else {
					delete newClientConnected;
				}
			}
		}
	}
}

void ManageConnectedClients()
{
	if (end) return;
	std::cout << "MANAGE CONNECTED CLIENTS" << std::endl;

	MyNetwork::Status status;

	bool endManagement = false;

	while (!endManagement)
	{
		// Make the selector wait for data on any socket
		if (selector.Wait())
		{
			for (size_t i = 0; i < socksToClients.size(); i++) {

				MyNetwork::Socket* connection = socksToClients.at(i);

				if (selector.IsReady(connection)) {
					InputMemoryStream* ims;
					char buffer[1000];
					size_t br = 0;
					status = connection->Receive(&ims, buffer, 1000, br);
					if (status != MyNetwork::Status::DONE) {
						selector.Remove(connection);
						socksToClients.erase(socksToClients.begin() + i);
						connection->Disconnect();
						delete connection;
						std::cout << "Elimino el socket (cliente) que se ha desconectado\n";
						i--;
						continue;
					}
					std::cout << "Paquete recibido" << std::endl;

					int header;
					ims->Read(&header);
					std::string msg;
					switch (header)
					{
					case CHAT:
						std::cout << "He recibido CHAT\n";
						msg = ims->ReadString();
						std::cout << "CHAT: " << msg << std::endl;
						if (msg == "ready") {
							std::cout << "ALGUIEN HA MANDADO READY" << std::endl;
							int idPlayerReady = -1;
							ims->Read(&idPlayerReady);
							for (size_t i = 0; i < game.players.size(); i++)
							{
								if (game.players[i].ID == idPlayerReady)
									game.players[i].isReady = true;
							}
						}
						break;
					default:
						break;
					}
				}
			}
		}
	}

}

bool SelectRoom(InputMemoryStream* ims)
{
	std::cout << "SELECT ROOM" << std::endl;
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

			std::string hasPassword = roomList[i].hasPassword ? "Yes" : "No";
			std::cout << "Tiene contraseña:" << hasPassword << std::endl;
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
					std::cout << "Sala " << roomList[i].name << " encontrada" << std::endl;
					break;
				}
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
		int headerType = SELECTEDROOM;
		oms.Write(headerType);
		oms.WriteString(selectedRoom.name);
		oms.WriteString(tempString);
		MyNetwork::Status status = sockToServer.Send(&oms);
		if (status != MyNetwork::Status::DONE) {
			std::cout << "No se ha enviado el paquete de selección de sala\n";
		}
		return false;
	}
	else {
		std::cout << "NO HAY SALAS DSPONIBLES " << std::endl;
		return true;
	}

}

bool AcknowledgeJoin(InputMemoryStream* ims)
{
	std::cout << "ACKNOWLEDGE JOIN" << std::endl;
	//Si recibo -1 es que no he conseguido entrar en la partida (contraseña/sala llena)
	int numPlayers;
	ims->Read(&numPlayers);

	if (numPlayers == -1) {
		std::cout << "No has podido unirte a la partida." << std::endl;
		return true;
	}

	int maxPlayers;
	ims->Read(&maxPlayers);

	int seed;
	ims->Read(&seed);
	game.seed = seed;
	std::cout << std::endl;
	std::cout << "SEED: " << game.seed << std::endl;
	std::cout << std::endl;
	std::cout << "Password correcta!!" << std::endl;

	//establecemos conexion con los demas clientes de la sala
	for (int i = 0; i < numPlayers; i++)
	{
		std::string IPReceived;
		IPReceived = ims->ReadString();

		unsigned short portReceived;
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
		socksToClients.push_back(std::move(newClient));
		selector.Add(newClient);

		Player newPlayer;
		newPlayer.ID = IDReceived;
		game.players.push_back(newPlayer);


	}

	Player localPlayer;
	localPlayer.ID = game.players.size() + 1;
	game.localId = localPlayer.ID;
	game.players.push_back(localPlayer);


	std::cout << "Hay en el game:" << std::endl;
	for (int i = 0; i < game.players.size(); i++)
	{
		std::cout << "Player " << game.players[i].ID << std::endl;
	}
	std::cout << "LOCAL Player ID" << game.localId << std::endl;

	//Me guardo el puerto del socket porqe ponemos a escuchar al listener por este
	unsigned short listenerPort = sockToServer.GetLocalPort();
	//Me desconecto del server
	selector.Remove(&sockToServer);
	sockToServer.Disconnect();
	std::cout << "Elimino el socket conectado al server BSS cuando entro en una partida\n";

	//Sumamos uno que seríamos nosotros
	numPlayers++;

	game.currentPlayers = numPlayers;
	game.maxPlayers = maxPlayers;
	//TODO GESTION DEL LISTENER CUANDO LA SALA ESTA LLENA
	if (numPlayers == maxPlayers) {

		std::cout << "No abro el listener porque ya estamos todos" << std::endl;
		Shuffle();
		Deal();
		std::thread tChat(CheckCommand);
		tChat.detach();
		hasToListen = false;
	}
	else {
		//Empiezo a escuchar por el puerto del sock descontado (listener)
		MyNetwork::Status status = listener.Listen(listenerPort);
		if (status != MyNetwork::Status::DONE) {
			std::cout << "No se ha podido escuchar por este puerto" << std::endl;
		}
		//añado listener al selector
		selector.Add(&listener);
		hasToListen = true;
		std::cout << "Empiezo a escuchar por el puerto " << listenerPort << std::endl;

	}


	return true;
}

bool AcknowledgeCreate(InputMemoryStream* ims)
{
	std::cout << "ACKNOWLEDGE CREATE" << std::endl;
	bool ackCreate;
	ims->Read(&ackCreate);

	if (!ackCreate) {
		std::cout << "No puedo crear una sala\n";
		return true;
	}
	std::cout << "Puedo crear una sala\n";

	int maxPlayers;
	ims->Read(&maxPlayers);

	game.seed = sockToServer.GetLocalPort();
	std::cout << std::endl;
	std::cout << "SEED: " << game.seed << std::endl;
	std::cout << std::endl;


	unsigned short listenerPort = sockToServer.GetLocalPort();
	selector.Remove(&sockToServer);
	sockToServer.Disconnect();
	std::cout << "Elimino el socket conectado al server BSS al crear una partida\n";

	//Empiezo a escuchar por el puerto del sock descontado (listener)

	MyNetwork::Status status = listener.Listen(listenerPort);
	if (status != MyNetwork::Status::DONE) {
		std::cout << "No se ha podido escuchar por el puerto\n";
	}

	//añado listener al selector
	selector.Add(&listener);
	hasToListen = true;


	game.currentPlayers = 1;
	game.maxPlayers = maxPlayers;

	Player newPlayer;
	newPlayer.ID = 1;
	game.players.push_back(newPlayer);
	game.localId = 1;
	return true;

}

void CheckCommand() {
	auto future_line = std::async(std::launch::async, GetLineFromCin);
	while (!endChatThread) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		if (future_line.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
			std::cout << "getLine" << std::endl;
			auto line = future_line.get(); // Se lee el dato introducido por consola
			future_line = std::async(std::launch::async, GetLineFromCin); // Se vuelve a lanzar la fun asÃ­ncrona
			std::string message = line;
			if (line.length() > 0) {

				if (endChatThread) return;

				OutputMemoryStream oms;
				oms.Write(CHAT);
				oms.WriteString(message);

				//GESTION LOCAL DEL READY
				if (message == "ready") {
					for (size_t i = 0; i < game.players.size(); i++)
					{
						if (game.localId == game.players[i].ID) {
							game.players[i].isReady = true;
						}
					}
					oms.Write(game.localId);
				}

				std::cout << "Clientes: " << socksToClients.size() << std::endl;
				for (int i = 0; i < socksToClients.size(); i++)
				{
					std::cout << "HIS PORT: " << socksToClients[i]->GetRemotePort() << std::endl;
					std::cout << "MY PORT: " << socksToClients[i]->GetLocalPort() << std::endl;
					MyNetwork::Status status = socksToClients[i]->Send(&oms);
					if (status != MyNetwork::Status::DONE) {
						std::cout << "No se ha podido enviar el mensaje al cliente " << i << std::endl;

					}
				}
			}
		}
	}

}

// Esta funciÃ³n lee el texto que escriba el usuario por consola.
std::string GetLineFromCin() {
	std::string line;
	std::getline(std::cin, line);
	std::cout << "getlinecin" << std::endl;
	return line;
}

void Shuffle() {

	std::vector<Card> deck = std::vector<Card>();
	//organos
	for (int i = 0; i < 21; i++) {
		if (i >= 0 && i < 5) {
			Card newCard = Card{ Type::ORGANO, Color::ROJO };
			deck.push_back(newCard);
		}
		else if (i >= 5 && i < 10) {
			Card newCard = Card{ Type::ORGANO, Color::VERDE };
			deck.push_back(newCard);
		}
		else if (i >= 10 && i < 15) {
			Card newCard = Card{ Type::ORGANO, Color::AZUL };
			deck.push_back(newCard);
		}
		else if (i >= 15 && i < 20) {
			Card newCard = Card{ Type::ORGANO, Color::AMARILLO };
			deck.push_back(newCard);
		}
		else {
			Card newCard = Card{ Type::ORGANO, Color::COMODIN };
			deck.push_back(newCard);
		}
	}

	//VIRUS
	for (int i = 0; i < 17; i++) {
		if (i >= 0 && i < 4) {
			Card newCard = Card{ Type::VIRUS, Color::ROJO };
			deck.push_back(newCard);
		}
		else if (i >= 4 && i < 8) {
			Card newCard = Card{ Type::VIRUS, Color::VERDE };
			deck.push_back(newCard);
		}
		else if (i >= 8 && i < 12) {
			Card newCard = Card{ Type::VIRUS, Color::AZUL };
			deck.push_back(newCard);
		}
		else if (i >= 12 && i < 16) {
			Card newCard = Card{ Type::VIRUS, Color::AMARILLO };
			deck.push_back(newCard);
		}
		else {
			Card newCard = Card{ Type::VIRUS, Color::COMODIN };
			deck.push_back(newCard);
		}
	}
	//MEDICINAS
	for (int i = 0; i < 20; i++) {
		if (i >= 0 && i < 4) {
			Card newCard = Card{ Type::VIRUS, Color::ROJO };
			deck.push_back(newCard);
		}
		else if (i >= 4 && i < 8) {
			Card newCard = Card{ Type::VIRUS, Color::VERDE };
			deck.push_back(newCard);
		}
		else if (i >= 8 && i < 12) {
			Card newCard = Card{ Type::VIRUS, Color::AZUL };
			deck.push_back(newCard);
		}
		else if (i >= 12 && i < 16) {
			Card newCard = Card{ Type::VIRUS, Color::AMARILLO };
			deck.push_back(newCard);
		}
		else {
			Card newCard = Card{ Type::VIRUS, Color::COMODIN };
			deck.push_back(newCard);
		}
	}
	srand(game.seed);


	for (int i = 0; i < deck.size(); i++) {
		int random = rand() % deck.size();
		std::swap(deck[i], deck[random]);
	}

	//game.deck = std::move(deck);
	game.deck = deck;

	/*for (int i = 0; i < 12; i++) {
		switch (game.deck[i].type)
		{
		case ORGANO:
			std::cout << "ORGANO" << std::endl;
			break;
		case VIRUS:
			std::cout << "VIRUS" << std::endl;
			break;
		case MEDICINA:
			std::cout << "MEDICINA" << std::endl;
			break;
		case TRATAMIENTO:
			std::cout << "TRATAMIENTO" << std::endl;
			break;
		default:
			break;
		}

		switch (game.deck[i].color)
		{
		case ROJO:
			std::cout << "ROJO" << std::endl;
			break;
		case AZUL:
			std::cout << "AZUL" << std::endl;
			break;
		case VERDE:
			std::cout << "VERDE" << std::endl;
			break;
		case AMARILLO:
			std::cout << "AMARILLO" << std::endl;
			break;
		case COMODIN:
			std::cout << "COMODIN" << std::endl;
			break;
		default:
			break;
		}

	}*/
}

void Deal() {

	int dealToPlayerNum = 0;
	int dealedCards = 0;
	for (int i = 0; i < game.currentPlayers; i++)
	{
		while (dealedCards < 3)
		{
			game.players[dealToPlayerNum].hand.push_back(game.deck[0]);
			game.deck.erase(game.deck.begin());
			dealedCards++;
		}
		dealToPlayerNum++;
		dealedCards = 0;
	}

	SetInitialTurn();
}

void SetInitialTurn() {
	int organsCount = 0;
	int maxOrgansCounted = -1;
	for (size_t i = 0; i < game.players.size(); i++)
	{
		for (size_t j = 0; j < game.players[i].hand.size(); j++)
		{
			if (game.players[i].hand[j].type == Type::ORGANO) {
				organsCount++;
			}
		}
		if (organsCount > maxOrgansCounted) {
			currentTurnNumber = game.players[i].ID;
		}
	}

	std::cout << "EMPIEZA EL JUGADOR " << currentTurnNumber << std::endl;
}

void GameManager()
{
	std::cout << "EMPIEZA EL JUEGO\n";

	bool allPlayersReady = false;

	//ESPERAMOS A QUE TODOS MANDEN READY
	while (!allPlayersReady) {
		int playersReadyCount = 0;
		for (size_t i = 0; i < game.players.size(); i++)
		{
			if (game.players[i].isReady)
				playersReadyCount++;
		}
		if (game.maxPlayers == playersReadyCount) {

			allPlayersReady = true;
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
		ConsoleClear();
		for (size_t i = 0; i < game.players.size(); i++)
		{
			std::cout << "Player " << game.players[i].ID << " isReady: " << (int)game.players[i].isReady << std::endl;
		}
	}

	endChatThread = true;


	int command;

	//LOGICA DE JUEGO
	while (!end) {
		std::this_thread::sleep_for(std::chrono::milliseconds(3000));
		ConsoleClear();

		if (game.localId != currentTurnNumber) {
			std::cout << "NO Es tu turno\n";
		}

		for (size_t i = 0; i < game.players.size(); i++)
		{
			std::cout << "MESA DEL JUGADOR " << game.players[i].ID << std::endl;
			for (size_t j = 0; j < game.players[i].table.size(); i++)
			{
				std::cout << "Carta " << j << " " << CardToString(game.players[i].table[j]);
			}
		}

		if (game.localId == currentTurnNumber) {
			for (size_t i = 0; i < game.players.size(); i++)
			{
				if (game.players[i].ID == currentTurnNumber)
				{
					for (size_t j = 0; j < game.players[i].hand.size(); j++)
					{
						std::cout << "Carta " << j << " " << CardToString(game.players[i].hand[j]) << std::endl;
					}
				}
				std::cout << "Elige accion "<< std::endl;
				std::cout << "1 - Jugar Carta "<< std::endl;
				std::cout << "2 - Descartar carta "<< std::endl;
				std::cin >> command;
			}
		}
	}
}

std::string CardToString(Card card) {

	std::string strCard;

	switch (card.type)
	{
	case ORGANO:
		strCard += "ORGANO ";
		break;
	case VIRUS:
		strCard += "VIRUS ";
		break;
	case MEDICINA:
		strCard += "MEDICINA ";
		break;
	case TRATAMIENTO:
		strCard += "TRATAMIENTO ";
		break;
	default:
		break;
	}

	switch (card.color)
	{
	case VERDE:
		strCard += "VERDE";
		break;
	case ROJO:
		strCard += "ROJO";
		break;
	case AZUL:
		strCard += "AZUL";
		break;
	case AMARILLO:
		strCard += "AMARILLO";
		break;
	case COMODIN:
		strCard += "COMODIN";
		break;
	default:
		break;
	}

	return strCard;
}

