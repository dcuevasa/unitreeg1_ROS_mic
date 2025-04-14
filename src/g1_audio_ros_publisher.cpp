#include <fstream>
#include <iostream>
#include <thread>
#include <signal.h>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// ROS headers
#include <ros/ros.h>
#include <naoqi_bridge_msgs/AudioBuffer.h>

// Configuración multicast para los micrófonos
#define GROUP_IP "239.168.123.161"
#define PORT 5555

// Configuración de audio
#define SAMPLE_RATE 16000
#define BUFFER_SIZE 2048
#define CHANNELS 1

bool running = true;

void signalHandler(int sig)
{
    running = false;
    ros::shutdown();
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "Usage: g1_audio_ros_publisher [NetWorkInterface(eth0)]" << std::endl;
        exit(1);
    }

    // Inicializar nodo ROS
    ros::init(argc, argv, "g1_audio_publisher", ros::init_options::NoSigintHandler);
    ros::NodeHandle nh;
    
    // Definir publicador
    ros::Publisher audio_pub = nh.advertise<naoqi_bridge_msgs::AudioBuffer>("/mic", 10);
    
    // Configurar controlador de señales
    signal(SIGINT, signalHandler);
    
    // Configurar socket multicast para recibir audio
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        ROS_ERROR("Error creating socket");
        return 1;
    }
    
    // Configuración de dirección local
    sockaddr_in local_addr{};
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        ROS_ERROR("Error configuring socket reuse");
        close(sock);
        return 1;
    }
    
    if (bind(sock, (sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        ROS_ERROR("Error binding socket");
        close(sock);
        return 1;
    }
    
    // Unirse al grupo multicast
    ip_mreq mreq{};
    inet_pton(AF_INET, GROUP_IP, &mreq.imr_multiaddr);
    
    // Usar la interfaz de red especificada
    if (strlen(argv[1]) > 0) {
        mreq.imr_interface.s_addr = inet_addr(argv[1]);
    } else {
        mreq.imr_interface.s_addr = INADDR_ANY;
    }
    
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        ROS_ERROR("Error joining multicast group");
        close(sock);
        return 1;
    }
    
    // Buffer para recibir datos de audio
    char buffer[BUFFER_SIZE];
    ros::Rate loop_rate(30);  // Frecuencia de publicación
    
    ROS_INFO("G1 Audio Publisher iniciado. Publicando en tópico /mic");
    
    while (running && ros::ok()) {
        // Configurar timeout para recvfrom
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;  // 100ms timeout
        
        int ready = select(sock + 1, &readSet, NULL, NULL, &timeout);
        
        if (ready > 0) {
            ssize_t len = recvfrom(sock, buffer, sizeof(buffer), 0, nullptr, nullptr);
            
            if (len > 0) {
                // Crear un mensaje AudioBuffer
                naoqi_bridge_msgs::AudioBuffer msg;
                
                // Configurar encabezado
                msg.header.stamp = ros::Time::now();
                msg.header.frame_id = "audio";
                
                // Configurar información de audio
                msg.frequency = SAMPLE_RATE;
                
                // Configurar mapa de canales (mono = CHANNEL_FRONT_CENTER)
                msg.channelMap.resize(1);
                msg.channelMap[0] = naoqi_bridge_msgs::AudioBuffer::CHANNEL_FRONT_CENTER;
                
                // Convertir los datos de audio al formato del mensaje
                msg.data.resize(len / 2);  // Dividir por 2 porque cada muestra es de 16 bits
                const int16_t* samples = reinterpret_cast<const int16_t*>(buffer);
                
                // Copiar las muestras directamente
                for (size_t i = 0; i < len/2; i++) {
                    msg.data[i] = samples[i];
                }
                
                // Publicar el mensaje
                audio_pub.publish(msg);
                
                ROS_DEBUG("Published audio buffer with %zu samples", msg.data.size());
            }
        }
        
        ros::spinOnce();
        loop_rate.sleep();
    }
    
    // Limpiar
    close(sock);
    ROS_INFO("G1 Audio Publisher finalizado");
    
    return 0;
}