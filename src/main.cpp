/*
 * ==================================================================================
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * File_name : MQTTdemo.ino
 * Describe  : MQTT使用模板
 * Author	   : GHJ
 * Date		   : 2020年07月05日 13:40:00
 * 
 * PS:
 *    需要MQTT->PubSubClient库:https://github.com/knolleary/pubsubclient.git
 *    需要SHA256->Crypto库:https://github.com/OperatorFoundation/Crypto.git
 *    需要WebServer->ESP Async WebServer库:https://github.com/me-no-dev/ESPAsyncWebServer.git
 *    需要Json->ArduinoJson库:https://github.com/bblanchon/ArduinoJson.git
 *    需要OLED->U8g2库:https://github.com/olikraus/u8g2.git
 *    X 需要BMP280->BMP280库:https://github.com/adafruit/Adafruit_BMP280_Library.git
 *    需要DHT11->DHT库:https://github.com/adafruit/DHT-sensor-library.git
 *    需要BME280->BME280库:https://github.com/adafruit/Adafruit_BME280_Library.git
 *    需要Blinker->Blinker库:https://github.com/blinker-iot/blinker-library.git   
 * 
 * Change Logs:
 * Date               Author          Notes
 * 2020年07月05日       GHJ         the first version
 * 2020年07月14日       GHJ         add mqtt to connect EMQX
 * 2020年10月05日       GHJ         add Aliyun MQTT rules to connect Aliyun IoT
 * 2020年10月06日       GHJ         add AliyunIoT function to connect Aliyun IoT 
 * 2020年10月16日       GHJ         add WebServer & freeRTOS 
 * 2020年10月18日       GHJ         add FileSystem & Tool of JSON & Alarm LED for TIME TASK
 * 2020年10月29日       GHJ         add 0.96 inch OLED and BMP280 sensor
 * 2020年10月30日       GHJ         add HTTP and DHT11 sensor
 * 2020年11月17日       GHJ         add BME280 & remove BMP280
 * 2020年12月13日       GHJ         add Blinker to connect Mi-IoT
 * 2020年12月30日       GHJ         add two threads : Thread_OELD & Thead_HTTP 
 * 
 * ===================================================================================
 */

/*=========================== 头文件 =============================*/
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Crypto.h>
#include <SHA256.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <U8g2lib.h>
#include <Adafruit_BME280.h>
#include <SPI.h>
#include <Wire.h>
#include <DHT.h>
#include <AsyncUDP.h>
#include <WiFiUdp.h>
#define BLINKER_WIFI
#define BLINKER_MIOT_SENSOR
#include <Blinker.h>

// #include <Update.h>
/*=========================== 全局变量及宏定义 =============================*/
#define port 1883                   // port : 端口 通常 1883 不需要改
#define DHTPIN 0                    // DHTPIN : dht11 data pin
String auth = "93b906890a38";       // Blinker 密钥
String SSID = "heartbeats";         // ssid : WIFI名称
String PASSWORD = "24682468";       // password : WIFI密码
String content;                     // content : Web配网存放HTML页面
WiFiClient espClient;               // WiFiClient : 创建wifi 客户端对象,用于接入WIFI
HTTPClient espHttp;                 // HTTP : HTTP请求
AsyncWebServer server(80);          // server : Web配网服务器
PubSubClient mqttClient(espClient); // PubSubClient 创建mqtt 客户端对象,用于接入MQTT
File file;                          // File : 文件句柄
StaticJsonDocument<20000> doc;      // jSON数据处理
//Adafruit_BMP280 bmp(SS);           // bmp280
Adafruit_BME280 bme(SS);                                                                      // bme280
DHT dht11(DHTPIN, DHT11);                                                                     // DHT11
int httpResponseCode;                                                                         // http响应状态
String httpBaseURL = "http://47.95.249.141:1880";                                             // http访问的地址
String serverBath = "https://tianqiapi.com/api?version=v6&appid=29776943&appsecret=GtjmmR3Y"; //天气API地址

float temperature_bme = 0;      //BME280读取的温度
float humidity_bme = 0;         //BME280读取的湿度
float pressure_bme = 0;         //BME280读取的气压
String date = "none";           // "日期：2020-12-21"
const char *week = "none";      // "星期：星期一"
const char *city = "none";      // "所在城市：杭州"
const char *wea = "none";       // "天气：晴"
const char *wea_img = "none";   // "天气 icon：晴"
const char *humidity = "none";  // "当前湿度：32%"
const char *pressure = "none";  // "当前气压：1023"
const char *win = "none";       // "风向：东风"
const char *win_speed = "none"; // "风速：2级"
const char *air_level = "none"; // "空气质量：良"
int air_pm25 = 0;               // "PM2.5浓度：80"
int tem = 0;                    // "当前气温：9"
int tem1 = 0;                   // "最高温：8"
int tem2 = 0;                   // "最低温：0"

/* u8g2 constructer */
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/SCL, /* data=*/SDA); // ESP32 Thing, HW I2C with pin remapping

/** if you want to use Aliyun you can use the following instructions.
 *  
 *  STEP 1 定义宏定义 #define Aliyun_IoT
 *            such as  //#define Aliyun_IoT -> #define Aliyun_IoT
 *  STEP 2 设置三元组信息 
 *            such as     String productKey = "a1AiKMtUf57";   //产品Key
 *                        String deviceName = "demo";          //设备名
 *                        String deviceSecret = "77dcf17f5bf94e05f370a55ae2fc085b"; //设备密码  
 *  PS : 默认使用用户私人的云平台，请自行配置
 */
#define Aliyun_IoT
#ifdef Aliyun_IoT
/********************************* 阿里云IoT 物联网云平台 ********************************/
#define SHA256HMAC_SIZE 32
/*三元组*/
String productKey = "a188WDNaBJg";                        //产品Key
String deviceName = "test";                               //设备名
String deviceSecret = "f2af06e21a62147b92127cdd0d25829e"; //设备密码
/*以下信息由AliyunIoT()接口实现,不必手动修改*/
//${YourProductKey}.iot-as-mqtt.${YourRegionId}.aliyuncs.com
String aliyun_mqtt_server = "YourProductKey.iot-as-mqtt.cn-shanghai.aliyuncs.com";
//${clientId}|securemode=3,signmethod=hmacsha1|
//${clientId}为设备的ID信息。可取任意值，长度在64字符以内。建议使用设备的MAC地址或SN码。
String aliyun_clientID = "clientId|securemode=3,signmethod=hmacsha1|";
//${YourDeviceName}&${YourProductKey}
String aliyun_clientName = "YourDeviceName&YourProductKey";
String aliyun_clientPwd = "3BE07969F6995653F4FB53A86878DF0BB0928576";

/* Alinker 上报消息 methods 规范 */
String POST_PROPERY = "thing.event.property.post"; //属性上报方法
// String POST_EVENT = "thing.event.";                //事件上报
// String POST_TAGS = "thing.deviceinfo.update";      //设备标签上报方法
// String DELETE_TAGS = "thing.deviceinfo.delete";    //设备标签删除方法
// String GET_CONFIG = "thing.config.get";            //设备配置获取方法

