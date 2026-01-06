// websocket_server_minimal.cpp
#include <iostream>
#include <thread>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cstdint>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using socklen_t = int;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define INVALID_SOCKET -1
#define SOCKET int
#define closesocket close
#endif

// ------------------- Simple SHA1 + Base64 -------------------
// Pure C++17 implementation for WebSocket handshake
struct SHA1 {
    uint32_t h0=0x67452301, h1=0xEFCDAB89, h2=0x98BADCFE, h3=0x10325476, h4=0xC3D2E1F0;
    std::vector<unsigned char> buffer;
    uint64_t message_len=0;

    static uint32_t rol(uint32_t val,int bits){ return (val<<bits)|(val>>(32-bits)); }

    void update(const unsigned char* data, size_t len) {
        buffer.insert(buffer.end(), data, data+len);
        message_len += len*8;
        while (buffer.size()>=64) { process_chunk(buffer.data()); buffer.erase(buffer.begin(), buffer.begin()+64);}
    }

    void process_chunk(const unsigned char* chunk) {
        uint32_t w[80];
        for(int i=0;i<16;i++)
            w[i]=(chunk[i*4]<<24)|(chunk[i*4+1]<<16)|(chunk[i*4+2]<<8)|chunk[i*4+3];
        for(int i=16;i<80;i++) w[i]=rol(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
        uint32_t a=h0,b=h1,c=h2,d=h3,e=h4,f,k,temp;
        for(int i=0;i<80;i++){
            if(i<20){f=(b&c)|((~b)&d); k=0x5A827999;}
            else if(i<40){f=b^c^d; k=0x6ED9EBA1;}
            else if(i<60){f=(b&c)|(b&d)|(c&d); k=0x8F1BBCDC;}
            else {f=b^c^d; k=0xCA62C1D6;}
            temp=rol(a,5)+f+e+k+w[i]; e=d; d=c; c=rol(b,30); b=a; a=temp;
        }
        h0+=a; h1+=b; h2+=c; h3+=d; h4+=e;
    }

    std::vector<unsigned char> digest() {
        std::vector<unsigned char> msg=buffer;
        msg.push_back(0x80);
        while ((msg.size()*8+64)%512!=0) msg.push_back(0);
        uint64_t ml=message_len;
        for(int i=7;i>=0;i--) msg.push_back((ml>>(i*8))&0xFF);
        for(size_t i=0;i<msg.size();i+=64) process_chunk(msg.data()+i);

        std::vector<unsigned char> result(20);
        uint32_t vals[5]={h0,h1,h2,h3,h4};
        for(int i=0;i<5;i++){
            result[i*4]=(vals[i]>>24)&0xFF;
            result[i*4+1]=(vals[i]>>16)&0xFF;
            result[i*4+2]=(vals[i]>>8)&0xFF;
            result[i*4+3]=vals[i]&0xFF;
        }
        return result;
    }
};

std::string sha1_base64(const std::string& input){
    SHA1 sha;
    sha.update(reinterpret_cast<const unsigned char*>(input.c_str()), input.size());
    auto hash=sha.digest();
    static const char* base64_chars="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    int val=0,valb=-6;
    for(unsigned char c:hash){
        val=(val<<8)+c; valb+=8;
        while(valb>=0){ encoded.push_back(base64_chars[(val>>valb)&0x3F]); valb-=6; }
    }
    if(valb>-6) encoded.push_back(base64_chars[((val<<8)>>(valb+8))&0x3F]);
    while(encoded.size()%4) encoded.push_back('=');
    return encoded;
}

// ------------------- WebSocket helpers -------------------
void send_ws_text(SOCKET client,const std::string& msg){
    unsigned char header[2]={0x81,0};
    size_t len=msg.size();
    if(len<=125){ header[1]=len; send(client,(char*)header,2,0);}
    else if(len<=65535){ header[1]=126; send(client,(char*)header,2,0); uint16_t l=htons(len); send(client,(char*)&l,2,0);}
    else{ header[1]=127; send(client,(char*)header,2,0); uint64_t l=len; // big endian
        unsigned char b[8];
        for(int i=0;i<8;i++) b[7-i]=(l>>(i*8))&0xFF;
        send(client,(char*)b,8,0);
    }
    send(client,msg.c_str(),msg.size(),0);
}

void handle_client(SOCKET client){
    char buffer[4096];
    
    std::string req(buffer);

    

    std::cout<<"Client connected via WebSocket\n";

    // read frames (text only)
    while(true){
        int n=recv(client,buffer,sizeof(buffer),0);
        unsigned char fin=buffer[0]&0x80;
        unsigned char opcode=buffer[0]&0x0F;
        unsigned char mask=buffer[1]&0x80;
        uint64_t payload_len=buffer[1]&0x7F;
        size_t offset=2;
        if(payload_len==126){ payload_len=(buffer[offset]<<8)|buffer[offset+1]; offset+=2;}
        else if(payload_len==127){
            payload_len=0;
            for(int i=0;i<8;i++) payload_len=(payload_len<<8)|buffer[offset+i];
            offset+=8;
        }
        unsigned char masking_key[4]={0};
        if(mask){ memcpy(masking_key,buffer+offset,4); offset+=4;}
        std::string msg;
        msg.resize(payload_len);
        for(size_t i=0;i<payload_len;i++) msg[i]=buffer[offset+i]^(mask?masking_key[i%4]:0);
        std::cout<<"Received: "<<msg<<"\n";

        // Echo back
        send_ws_text(client,msg);
    }

    closesocket(client);
    std::cout<<"Client disconnected\n";
}

// ------------------- Main -------------------
int main(){
    const int port=9000;

#if defined(_WIN32)
    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2),&wsa)!=0){ std::cerr<<"WSAStartup failed\n"; return 1; }
#endif

    SOCKET server_socket=socket(AF_INET,SOCK_STREAM,0);
    if(server_socket==INVALID_SOCKET){ std::cerr<<"Socket failed\n"; return 1;}

    sockaddr_in server_addr{};
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=INADDR_ANY;
    server_addr.sin_port=htons(port);

    int opt=1;
    setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,(char*)&opt,sizeof(opt));

    if(bind(server_socket,(sockaddr*)&server_addr,sizeof(server_addr))<0){ std::cerr<<"Bind failed\n"; return 1;}
    if(listen(server_socket,10)<0){ std::cerr<<"Listen failed\n"; return 1;}

    std::cout<<"WebSocket server running on ws://0.0.0.0:"<<port<<"\n";

    while(true){
        sockaddr_in client_addr;
        socklen_t client_len=sizeof(client_addr);
        SOCKET client=accept(server_socket,(sockaddr*)&client_addr,&client_len);
        if(client==INVALID_SOCKET) continue;
        std::thread(handle_client,client).detach();
    }

#if defined(_WIN32)
    closesocket(server_socket);
    WSACleanup();
#else
    close(server_socket);
#endif

    return 0;
}
