cmake_minimum_required(VERSION 2.8.12)

project(rv1108_receive)

set(STAGING_USR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../staging/usr/)

set(CMAKE_CXX_STANDARD 11)

set(RV1108_RECEIVE_LIST
    rv1108_receive.cpp
    mpp/mpp_decoder_h264.cpp 
    socket/Tcp.cpp
    socket/UdpServer.cpp
    socket/UdpCombinePackage.cpp
    socket/UdpClient.cpp
    socket/TcpNativeInfo.cpp
    socket/TcpClient.cpp
    socket/SendPackageCache.cpp
    socket/RingBuffer.cpp
    socket/CombinePackage.cpp
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/mpp
    ${CMAKE_CURRENT_SOURCE_DIR}/socket
)

add_definitions(-DLINUX -DSUPPORT_ION -DENABLE_ASSERT)

add_executable(rv1108_receive ${RV1108_RECEIVE_LIST})

target_link_libraries(rv1108_receive
    z 
    c 
    pthread
)


target_link_libraries(rv1108_receive
                      rockchip_mpp)
                      
install(TARGETS rv1108_receive DESTINATION bin)