/* 物理模型 topic */
// 设备属性上报 publish topic
String PROPERTY_POST_TOPIC = "/sys/" + productKey + "/" + deviceName + "/thing/event/property/post";
// 云端响应属性上报 subscribe topic
String PROPERTY_POST_REPLY_TOPIC = "/sys/" + productKey + "/" + deviceName + "/thing/event/property/post_reply";
// 云端设备属性设置 subscribe topic
String ONSET_PROPS_TOPIC = "/sys/" + productKey + "/" + deviceName + "/thing/service/property/set";

// 设备服务调用 publish topic
// String SERVICE_TOPIC = "/sys/" + productKey + "/" + deviceName + "/thing/service/";
// 设备端响应服务调用 subscribe topic
// String SERVICE_REPLY_TOPIC = "/sys/" + productKey + "/" + deviceName + "/thing/service/";

// 设备事件上报 publish topic
// String EVENT_POST_TOPIC = "/sys/" + productKey + "/" + deviceName + "/thing/event/";
// 云端响应事件上报 subscribe topic
// String EVENT_POST_REPLY_TOPIC = "/sys/" + productKey + "/" + deviceName + "/thing/event/";

/*
//设备标签上报topic。参数：{productKey}/{deviceName}
String TAG_TOPIC = "/sys/" + productKey + "/" + deviceName + "/thing/deviceinfo/update";
//设备标签上报reply的topic。参数：{productKey}/{deviceName}
String TAG_REPLY_TOPIC = "/sys/" + productKey + "/" + deviceName + "/thing/deviceinfo/update_reply";
//删除设备标签topic。参数：{productKey}/{deviceName}
String TAG_DELETE_TOPIC = "/sys/" + productKey + "/" + deviceName + "/thing/deviceinfo/delete";
//删除设备标签reply的topic。参数：{productKey}/{deviceName}
String TAG_DELETE_REPLY_TOPIC = "/sys/" + productKey + "/" + deviceName + "/thing/deviceinfo/delete_reply";
// 远程配置通用topic
String CONFIG_WILDCARD_TOPIC = "/sys/" + productKey + "/" + deviceName + "/thing/config/#";
// 远程配置topic 参数：{productKey}/{deviceName}
String CONFIG_TOPIC = "/sys/" + productKey + "/" + deviceName + "/thing/config/get";
// 远程配置reply topic 参数：{productKey}/{deviceName}
String CONFIG_REPLY_TOPIC = "/sys/" + productKey + "/" + deviceName + "/thing/config/get_reply";
// 远程配置订阅变化topic 参数：{productKey}/{deviceName}
String CONFIG_SUBSCRIBE_TOPIC = "/sys/" + productKey + "/" + deviceName + "/thing/config/push";
// 远程配置reply topic 参数：{productKey}/{deviceName}
String CONFIG_SUBSCRIBE_RESP_TOPIC = "/sys/" + productKey + "/" + deviceName + "/thing/config/push_reply";
// 设备影子订阅topic
String SHADOW_SUBSCRIBE_TOPIC = "/shadow/get/" + productKey + "/" + deviceName;
// 设备影子获取topic
String SHADOW_GET_TOPIC = "/shadow/update/" + productKey + "/" + deviceName;
// 设备影子更新topic
String SHADOW_POST_TOPIC = "/shadow/update/" + productKey + "/" + deviceName;
*/

/**点击了解更多**/
/**https://help.aliyun.com/document_detail/140507.html?spm=a2c4g.11186623.6.571.1b5a24dfCbOOFK**/
/********************************* 阿里云IoT 物联网云平台 ********************************/
#else
/*
 * mqtt_server : MQTT服务器域名/IP地址
 * clientID : 接入服务器的MQTT客户端ID
 * clientName ：接入服务器的用户
 * clientPwd ：接入服务器的密码
 */
const char *mqtt_server = "47.95.249.141";
const char *clientID = "mqtt-31902342";
const char *clientName = "ESP32";
const char *clientPwd = "public";
#endif

/*定义线程句柄*/
TaskHandle_t ThreadWiFi;    //WIFI线程
TaskHandle_t ThreadMQTT;    //MQTT线程
TaskHandle_t ThreadSensor;  //Sensor线程
TaskHandle_t ThreadWeather; //Weather线程
TaskHandle_t ThreadOled;    //OLED线程
TaskHandle_t ThreadIDLE;    //空闲线程

/*定义定时器句柄*/
TimerHandle_t TimeAlarmLED;

/*定义信号量*/
SemaphoreHandle_t sem_WIFI;   //信号量用于WIFI线程
SemaphoreHandle_t sem_MQTT;   //信号量用于MQTT
SemaphoreHandle_t sem_Sensor; //信号量传感器用于上传数据
SemaphoreHandle_t sem_HTTP;   //信号量HTTP
SemaphoreHandle_t sem_Oled;   //信号量HTTP

/*线程入口声明*/
void ThreadWiFiEntry(void *pvParameters);    //WIFI线程入口
void ThreadMqttEntry(void *pvParameters);    //MQTT线程入口
void ThreadSensorEntry(void *pvParameters);  //Sensor线程入口
void ThreadWeatherEntry(void *pvParameters); //HTTP线程入口
void ThreadOledEntry(void *pvParameters);    //OLED线程入口
void ThreadIdleEntry(void *pvParameters);    //空闲线程入口

/*定时器入口声明*/
void TIMAlarmLEDEntry(TimerHandle_t xTimer); //LED警告定时器入口

