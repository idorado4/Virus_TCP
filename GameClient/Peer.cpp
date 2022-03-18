#include <iostream>
#include <thread>
#include <SFML/Network.hpp>
#include <MyNetwork.h>
#include <InputMemoryStream.h>
#include <OutputMemoryStream.h>

void Connections(MyNetwork::Socket* _sock, std::vector<MyNetwork::Socket*>* _clientes);
void SendMessage(std::vector<MyNetwork::Socket*>* _clientes);

int main() {

	MyNetwork::Socket sock;
	sf::Socket::Status status = sock.Connect("localhost", 50000);
	switch (status)
	{
	case sf::Socket::Done:
		std::cout << "Done" << std::endl;
		break;
	case sf::Socket::NotReady:
		std::cout << "NotReady" << std::endl;
		break;
	case sf::Socket::Partial:
		std::cout << "Partial" << std::endl;
		break;
	case sf::Socket::Disconnected:
		std::cout << "Disconnected" << std::endl;
		break;
	case sf::Socket::Error:
		std::cout << "Error" << std::endl;
		break;
	default:
		break;
	}
	std::vector<MyNetwork::Socket*> _clientes;

	//std::thread tMessage(SendMessage, &_clientes);
	//tMessage.detach();

	Connections(&sock, &_clientes);

	return 0;
}

void Connections(MyNetwork::Socket* _sock, std::vector<MyNetwork::Socket*>* _clientes) {



	///*sf::Socket::Status status = */int numPlayers = _sock->ReceiveInt();

	/*if (status != sf::Socket::Status::Done) {
		std::cout << "Error al recibir los datos del servidor" << std::endl;
		char exit;
		std::cin >> exit;
		return;
	}*/

	uint16_t localPort = _sock->GetLocalPort();
	_sock->Disconnect();

	MyNetwork::Listener listener;
	/*status = */listener.Listen(localPort);
	/*if (status != sf::Socket::Status::Done) {
		std::cout << "Error al escuchar por el puerto " << localPort << std::endl;
		char exit;
		std::cin >> exit;
		return;
	}*/


	// Create a selector
	MyNetwork::Selector selector;

	// Add the listener to the selector
	selector.Add(listener.Get());

	//Hacemos conexion con los peers recibidos del server
	size_t currentClients = _sock->ReceiveInt();
	std::cout << currentClients << std::endl;


	//for (int i = 0; i < currentClients; i++)
	//{
	//	sf::TcpSocket* client = new sf::TcpSocket;
	//	std::string IP = "";
	//	uint16_t port = 0;
	//	packet >> IP;
	//	packet >> port;

	//	status = client->connect(IP, port);

	//	if (status != sf::Socket::Status::Done) {
	//		std::cout << "Error al conectar el cliente: " << IP << " " << port << std::endl;
	//		char exit;
	//		std::cin >> exit;
	//		return;
	//	}

	//	selector.add(*client);
	//	_clientes->push_back(std::move(client));
	//}

	//// Endless loop that waits for new connections
	//while (true)
	//{
	//	// Make the selector wait for data on any socket
	//	if (selector.wait())
	//	{
	//		if (selector.isReady(listener)) {
	//			// The listener is ready: there is a pending connection
	//			sf::TcpSocket* client = new sf::TcpSocket;
	//			if (listener.accept(*client) == sf::Socket::Done)
	//			{
	//				// Add the new client to the clients list
	//				std::cout << "Llega el cliente con puerto: " << client->getRemotePort() << std::endl;
	//				_clientes->push_back(std::move(client));
	//				// Add the new client to the selector so that we will
	//				// be notified when he sends something
	//				selector.add(*client);
	//			}
	//			else
	//			{
	//				// Error, we won't get a new connection, delete the socket
	//				std::cout << "Error al recoger conexión nueva\n";
	//				delete client;
	//			}
	//		}
	//		else {
	//			for (size_t i = 0; i < _clientes->size(); i++)
	//			{
	//				sf::TcpSocket* client = _clientes->at(i);
	//				if (selector.isReady(*client))
	//				{
	//					// The client has sent some data, we can receive it
	//					sf::Packet packet;
	//					status = client->receive(packet);
	//					if (status == sf::Socket::Done)
	//					{
	//						std::string strRec;
	//						packet >> strRec;
	//						std::cout << "He recibido " << strRec << " del puerto " << client->getRemotePort() << std::endl;
	//					}
	//					else if (status == sf::Socket::Disconnected)
	//					{
	//						client->disconnect();
	//						selector.remove(*client);
	//						_clientes->erase(_clientes->begin() + i);
	//						delete client;
	//						std::cout << "Elimino el socket que se ha desconectado\n";
	//					}
	//					else
	//					{
	//						std::cout << "Error al recibir de " << client->getRemotePort() << std::endl;
	//					}
	//				}
	//			}

	//		}
	//	}
	//}

}
void SendMessage(std::vector<MyNetwork::Socket*>* _clientes)
{
	//sf::Packet packet;
	//std::string message;
	//std::cin >> message;
	//packet << message;
	//for (int i = 0; i < _clientes->size(); i++) {
	//	_clientes->at(i)->send(packet);
	//}
}