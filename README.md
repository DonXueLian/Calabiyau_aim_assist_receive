## 功能

可以实现射击游戏的自瞄或者辅助瞄准，原理是电脑找色，发送数据给esp32，再由esp32模拟鼠标实现自瞄

**使用两块esp32的原因(防反作弊)**：有的反作弊程序会检测是否有两个鼠标设备同时插入电脑，所以先由一块esp32解析鼠标数据，再由另一块esp32还原鼠标数据并融合进自瞄数据，这样电脑只需要插入一个鼠标设备，就避免了被反作弊检测到的可能性

理论上所有可以找色的射击游戏都可以使用，仅在卡拉比丘和瓦罗兰特进行了测试

实现原理：通过esp32接收鼠标数据，并通过spi发送给另一块esp32，由另一块esp32模拟鼠标还原接收到的鼠标数据并插入自瞄数据

## 硬件准备

两个esp32s3（一个作为发送端，烧入CalabiYau_aim_assist_send下程序，一个作为接收端，烧入CalabiYau_aim_assist_receive下的程序）

一个usb母口

## 电路连接

### spi通信连线

将两个esp32s3的GPIO2 12 13 14 15 分别相连，无需上拉电阻

发送端esp32----->-------接收端esp32  
2------------->----------2  
12------------>---------12  
13------------>---------13  
15------------>---------15  
14------------>---------14

以下是spi的引脚定义（位于`components/spi_send/include/spi_send.h`），可以修改到其他引脚（发送端和r接收端需要同步修改，保持一致）

```
#define GPIO_HANDSHAKE      2
#define GPIO_MOSI           12
#define GPIO_MISO           13
#define GPIO_SCLK           15
#define GPIO_CS             14
```

### 电源

usb母口需要5V电源供电

## 编译上传

项目使用espidf编译

请先使用`idf.py menuconfig`修改ram和flash到对应的大小，测试使用的是8MB RAM  16MB flash (esp32s3N16R8)

将作为发送端的esp32通过串口连接到电脑，在linux中，进到本目录下`***/Calabiyau_aim_assist_receive`，然后运行`idf.py build flash`，即可编译并烧入

**需要发送端的esp32和接收端的esp32正确连接并分别烧入了对应的程序才可正常运行**

## 使用

使用两根usb数据线分别将接收端esp32的串口和usb连接到电脑

将鼠标插入到发送端esp32的usb接口上（鼠标推荐使用500hz回报率，最高支持1000Hz回报率）

电脑上运行kalabiqiu.py(位于此项目python_program/下) ，cmd中切换到kalabiqiu.py所在目录然后运行`python kalabiqiu.py`即可
