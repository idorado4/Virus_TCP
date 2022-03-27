#include <iostream>
#include <thread>
#include <SFML/Network.hpp>
#include <InputMemoryStream.h>
#include <OutputMemoryStream.h>
#include <MyNetwork.h>

bool running;

enum Header { CREATE = 0, SHOW, ROOMS, SELECTEDROOM, ACKJOIN };

//guardar internamente el socket??
struct Peer
{
	std::string IP;
	uint16_t PORT; //Unsigned Short

	Peer(std::string _ip, uint16_t _port) {
		IP = _ip;
		PORT = _port;
	}
};

struct Room
{
	std::string name;
	std::string password;
	int maxPlayers;
	int currentPlayers;
	//Los clientes conectados
	std::vector<Peer> clients;

};

void Manager();
void ShowFilteredRooms(MyNetwork::Socket* client, std::vector<Room>& rooms, int maxPlayers, bool hasPassword);
void ShowAllRooms(MyNetwork::Socket* client, std::vector<Room>& rooms);

int main() {

	running = true;

	//Manager();

	std::thread tManager(Manager);
	tManager.detach();

	/*char command;
	do {
		std::cout << "Type 'c' to close the Server" << std::endl;
		std::cin >> command;
	} while (command != 'c');*/
	while (true) {

	}


	return 0;
}

void Manager() {


	MyNetwork::Listener listener;

	MyNetwork::Status status = listener.Listen(50000);
	if (status != MyNetwork::Status::DONE) {
		std::cout << "Error al escuchar por el puerto 50000" << std::endl;
		char exit;
		std::cin >> exit;
		return;
	}


	MyNetwork::Selector selector;
	selector.Add(&listener);

	//la gente que busca como la que crea
	std::vector<MyNetwork::Socket*> clientes;
	std::vector<Room> rooms;

	while (true) {
		if (selector.Wait()) {
			if (selector.IsReady(&listener)) {
				MyNetwork::Socket* sock = new MyNetwork::Socket();
				if (listener.Accept(sock) == MyNetwork::Status::DONE) {
					std::cout << "Se ha establecido conexion!" << std::endl;

					clientes.push_back(std::move(sock));
					//Add the new client to the selector so that we will
					//be notified when he sends something
					selector.Add(sock);

				}
			}
			else {
				//recorre todos los sockets ya conectados
				for (size_t i = 0; i < clientes.size(); i++)
				{
					//guardamos direccion del socket que estoy recorriendo
					MyNetwork::Socket* client = clientes.at(i);
					//miro si es este el socket reacciona
					if (selector.IsReady(client)) {
						InputMemoryStream* ims;
						char buffer[1000];
						size_t br = 0;
						client->Receive(&ims, buffer, 1000, br);
						std::cout << "Paquete recibido" << std::endl;
						int header;
						ims->Read(&header);

						Room newRoom;

						switch (header)
						{
						case CREATE:
							std::cout << "el cliente quiere crear partida" << std::endl;
							//Creamos una nueva sala con la informacion recibida
							newRoom.name = ims->ReadString();
							newRoom.password = ims->ReadString();
							ims->Read(&newRoom.maxPlayers);
							newRoom.currentPlayers = 1;
							newRoom.clients.push_back({ client->GetRemoteAdress(), client->GetRemotePort() });
							rooms.push_back(newRoom);
							//Gestionar desconexión del cliente 
							break;
						case SHOW:
							std::cout << "el cliente quiere buscar partida" << std::endl;
							//Miramos si quiere filtrar las salas
							bool filterRooms;
							ims->Read(&filterRooms);

							if (filterRooms) {
								int maxPlayers;
								ims->Read(&maxPlayers);
								bool passwordFilter;
								ims->Read(&passwordFilter);
								//Enviamos los filtros
								ShowFilteredRooms(client, rooms, maxPlayers, passwordFilter);
							}
							else {
								ShowAllRooms(client, rooms);
							}



							break;

						case SELECTEDROOM:


							break;
						
						default:
							break;
						}

					}

				}
			}
		}
	}


	//enviar la info de los otros peers al nuevo
	//OutputMemoryStream oms;
	//
	////Le envío el número de clientes que hay ya en la partida
	//int clientsSize = clients.size();
	//oms.Write(clientsSize);
	//std::cout << "Clients Sended " << clientsSize << std::endl;


	//std::string IP = sock->GetRemoteAdress();
	//uint16_t port = sock->GetRemotePort();
	//std::cout << "Conneted Client Info: IP-> " << IP << " Puerto -> " << port << std::endl;


	////le envio la info de los clientes en partida al nuevo
	//for (int i = 0; i < clients.size(); i++) {
	//	std::string ipClient = clients[i].IP;
	//	oms.WriteString(ipClient);

	//	uint16_t portClient = clients[i].PORT;
	//	oms.Write(portClient);
	//}

	//status = sock->Send(&oms);

	//if (status != MyNetwork::Status::DONE) {
	//	std::cout << "Error al enviar el mensaje" << std::endl;
	//	continue;
	//}

	//Peer newClient = { sock->GetRemoteAdress(), sock->GetRemotePort() };

	//clients.push_back(newClient);

	//std::cout << "Current clients list: " << std::endl;
	//for (int i = 0; i < clients.size(); i++) {
	//	std::cout << "IP->" << clients[i].IP << " PORT->" << clients[i].PORT << std::endl;
	//}
	//sock->Disconnect();

}

