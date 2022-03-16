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
void MyNetwork::Socket::Send(OutputMemoryStream oms)
{
	mySocket->send(oms.GetBufferPtr(), oms.GetLength());
}

//????
sf::lpAddress MyNetwork::Socket::GetRemoteAdress()
{
	mySocket->getRemoteAddress();
}
#pragma endregion

//Selector
#pragma region Selector
sf::SocketSelector* MyNetwork::Selector::Get()
{
	return nullptr;
}


#pragma endregion
