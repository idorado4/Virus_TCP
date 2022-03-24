#include <iostream>
#include <thread>
#include <SFML/Network.hpp>
#include <InputMemoryStream.h>
#include <OutputMemoryStream.h>
#include <MyNetwork.h>

bool running;


struct Peer
{
	std::string IP;
	uint16_t PORT; //Unsigned Short

	Peer(std::string _ip, uint16_t _port) {
		IP = _ip;
		PORT = _port;
	}
};

void Manager();

int main() {

	running = true;

	Manager();

	/*std::thread tManager(Manager);
	tManager.detach();*/

	/*char command;
	do {
		std::cout << "Type 'c' to close the Server" << std::endl;
		std::cin >> command;
	} while (command != 'c');*/


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

	//Los clientes conectados
	std::vector<Peer> clients;

	while (clients.size() < 4) {

		MyNetwork::Socket* sock = new MyNetwork::Socket();
		status = (MyNetwork::Status)listener.Accept(sock);

		if (status != MyNetwork::Status::DONE) {
			std::cout << "Error al conectar el nuevo cliente" << std::endl;
			continue;
		}

		//enviar la info de los otros peers al nuevo
		OutputMemoryStream oms;
		int clientsSize = clients.size();
		oms.Write(clientsSize);
		uint16_t port = sock->GetRemotePort();
		std::cout << "port que envio " << port << std::endl;
		oms.Write(port);

		std::cout << "Clientes que envio " << clientsSize << std::endl;

		std::string IP = sock->GetRemoteAdress();

		oms.WriteString(IP);

		sock->Send(&oms);

		OutputMemoryStream oms2;

		oms2.WriteString("hola");

		int uno = 1;

		oms2.Write(uno);

		//le envio la info de los clientes en partida al nuevo
		/*for (int i = 0; i < clients.size(); i++) {
			oms.WriteString(clients[i].IP);
			oms.Write(clients[i].PORT);
		}*/

		sock->Send(&oms2);

		if (status != MyNetwork::Status::DONE) {
			std::cout << "Error al enviar el mensaje" << std::endl;
			continue;
		}

		Peer newClient = { sock->GetRemoteAdress(), sock->GetRemotePort() };

		clients.push_back(newClient);

		for (int i = 0; i < clients.size(); i++) {
			std::cout << "Current clients list: " << clients[i].IP << " " << clients[i].PORT << std::endl;
		}

		sock->Disconnect();

		std::cout << clients.size() << std::endl;
	}





}