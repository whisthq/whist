/*
 * This file creates a connection on the client (Windows, MacOS, Linux) device
 * that initiates a connection to their associated server (cloud VM), which then
 * starts streaming its desktop video and audio to the client, while the client
 * starts streaming its user inputs to the server.

 Protocol version: 1.0
 Last modification: 11/24/2019

 By: Philippe Noël

 Copyright Fractal Computers, Inc. 2019
*/
#define _WINSOCK_DEPRECATED_NO_WARNINGS // silence the deprecated warnings

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "include/json-c/json.h" // for parsing the web server query
#include "../include/fractal.h" // header file for protocol functions

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"

#define MSG_CLIPBOARD 7

#define WINDOW_W 1280
#define WINDOW_H 720

// include Winsock library & disable warning if on Windows client
#if defined(_WIN32)
  #include <winsock2.h> // lib for socket programming on windows
  #pragma comment(lib, "wldap32.lib" )
  #pragma comment(lib, "crypt32.lib" )
  #pragma comment(lib, "ws2_32.lib")
  #pragma comment(lib, "libcurl.lib")

  #define CURL_STATICLIB 
  #include "include/curl/curl.h" // for querying the web server

  #pragma warning(disable: 4201)
  #pragma warning(disable: 4244) // disable u_int to u_short conversion warning
  #pragma warning(disable: 4047) // disable char * indirection levels warning
#endif

#ifdef __cplusplus
extern "C" {
#endif

static bool loggedIn = false; // user login variable
const char* vm_ip = ""; // server VM IP address

/*** LOGIN FUNCTIONS START ***/
// authenticate use and get data size for curl
static size_t function_pt(void *ptr, size_t size, size_t nmemb, void *stream)
{
  // unused stream
  (void) stream;

  // JSON parameters
  struct json_object *parsed_json;
  struct json_object *username;
  struct json_object *password;
  struct json_object *public_ip;

  // parse JSON
  parsed_json = json_tokener_parse(ptr);
  json_object_object_get_ex(parsed_json, "username", &username);
  json_object_object_get_ex(parsed_json, "password", &password);
  json_object_object_get_ex(parsed_json, "public_ip", &public_ip);

  // authenticate the user
  if (username && password && public_ip)
  {
    // confirm authentication and get inputs
    loggedIn = true;
    const char *public_ipChar = json_object_to_json_string(public_ip); // only need the IP

    // write VM IP
    vm_ip = public_ipChar;
    }
    else
    {
      loggedIn = false;
      printf("Could not authenticate user\n");
    }
    // return size for curl
    return size * nmemb;
}

// send JSON post to query the database
static int sendJSONPost(char *url, char *jsonObj, bool callback)
{
  // curl vars
  CURL *curl;
  CURLcode res;

  // generate curl headers
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Accept: application/json");
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "charsets: utf-8");
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();

  // set curl for fetching data
  if (curl)
  {
    // set curl parameters
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if (callback)
    {
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, function_pt);
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonObj);

    // get data using curl
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

    // cleanup and terminate
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
  return 0;
}

// log the user in and log its connection time
bool login(const char *username, const char *password)
{
  // generate JSON logout format
  char  *jsonFrame = "{\"username\" : \"%s\", \"password\" : \"%s\"}";
  size_t jsonSize  = strlen(jsonFrame) + strlen(username) + strlen(password) - 1;
  char  *jsonObj   = malloc(jsonSize);

  // send the logout JSON to log the user in
  if(jsonObj != NULL) {
    // write JSON, callback since we start the app
    snprintf(jsonObj, jsonSize, jsonFrame, username, password);
    char *url = "https://cube-celery-vm.herokuapp.com/user/login";
    bool callback = true;

    // send JSON and free memory
    sendJSONPost(url, jsonObj, callback);
    free(jsonObj);
  }
  return loggedIn;
}