void ShowFilteredRooms(MyNetwork::Socket* client, std::vector<Room>& rooms, int maxPlayers, bool passwordFilter) {

	OutputMemoryStream oms;
	oms.Write(2);
	std::vector<std::string> tempNames;
	std::vector<int> tempCurrentPlayers;
	

	int numRooms = 0;
	
	for (int i = 0; i < rooms.size(); i++)
	{
		if (passwordFilter) {
			switch (maxPlayers)
			{
				case 2:
					if (rooms[i].maxPlayers == 2 && rooms[i].password != "") {
						numRooms++;
						tempNames.push_back(rooms[i].name);
						tempCurrentPlayers.push_back(rooms[i].currentPlayers);
					}
					break;
				case 3:
					if (rooms[i].maxPlayers == 3 && rooms[i].password != "") {
						numRooms++;
						tempNames.push_back(rooms[i].name);
						tempCurrentPlayers.push_back(rooms[i].currentPlayers);
					}
					break;
				case 4:
					if (rooms[i].maxPlayers == 4 && rooms[i].password != "") {
						numRooms++;
						tempNames.push_back(rooms[i].name);
						tempCurrentPlayers.push_back(rooms[i].currentPlayers);
					}
					break;
			default:
				break;
			}
		}
		else {
			switch (maxPlayers)
			{
			case 2:
				if (rooms[i].maxPlayers == 2 && rooms[i].password == "") {
					numRooms++;
					tempNames.push_back(rooms[i].name);
					tempCurrentPlayers.push_back(rooms[i].currentPlayers);
				}
				break;
			case 3:
				if (rooms[i].maxPlayers == 3 && rooms[i].password == "") {
					numRooms++;
					tempNames.push_back(rooms[i].name);
					tempCurrentPlayers.push_back(rooms[i].currentPlayers);
				}
				break;
			case 4:
				if (rooms[i].maxPlayers == 4 && rooms[i].password == "") {
					numRooms++;
					tempNames.push_back(rooms[i].name);
					tempCurrentPlayers.push_back(rooms[i].currentPlayers);
				}
				break;
			default:
				break;
			}
		}
	}


	oms.Write(numRooms);
	
	for (int i = 0; i < numRooms; i++) {
		oms.WriteString(tempNames[i]);
		oms.Write(passwordFilter);
		oms.Write(tempCurrentPlayers[i]);
		oms.Write(maxPlayers);

	}

	client->Send(&oms);
}

void ShowAllRooms(MyNetwork::Socket* client, std::vector<Room>& rooms) {
	OutputMemoryStream oms;
	oms.Write(2);
	int numRooms = rooms.size();
	oms.Write(numRooms);
	for (int i = 0; i < rooms.size(); i++)
	{
		std::string roomName = rooms[i].name;
		oms.WriteString(roomName);
		bool hasPassword = rooms[i].password != "";
		oms.Write(hasPassword);	
		int currentPlayers = rooms[i].currentPlayers;
		oms.Write(currentPlayers);
		int maxPlayers = rooms[i].maxPlayers;
		oms.Write(maxPlayers);
	}

	client->Send(&oms);
	//enviar el numero completo de rooms
	//recorrer el vector enviando la informacion sala por sala
}





