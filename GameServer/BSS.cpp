#include <iostream>
#include <thread>
#include <SFML/Network.hpp>
#include <InputMemoryStream.h>
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

	//sf::TcpListener listener;
	//sf::Socket::Status status = listener.listen(50000);
	
	
	MyNetwork::Listener listener;
	sf::Socket::Status status = listener.Get()->listen(50000);
	
	
	if (status != sf::Socket::Status::Done) {
		std::cout << "Error al escuchar por el puerto 50000" << std::endl;
		char exit;
		std::cin >> exit;
		return;
	}

	//Los clientes conectados
	std::vector<Peer> clients;

	////El gestor de 
	//sf::SocketSelector selector;
	//selector.add(listener);

	while (clients.size() < 4) {

		//sf::TcpSocket sock;
		MyNetwork::Socket sock;
		status = listener.Accept(sock);

		//QUE HACEMOS CON LOS STATUS??
		if (status != sf::Socket::Status::Done) {
			std::cout << "Error al conectar el nuevo cliente" << std::endl;
			continue;
		}

		//enviar la info de los otros peers al nuevo
		sf::Packet packet;
		packet << clients.size();
		std::cout << clients.size() << std::endl;
		for (int i = 0; i < clients.size(); i++) {
			packet << clients[i].IP;
			packet << clients[i].PORT;
		}

		//aqui no ha de ser un packet ha de ser un OMS
		status = sock.Send(packet);


		Peer newClient = { sock.getRemoteAddress().toString(), sock.getRemotePort() };
		
		clients.push_back(newClient);
		
		std::cout << "Conectado el cliente: " << newClient.IP << " " << newClient.PORT << std::endl;

		sock.disconnect();
	}





}