#include <iostream>
#include <thread>
#include <SFML/Network.hpp>
#include <InputMemoryStream.h>
#include <OutputMemoryStream.h>
#include <MyNetwork.h>
#include <mutex>

bool running;

std::mutex mtxRoom;
enum Header { CREATE = 0, SHOW, ROOMS, SELECTEDROOM, ACKJOIN, CHAT, ACKCREATE };

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
void JoinSelectedRoom(InputMemoryStream& ims, std::vector<Room>& rooms, MyNetwork::Socket* client);
void AcknowledgeCreate(InputMemoryStream& ims, std::vector<Room>& rooms, MyNetwork::Socket* client, MyNetwork::Selector& selector, std::vector<MyNetwork::Socket*>& clientes);

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
				else {
					std::cout << "Error al establecer conexion con el nuevo cliente" << std::endl;
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
						status = client->Receive(&ims, buffer, 1000, br);
						if (status != MyNetwork::Status::DONE) {
							selector.Remove(client);
							clientes.erase(clientes.begin() + i);
							client->Disconnect();
							delete client;
							std::cout << "Elimino el socket que se ha desconectado (Status!=DONE cuando recivo headers)\n";
							i--;
							continue;
						}
						std::cout << "Paquete recibido" << std::endl;
						int header;
						ims->Read(&header);



						switch (header)
						{
						case CREATE:
							AcknowledgeCreate(*ims, rooms, client, selector, clientes);

							break;
						case SHOW:
							std::cout << "el cliente quiere buscar partida" << std::endl;
							//Miramos si quiere filtrar las salas
							bool filterRooms;
							ims->Read(&filterRooms);

							if (filterRooms) {
								std::cout << "el cliente quiere partidas filtradas" << std::endl;
								int maxPlayers;
								ims->Read(&maxPlayers);
								bool passwordFilter;
								ims->Read(&passwordFilter);
								//Enviamos los filtros
								ShowFilteredRooms(client, rooms, maxPlayers, passwordFilter);
							}
							else {
								std::cout << "el cliente quiere todas las partidas " << std::endl;
								ShowAllRooms(client, rooms);
							}
							break;

						case SELECTEDROOM:
							std::cout << "Ha seleccionado una sala\n";
							JoinSelectedRoom(*ims, rooms, client);




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

void AcknowledgeCreate(InputMemoryStream& ims, std::vector<Room>& rooms, MyNetwork::Socket* client, MyNetwork::Selector& selector, std::vector<MyNetwork::Socket*>& clientes) {

	//Miro si se puede crear y retorno ACKCREATE
	std::cout << "el cliente quiere crear partida" << std::endl;
	Room newRoom;
	//Creamos una nueva sala con la informacion recibida
	newRoom.name = ims.ReadString();
	newRoom.password = ims.ReadString();
	ims.Read(&newRoom.maxPlayers);

	OutputMemoryStream oms;

	oms.Write((int)Header::ACKCREATE);

	for (int i = 0; i < rooms.size(); i++)
	{
		if (newRoom.name == rooms[i].name) {
			std::cout << "No se puede crear la sala porque ya existe el nombre" << std::endl;
			oms.Write(false);
			client->Send(&oms);
			return;
		}
	}

	std::cout << "No existe una sala con ese nombre, la creamos" << std::endl;
	oms.Write(true);

	client->Send(&oms);

	newRoom.currentPlayers = 1;
	newRoom.clients.push_back({ client->GetRemoteAdress(), client->GetRemotePort() });

	std::cout << "Nombre sala:" << newRoom.name << std::endl;
	std::cout << "Contraseña: -" << newRoom.password << "-" << std::endl;
	std::cout << "maxplayers: " << newRoom.maxPlayers << std::endl;

	mtxRoom.lock();
	rooms.push_back(newRoom);
	mtxRoom.unlock();

	//Gestionar desconexión del cliente 
	selector.Remove(client);
	for (int i = 0; i < clientes.size(); i++)
	{
		MyNetwork::Socket* tempSock = clientes.at(i);
		if (tempSock == client) {
			clientes.erase(clientes.begin() + i);
			client->Disconnect();
			delete client;
			std::cout << "Elimino el socket que se ha desconectado (Cuando creo una sala)\n";
			continue;
		}
	}
}

void JoinSelectedRoom(InputMemoryStream& ims, std::vector<Room>& rooms, MyNetwork::Socket* client) {
	std::string nameRoom = ims.ReadString();
	std::string password = ims.ReadString();
	OutputMemoryStream oms;
	oms.Write((int)Header::ACKJOIN);
	mtxRoom.lock();
	for (int i = 0; i < rooms.size(); i++) {
		if (rooms[i].name == nameRoom) {
			if (rooms[i].password == password) {
				//he encontrado la sala 
				//la contraseña es correcta
				if (rooms[i].currentPlayers != rooms[i].maxPlayers) {

					std::cout << "contraseña correcta" << std::endl;
					rooms[i].currentPlayers++;
					oms.Write(rooms[i].currentPlayers);
					rooms[i].clients.push_back({ client->GetRemoteAdress(),client->GetRemotePort() });
					for (int j = 0; j < rooms[i].clients.size(); j++)
					{
						oms.WriteString(rooms[i].clients[j].IP);
						oms.Write(rooms[i].clients[j].PORT);
					}

					//Compruebo si la sala esta llena, y si es asi la elimino
					if (rooms[i].currentPlayers == rooms[i].maxPlayers) {
						rooms.erase(rooms.begin() + i);
					}

					break;
				}
				else {
					//no caben mas clientes en la sala
					std::cout << "max players reached" << std::endl;
					oms.Write(-1);
					break;
				}
			}
			else {
				//la contraseña NO es correcta
				std::cout << "contraseña incorrecta" << std::endl;
				oms.Write(-1);
				break;
			}
		}
	}
	mtxRoom.unlock();

	MyNetwork::Status status = client->Send(&oms);
	if (status == MyNetwork::Status::DONE)
		std::cout << "Envio ACKJOIN" << std::endl;
	else std::cout << "Fallo al enviar ACKJOIN" << std::endl;

}

void ShowFilteredRooms(MyNetwork::Socket* client, std::vector<Room>& rooms, int maxPlayers, bool passwordFilter) {

	OutputMemoryStream oms;
	oms.Write((int)Header::ROOMS);
	std::vector<std::string> tempNames;
	std::vector<int> tempCurrentPlayers;
	std::vector<int> tempMaxPlayers;

	std::cout << "Busco salas con " << maxPlayers << " y con pass " << passwordFilter << std::endl;

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
					tempMaxPlayers.push_back(rooms[i].maxPlayers);
				}
				break;
			case 3:
				if (rooms[i].maxPlayers == 3 && rooms[i].password != "") {
					numRooms++;
					tempNames.push_back(rooms[i].name);
					tempCurrentPlayers.push_back(rooms[i].currentPlayers);
					tempMaxPlayers.push_back(rooms[i].maxPlayers);
				}
				break;
			case 4:
				if (rooms[i].maxPlayers == 4 && rooms[i].password != "") {
					numRooms++;
					tempNames.push_back(rooms[i].name);
					tempCurrentPlayers.push_back(rooms[i].currentPlayers);
					tempMaxPlayers.push_back(rooms[i].maxPlayers);
				}
				break;
			case -1:
				if (rooms[i].password != "") {
					numRooms++;
					tempNames.push_back(rooms[i].name);
					tempCurrentPlayers.push_back(rooms[i].currentPlayers);
					tempMaxPlayers.push_back(rooms[i].maxPlayers);
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
					tempMaxPlayers.push_back(rooms[i].maxPlayers);
				}
				break;
			case 3:
				if (rooms[i].maxPlayers == 3 && rooms[i].password == "") {
					numRooms++;
					tempNames.push_back(rooms[i].name);
					tempCurrentPlayers.push_back(rooms[i].currentPlayers);
					tempMaxPlayers.push_back(rooms[i].maxPlayers);
				}
				break;
			case 4:
				if (rooms[i].maxPlayers == 4 && rooms[i].password == "") {
					numRooms++;
					tempNames.push_back(rooms[i].name);
					tempCurrentPlayers.push_back(rooms[i].currentPlayers);
					tempMaxPlayers.push_back(rooms[i].maxPlayers);
				}
				break;
			case -1:
				numRooms++;
				tempNames.push_back(rooms[i].name);
				tempCurrentPlayers.push_back(rooms[i].currentPlayers);
				tempMaxPlayers.push_back(rooms[i].maxPlayers);
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
		oms.Write(tempMaxPlayers[i]);

	}

	client->Send(&oms);
}

void ShowAllRooms(MyNetwork::Socket* client, std::vector<Room>& rooms) {
	OutputMemoryStream oms;
	oms.Write((int)Header::ROOMS);
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





