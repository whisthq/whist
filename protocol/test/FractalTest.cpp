#include <iostream>
#include "gtest/gtest.h"

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
TEST(FractalTest, EncryptAndDecrpt) {

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
                                       (FractalPacket*)(sizeof(int) + &encrypted_packet),
                                       (unsigned char*)DEFAULT_BINARY_PRIVATE_KEY);

    //decrypt packet
    FractalPacket decrypted_packet;

    int decrypted_len = decrypt_packet(&encrypted_packet, encrypted_len,
                                        &decrypted_packet, (unsigned char*)DEFAULT_BINARY_PRIVATE_KEY);

    
    //compare original and decrypted packet
    EXPECT_EQ(decrypted_len, original_len);

    for(int index = 0; index < decrypted_len; index++){
        EXPECT_EQ(
            ((uint8_t*)&decrypted_packet)[index], 
            ((uint8_t*)&original_packet)[index]
            );
    }
}


//This test encrypts a packet with one key, then attempts to decrypt it with a differing
//key, confirms that it returns -1
TEST(FractalTest, BadDecrypt) {

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
                                       (FractalPacket*)(sizeof(int) + &encrypted_packet),
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
TEST(FractalTest, PngToBmpToPng) {
    const size_t IMAGE_SIZE_BYTES = 12219;

    //Read in PNG
    FILE* png_image = fopen("test/test_image.png", "rb");
    char png_data[IMAGE_SIZE_BYTES];
    fread(png_data, 1, IMAGE_SIZE_BYTES, png_image);

    //Convert to BMP
    char bmp_data[IMAGE_SIZE_BYTES];

    png_to_bmp(png_data, IMAGE_SIZE_BYTES, (char**)&bmp_data, (int*)&IMAGE_SIZE_BYTES);

    //Convert back to PNG
    char new_png_data[IMAGE_SIZE_BYTES];
    bmp_to_png(bmp_data, IMAGE_SIZE_BYTES, (char**)&new_png_data, (int*)&IMAGE_SIZE_BYTES);

    //compare for equality
    for(int index = 0; index < IMAGE_SIZE_BYTES; index++)
        EXPECT_EQ(png_data[index], new_png_data[index]);
}

//Tests that by converting a PNG to a BMP then converting that back
//to a PNG returns the original image
TEST(FractalTest, BmpToPngToBmp) {
    const size_t IMAGE_SIZE_BYTES = 12219;

    //Read in PNG
    FILE* bmp_image = fopen("test/test_image.bmp", "rb");
    char bmp_data[IMAGE_SIZE_BYTES];
    fread(bmp_data, 1, IMAGE_SIZE_BYTES, bmp_image);

    //Convert to BMP
    char png_data[IMAGE_SIZE_BYTES];

    bmp_to_png(bmp_data, IMAGE_SIZE_BYTES, (char**)&png_data, (int*)&IMAGE_SIZE_BYTES);

    //Convert back to PNG
    char new_bmp_data[IMAGE_SIZE_BYTES];
    png_to_bmp(png_data, IMAGE_SIZE_BYTES, (char**)&new_bmp_data, (int*)&IMAGE_SIZE_BYTES);

    //compare for equality
    for(int index = 0; index < IMAGE_SIZE_BYTES; index++)
        EXPECT_EQ(bmp_data[index], new_bmp_data[index]);
}

/** Testing avpacket_buffer.c/h **/

TEST(FractalTest, PacketsToBufferToPackets) {
    
    char* data1 = "testing...testing";

    //Make some dummy packets
    AVPacket avpkt1 = {
        .buf =NULL,
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


    char* data2 = "testing...testing...testing";

    AVPacket avpkt2 = {
        .buf =NULL,
        .pts = AV_NOPTS_VALUE,
        .dts = AV_NOPTS_VALUE,
        .data = (uint8_t*) data2,
        .size = strlen(data2),
        .stream_index = 0,
        .side_data = NULL,
        .side_data_elems = 0,
        .duration = 0,
        .pos = -1
    };

    //add them to AVPacket array
    AVPacket packets[2];
    memcpy(packets, &avpkt1, sizeof(AVPacket));
    memcpy(packets+1, &avpkt2, sizeof(AVPacket));

    //create buffer and add them to a buffer
    int* buffer = (int*) calloc(2, sizeof(AVPacket));
    write_packets_to_buffer(2, packets, buffer);

    //Read them back
    AVPacket new_packets[2];
    extract_packets_from_buffer(buffer, 2*sizeof(AVPacket), new_packets);

    //Compare equality of packets 
    EXPECT_EQ(new_packets[0], packets[0]);
    EXPECT_EQ(new_packets[1], packets[1]);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}