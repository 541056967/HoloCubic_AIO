#ifndef APP_SERVER_H
#define APP_SERVER_H

#include <WString.h>
#include "sys/interface.h"
#include "sys/app_controller.h"

extern AppController *app_controller; // APP控制器
#define SERVER_APP_NAME "WebServer"
#define IMAGE_PATH "/image"

#ifdef ESP8266
#include <ESP8266WiFi.h>      // Built-in
#include <ESP8266WiFiMulti.h> // Built-in
#include <ESP8266WebServer.h> // Built-in
#include <ESP8266mDNS.h>
extern ESP8266WebServer server;
#else
#include <WiFi.h>      // Built-in
#include <WiFiMulti.h> // 当我们需要使用ESP8266开发板存储多个WiFi网络连接信息时，可以使用ESP8266WiFiMulti库来实现。
#include <WebServer.h> // https://github.com/Pedroalbuquerque/ESP32WebServer
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <HTTPClient.h>
extern WebServer server;
#endif

extern APP_OBJ udp_server_app;

void initAP();
void  initUdp();

//保存设置
//保存相册的播放间隔，单位为毫秒
void savePictureConf(const String intervalTime);

#endif