#include "server_gui.h"
#include "sys/app_controller.h"
#include "app/app_conf.h"
#include "network.h"
#include "common.h"
#include "AsyncUDP.h"

AsyncUDP udp;
WebServer server(80);
const char *ssid = "WonderLand";
const char *password = "happy2years";

#define SERVER_REFLUSH_INTERVAL 5000UL

char rBuff[18]; //UDP接收缓存
unsigned int localUdpPort = 10011;  //接收数据的端口

struct ServerAppRunData
{
    boolean web_start;                    // 标志是否开启web server服务，0为关闭 1为开启
    boolean req_sent;                     // 标志是否发送wifi请求服务，0为关闭 1为开启
    unsigned long serverReflushPreMillis; // 上一回更新的时间
};

static ServerAppRunData *run_data = NULL;

static int server_init(AppController *sys)
{
    server_gui_init();
    initAP();
    start_web_config();
    run_data = (ServerAppRunData *)malloc(sizeof(ServerAppRunData));
    run_data->web_start = 1;
    run_data->req_sent = 0;
    run_data->serverReflushPreMillis = 0;
    //initUdp();
    Serial.println("初始化结束");
    return 0;
}

void initAP()
{
    WiFi.mode(WIFI_AP);                  //wifi初始化
    WiFi.softAPConfig(IPAddress(192, 168, 4, 2), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    while (!WiFi.softAP(ssid, password)) //启动AP
    {

    }
}

//初始化UDP服务
void initUdp() {
    while(!udp.listen(localUdpPort)){};
    udp.onPacket([](AsyncUDPPacket packet)
    {
        char* data;
        data = (char*)(packet.data());
        Serial.print("读到信息：");
        Serial.println(data); 
         display_setting(
                "WebServer Start",
                "Domain: holocubic",
                WiFi.localIP().toString().c_str(),
                data,
                LV_SCR_LOAD_ANIM_NONE);
    });
}

void start_web_config()
{
    server.on("/savePictureConf", savePictureConf);
    server.on("/saveMediaConf", saveMediaConf);
    server.on(
        "/fupload", HTTP_POST,
        []()
        { server.send(200); },
        handleFileUpload);
    server.begin();
}

void stop_web_config()
{
    run_data->web_start = 0;
    run_data->req_sent = 0;
    server.stop();
    server.close();
}

void savePictureConf()
{
   app_controller->send_to(SERVER_APP_NAME, "Picture",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"switchInterval",
                            (void *)server.arg("switchInterval").c_str());
    // 持久化数据
    app_controller->send_to(SERVER_APP_NAME, "Picture", APP_MESSAGE_WRITE_CFG,
                            NULL, NULL);
}

void saveMediaConf()
{
    app_controller->send_to(SERVER_APP_NAME, "Media",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"switchFlag",
                            (void *)server.arg("switchFlag").c_str());
    app_controller->send_to(SERVER_APP_NAME, "Media",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"powerFlag",
                            (void *)server.arg("powerFlag").c_str());
    // 持久化数据
    app_controller->send_to(SERVER_APP_NAME, "Media", APP_MESSAGE_WRITE_CFG,
                            NULL, NULL);
}

File UploadFile;
void handleFileUpload()
{                                                   // upload a new file to the Filing system
    HTTPUpload &uploadFileStream = server.upload(); // See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/srcv
                                                    // For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
    String filename = uploadFileStream.filename;
    if (uploadFileStream.status == UPLOAD_FILE_START)
    {
        // String filename = uploadFileStream.filename;
        // if (!filename.startsWith("/image"))
        filename = "/image/" + filename;
        Serial.print(F("Upload File Name: "));
        Serial.println(filename);
        tf.deleteFile(filename);                    // Remove a previous version, otherwise data is appended the file again
        UploadFile = tf.open(filename, FILE_WRITE); // Open the file for writing in SPIFFS (create it, if doesn't exist)
    }
    else if (uploadFileStream.status == UPLOAD_FILE_WRITE)
    {
        if (UploadFile)
            UploadFile.write(uploadFileStream.buf, uploadFileStream.currentSize); // Write the received bytes to the file
    }
    else if (uploadFileStream.status == UPLOAD_FILE_END)
    {
        if (UploadFile) // If the file was successfully created
        {
            UploadFile.close(); // Close the file again
            Serial.print(F("Upload Size: "));
            Serial.println(uploadFileStream.totalSize);
            tf.listDir("/image", 250);
        }
        else
        {
            
        }
    }
}

static void server_process(AppController *sys, const ImuAction *action)
{
    lv_scr_load_anim_t anim_type = LV_SCR_LOAD_ANIM_NONE;

    if (RETURN == action->active) 
    {
        stop_web_config();
        sys->app_exit();
        return;
    }
    else if (1 == run_data->web_start)
    {
        server.handleClient(); // 一定需要放在循环里扫描
        // dnsServer.processNextRequest();
        if (doDelayMillisTime(SERVER_REFLUSH_INTERVAL, &run_data->serverReflushPreMillis, false) == true)
        {
            // 发送wifi维持的心跳
            sys->send_to(SERVER_APP_NAME, CTRL_NAME,
                         APP_MESSAGE_WIFI_ALIVE, NULL, NULL);

                display_setting(
                "WebServer Start",
                "Welcome to WonderLand",
                WiFi.localIP().toString().c_str(),
                WiFi.softAPIP().toString().c_str(),
                LV_SCR_LOAD_ANIM_NONE);
        }
    }
}

static void server_background_task(AppController *sys,
                                   const ImuAction *act_info)
{
    // 本函数为后台任务，主控制器会间隔一分钟调用此函数
    // 本函数尽量只调用"常驻数据",其他变量可能会因为生命周期的缘故已经释放
}

static int server_exit_callback(void *param)
{
    setting_gui_del();

    // 释放运行数据
    if (NULL != run_data)
    {
        free(run_data);
        run_data = NULL;
    }
    return 0;
    return 0;
}

static void server_message_handle(const char *from, const char *to,
                                  APP_MESSAGE_TYPE type, void *message,
                                  void *ext_info)
{
    
}


APP_OBJ udp_server_app = {SERVER_APP_NAME, &app_server, "",
                      server_init, server_process, server_background_task,
                      server_exit_callback, server_message_handle};