// log the logout time of a user
bool logout(const char *username)
{
  // generate JSON logout format
  char  *jsonFrame = "{\"username\" : \"%s\"}";
  size_t jsonSize  = strlen(jsonFrame) + strlen(username) - 1;
  char  *jsonObj   = malloc(jsonSize);

  // send the logout JSON to log the logout time
  if (jsonObj != NULL) {
    // write JSON, no callback since we terminate the app
    snprintf(jsonObj, jsonSize, jsonFrame, username);
    char *url = "https://cube-celery-vm.herokuapp.com/tracker/logoff";
    bool callback = false;

    // send JSON and free memory
    sendJSONPost(url, jsonObj, callback);
    free(jsonObj);
  }
  return true;
}
/*** LOGIN FUNCTIONS END ***/

// main client function
int32_t main(int32_t argc, char **argv)
{
  // usage check for authenticating on our web servers
  if (argc != 3) {
    printf("Usage: client fractal-username fractal-password\n");
    return 1;
  }

  // user inputs for username and password
  char *username = argv[1];
  char *password = argv[2];

  // query Fractal web servers to authenticate the user
  bool authenticated = login(username, password);
  if (!authenticated) {
    printf("Could not authenticate user, invalid username or password\n");
    return 2;
  }

  // all good, we have a user and the VM IP written, time to set up the sockets
  // socket environment variables
  SOCKET RECVsocket, SENDsocket; // socket file descriptors
  struct sockaddr_in clientRECV, clientSEND, server; // this client two ports
  FractalConfig config = FRACTAL_DEFAULTS; // default port settings

  // initialize Winsock if this is a Windows client
  #if defined(_WIN32)
    WSADATA wsa;
    // initialize Winsock (sockets library)
    printf("Initialising Winsock...\n");
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
    {
      printf("Failed. Error Code : %d\n", WSAGetLastError());
      return 2;
    }
    printf("Winsock Initialised.\n");
  #endif

  // Creating our TCP (sending) socket (need it first to initiate connection)
  // AF_INET = IPv4
  // SOCK_STREAM = TCP Socket
  // 0 = protocol automatically detected
  SENDsocket = socket(AF_INET, SOCK_STREAM, 0);
  if (SENDsocket == INVALID_SOCKET || SENDsocket < 0) // Windows & Unix cases
  {
    printf("Could not create send TCP socket\n");
  }
  printf("Send TCP Socket created.\n");

  // prepare the sockaddr_in structure for the send socket
	clientSEND.sin_family = AF_INET; // IPv4
  clientSEND.sin_addr.s_addr = inet_addr(vm_ip); // VM (server) IP received from authenticating
	clientSEND.sin_port = htons(config.serverPortRECV); // initial default port 48888

	// connect to VM (server)
	if (connect(SENDsocket, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
    printf("Could not connect to the VM (server)\n");
    return 3;
	}
  printf("Connected\n");

  // now that we're connected, we need to create our receiving UDP socket
  // Creating our UDP (receiving) socket
  // AF_INET = IPv4
  // SOCK_DGAM = UDP Socket
  // 0 = protocol automatically detected
  RECVsocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (RECVsocket == INVALID_SOCKET || RECVsocket < 0) // Windows & Unix cases
  {
    printf("Could not create receive UDP socket\n");
  }
  printf("Receive UDP Socket created.\n");

  // prepare the sockaddr_in structure for the receiving socket
  clientRECV.sin_family = AF_INET; // IPv4
  clientRECV.sin_addr.s_addr = INADDR_ANY; // any IP
  clientRECV.sin_port = htons(config.clientPortSEND); // initial default port 48889

  // since this is a UDP socket, there is no connection necessary
  // time to start receiving the stream and sending user input
  while (true) {

    // now that our two ports exist and are ready to stream, we need to start strea










  }
  // client or server disconnected, close everything
  // Windows case, closing sockets
  #if defined(_WIN32)
    closesocket(RECVsocket);
    closesocket(SENDsocket);
    WSACleanup(); // close Windows socket library
  #else
    close(RECVsocket);
    close(SENDsocket);
  #endif

  // write close time to server, set loggedin to false and return
  logout(username);
  loggedIn = false;

  return 0;
}

#ifdef __cplusplus
}
#endif

// renable Windows warning, if Windows client
#if defined(_WIN32)
	#pragma warning(default: 4201)
#endif
