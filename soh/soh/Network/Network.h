#ifndef NETWORK_H
#define NETWORK_H
#ifdef __cplusplus

#include <thread>
#include <SDL2/SDL_net.h>
#include <nlohmann/json.hpp>

class Network {
  private:
    IPaddress networkAddress;
    TCPsocket networkSocket;
    std::thread receiveThread;
    std::string receivedData;

    void ReceiveFromServer();
    void HandleRemoteData(char payload[512]);
    void HandleRemoteData2(const char* payload, int length);
    void HandleRemoteJson(std::string payload);

  public:
    bool isEnabled = false;
    bool isConnected = false;

    void Enable(const char* host, uint16_t port);
    void Disable();

    virtual void OnIncomingData(char payload[512]);

    virtual void OnIncomingJson(nlohmann::json payload);
    virtual void OnConnected();
    virtual void OnDisconnected();
    virtual void ProcessOutgoingPackets();
    void SendDataToRemote(const char* payload);
    virtual void SendJsonToRemote(nlohmann::json packet);
};

#endif // __cplusplus
#endif // NETWORK_H