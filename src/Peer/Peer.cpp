#include <iostream>
#include <thread>
#include <SFML/Network.hpp>

int main() {

	sf::TcpSocket sock;
	sock.connect("localhost", 50000);

	while (true)
	{

	}

	return 0;
}