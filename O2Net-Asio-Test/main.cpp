#include <O2/Net/Client.h>
#include <O2/Net/Server.h>
#include <iostream>

enum class PacketType : uint32_t {
	PingRequest,
	PingResponse
};

class CustomClient : public o2::net::Client {
public:
	void pingServer() {
		o2::net::Packet packet;
		packet.header.type = (uint32_t)PacketType::PingRequest;
		packet.header.size = 0;

		std::cout << "Sending ping request" << std::endl;
		send(packet);
	}
};

class CustomServer : public o2::net::Server {
public:
	CustomServer(uint16_t port) : o2::net::Server(port) {}

	bool onClientConnect(const std::string& clientAddress) override {
		std::cout << "Client connected: " << clientAddress << std::endl;
		return true;
	}

	void onMessage(o2::net::ConnectionId client, o2::net::Packet& packet) override {
		if (packet.header.type == (uint32_t)PacketType::PingRequest) {
			packet.header.type = (uint32_t)PacketType::PingResponse;
			messageClient(client, packet);
		}
	}
};

int main() {

	int choice;
	std::cout << "1 for Client, 2 for Server\n > ";
	std::cin >> choice;

	if (choice == 1) {
		CustomClient client;

		client.connect("localhost", 42069);

		while (!client.isConnected()) {}

		if (client.isConnected()) {
			std::cout << "Connected to the server" << std::endl;
			client.pingServer();
		}

		while (true) {
			if (!client.incoming().empty()) {
				o2::net::OwnedPacket packet = client.incoming().front();
				client.incoming().pop();
				if (packet.packet.header.type == (uint32_t)PacketType::PingResponse) {
					std::cout << "Received ping response" << std::endl;
				}
			}
		}
	}
	else if (choice == 2) {
		CustomServer server((uint16_t)42069);
		server.start();

		while (true) {
			server.tick(-1, true);
		}
	}

	return 0;
}