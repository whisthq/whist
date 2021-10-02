#include <iostream>
#include "gtest/gtest.h"
#include <fstream>
#include <iterator>
#include <algorithm>

extern "C" {
    #include "client/client_utils.h"
    #include "fractal/utils/aes.h"
    #include "fractal/utils/png.h"
    #include "fractal/utils/avpacket_buffer.h"
}

#define DEFAULT_BINARY_PRIVATE_KEY \
    "\xED\x5E\xF3\x3C\xD7\x28\xD1\x7D\xB8\x06\x45\x81\x42\x8D\x19\xEF"
#define SECOND_BINARY_PRIVATE_KEY \
    "\xED\xED\xED\xED\xD7\x28\xD1\x7D\xB8\x06\x45\x81\x42\x8D\xED\xED"


/** Testing aes.c/h **/

//This test makes a packet, encrypts it, decrypts it, and confirms the latter is 
//the original packet
TEST(FractalLibraryTest, EncryptAndDecrpt) {

    char* data = "testing...testing";
    size_t len = strlen(data);

    //Construct test packet 
    FractalPacket original_packet; 

    // Contruct packet metadata
    original_packet.id = -1;
    original_packet.type = PACKET_MESSAGE; 
    original_packet.index = 0;
    original_packet.payload_size = len;
    original_packet.num_indices = 1;
    original_packet.is_a_nack = false;

    // Copy packet data
    memcpy(original_packet.data, data, len);

    // Encrypt the packet using aes encryption
    int original_len = PACKET_HEADER_SIZE + original_packet.payload_size;

    FractalPacket encrypted_packet;
    int encrypted_len = encrypt_packet(&original_packet, original_len,
                                       &encrypted_packet,
                                       (unsigned char*)DEFAULT_BINARY_PRIVATE_KEY);

    //decrypt packet
    FractalPacket decrypted_packet;

    int decrypted_len = decrypt_packet(&encrypted_packet, encrypted_len,
                                        &decrypted_packet, (unsigned char*)DEFAULT_BINARY_PRIVATE_KEY);


    //compare original and decrypted packet
    EXPECT_EQ(decrypted_len, original_len);
    EXPECT_EQ(strcmp((char*)decrypted_packet.data, (char*)original_packet.data), 0);
}


//This test encrypts a packet with one key, then attempts to decrypt it with a differing
//key, confirms that it returns -1
TEST(FractalLibraryTest, BadDecrypt) {

    char* data = "testing...testing";
    size_t len = strlen(data);

    //Construct test packet 
    FractalPacket original_packet; 

    // Contruct packet metadata
    original_packet.id = -1;
    original_packet.type = PACKET_MESSAGE; 
    original_packet.index = 0;
    original_packet.payload_size = len;
    original_packet.num_indices = 1;
    original_packet.is_a_nack = false;

    // Copy packet data
    memcpy(original_packet.data, data, len);

    // Encrypt the packet using aes encryption
    int original_len = PACKET_HEADER_SIZE + original_packet.payload_size;

    FractalPacket encrypted_packet;
    int encrypted_len = encrypt_packet(&original_packet, original_len,
                                       &encrypted_packet,
                                       (unsigned char*)DEFAULT_BINARY_PRIVATE_KEY);

    //decrypt packet with differing key
    FractalPacket decrypted_packet;

    int decrypted_len = decrypt_packet(&encrypted_packet, encrypted_len,
                                        &decrypted_packet, (unsigned char*)SECOND_BINARY_PRIVATE_KEY);

    EXPECT_EQ(decrypted_len, -1); 
}

/** Testing png.c/h **/

//Tests that by converting a PNG to a BMP then converting that back
//to a PNG returns the original image
TEST(FractalLibraryTest, PngToBmpToPng) {

    //Read in PNG
    std::ifstream png_image( "images/image.png", std::ios::binary );
    // copies all data into buffer
    std::vector<unsigned char> png_vec(std::istreambuf_iterator<char>(png_image), {});
    int img_size = png_vec.size();
    char* png_buffer = (char*) &png_vec[0];

    //Convert to BMP
    char bmp_buffer[img_size];
    png_to_bmp(png_buffer, img_size, (char**)&bmp_buffer, &img_size);
    //Convert back to PNG
    char new_png_data[img_size];
    bmp_to_png(bmp_buffer, img_size, (char**)&new_png_data, &img_size);

    //compare for equality
    for(int index = 0; index < img_size; index++)
        EXPECT_EQ(png_buffer[index], new_png_data[index]);
}

//Tests that by converting a PNG to a BMP then converting that back
//to a PNG returns the original image
TEST(FractalLibraryTest, BmpToPngToBmp) {
    //Read in PNG
    std::ifstream bmp_image( "images/image.bmp", std::ios::binary );
    // copies all data into buffer
    std::vector<unsigned char> bmp_vec(std::istreambuf_iterator<char>(bmp_image), {});
    int img_size = bmp_vec.size();
    char* bmp_buffer = (char*) &bmp_vec[0];

    //Convert to PNG
    char png_buffer[img_size];
    bmp_to_png(bmp_buffer, img_size, (char**)&png_buffer, &img_size);
    //Convert back to BMP
    char new_bmp_data[img_size];
    png_to_bmp(png_buffer, img_size, (char**)&new_bmp_data, &img_size);

    //compare for equality
    for(int index = 0; index < img_size; index++)
        EXPECT_EQ(bmp_buffer[index], new_bmp_data[index]);
}

/** Testing avpacket_buffer.c/h **/
void avpackets_equal(AVPacket pkt1, AVPacket pkt2) {
    LOG_INFO("We good");
    EXPECT_TRUE(pkt1.buf == pkt2.buf); //both should be NULL
    LOG_INFO("We good");
    EXPECT_EQ(pkt1.pts, pkt2.pts);
    LOG_INFO("We good");
    EXPECT_EQ(pkt1.dts, pkt2.dts);
    LOG_INFO("We good");
    EXPECT_TRUE(strcmp((char*) pkt1.data, (char*) pkt2.data) == 0);
    LOG_INFO("We good");
    EXPECT_EQ(pkt1.size, pkt2.size);
    LOG_INFO("We good");
    EXPECT_EQ(pkt1.stream_index, pkt2.stream_index);
    EXPECT_TRUE(pkt1.side_data == pkt2.side_data);
    EXPECT_EQ(pkt1.side_data_elems, pkt2.side_data_elems);
    EXPECT_EQ(pkt1.duration, pkt2.duration);
    EXPECT_EQ(pkt1.pos, pkt2.pos);
}

TEST(FractalLibraryTest, PacketsToBuffer) {

    //Make some dummy packets

    char* data1 = "testing...testing";

    AVPacket avpkt1 = {
        .buf = NULL,
        .pts = AV_NOPTS_VALUE,
        .dts = AV_NOPTS_VALUE,
        .data = (uint8_t*) data1,
        .size = strlen(data1),
        .stream_index = 0,
        .side_data = NULL,
        .side_data_elems = 0,
        .duration = 0,
        .pos = -1
    };
    

    //add them to AVPacket array
    AVPacket* packets = &avpkt1;

    //create buffer and add them to a buffer
    int buffer[28];
    write_packets_to_buffer(1, packets, buffer);

    //Confirm buffer creation was successful
    EXPECT_EQ(*buffer, 1);
    EXPECT_EQ(*(buffer+1), strlen(data1));
    EXPECT_EQ(strncmp((char*)(buffer+2), data1, strlen(data1)), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}