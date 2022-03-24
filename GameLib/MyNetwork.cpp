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

MyNetwork::Listener::~Listener()
{
	delete myListener;
	myListener = nullptr;
}

MyNetwork::Status MyNetwork::Listener::Listen(uint16_t PORT)
{
	return (MyNetwork::Status)myListener->listen(PORT);
}

sf::TcpListener* MyNetwork::Listener::Get()
{
	return myListener;
}

MyNetwork::Status MyNetwork::Listener::Accept(Socket* _socket)
{
	return (MyNetwork::Status)myListener->accept(*(_socket->Get()));
}

#pragma endregion

//Socket
#pragma region Socket
MyNetwork::Socket::Socket()
{
	mySocket = new sf::TcpSocket();
}
MyNetwork::Socket::~Socket()
{
	delete mySocket;
	mySocket = nullptr;
}
MyNetwork::Status MyNetwork::Socket::Connect(std::string IP, uint16_t PORT)
{
	return (MyNetwork::Status)mySocket->connect(IP, PORT);
}
sf::TcpSocket* MyNetwork::Socket::Get()
{
	return mySocket;
}
MyNetwork::Status MyNetwork::Socket::Send(OutputMemoryStream* oms)
{
	return (MyNetwork::Status)mySocket->send(oms->GetBufferPtr(), oms->GetLength());
}


InputMemoryStream* MyNetwork::Socket::Receive(char buffer[], int bufferSize, size_t br)
{
	mySocket->receive(buffer, 1000, br);
	return new InputMemoryStream(buffer, br);
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
MyNetwork::Selector::~Selector()
{
	delete mySelector;
	mySelector = nullptr;
}
sf::SocketSelector* MyNetwork::Selector::Get()
{
	return mySelector;
}

void MyNetwork::Selector::Add(MyNetwork::Listener* listener)
{
	mySelector->add(*listener->Get());
}
void MyNetwork::Selector::Add(MyNetwork::Socket* socket)
{
	mySelector->add(*socket->Get());
}

bool MyNetwork::Selector::Wait()
{
	return mySelector->wait();
}

bool MyNetwork::Selector::IsReady(MyNetwork::Listener* listener)
{
	return mySelector->isReady(*listener->Get());
}

bool MyNetwork::Selector::IsReady(MyNetwork::Socket* socket)
{
	return mySelector->isReady(*socket->Get());
}

void MyNetwork::Selector::Remove(MyNetwork::Socket* socket)
{
	mySelector->remove(*socket->Get());
}


#pragma endregion
