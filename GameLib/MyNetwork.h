#pragma once
#include <SFML/Network.hpp>
#include "InputMemoryStream.h"
#include "OutputMemoryStream.h"


class MyNetwork
{
	class Listener;
	class Socket;
	class Selector;
public:
	


	class Listener {

		sf::TcpListener* myListener;
	public:
		Listener();
		sf::TcpListener* Get();
		sf::Socket::Status Accept(Socket _socket);


	};

	class Socket {
		sf::TcpSocket* mySocket;
	public:
		Socket();
		~Socket();
		sf::TcpSocket* Get();
		void Send(OutputMemoryStream oms);
		

	};

	class Selector {
		sf::SocketSelector* selector;
	public:
		Selector();
		sf::SocketSelector* Get();
	};

	MyNetwork();


};

