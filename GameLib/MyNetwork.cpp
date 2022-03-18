#include "MyNetwork.h"


MyNetwork::MyNetwork()
{
}


//Listener
#pragma region Listener

MyNetwork::Listener::Listener()
{
	myListener = new sf::TcpListener();
}

void MyNetwork::Listener::Listen(uint16_t PORT)
{
	myListener->listen(PORT);
}

sf::TcpListener* MyNetwork::Listener::Get()
{
	return myListener;
}

sf::Socket::Status MyNetwork::Listener::Accept(Socket _socket)
{
	sf::Socket::Status status = myListener->accept(*_socket.Get());
	return  status;
}

#pragma endregion

//Socket
#pragma region Socket
MyNetwork::Socket::Socket()
{
	mySocket = new sf::TcpSocket();
}
sf::Socket::Status MyNetwork::Socket::Connect(std::string IP, uint16_t PORT)
{
	return mySocket->connect(IP, PORT);
}
sf::TcpSocket* MyNetwork::Socket::Get()
{
	return mySocket;
}
void MyNetwork::Socket::Send(OutputMemoryStream oms)
{
	mySocket->send(oms.GetBufferPtr(), oms.GetLength());
}

int MyNetwork::Socket::ReceiveInt()
{
	size_t br = 0;
	char buffer[1000];
	InputMemoryStream ims(buffer, br);
	return 0;
}

std::string MyNetwork::Socket::ReceiveString()
{
	size_t br;
	char buffer[1000];

	mySocket->receive(buffer, 1000, br);

	InputMemoryStream ims(buffer, br);

	return ims.ReadString();
}

std::string MyNetwork::Socket::GetRemoteAdress()
{
	return mySocket->getRemoteAddress().toString();
}

void MyNetwork::Socket::Disconnect()
{
	mySocket->disconnect();
}

uint16_t MyNetwork::Socket::GetRemotePort()
{
	return mySocket->getRemotePort();
}

uint16_t MyNetwork::Socket::GetLocalPort() {

	return mySocket->getLocalPort();
}
#pragma endregion

//Selector
#pragma region Selector
MyNetwork::Selector::Selector()
{
	mySelector = new sf::SocketSelector();
}
sf::SocketSelector* MyNetwork::Selector::Get()
{
	return mySelector;
}

void MyNetwork::Selector::Add(sf::TcpListener* listener)
{
	mySelector->add(*listener);
}


#pragma endregion