/*函数声明*/
void TaskAPPStart();                                                       // 应用任务启动函数
void wifi_connect();                                                       // WiFi连接
void webwifi();                                                            // web服务器配置wifi
void webmain(AsyncWebServerRequest *request);                              // web服务器监听配网主页面
void webcheck(AsyncWebServerRequest *request);                             // web服务器监听用户请求页面
void AliyunIoT(String productKey, String deviceName, String deviceSecret); // Aliyun IoT 连接
void mqtt_connect();                                                       // MQTT私有云平台连接
void callback(char *topics, byte *payload, unsigned int length);           // MQTT统一监听订阅主题回调函数
String hmac256(const String signcontent, const String deviceSecret);       // hmac256加密运算
void FileSystem();                                                         // 文件系统
void OLEDInit();                                                           // OLED初始化
void OLEDprint(int x, int y, String text);                                 // OLED打印
void SensorInit();                                                         // 传感器初始化
void Weather(String icon);                                                 // 显示天气icon
void BlinkerInit();                                                        // Blinker初始化
void miotQuery(int32_t queryCode);                                         // 小米小爱查询返回接口
void dataRead(const String &data);                                         // Blinker数据绑定回调函数
void drawScrollString(int16_t offset, const char *s);                      //
/*=========================== 用户自定义全局变量&宏定义 =============================*/
#define LED 4  //GPIO4 电源指示灯
#define Bee 13 //有源蜂鸣器
// width: 128, height: 20
// U8X8_PROGMEM
const unsigned char col[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// width: 128, height: 32
//const unsigned char col_clear[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/*=========================== 用户自定义全局变量&宏定义 =============================*/

/*====================================== 初始化 =========================================*/
/*
 * @brief:初始化 
 * @param:none
 * @retval:none
*/
void setup()
{
  Serial.begin(115200); //设置串口波特率
  /*====================用户初始化=======================*/
  /**如 IO口模式初始化**/
  pinMode(LED, OUTPUT);

  /*====================用户初始化=======================*/
  OLEDInit();     //OLED初始化
  FileSystem();   //文件系统创建
  SensorInit();   //传感器初始化
  BlinkerInit();  //Blinker初始化
  TaskAPPStart(); //任务启动
}

/*===================================== 循环执行 不再使用 =================================*/
/*
 * @brief:需要循环执行的代码,上操作系统后,其内代码不再使用 
 * @param:none
 * @retval:none
*/
void loop()
{
  /**
   * 不放程序,所有代码放在线程任务中进行 
   */
}

/*======================================= TaskAPPStart ====================================*/
/*
 * @brief: 启动创建任务
 * @param:none
 * @retval:none
*/
void TaskAPPStart()
{
  sem_WIFI = xSemaphoreCreateCounting(1, 0);
  if (sem_WIFI == NULL)
  {
    Serial.println("WIFI信号量创建失败......");
  }
  else
  {
    Serial.println("WIFI信号量创建成功......");
  }
  sem_MQTT = xSemaphoreCreateCounting(1, 0);
  if (sem_MQTT == NULL)
  {
    Serial.println("MQTT信号量创建失败......");
  }
  else
  {
    Serial.println("MQTT信号量创建成功......");
  }
  sem_Sensor = xSemaphoreCreateCounting(1, 0);
  if (sem_Sensor == NULL)
  {
    Serial.println("Sensor信号量创建失败......");
  }
  else
  {
    Serial.println("Sensor信号量创建成功......");
  }
  sem_HTTP = xSemaphoreCreateCounting(1, 0);
  if (sem_HTTP == NULL)
  {
    Serial.println("HTTP信号量创建失败......");
  }
  else
  {
    Serial.println("HTTP信号量创建成功......");
  }
  sem_Oled = xSemaphoreCreateCounting(1, 0);
  if (sem_Oled == NULL)
  {
    Serial.println("Oled信号量创建失败......");
  }
  else
  {
    Serial.println("Oled信号量创建成功......");
  }
  /*创建WiFi线程*/
  BaseType_t status;
  status = xTaskCreatePinnedToCore(
      (TaskFunction_t)ThreadWiFiEntry,   //WIFI线程入口
      (const char *const) "Thread_WIFI", //线程名称
      (const uint32_t)2048,              //线程栈
      (void *const)NULL,                 //WIFI线程入口参数
      (UBaseType_t)15,                   //线程优先级 0-24 数值越大优先级越高
      (TaskHandle_t *)&ThreadWiFi,       //线程句柄
      (const BaseType_t)APP_CPU_NUM);    //指定内核1
  if (status == pdPASS)
  {
    Serial.println("WiFi线程创建成功...");
  }
  else
  {
    Serial.println("WiFi线程创建失败...");
  }
  /*创建MQTT线程*/
  status = xTaskCreatePinnedToCore(
      (TaskFunction_t)ThreadMqttEntry,   //MQTT线程入口
      (const char *const) "Thread_MQTT", //线程名称
      (const uint32_t)8192,              //线程栈
      (void *const)NULL,                 //MQTT线程入口参数
      (UBaseType_t)13,                   //线程优先级 0-24 数值越大优先级越高
      (TaskHandle_t *)&ThreadMQTT,       //线程句柄
      (const BaseType_t)APP_CPU_NUM);    //指定内核1
  if (status == pdPASS)
  {
    Serial.println("MQTT线程创建成功...");
  }
  else
  {
    Serial.println("MQTT线程创建失败...");
  }
  /*创建Sensor线程*/
  status = xTaskCreatePinnedToCore(
      (TaskFunction_t)ThreadSensorEntry,   //Sensor线程入口
      (const char *const) "Thread_Sensor", //线程名称
      (const uint32_t)2048,                //线程栈
      (void *const)NULL,                   //Sensor线程入口参数
      (UBaseType_t)10,                     //线程优先级 0-24 数值越大优先级越高
      (TaskHandle_t *)&ThreadSensor,       //线程句柄
      (const BaseType_t)PRO_CPU_NUM);      //指定内核1
  if (status == pdPASS)
  {
    Serial.println("Sensor线程创建成功...");
  }
  else
  {
    Serial.println("Sensor线程创建失败...");
  }
  /*创建Weather线程*/
  status = xTaskCreatePinnedToCore(
      (TaskFunction_t)ThreadWeatherEntry, //Weather线程入口
      (const char *const) "Thread_HTTP",  //线程名称
      (const uint32_t)4096 * 2,           //线程栈
      (void *const)NULL,                  //Weather线程入口参数
      (UBaseType_t)8,                     //线程优先级 0-24 数值越大优先级越高
      (TaskHandle_t *)&ThreadWeather,     //线程句柄
      (const BaseType_t)APP_CPU_NUM);     //指定内核1
  if (status == pdPASS)
  {
    Serial.println("HTTP线程创建成功...");
  }
  else
  {
    Serial.println("HTTP线程创建失败...");
  }
  /*创建OLED线程*/
  status = xTaskCreatePinnedToCore(
      (TaskFunction_t)ThreadOledEntry,   //OLED线程
      (const char *const) "Thread_Oled", //线程名称
      (const uint32_t)4096 * 2,          //线程栈
      (void *const)NULL,                 //OLED线程入口参数
      (UBaseType_t)6,                    //线程优先级 0-24 数值越大优先级越高
      (TaskHandle_t *)&ThreadOled,       //线程句柄
      (const BaseType_t)APP_CPU_NUM);    //指定内核1
  if (status == pdPASS)
  {
    Serial.println("Oled线程创建成功...");
  }
  else
  {
    Serial.println("Oled线程创建失败...");
  }

  /*创建空闲线程*/
  xTaskCreatePinnedToCore(
      (TaskFunction_t)ThreadIdleEntry,   //IDLE线程入口
      (const char *const) "Thread_IDLE", //线程名称
      (const uint32_t)1024,              //线程栈
      (void *const)NULL,                 //IDLE线程入口参数
      (UBaseType_t)1,                    //线程优先级 0-24 数值越大优先级越高
      (TaskHandle_t *)&ThreadIDLE,       //线程句柄
      (const BaseType_t)PRO_CPU_NUM);    //指定内核0

  /*连接网络提示定时器*/
  TimeAlarmLED = xTimerCreate(
      "TIM_Alarm_LED",    //定时器名称
      pdMS_TO_TICKS(100), //定时器定时时间
      pdTRUE,             //pdTRUE = 周期性 , pdFALSE = 一次性
      0,                  //定时器ID
      TIMAlarmLEDEntry);  //定时器回调函数

  u8g2.clearBuffer();
  OLEDprint(35, 32, "App Start");
  xTimerStart(TimeAlarmLED, 0);
  xSemaphoreGive(sem_WIFI);
  delay(1000);
}

/*======================================= 线程 定时器 入口 ====================================*/
/*
 * @brief:WIFI线程入口
 * @param:none
 * @retval:none
*/
void ThreadWiFiEntry(void *pvParameters)
{
  /*无限等待WIFI信号量*/
  xSemaphoreTake(sem_WIFI, portMAX_DELAY);
  /*删除主线程*/
  //vTaskDelete(ThreadMain);

  Serial.println("===WiFi连接任务启动===");
  wifi_connect(); //wifi连接
  while (1)
  {
    /* code */
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

/*
 * @brief:MQTT线程入口
 * @param:none
 * @retval:none
*/
void ThreadMqttEntry(void *pvParameters)
{
  /*无限等待MQTT信号量*/
  xSemaphoreTake(sem_MQTT, portMAX_DELAY);
  /*挂起WIFI线程*/
  //vTaskSuspend(ThreadWiFi);
  Serial.println("===MQTT连接任务启动===");
  /*根据宏定义,判别 Aliyun 或  私有云 */
#ifdef Aliyun_IoT
  AliyunIoT(productKey, deviceName, deviceSecret); //Aliyun 连接
#else
  mqtt_connect(); //mqtt连接
#endif
  /**/
  //vTaskResume(ThreadWiFi);
  // vTaskDelete(ThreadWiFi);
  while (1)
  {
    if (!mqttClient.connected() || !WiFi.isConnected())
    {
      /*检验MQTT和WIFI是否断连,断连芯片重启重新配网*/
      vTaskSuspendAll();
      u8g2.clearBuffer();
      Serial.println("WIFI or MQTT has been disconnected");
      OLEDprint(20, 32, "WIFI or MQTT");
      OLEDprint(0, 48, "has been disconnected");
      u8g2.sendBuffer();
      delay(1500);
      OLEDprint(0, 64, "ESP32 will restart");
      u8g2.sendBuffer();
      delay(2500);
      ESP.restart();
    }
    mqttClient.loop(); //MQTT客户端保活
    Blinker.run();     //Blinker客户端保活
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/*
 * @brief:Sensor线程入口
 * @param:none
 * @retval:none
*/
void ThreadSensorEntry(void *pvParameters)
{
  /*等待获取传感器信号量*/
  xSemaphoreTake(sem_Sensor, portMAX_DELAY);

  while (1)
  {
    temperature_bme = bme.readTemperature();
    humidity_bme = bme.readHumidity();
    pressure_bme = bme.readPressure();

    if(analogRead(34) > 1000)
    {
      digitalWrite(Bee,HIGH);
    }
    else
    {
      digitalWrite(Bee,LOW);
    }
    String payload; //存放MQTT发送消息的变量
    doc.clear();    //清空原有存放JSON数据和释放内存
    /* Aliyun消息体格式 */
    doc["id"] = esp_random();                                         //每个消息都有自己的ID
    doc["version"] = "1.0";                                           //消息的版本
    JsonObject params = doc.createNestedObject("params");             //JSON数据套娃
    params["carbon"] = analogRead(34);                                //烟雾浓度
    params["humidity"] = bme.readHumidity();                          //BME280湿度
    params["altitude"] = bme.readAltitude(1013.0);                    //BME280高度(需要设置当前海平面高度)
    params["airPressure"] = bme.readPressure() / 100.0;               //BME280气压
    params["temperature"] = bme.readTemperature();                    //BEM280温度
    doc["method"] = POST_PROPERY;                                     //消息的方法(根据阿里云Alinker协议)
    serializeJson(doc, payload);                                      //封装JSON数据并且放入payload里面
    Serial.println(payload);                                          //串口查看
    mqttClient.publish(PROPERTY_POST_TOPIC.c_str(), payload.c_str()); //向阿里云进行属性数据上报
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

/*
 * @brief:HTTP线程入口
 * @param:none
 * @retval:none
*/
void ThreadWeatherEntry(void *pvParameters)
{
  xSemaphoreTake(sem_HTTP, portMAX_DELAY);

  while (1)
  {
    /*GET请求*/
    //Serial.println("===================== GET =====================");
    // espHttp.begin(httpBaseURL + "/get");
    espHttp.begin(serverBath);
    // espHttp.begin("http://47.95.249.141:1880/api/test");
    httpResponseCode = espHttp.GET();
    if (httpResponseCode > 0)
    {
      // Deserialize the JSON document
      doc.clear();
      Serial.println(httpResponseCode);
      deserializeJson(doc, espHttp.getString());
      week = doc["week"];
      city = doc["city"];
      wea = doc["wea"];
      wea_img = doc["wea_img"];
      tem = doc["tem"];
      tem1 = doc["tem1"];
      tem2 = doc["tem2"];
      humidity = doc["humidity"];
      pressure = doc["pressure"];
      win = doc["win"];
      win_speed = doc["win_speed"];
      air_pm25 = doc["air_pm25"];
      air_level = doc["air_level"];
      String time = doc["date"];
      date = time;
      vTaskDelay(pdMS_TO_TICKS(1000));
      //打印测试
      // Serial.println("-------API获取数据打印------");
      // Serial.print("日期："); // "日期：2020-12-21"
      // Serial.println(date);
      // Serial.print("星期："); // "星期：星期一"
      // Serial.println(week);
      // Serial.print("所在城市："); // "所在城市：杭州"
      // Serial.println(city);
      // Serial.print("天气："); // "天气：晴"
      // Serial.println(wea);
      // Serial.print("天气标志："); // "天气：晴"
      // Serial.println(wea_img);
      // Serial.print("当前气温："); // "当前气温：9"
      // Serial.println(tem);
      // Serial.print("最高温："); // "最高温：8"
      // Serial.println(tem1);
      // Serial.print("最低温："); // "最低温：0"
      // Serial.println(tem2);
      // Serial.print("当前湿度："); // "当前湿度：32%"
      // Serial.println(humidity);
      // Serial.print("当前气压："); // "当前气压：1023"
      // Serial.println(pressure);
      // Serial.print("风向："); // "风向：东风"
      // Serial.println(win);
      // Serial.print("风速："); // "风速：2级"
      // Serial.println(win_speed);
      // Serial.print("PM2.5浓度："); // "PM2.5浓度：80"
      // Serial.println(air_pm25);
      // Serial.print("空气质量："); // "空气质量：良"
      // Serial.println(air_level);
      // Serial.println("-------------  -------------");
      // Serial.println("");
      espHttp.end();
    }
    //Serial.println("===================== GET =====================");
    /*POST请求*/
    // Serial.println("===================== POST =====================");
    //espHttp.begin(httpBaseURL + "/sensor?");
    //espHttp.begin("http://192.168.1.158:8888/v1/info");
    espHttp.begin("http://47.95.249.141:1880/sensor");
    espHttp.addHeader("Content-Type", "application/json");
    doc.clear();
    doc["stuNum"] = "31902342";
    doc["temperature"] = bme.readTemperature();
    doc["humidity"] = bme.readHumidity();
    doc["airpressure"] = bme.readPressure() / 100.0;
    String params;
    serializeJson(doc, params);
    espHttp.addHeader("Content-Type", "application/json");
    httpResponseCode = espHttp.POST(params);
    // Serial.println(params);
    // espHttp.addHeader("Content-Type", "application/x-www-form-urlencoded");
    //httpResponseCode = espHttp.POST("api_key=tPmAT5Ab3j7F9&sensor=BME280&value1=24.25&value2=49.54&value3=1005.14");

    //espHttp.addHeader("Content-Type", "application/json");
    //httpResponseCode = espHttp.POST("{\"api_key\":\"tPmAT5Ab3j7F9\",\"sensor\":\"BME280\",\"value1\":\"24.25\",\"value2\":\"49.54\",\"value3\":\"1005.14\"}");
    if (httpResponseCode > 0)
    {
      Serial.println(espHttp.getString());
    }
    espHttp.end();
    // Serial.println("===================== POST =====================");
    vTaskDelay(pdMS_TO_TICKS(1000 * 60 * 60));
  }
}

/*
 * @brief:Oled线程入口
 * @param:none
 * @retval:none
*/
void ThreadOledEntry(void *pvParameters)
{
  xSemaphoreTake(sem_Oled, portMAX_DELAY);
  while (1)
  {
    u8g2.clearBuffer();                                //清屏
    u8g2.setFont(u8g2_font_open_iconic_embedded_2x_t); //设置字库 icon
    u8g2.drawGlyph(111, 16, 64);                       //显示电池
    u8g2.drawGlyph(0, 16, 80);                         //显示WIFI
    u8g2.setFont(u8g2_font_open_iconic_weather_4x_t);  //设置天气状态 icon
    //u8g2.drawGlyph(10, 50, 69);                        //显示天气icon
    u8g2.setFont(u8g2_font_open_iconic_weather_4x_t);

    if (String(wea_img).equals("qing"))
    {
      u8g2.drawGlyph(10, 50, 69); //显示晴天
    }
    else if (String(wea_img).equals("yu") || String(wea_img).equals("lei"))
    {
      u8g2.drawGlyph(10, 50, 67); //显示雨天
    }
    else if (String(wea_img).equals("yun"))
    {
      u8g2.drawGlyph(10, 50, 65); //显示多云
    }
    else
    {
      u8g2.drawGlyph(10, 50, 64); //显示其它
    }

    u8g2.setFont(u8g2_font_logisoso20_tf);
    u8g2.setCursor(48 + 3, 48);
    float temp = bme.readTemperature();
    u8g2.printf("%.1f", temp);
    u8g2.print("°C");
    u8g2.setFont(u8g2_font_wqy13_t_gb2312);
    u8g2.sendBuffer(); //显示在屏幕上面
    vTaskDelay(pdMS_TO_TICKS(3000));

    /*********************************OLED显示温湿度、气压--Page1*********************************/
    /*固定图标：WIFI和电池图标*/                       //length 128 (0-128)  width 64(0-64)
    u8g2.clearBuffer();                                //清除缓冲区
    u8g2.setFont(u8g2_font_open_iconic_embedded_2x_t); //设置中文或字母表或图标的字体
    // u8g2.setFont(u8g2_font_open_iconic_embedded_1x_t); //图标太小
    u8g2.drawGlyph(111, 16, 64); //绘制字符集的符号，显示电池
    u8g2.drawGlyph(0, 16, 80);   //绘制字符集的符号，显示WIFI
    u8g2.setFont(u8g2_font_wqy12_t_gb2312a);

    u8g2.setCursor(10, 30); //OLED屏--Set the x,y coordinates
    u8g2.print("当前气温:");
    u8g2.printf("%.2f", temperature_bme);
    u8g2.print("°C");

    u8g2.setCursor(10, 45);
    u8g2.print("当前湿度:");
    u8g2.printf("%.2f", humidity_bme);
    u8g2.print("%");

    u8g2.setCursor(10, 60);
    u8g2.print("当前气压:");
    u8g2.printf("%.2f", pressure_bme);
    u8g2.print("hPa");
    u8g2.sendBuffer(); //绘制缓冲区内容
    Serial.println();
    vTaskDelay(pdMS_TO_TICKS(3000));
    /*****************************************************************************/

    /************每10秒请求获取一次数据--API数据获取 + OLED-Page2、3显示************/
    /*设置URL链接 Send an HTTP GET request*/ /*天气API获取数据*/

    /*********************************OLED显示API数据部分--Page2*********************************/

    u8g2.clearBuffer();                                //清空缓存
    u8g2.setFont(u8g2_font_open_iconic_embedded_2x_t); //设置中文或字母表或图标的字体
    u8g2.drawGlyph(111, 16, 64);                       //绘制字符集的符号，显示电池
    u8g2.drawGlyph(0, 16, 80);                         //绘制字符集的符号，显示WIFI
    // u8g2.setFont(u8g2_font_open_iconic_weather_4x_t);
    // u8g2.drawGlyph(10, 55, 69);
    Weather(wea_img); //wea_img取自JSON解析的数据变量
    // u8g2.setFont(u8g2_font_wqy12_t_gb2312a);
    // u8g2.setCursor(30,16);
    // u8g2.print(date.c_str());
    u8g2.setFont(u8g2_font_logisoso20_tf);
    u8g2.setFont(u8g2_font_wqy13_t_gb2312);
    u8g2.setCursor(43, 34);
    u8g2.print("室外天气:");
    u8g2.print(tem);
    u8g2.print("°C");
    u8g2.setFont(u8g2_font_wqy12_t_gb2312a);
    // u8g2.setFont(u8g2_font_wqy13_t_gb2312);
    // u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
    u8g2.setCursor(43, 52);
    u8g2.print("今日:");
    u8g2.print(tem2);
    // u8g2.print("%d",(int)tem);
    u8g2.print("°C");
    u8g2.print("~");
    u8g2.print(tem1);
    u8g2.print("°C");
    u8g2.sendBuffer(); //绘制缓冲区内容
    vTaskDelay(pdMS_TO_TICKS(3000));
    // /*********************************OLED显示API数据部分--Page3*********************************/
    u8g2.clearBuffer();                                //清除缓冲区
    u8g2.setFont(u8g2_font_open_iconic_embedded_2x_t); //设置中文或字母表或图标的字体
    // u8g2.setFont(u8g2_font_open_iconic_embedded_1x_t); //图标太小
    u8g2.drawGlyph(111, 16, 64); //绘制字符集的符号，显示电池
    u8g2.drawGlyph(0, 16, 80);   //绘制字符集的符号，显示WIFI
    u8g2.setFont(u8g2_font_wqy13_t_gb2312);
    u8g2.setCursor(32, 14);
    u8g2.print(date);
    u8g2.setFont(u8g2_font_wqy12_t_gb2312a);
    u8g2.setCursor(20, 28); //OLED屏--Set the x,y coordinates
    u8g2.print("湿度: ");
    u8g2.printf(humidity);
    u8g2.print("%");
    u8g2.setCursor(20, 44);
    u8g2.print("气压: ");
    u8g2.printf(pressure);
    u8g2.print("hPa");
    u8g2.setCursor(20, 60);
    u8g2.print("空气质量: ");
    u8g2.printf(air_level);
    u8g2.sendBuffer(); //绘制缓冲区内容

    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

/*
 * @brief:空闲线程入口
 * @param:none
 * @retval:none
*/
void ThreadIdleEntry(void *pvParameters)
{
  while (1)
  {
    //Serial.println("This is idel thread , nothing to do ...");

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

/*
 * @brief:空闲线程入口
 * @param:none
 * @retval:none
*/
void TIMAlarmLEDEntry(TimerHandle_t xTimer)
{
  digitalWrite(LED, !digitalRead(LED));
}
/*======================================= 线程 定时器 入口 ====================================*/

/*======================================== WIFI =========================================*/
/*
 * @brief:连接WiFi 
 * @param:none
 * @retval:none
*/
void wifi_connect()
{
  //设置wifi station模式
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  /*判断是否存在wifi.txt文件*/
  if (SPIFFS.exists("/root/wifi.txt"))
  {
    Serial.println("find /root/wifi.txt");
    /*以只读的方式打开wifi.txt文件*/
    file = SPIFFS.open("/root/wifi.txt", FILE_READ);
    /*定义临时变量存储wifi.txt的内容*/
    String Temp = file.readString();
    /*关闭文件*/
    file.close();
    /*对字符串解包生成所需要的json数据*/
    DeserializationError error = deserializeJson(doc, Temp);
    if (error)
    {
      /*格式化失败*/
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      ESP.restart(); //重启
    }
    else
    {
      /*格式化成功*/
      /*取出变量ssid & password的值*/
      String temp_ssid = doc["ssid"];
      String temp_pwd = doc["password"];
      /*赋值给 SSID & PASSWORD */
      SSID = temp_ssid;
      PASSWORD = temp_pwd;
      //Serial.println("ssid: " + SSID);
      //Serial.println("password: " + PASSWORD);
    }
  }
  /*WIFI连接*/
  //WiFi.begin(SSID.c_str(), PASSWORD.c_str());
  /*Blinker 连接上 及确保WIFI连接上*/
  Blinker.begin(auth.c_str(), SSID.c_str(), PASSWORD.c_str());
  //判断连接状态
  int count = 0;
  // while (WiFi.status() != WL_CONNECTED)
  while(!Blinker.connected())
  {
    //vTaskDelay(pdMS_TO_TICKS(1000));
    delay(1000);
    if (count >= 5)
    {
      /*超时5s 结束程序*/
      break;
    }
    u8g2.clearBuffer();
    OLEDprint(0, 40, "WIFI is connecting ...");
    Serial.println("WIFI is connecting ...");
    count++;
  }

  if (WiFi.isConnected())
  {
    Serial.println("WIFI connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP().toString());
    u8g2.clearBuffer();
    OLEDprint(0, 32, "WIFI connected");
    OLEDprint(0, 48, "IP: " + WiFi.localIP().toString());
    u8g2.setFont(u8g2_font_open_iconic_embedded_2x_t);
    u8g2.drawGlyph(0, 16, 80);
    u8g2.sendBuffer();
    delay(1500);
    /*发送信号量MQTT,触发MQTT线程*/
    xSemaphoreGive(sem_MQTT);
  }
  else
  {
    /*进入Web配网*/
    Serial.println("Web配网.....");
    u8g2.clearBuffer();
    OLEDprint(0, 32, "WIFI is connecting ...");
    webwifi();
  }
}
/*
 * @brief:Web配网主页,是url="/"根目录下的监听回调函数 
 * @param:none
 * @retval:none
*/
void webmain(AsyncWebServerRequest *request)
{
  /*设置配网Web页面*/
  content = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>WIFI配置</title></head><body>";
  content += "<h1 align=\"center\">Connect device to your WiFi</h1>";
  content += "<br><form action=\"/wifi\"><table align=\"center\" frame=\"box\">";
  content += "<tr><td><input type=\"radio\" name=\"method\" value=\"Auto\" checked onclick=\"Auto_wifi()\">扫描WIFI</td>";
  content += "<td><input type=\"radio\" name=\"method\" value=\"Manual\" onclick=\"Manual_wifi()\">手动输入</td></tr>";
  content += "<tr id=\"scan\"><td align=\"right\">SSID:</td><td><select name=\"ssid\" style=\"width:200px\">";
  content += "<option selected>Select a Network</option>";
  /*获取热点数量*/
  int WIFI_number = WiFi.scanNetworks(false, false, false, 15);
  /*判断有没有扫描到周围热点*/
  if (WIFI_number != 0)
  {
    for (int i = 1; i < WIFI_number; i++)
    {
      /*配置Web表单热点所选项*/
      content += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</option>";
      delay(100);
    }
    content += "</select></td></tr>";
    content += "<tr id=\"Manual\"><td align=\"right\">SSID:</td><td><input name=\"ssid_manual\" style=\"width:200px\"></td></tr>";
    content += "<tr><td align=\"right\">Password:</td><td><input name=\"password\" style=\"width:200px\" ></td><tr>";
    content += "<tr><td></td><td align=\"right\"><button type=\"submit\" style=\"width:100px\">开始配置</button></td></tr></table></form></body>";
    content += "<script>document.getElementById(\"Manual\").style.display = \"none\";function Auto_wifi() {";
    content += "document.getElementById(\"scan\").style.display = \"table-row\";";
    content += "document.getElementById(\"Manual\").style.disable = true;";
    content += "document.getElementById(\"Manual\").style.display = \"none\";";
    content += "window.location.reload();}";
    content += "function Manual_wifi() {";
    content += "document.getElementById(\"scan\").style.display = \"none\";";
    content += "document.getElementById(\"scan\").style.disable = true;";
    content += "document.getElementById(\"Manual\").style.display = \"table-row\";";
    content += "}</script></html>";

    WiFi.scanDelete(); //清除扫描数据
    //Serial.println(content);
    /*Web服务器返回状态码200,以及配置WiFi页面*/
    request->send(200, "text/html", content);
  }
  else
  {
    content = "<h1 algin=\"center\">The server is busy, please refresh the page.</h1>";
    request->send(200, "text/html", content);
  }
}
/*
 * @brief:获取wifi表单数据连接wifi,是url="/wifi"的监听回调函数
 * @param:none
 * @retval:none
*/
void webcheck(AsyncWebServerRequest *request)
{
  /*获取ssid 以及 wifi 密码*/
  /*WIFI扫描配网*/
  if (request->arg("method") == "Auto")
  {
    SSID = request->arg("ssid");
    PASSWORD = request->arg("password");
    Serial.println("==========Auto===========");
  }
  /*WIFI手动配网*/
  if (request->arg("method") == "Manual")
  {
    SSID = request->arg("ssid_manual");
    PASSWORD = request->arg("password");
    Serial.println("==========manual===========");
  }
  char buffer[64];                                  //临时变量 存放json字符串
  doc["ssid"] = SSID;                               //添加成员ssid
  doc["password"] = PASSWORD;                       //添加成员password
  serializeJson(doc, buffer);                       //格式化JSON 转存到 buffer
  file = SPIFFS.open("/root/wifi.txt", FILE_WRITE); //以写的方式打开wifi.txt文件,每次覆盖之前,没有就创建
  file.write((uint8_t *)buffer, strlen(buffer));    //把buffer里面的写入到wifi.txt
  file.close();                                     //关闭文件

  /*Web服务器返回状态码200,提示配网和Web服务器结束*/
  request->send(200, "text/html", "<h1 align=\"center\">Successfully connected to wifi</h1><br><h1 align=\"center\">Web servers will be shut down</h1>");
  //vTaskDelay(pdMS_TO_TICKS(1000));
  /*关闭Web服务器,重新连接wifi*/
  server.end();
  delay(1000);
  /*重启ESP32*/
  ESP.restart();
}
/*
 * @brief:Web配网
 * @param:none
 * @retval:none
*/
void webwifi()
{
  /*设置wifi为 AP热点*/
  WiFi.mode(WIFI_AP);
  /*设置热点WIFI*/
  WiFi.softAP("ESP32配网", "");
  /*获取热点IP地址*/
  IPAddress myIP = WiFi.softAPIP();
  /*串口显示热点IP地址*/
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  /*设置服务器监听 url 以及 服务器回应(回调函数) */
  server.on("/", webmain);
  server.on("/wifi", webcheck);
  /*设置服务器端口 并且开启*/
  server.begin();
  u8g2.clearBuffer();
  OLEDprint(0, 32, "WebServer Start");
  OLEDprint(0, 50, "Visit the website");
  delay(2000);
  u8g2.clearBuffer();
  OLEDprint(0, 32, "Connect ESP32WIFI");
  OLEDprint(0, 50, "http://192.168.4.1");

  //while(true);
}

/*======================================== MQTT =========================================*/
/*
 * @brief:MQTT回调函数,用于获取订阅的消息
 * @param:topics 订阅的主题
 * @param:payload 订阅的主题的消息 注：byte 只能一个字节一个字节读
 * @param:length 消息大小
 * @retval:none
*/
void callback(char *topics, byte *payload, unsigned int length)
{
  String topic = topics; //Stirng方便处理
  /**打印消息**/
  // Serial.print("Message arrived [");
  // Serial.print(topics);
  // Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    // Serial.print((char)payload[i]);
  }
  Serial.println();

  /**对消息进行处理,用户自己完成**/
  /**===================== Example ====================**/
  /*
   * 可以对 topic 筛选  
  */

  if (topic == "/LED_ON")
  {
    digitalWrite(LED, LOW); //开灯
  }
  if (topic == "/LED_OFF")
  {
    digitalWrite(LED, HIGH); //关灯
  }
  /**===================== Example ====================**/
}
#ifdef Aliyun_IoT
/*====================================== Aliyun_IOT =========================================*/
/*
 * @brief:hmac256 加密 用于计算Aliyun接入密码
 * @param: signcontent 加密对象 拼接规则 ${"clientId" + deviceName + "deviceName" + deviceName + "productKey" + productKey }
 * @param: deviceSecret 设备密码用于加密   
 * @retval: Password 接入Aliyun的密码 
*/
String hmac256(const String signcontent, const String deviceSecret)
{
  byte hashCode[SHA256HMAC_SIZE];
  SHA256 sha256;

  const char *key = deviceSecret.c_str();
  size_t keySize = deviceSecret.length();

  sha256.resetHMAC(key, keySize);
  sha256.update((const byte *)signcontent.c_str(), signcontent.length());
  sha256.finalizeHMAC(key, keySize, hashCode, sizeof(hashCode));

  String Password = "";
  for (byte i = 0; i < SHA256HMAC_SIZE; ++i)
  {
    Password += "0123456789ABCDEF"[hashCode[i] >> 4];
    Password += "0123456789ABCDEF"[hashCode[i] & 0xf];
  }
  return Password;
}

/*
 * @brief:Aliyun_IoT 接入函数
 * @param: productKey 产品Key 
 * @param: deviceName 设备名
 * @param: deviceSecret 设备密码
 * @retval: none
*/
void AliyunIoT(String productKey, String deviceName, String deviceSecret)
{
  String signcontent;
  /*拼接服务器地址(只针对华东2,其它地区需要修改cn-shanghai)*/
  aliyun_mqtt_server = productKey + ".iot-as-mqtt.cn-shanghai.aliyuncs.com";
  /*拼接MQTT客户端ID(只采用TCP直连和hmacsha256加密方式)*/
  aliyun_clientID = deviceName + "|securemode=3,signmethod=hmacsha256|";
  /*拼接MQTT客户端名*/
  aliyun_clientName = deviceName + "&" + productKey;
  /*拼接MQTT客户端密码加密对象*/
  signcontent = "clientId" + deviceName + "deviceName" + deviceName + "productKey" + productKey;
  /*获取MQTT客户端密码*/
  aliyun_clientPwd = hmac256(signcontent, deviceSecret);
  /*设置mqtt服务器地址和端口*/
  mqttClient.setServer(aliyun_mqtt_server.c_str(), 1883);
  /*设置mqtt保持活跃时间 ms*/
  mqttClient.setKeepAlive(60);
  /*设置消息回调函数*/
  mqttClient.setCallback(callback);
  OLEDprint(0, 62, "Aliyun is connecting ...");
  u8g2.sendBuffer();
  int count = 0;
  while (!mqttClient.connect(aliyun_clientID.c_str(), aliyun_clientName.c_str(), aliyun_clientPwd.c_str()))
  {
    Serial.println("Aliyun_IoT is connecting ...");
    if (count == 1)
    {
      SPIFFS.format();
      break;
    }
    count++;
    delay(1000);
  }
  if (mqttClient.connected())
  {
    /*连接成功 关闭AlarmLED定时器*/
    xTimerStop(TimeAlarmLED, 0);
    /*LED常亮示意正常工作*/
    digitalWrite(LED, LOW);

    /* 订阅云端响应属性上报 */
    mqttClient.subscribe(PROPERTY_POST_REPLY_TOPIC.c_str(), 0);
    /* 订阅云端设备属性设置 */
    mqttClient.subscribe(ONSET_PROPS_TOPIC.c_str(), 0);

    /*O*/  
    Serial.println("Aliyun_IoT is connected");
    Serial.println("MQTT is connected");
    OLEDprint(0, 62, "Aliyun is connecting ...");
    u8g2.drawXBM(0, 54, 128, 20, col);
    OLEDprint(0, 62, "Aliyun connected");
    u8g2.sendBuffer();
    delay(1500);
    
    /* WIFI,MQTT 均连上之后 发送信号量 */
    xSemaphoreGive(sem_Sensor); //触发Sensor 传感器线程
    xSemaphoreGive(sem_HTTP);   //触发HTTP weather线程
    xSemaphoreGive(sem_Oled);   //触发OLED weather线程
  }
}
/*====================================== Aliyun_IOT =========================================*/
#else
/*
 * @brief:MQTT连接
 * @param:none
 * @retval:none
*/
void mqtt_connect()
{
  /*设置mqtt服务器地址和端口*/
  mqttClient.setServer(mqtt_server, 1883);
  /*设置mqtt保持活跃时间 ms*/
  mqttClient.setKeepAlive(60);
  /*设置消息回调函数*/
  mqttClient.setCallback(callback);
  /**
   * 连接服务器 
   * 
   * client.connect(ClientID,Name,Password)
   * 
   * ClientID : 接入服务器的MQTT客户端ID
   * Name ：接入服务器的用户
   * Password ：接入服务器的密码
   * 
   */
  OLEDprint(0, 62, "MQTT is connecting ...");
  u8g2.sendBuffer();
  int count = 0;
  while (!mqttClient.connect(clientID, clientName, clientPwd))
  {
    Serial.println("MQTT is connecting ...");
    if (count == 1)
    {
      SPIFFS.format();
      break;
    }
    count++;
    delay(1000);
  }
  if (mqttClient.connected())
  {
    /*连接成功 关闭AlarmLED定时器*/
    xTimerStop(TimeAlarmLED, 0);
    /*LED常亮示意正常工作*/
    digitalWrite(LED, LOW);
    Serial.println("MQTT is connected");

    u8g2.drawXBM(0, 54, 128, 20, col);
    OLEDprint(0, 62, "MQTT connected");
    u8g2.sendBuffer();
    /**
    * 订阅主题
    * 
    * client.subscribe(topic,qos)
    * 
    * topic ： 订阅的主题
    * qos : 服务质量,Broker 与 Client 之间消息通信关系. 0 or 1 or 2
    * 最多一次（0）只发不管收没收到
    * 最少一次（1）至少接受到一次
    * 只一次  （2）确保只接受一次,慢,安全最高  
    * 
    */
    mqttClient.subscribe("/LED_ON", 0);
    mqttClient.subscribe("/LED_OFF", 0);
    mqttClient.subscribe("/test", 0);
    /**
    * 发出消息
    * 
    * client.publish(topic,payload,retained)
    * 
    * topic ： 发布的主题
    * payload ： 消息体
    * retained ：保留消息 true or false
    */
    mqttClient.publish("/test", "... this is a test of mqtt ...", false);
    xSemaphoreGive(sem_Sensor);
    xSemaphoreGive(sem_HTTP);
  }
}
#endif

/*======================================== 文件系统 =========================================*/
/*
 * @brief:文件系统挂载
 * @param:none
 * @retval:none
*/
void FileSystem()
{
  //文件系统挂载
  if (SPIFFS.begin(true, "/root", 3))
  {
    Serial.println("The file system successfully mounts in /root");
    Serial.println(SPIFFS.totalBytes()); //文件系统总大小
    Serial.println(SPIFFS.usedBytes());  //文件系统已使用
    u8g2.clearBuffer();
    OLEDprint(0, 32, "The File System");
    OLEDprint(35, 50, "is initialized");
    delay(1500);
  }
}

/*======================================== OLED =========================================*/
/*
 * @brief: oled初始化
 * @param:none
 * @retval:none
*/
void OLEDInit()
{
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_open_iconic_embedded_2x_t);
  u8g2.drawGlyph(111, 16, 64); //显示电池
  u8g2.setFont(u8g2_font_Born2bSportyV2_tr);
  u8g2.setCursor(35, 32);
  u8g2.print("welcome");
  u8g2.setCursor(0, 60);
  u8g2.print("System in Initialization");
  u8g2.sendBuffer();
  delay(1500);
}

/*
 * @brief: 天气icon
 * @param: icon : 对应的icon的类型
 * @retval:none
*/
void Weather(String icon)
{
  //xue、lei、shachen、wu、bingbao、yun、yu、yin、qing
  u8g2.setFont(u8g2_font_open_iconic_weather_4x_t);
  if (icon.equals("qing"))
  {
    u8g2.drawGlyph(8, 55, 69); //显示晴天
  }
  else if (icon.equals("yu") || icon.equals("lei"))
  {
    u8g2.drawGlyph(8, 55, 67); //显示雨天
  }
  else if (icon.equals("yun"))
  {
    u8g2.drawGlyph(8, 55, 65); //显示多云
  }
  else
  {
    u8g2.drawGlyph(8, 55, 64); //显示其它
  }
}

/*
 * @brief: OLED文字显示
 * @param: text String 
 * @param: x 横坐标 像素 
 * @param: y 纵坐标 像素 
 * @retval:none
*/
void OLEDprint(int x, int y, String text)
{
  //u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_open_iconic_embedded_2x_t);
  u8g2.drawGlyph(111, 16, 64); //显示电池
  u8g2.setFont(u8g2_font_ncenB08_te);
  u8g2.setCursor(x, y);
  u8g2.print(text);
  u8g2.sendBuffer();
}

/*======================================== Sensor =========================================*/
/*
 * @brief: 传感器初始化
 * @param:none
 * @retval:none
*/
void SensorInit()
{

  int count = 0;
  /*蜂鸣器*/
  pinMode(Bee,OUTPUT);  
  /*bme280初始化*/
  while (!bme.begin())
  {
    Serial.println("Can't find it ");
    if (count == 3)
    {
      ESP.restart();
    }
    count++;
    delay(500);
  }
  dht11.begin();
}

/*======================================== Blinker =========================================*/
void miotPowerState(const String &state)
{
  BLINKER_LOG("need set power state: ", state);

  if (state == BLINKER_CMD_ON)
  {
    digitalWrite(LED, HIGH);

    BlinkerMIOT.powerState("on");
    BlinkerMIOT.print();
  }
  else if (state == BLINKER_CMD_OFF)
  {
    digitalWrite(LED, LOW);

    BlinkerMIOT.powerState("off");
    BlinkerMIOT.print();
  }
}

/*
 * @brief:Blinker 初始化
 * @param:none
 * @retval:none
*/
void BlinkerInit()
{
  BLINKER_DEBUG.stream(Serial);       //设置Blinker串口
  Blinker.attachData(dataRead);       //绑定回调函数
  BlinkerMIOT.attachQuery(miotQuery); //小米小爱查询回调绑定
  // BlinkerMIOT.attachPowerState(miotPowerState);
}

/*
 * @brief: 小米小爱查询
 * @param: queryCode小爱查询状态码,Blinker已实现
 * @retval:none
*/
void miotQuery(int32_t queryCode)
{
  BLINKER_LOG("MIOT Query codes: ", queryCode);
  Serial.printf("MIOT Query codes: %d", queryCode);
  switch (queryCode)
  {
  case BLINKER_CMD_QUERY_ALL_NUMBER:
    BLINKER_LOG("MIOT Query All");
    BlinkerMIOT.temp(temperature_bme); //小米小爱温度
    BlinkerMIOT.humi(humidity_bme);    //小米小爱湿度
    BlinkerMIOT.pm25(air_pm25);        //小米小爱PM2.5浓度
    BlinkerMIOT.co2(analogRead(34));
    BlinkerMIOT.print();
    break;
  default:
    BlinkerMIOT.temp(20);
    BlinkerMIOT.humi(20);
    BlinkerMIOT.pm25(20);
    BlinkerMIOT.co2(20);
    BlinkerMIOT.print();
    break;
    // case BLINKER_CMD_QUERY_ALL_NUMBER:
    //   // BLINKER_LOG("MIOT Query PM25");
    //   // BlinkerMIOT.pm25((int)air_pm25);         //小爱获取PM2.5浓度
    //   BlinkerMIOT.humi(bme.readHumidity());    //小爱获取温度
    //   BlinkerMIOT.temp(bme.readTemperature()); //小爱获取温度
    //   BlinkerMIOT.print();
    //   break;
    // // case BLINKER_CMD_QUERY_HUMI_NUMBER:
    // //   BLINKER_LOG("MIOT Query HUMI");
    // //   BlinkerMIOT.humi(bme.readHumidity());    //小爱获取温度
    // //   BlinkerMIOT.print();
    // //   break;
    // // case BLINKER_CMD_QUERY_TEMP_NUMBER:
    // //   BLINKER_LOG("MIOT Query TEMP");

    // //   BlinkerMIOT.print();
    // //   break;
    // // case BLINKER_CMD_QUERY_TIME_NUMBER:
    // //   BLINKER_LOG("MIOT Query Time");

    // //   BlinkerMIOT.print();
    // //   break;
    // default:
    //   Serial.println("MiIoT error!!!");
    //   BlinkerMIOT.print();
    //   break;
  }
}

/*
 * @brief: Blinker控制回调函数
 * @param: data 发送事件,Blinker已实现
 * @retval:none
*/
void dataRead(const String &data)
{
  BLINKER_LOG("Blinker readString: ", data);

  Blinker.vibrate();

  uint32_t BlinkerTime = millis();

  Blinker.print("millis", BlinkerTime);
}