#pragma once
#include <SFML/Network.hpp>
#include "InputMemoryStream.h"
#include "OutputMemoryStream.h"


class MyNetwork
{
	class Listener;
	class Socket;
	class Selector;
	enum Status;

public:

	class Listener {
	private:
		sf::TcpListener* myListener;
	public:
		Listener();
		~Listener();
		MyNetwork::Status Listen(uint16_t PORT);
		sf::TcpListener* Get();
		MyNetwork::Status Accept(Socket* _socket);


	};

	class Socket {
	private:
		sf::TcpSocket* mySocket;
	public:
		Socket();
		~Socket();

		MyNetwork::Status Connect(std::string IP, uint16_t PORT);
		sf::TcpSocket* Get();
		MyNetwork::Status Send(OutputMemoryStream* oms);
		InputMemoryStream* Receive(char buffer[], int bufferSize, size_t br);
		MyNetwork::Status MyNetwork::Socket::Receive(InputMemoryStream** ims, char buffer[], int bufferSize, size_t br);
		std::string GetRemoteAdress();
		uint16_t MyNetwork::Socket::GetRemotePort();
		uint16_t MyNetwork::Socket::GetLocalPort();

		void Disconnect();


	};

	class Selector {
	private:
		sf::SocketSelector* mySelector;
	public:
		Selector();
		~Selector();
		sf::SocketSelector* Get();
		void Add(MyNetwork::Listener* listener);
		void Add(MyNetwork::Socket* socket);
		bool Wait();
		bool IsReady(MyNetwork::Listener* listener);
		bool IsReady(MyNetwork::Socket* socket);
		void Remove(MyNetwork::Socket* socket);
		void Remove(MyNetwork::Listener* listener);

	};

	enum Status {
		DONE = sf::Socket::Status::Done,
		NOTREADY = sf::Socket::Status::NotReady,
		PARTIAL = sf::Socket::Status::Partial,
		DISCONNECTED = sf::Socket::Status::Disconnected,
		ERROR = sf::Socket::Status::Error
	};

	MyNetwork();


};

