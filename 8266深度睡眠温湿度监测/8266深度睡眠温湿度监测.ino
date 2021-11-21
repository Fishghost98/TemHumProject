#include <SimpleDHT.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

WiFiClient wificlient;
PubSubClient mqttClient(wificlient);

bool WIFI_Status = true; //WIFI状态标志位
char auth[] = "ac8cb23e3b49";
int count = 0; //时间计数

//设置WiFi接入信息
const char *ssid = "your ssid";
const char *password = "your wifi password";
const char *mqttServer = "your mqtt server";

//MQTT服务器账号密码
const char *mqttId = "your mqtt id";
const char *mqttPwd = "your mqtt password";

//定义温湿度模块引脚及变量
int pinDHT11 = D2;
SimpleDHT11 dht11(pinDHT11);
byte temp_read = 0;
byte humi_read = 0;

// 遗嘱设置
const char *willMsg = "CLIENT-OFFLINE"; // 遗嘱消息内容
const int willQoS = 2;                  // 遗嘱QoS
const bool willRetain = true;           // 遗嘱保留

void setup() {
  
  //初始化端口
  Serial.begin(115200);

  //连接WiFi
  connectWiFi();

  //设置MQTT服务器和端口号
  mqttClient.setServer(mqttServer, 61613);
  mqttClient.setKeepAlive(5);
  //设置MQTT订阅回调函数
  mqttClient.setCallback(receiveCallback);

  //连接MQTT服务器
  connectMQTTServer();
  delay(2000);
  if (mqttClient.connected())
    { //如果开发板成功连接服务器
      getHumChk();   //获取温湿度传感器信息并通过MQTT发送
      mqttClient.loop(); //保持客户端心跳
    }
    else
    { //如果开发板未能成功连接服务器
      connectMQTTServer(); //则尝试连接服务器
    }
  
  
  Serial.println("deep sleep for 5 seconds");
  ESP.deepSleep(600e6);    //8266深度睡眠600秒后重启
  
  
}

void loop() {
  // put your main code here, to run repeatedly:
}

//连接MQTT服务器
void connectMQTTServer()
{
  //根据ESP8266的MAC地址生成客户端ID（避免与其他设备产生冲突）
  String clientId = "你的设备clientId";

  // 建立遗嘱主题。
  String willString = "Mymqtt/module1/willMessage";
  char willTopic[willString.length() + 1];
  strcpy(willTopic, willString.c_str());

  //连接MQTT服务器
  if (mqttClient.connect(clientId.c_str(), mqttId, mqttPwd, willTopic, willQoS, willRetain, willMsg))
  {
    Serial.println("MQTT Server Connected.");
    Serial.println("Server Address:");
    Serial.println(mqttServer);
    Serial.println("ClientId:");
    Serial.println(clientId);
    publishOnlineStatus(); //发布在线状态
  }
  else
  {
    Serial.println("MQTT Server Connect Failed.Client State: ");
    Serial.println(mqttClient.state());
    delay(2000);
  }
}

//订阅MQTT消息
void subscribeTopic()
{
  
}

// 收到信息后的回调函数
void receiveCallback(char *topic, byte *payload, unsigned int length)
{
  
}

// 发布在线信息
void publishOnlineStatus()
{
  // 建立遗嘱主题。
  String willString = "Mymqtt/module1/willMessage";
  char willTopic[willString.length() + 1];
  strcpy(willTopic, willString.c_str());

  // 建立设备在线的消息。此信息将以保留形式向遗嘱主题发布
  String onlineMessageString = "CLIENT-ONLINE";
  char onlineMsg[onlineMessageString.length() + 1];
  strcpy(onlineMsg, onlineMessageString.c_str());

  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temp_read, &humi_read, NULL)) != SimpleDHTErrSuccess)
  {
    Serial.print("Read DHT11 failed, err=");
    Serial.println(err);
    delay(1500);
    return;
  }

  // 向遗嘱主题发布设备在线消息
  if (mqttClient.publish(willTopic, onlineMsg, true))
  {
    Serial.print("Published Online Message: ");
    Serial.println(onlineMsg);
  }
  else
  {
    Serial.println("Online Message Publish Failed.");
  }
}

//发布MQTT信息
void pubMQTTmsg()
{

  //建立发布温湿度主题。主题名称为“DHT11”，位于“mymqtt”主题下一级
  String DHT11Topic = "Mymqtt/module1/DHT11";
  char publishDHT11Topic[DHT11Topic.length() + 1];
  strcpy(publishDHT11Topic, DHT11Topic.c_str());

  //建立发布温湿度信息。
  char json[100];
  String tem = (String)temp_read;
  String hum = (String)humi_read;
  String payload = "{";
  payload += "\"temperature\":";
  payload += tem;
  payload += ",";
  payload += "\"humidity\":";
  payload += hum;
  payload += "}";
  char attributes[100];
  payload.toCharArray(attributes, 100);
  //实现8266向主题发布信息
  if (mqttClient.publish(publishDHT11Topic, attributes,true))
  {
    //温湿度
    Serial.print("Publish DHT11Topic:");
    Serial.println(publishDHT11Topic);
    Serial.print("Publish DHT11Message:");
    Serial.println(attributes);
  }
  else
  {
    Serial.println("Message Publish Failed.");
  }
}

//温湿度检测
void getHumChk()
{
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temp_read, &humi_read, NULL)) != SimpleDHTErrSuccess)
  {
    Serial.print("Read DHT11 failed, err=");
    Serial.println(err);
    delay(1500);
    return;
  }
  Serial.print("hum:");
  Serial.print(humi_read);
  Serial.print("%");

  Serial.print("tem:");
  Serial.print(temp_read);
  Serial.println("*C");

  pubMQTTmsg();
}

/* 微信智能配网 */
void smartConfig()
{
  WiFi.mode(WIFI_STA);                           //设置STA模式
  Serial.println("\r\nWait for Smartconfig..."); //打印log信息
  WiFi.beginSmartConfig();                       //开始SmartConfig，等待手机端发出用户名和密码
  while (1)
  {
    Serial.println(".");
    digitalWrite(LED_BUILTIN, HIGH); //指示灯闪烁
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW); //指示灯闪烁
    delay(1000);
    if (WiFi.smartConfigDone()) //配网成功，接收到SSID和密码
    {
      Serial.println("SmartConfig Success");
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      break;
    }
  }
}

/*连接网络*/
void connectWiFi()
{
  delay(2000);
  WiFi.mode(WIFI_STA); //设置STA模式
  WiFi.begin(ssid, password);
  Serial.println("\r\n正在连接WIFI...");
  while (WiFi.status() != WL_CONNECTED) //判断是否连接WIFI成功
  {
    if (WIFI_Status)
    {
      Serial.print(".");
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
      count++;
      if (count >= 10)
      {
        WIFI_Status = false;
        Serial.println("WiFi连接失败，请用手机进行配网");
      }
    }
    else
    {
      smartConfig(); //微信智能配网
    }
  }
  Serial.println("连接成功");
  Serial.print("IP:");
  Serial.println(WiFi.localIP());
}
