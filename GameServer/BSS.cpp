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
		
		//Le envío el número de clientes que hay ya en la partida
		int clientsSize = clients.size();
		oms.Write(clientsSize);
		std::cout << "Clients Sended " << clientsSize << std::endl;


		std::string IP = sock->GetRemoteAdress();
		uint16_t port = sock->GetRemotePort();
		std::cout << "Conneted Client Info: IP-> " << IP << " Puerto -> " << port << std::endl;


		//le envio la info de los clientes en partida al nuevo
		for (int i = 0; i < clients.size(); i++) {
			std::string ipClient = clients[i].IP;
			oms.WriteString(ipClient);

			uint16_t portClient = clients[i].PORT;
			oms.Write(portClient);
		}

		status = sock->Send(&oms);

		if (status != MyNetwork::Status::DONE) {
			std::cout << "Error al enviar el mensaje" << std::endl;
			continue;
		}

		Peer newClient = { sock->GetRemoteAdress(), sock->GetRemotePort() };

		clients.push_back(newClient);

		std::cout << "Current clients list: " << std::endl;
		for (int i = 0; i < clients.size(); i++) {
			std::cout << "IP->" << clients[i].IP << " PORT->" << clients[i].PORT << std::endl;
		}

		sock->Disconnect();
	}





}