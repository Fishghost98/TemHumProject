var mqtt = require('mqtt')
var mysql = require('mysql')
var connection = mysql.createConnection({
  host: '47.97.245.136',
  user: 'root',
  password: 'root',
  port: '3306',
  database: 'iot',
})
connection.connect()

const routes = require('./module/routes')

var mqttUrl = 'mqtt://47.97.245.136:61613'
var option = {
  username: 'admin',
  password: 'password',
  clientId: 'localNodeClient',
}

var app = require('http').createServer(handler),
  io = require('socket.io')(app),
  fs = require('fs')

function handler(req, res) {
  routes.static(req, res, 'static')
  res.writeHead(200, { 'Content-Type': 'text/html;charset="utf-8"' })
  res.end()
}

var client = mqtt.connect(mqttUrl, option)
//订阅的MQTT主题
client.subscribe('Mymqtt/module1/willMessage', { qos: 0 })    //设备一的状态信息
client.subscribe('Mymqtt/module1/DHT11', { qos: 0 })          //温湿度
client.subscribe('Mymqtt/module1/light', { qos: 0 })          //光照
client.subscribe('Mymqtt/module2/willMessage', { qos: 0 })    //设备二的状态信息
client.subscribe('Mymqtt/module2/led', { qos: 2 })            //灯的状态信息
console.log('订阅成功')

global.willMessage1 = '客户端离线'
global.willMessage2 = '客户端离线'
global.time = '2021.10.07'
global.light = '**'
global.json = '**'
var tem = new Array()
var hum = new Array()
var alldate = new Array()
var lasttime = new Date().getTime()

client.on('message', function (topic, message) {
  if (topic == 'Mymqtt/module1/willMessage') {
    willMessage1 = message.toString()
    console.log('主题：' + topic + '  消息：' + willMessage1)
  } else if (topic == 'Mymqtt/module2/willMessage') {
    willMessage2 = message.toString()
    console.log('主题：' + topic + '  消息：' + willMessage2)
    time = new Date().toLocaleString('chinese', { hour12: false })
  } else if (topic == 'Mymqtt/module1/DHT11') {
    json = message.toString()
    console.log('主题：' + topic + '  消息：' + json)
  }
})

io.on('connection', function (socket) {
  //向前端发送从数据库中获取到的最近10条温湿度数据
  socket.on('getMessage', function (data) {
    connection.query(
      'select * from dht11 order by id desc limit 10',
      function (err, results) {
        if (err) throw err
        console.log('======start=====')
  
        for (i = 0; i < 10; i++) {
          var temperature = results[i].temperature
          var humidity = results[i].humidity
          var date = results[i].date
          var cdate = RiQi(date)
          tem[i] = temperature
          hum[i] = humidity
          alldate[i] = cdate
        }
  
        var ctem = tem.reverse()
        var chum = hum.reverse()
        var cdate = alldate.reverse()
  
        data = ctem.toString() + ',' + chum.toString() + ',' + cdate.toString()
        console.log(data)
        console.log('======end=======')
        socket.emit('mychart', { msg: data })
        console.log('emit successfully!')
      }

    )
    //当有客户端连接时，向客户端发送设备状态
    socket.emit('module1', { msg: willMessage1 })
    socket.emit('dht11', { msg: json })
    socket.emit('time', { msg: time })
    
  })
 
   //监听MQTT消息
   client.on("message", function (topic, message) {
    if (topic == "Mymqtt/module1/willMessage") {
      nowtime = new Date().getTime();
      t = nowtime - lasttime;
      if (t > 3000) {
        willMessage1 = message.toString();
        time = new Date().toLocaleString("chinese", { hour12: false });
        console.log(time);
        console.log("主题：" + topic + "  消息：" + willMessage1);
        socket.emit("module1", { msg: willMessage1 });
        socket.emit("time", { msg: time });
      }
      lasttime = nowtime;
    } else if (topic == "Mymqtt/module2/willMessage") {
      willMessage2 = message.toString();
      console.log("主题：" + topic + "  消息：" + willMessage2);
      socket.emit("module2", { msg: willMessage2 });
    } else if (topic == "Mymqtt/module1/light") {
      light = message.toString();
      console.log("主题：" + topic + "  消息：" + light);
      socket.emit("light", { msg: light });
    } else if (topic == "Mymqtt/module1/DHT11") {
      json = message.toString();
      socket.emit("dht11", { msg: json });
      nowTime = new Date().toLocaleTimeString("chinese", { hour12: false });
      if (time != nowTime) {
        console.log("主题：" + topic + "  消息：" + json);
        //将温湿度信息插入到数据库中
        var json = JSON.parse(message.toString());
        var addDht11 = "INSERT INTO dht11(temperature,humidity) VALUES(?,?)";
        var temperature = parseInt(json.temperature);
        var humidity = parseInt(json.humidity);
        var addSqlParams = [temperature, humidity];

        connection.query(addDht11, addSqlParams, function (err, result) {
          if (err) {
            console.log("[INSERT ERROR] - ", err.message);
            return;
          }
          console.log("Insert successfully");
        });

        time = nowTime;
      }
    } else if (topic == "Mymqtt/module2/led") {
      socket.emit("LED", { msg: message.toString() });
    }
  });

  //前端灯控制页面打开时，像页面发送设备状态信息并开始让远程开发板上报自身状态
  socket.on('requireState', function (data) {
    console.log(data)
    var requireState = data.toString()
    var rTopic = 'Mymqtt/module2/requireState'
    socket.emit('module2', { msg: willMessage2 })
    client.publish(rTopic, requireState, { qos: 1, retain: false })
  })

  //当前端灯控制页面关闭时，停止上报灯的状态
  socket.on('disconnect', function () {
    var requireState = 'no'
    var rTopic = 'Mymqtt/module2/requireState'
    client.publish(rTopic, requireState, { qos: 1, retain: false })
  })

  //发送灯的控制指令
  socket.on('ledCMD', function (data) {
    console.log(data)
    var ledTopic = 'Mymqtt/module2/ledCMD'
    client.publish(ledTopic, data, { qos: 1, retain: false })
  })
})

//时间戳（UTC）转标准时间
function RiQi(sj) {
  var now = new Date(sj)
  var year = now.getFullYear()
  var month = now.getMonth() + 1
  var date = now.getDate()
  var hour = now.getHours()
  var minute = now.getMinutes()
  var second = now.getSeconds()
  return now.toLocaleTimeString('chinese', { hour12: false })
}

//启动HTTP服务，绑定端口3000
app.listen(3000, function () {
  console.log('listening on *:3000')
})
