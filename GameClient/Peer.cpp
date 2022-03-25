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


void Connections(MyNetwork::Socket* _sock, std::vector<MyNetwork::Socket*>* _clientes);
void SendMessage(std::vector<MyNetwork::Socket*>* _clientes);

int main() {

	MyNetwork::Socket sock;
	//conecta con BSS
	MyNetwork::Status status = sock.Connect("localhost", 50000);

	std::vector<MyNetwork::Socket*> _clientes;

	std::thread tMessage(SendMessage, &_clientes);
	tMessage.detach();

	Connections(&sock, &_clientes);

	return 0;
}

//me he conectado ya al server
//me da la info de los demas
//me desconecto del server
//me conecto con los demas
void Connections(MyNetwork::Socket* _sock, std::vector<MyNetwork::Socket*>* _clientes) {

	MyNetwork::Status status;

	//Creamos un contenedor rececptor de datos
	InputMemoryStream* ims = nullptr;	

	//Recibimos 
	size_t br = 0;
	char buffer[1000];
	status = _sock->Receive(&ims, buffer, 1000, br);
	
	if (status != MyNetwork::Status::DONE) {
		std::cout << "Error al recibir el mensaje" << std::endl;
	}

	int currentClients = 0;
	ims->Read(&currentClients);

	std::vector<Player> players;

	//Hacemos conexion con los peers recibidos del server
	for (int i = 0; i < currentClients; i++)
	{
		//MyNetwork::Socket* client = new MyNetwork::Socket();
		std::string IP = "";
		IP = ims->ReadString();
		
		uint16_t port = 0;
		ims->Read(&port);

		std::cout << "Recibido: " << IP << " " << port << std::endl;
		Player newPlayer = { IP, port };
		players.push_back(newPlayer);
	}
	//Desconectas del servidor
	_sock->Disconnect();

	while (true)
	{

	}


	//uint16_t localPort = _sock->GetLocalPort();
	//MyNetwork::Listener listener;
	//status = listener.Listen(localPort);
	//if (status != sf::Socket::Status::Done) {
	//	std::cout << "Error al escuchar por el puerto " << localPort << std::endl;
	//	char exit;
	//	std::cin >> exit;
	//	return;
	//}
	//// Create a selector
	//MyNetwork::Selector selector;

	//// Add the listener to the selector
	//selector.Add(&listener);
	////Conectas con el nuevo cliente
	//status = client->Connect(IP, port);

	//if (status != MyNetwork::Status::DONE) {
	//	std::cout << "Error al conectar el cliente: " << IP << " " << port << std::endl;
	//	char exit;
	//	std::cin >> exit;
	//	return;
	//}

	//selector.Add(client);
	//_clientes->push_back(std::move(client));


	//// Endless loop that waits for new connections
	//while (true)
	//{
	//	// Make the selector wait for data on any socket
	//	if (selector.Wait())
	//	{
	//		if (selector.IsReady(&listener)) {
	//			// The listener is ready: there is a pending connection
	//			MyNetwork::Socket* client = new MyNetwork::Socket;
	//			if (listener.Accept(client) == sf::Socket::Done)
	//			{
	//				// Add the new client to the clients list
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
	//					// The client has sent some data, we can receive it
	//					std::string strRec;
	//					status = client->ReceiveString(&strRec);
	//					if (status == sf::Socket::Done)
	//					{
	//						std::cout << "He recibido " << strRec << " del puerto " << client->GetRemotePort() << std::endl;
	//					}
	//					else if (status == sf::Socket::Disconnected)
	//					{
	//						client->Disconnect();
	//						selector.Remove(client);
	//						_clientes->erase(_clientes->begin() + i);
	//						delete client;
	//						std::cout << "Elimino el socket que se ha desconectado\n";
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
void SendMessage(std::vector<MyNetwork::Socket*>* _clientes)
{
	//OMS SEND PACKET
	OutputMemoryStream oms;
	std::string message;
	std::cin >> message;
	oms.WriteString(message);
	for (int i = 0; i < _clientes->size(); i++) {
		_clientes->at(i)->Send(&oms);
	}
}