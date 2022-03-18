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
		void Listen(uint16_t PORT);
		sf::TcpListener* Get();
		sf::Socket::Status Accept(Socket _socket);


	};

	class Socket {
		sf::TcpSocket* mySocket;
	public:
		Socket();
		//~Socket();

		sf::Socket::Status Connect(std::string IP, uint16_t PORT);
		sf::TcpSocket* Get();
		void Send(OutputMemoryStream oms);
		int ReceiveInt();
		std::string ReceiveString();
		std::string GetRemoteAdress();
		uint16_t MyNetwork::Socket::GetRemotePort();
		uint16_t MyNetwork::Socket::GetLocalPort();

		void Disconnect();
		

	};

	class Selector {
		sf::SocketSelector* mySelector;
	public:
		Selector();
		sf::SocketSelector* Get();
		void Add(sf::TcpListener* listener);
	}; 

	MyNetwork();


};

