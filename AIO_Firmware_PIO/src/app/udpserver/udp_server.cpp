#include "server_gui.h"
#include "sys/app_controller.h"
#include "app/app_conf.h"
#include "network.h"
#include "common.h"
#include "AsyncUDP.h"

AsyncUDP udp;
const char *ssid = "WonderLand";
const char *password = "happy2years";

#define SERVER_REFLUSH_INTERVAL 5000UL

char rBuff[18]; //UDP接收缓存
unsigned int localUdpPort = 10011;  //接收数据的端口

                

static int server_init(AppController *sys)
{
    server_gui_init();
    initAP();
    initUdp();
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

void savePictureConf(const String intervalTime)
{
    app_controller->send_to(SERVER_APP_NAME, "Picture",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"switchInterval",
                            (void *)intervalTime.c_str());
    // 持久化数据
    app_controller->send_to(SERVER_APP_NAME, "Picture", APP_MESSAGE_WRITE_CFG,
                            NULL, NULL);
}

static void server_process(AppController *sys, const ImuAction *action)
{
    lv_scr_load_anim_t anim_type = LV_SCR_LOAD_ANIM_NONE;

    if (RETURN == action->active) 
    {
        sys->app_exit();
        return;
    }

  
    display_setting(
                "WebServer Start",
                "Welcome to WonderLand",
                WiFi.localIP().toString().c_str(),
                WiFi.softAPIP().toString().c_str(),
                LV_SCR_LOAD_ANIM_NONE);
        
    delay(300);
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