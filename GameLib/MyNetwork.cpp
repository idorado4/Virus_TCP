#include "MyNetwork.h"


MyNetwork::MyNetwork()
{
}

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

void MyNetwork::Socket::Send(OutputMemoryStream oms)
{
	mySocket->send(oms.GetBufferPtr(), oms.GetLength());
}
