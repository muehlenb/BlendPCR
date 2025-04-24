// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#pragma once

#include <boost/asio.hpp>
#include <iostream>
#include <thread>

#include "src/util/math/Mat4.h"

using namespace boost::asio;

class NetworkImageSender {
    boost::asio::io_service io_service;
    ip::udp::endpoint sender_endpoint;
    ip::udp::endpoint receiver_endpoint;
    ip::udp::socket sender_socket;
    ip::udp::socket receiver_socket;

    std::thread receivingThread;
    bool shouldTerminate = false;

    char receivingBuffer[65536];


public:
    Mat4f projection;
    Mat4f view;

    std::chrono::system_clock::time_point lastMsgReceived;

    uint8_t* imageBuffer;

    Mat4f fromUnrealMat(double raw[16]){
        Mat4f input;

        for(int i=0; i < 16; ++i)
            input.data[i] = float(raw[i]);

        input.data[12] /= 100.f;
        input.data[13] /= 100.f;
        input.data[14] /= 100.f;

        return input;
    }

    NetworkImageSender(int maxImageSize, int port = 12345)
        : sender_endpoint(ip::udp::endpoint(ip::address::from_string("127.0.0.1"), port))
        , receiver_endpoint(ip::udp::endpoint(ip::address::from_string("127.0.0.1"), port + 1))
        , sender_socket(ip::udp::socket(io_service, ip::udp::v4()))
        , receiver_socket(ip::udp::socket(io_service, receiver_endpoint))
    {

        imageBuffer = new uint8_t[maxImageSize * 4];

        receivingThread = std::thread([this]() {

            while (!shouldTerminate) {
                std::size_t len = receiver_socket.receive_from(boost::asio::buffer(receivingBuffer, 65536), receiver_endpoint);

                uint16_t msgID = ((uint16_t*)(receivingBuffer))[0];

                if(msgID == 32){
                    double projectionRaw[16];
                    double viewRaw[16];

                    int viewportWidth;
                    int viewportHeight;

                    std::memcpy(projectionRaw, receivingBuffer + 2, sizeof(double) * 16);
                    std::memcpy(viewRaw, receivingBuffer + 2 + sizeof(double) * 16, sizeof(double) * 16);
                    std::memcpy(&viewportWidth, receivingBuffer + 2 + sizeof(double) * 32, sizeof(int));
                    std::memcpy(&viewportHeight, receivingBuffer + 2 + sizeof(double) * 32 + sizeof(int), sizeof(int));

                    projection = fromUnrealMat(projectionRaw);
                    projection.data[10] = 1;
                    projection.data[14] *= -1;

                    view = fromUnrealMat(viewRaw);

                    Mat4f convertZUpToYUp({
                        1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f,
                        0.0f, -1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f
                    });

                    Mat4f convertLeftToRightHand({
                        -1.0f, 0.0f,  0.0f, 0.0f,
                        0.0f, -1.0f,  0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f,  0.0f, 1.0f
                    });


                    // Rotiere um 90°
                    view = Mat4f::rotationY(3.14159f / 2) * Mat4f::rotationX(-3.14159f / 2) * convertLeftToRightHand * view * convertZUpToYUp;

                    /*
                    std::cout << "Viewport Width: " << viewportWidth << " | Viewport Height: " << viewportHeight << std::endl;

                    std::cout << "Projection: " << std::endl << projection << std::endl;
                    std::cout << "View: " << std::endl << view << std::endl;*/

                    lastMsgReceived = std::chrono::system_clock::now();
                }
            }
        });
    }

    ~NetworkImageSender(){
        shouldTerminate = true;
        receiver_socket.shutdown(receiver_socket.shutdown_both);
        sender_socket.shutdown(sender_socket.shutdown_both);
        receivingThread.join();
    }


    void sendImage(unsigned int width, unsigned int height){
        unsigned int imageBytes = width * height * 4;
        unsigned int paketBytes = 60000;

        try {
            for(unsigned int pos = 0; pos < imageBytes; pos += paketBytes){
                int sendByteNum = std::min(imageBytes - pos, paketBytes);

                uint16_t msgID = 0;
                uint16_t stripeLen = sendByteNum;

                uint8_t sendBuffer[8];
                std::memcpy(sendBuffer, &msgID, 2);
                std::memcpy(sendBuffer + 2, &pos, 4);
                std::memcpy(sendBuffer + 6, &stripeLen, 2);

                std::vector<boost::asio::const_buffer> bufferSequence;
                bufferSequence.push_back(boost::asio::buffer(sendBuffer, 8));
                bufferSequence.push_back(boost::asio::buffer(imageBuffer + pos, sendByteNum));

                sender_socket.send_to(bufferSequence, sender_endpoint);
            }
        } catch (boost::system::system_error& e) {
            std::cerr << "Fehler beim Buffersenden: " << e.what() << ", Fehlercode: " << e.code() << std::endl;
        }

    }
};
