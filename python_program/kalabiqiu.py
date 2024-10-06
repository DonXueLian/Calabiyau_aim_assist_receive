import time
import cv2
import numpy as np
import dxcam
import mss
import ctypes
import serial
import serial.tools.list_ports
import keyboard
import sys

# 自瞄区域
aim_area_width = 100     # 像素
aim_area_height = 100    # 像素
# 最大帧率
max_fps = 60
# 截图方式
capture_fun = "dxcam"   # dxcam, mss两种可选(默认mss)
# PD参数
kp = 33
kd = 0
# 游戏内灵敏度
game_mouse_speed = 1.12

# 选择端口号
def chose_serial():
    ports_list = list(serial.tools.list_ports.comports())
    if len(ports_list) <= 0:
        print("无串口设备。")
        return 0
    else:
        print("可用的串口设备如下：")
        number=0
        for comport in ports_list:
            number += 1
            print("\t", number, ":\t", list(comport)[0], list(comport)[1])
        chose_number = input("请选择第几个端口 (输入0可退出程序):")
        if chose_number == '0':
            print("程序退出")
            time.sleep(0.5)
            sys.exit(0)
        port_num = ports_list[int(chose_number)-1][0]
        print("选择", port_num)
        return port_num


# 选择截图工具
def chose_capture_fun():
    print("选择截图工具：")
    print("\t1 : dxcam", "\n", "\t2 : mss")
    chose_number = input("请选择截图工具：")
    if chose_number == "1":
        capture_fun = "dxcam"
    else:
        capture_fun = "mss"
    print("选择", capture_fun)
    return capture_fun


# 设定帧率
def set_capture_fps():
    capture_fps = input("输入目标帧率：")
    if int(capture_fps) >= 1:
        set_max_fps = int(capture_fps)
    else:
        set_max_fps = max_fps
    return set_max_fps


# 设置游戏内灵敏度比例
def set_mouse_speed(game_mouse_speed):
    mouse_speed = input("输入游戏内灵敏度：")
    if float(mouse_speed) > 0:
        game_mouse_speed = float(mouse_speed) / game_mouse_speed
    else:
        game_mouse_speed = 1
    print("灵敏度比率", game_mouse_speed)
    return game_mouse_speed


# 连接串口
def connect_serial(port_num):
    ser = serial.Serial(port_num, 115200)    # 打开串口，将波特率配置为115200，其余参数使用默认值
    if ser.isOpen():                         # 判断串口是否成功打开
        print("打开串口成功。")
        print(ser.name)                      # 输出串口号
        return ser
    else:
        print("打开串口失败，请确定串口号是否正确")
        return None


# 获取电脑分辨率
def get_screen_width_height():
    user32 = ctypes.windll.user32
    user32.SetProcessDPIAware()
    screen_width = user32.GetSystemMetrics(0)
    screen_height = user32.GetSystemMetrics(1)
    print("screen width:", screen_width, "screen height:", screen_height)
    return screen_width, screen_height


# 返回BGR格式的全屏截图
def get_full_screen(camera, monitor):
    # 使用dxcam截屏
    if capture_fun == "dxcam":
        return camera.get_latest_frame()
    # 使用mss截屏
    else:
        with mss.mss() as sct:
            img = np.array(sct.grab(monitor))
        return img


# 初始化截图
def capture_tool_init(screen_width, screen_height):
    camera = None
    monitor = None
    left, top = (screen_width - aim_area_width) // 2, (screen_height - aim_area_height) // 2
    right, bottom = left + aim_area_width, top + aim_area_height
    # dxcam init
    if capture_fun == "dxcam":
        camera = dxcam.create(output_color="BGRA")  # returns a DXCamera instance on primary monitor
        camera.start(region=(left, top, right, bottom), target_fps=max_fps, video_mode=True)  # Optional argument to capture a region
        camera.is_capturing  # True
    # mss init
    elif capture_fun == "mss":
        monitor = {"top": top, "left": left, "width": aim_area_width, "height": aim_area_height}
    return camera, monitor
    

# 从全屏截图中获取自瞄区域的图像，并转化为HSV
def get_aim_area_screen(screen_width, screen_height, img):
    # left = (screen_width - aim_area_width) // 2
    # top = (screen_height - aim_area_height) // 2
    # right = left + aim_area_width
    # bottom = top + aim_area_height
    # aim_area_img_BGR = img[top : bottom, left : right]
    # aim_area_img_HSV = cv2.cvtColor(aim_area_img_BGR, cv2.COLOR_BGR2HSV)

    aim_area_img_HSV = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
    return aim_area_img_HSV


# 紫色HSV范围
lower = np.array([147 - 10, 100, 100])
upper = np.array([147 + 10, 255, 255])
# 开闭运算子
kernel_close = np.ones((25, 25), np.uint8)
kernel_open = np.ones((5, 5), np.uint8)
# 获取紫色人物离屏幕中心的距离
# 返回人物离屏幕中心的偏差以及准星是否在人物身上
def get_bias_with_enemy(img_HSV):
    # 提取紫色
    img = cv2.inRange(img_HSV, lower, upper)
    # 开闭运算
    img = cv2.morphologyEx(img, cv2.MORPH_CLOSE, kernel_close)
    img = cv2.morphologyEx(img, cv2.MORPH_OPEN, kernel_open)
    # 获取人物轮廓
    contours, _ = cv2.findContours(img, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    # 判断哪个敌人被锁定
    max_value = 0
    target_x = target_y = target_w = target_h = -1000
    for contour in contours:
        x, y, w, h = cv2.boundingRect(contour)
        value = w + h
        if value > max_value:
            max_value = value
            target_x = x
            target_y = y
            target_w = w
            target_h = h
    # 获取人物离屏幕中心偏差
    if target_x != -1000:
        bias_x = (target_x + target_w // 2) - aim_area_width // 2
        bias_y = (target_y + target_h // 10) - aim_area_height // 2
        # 判断准星是否在人身上
        if target_x <= aim_area_width // 2 and target_x + target_w >= aim_area_width // 2 \
            and target_y <= aim_area_height // 2 and target_y + target_h >= aim_area_height // 2:
            is_fire = 1
        else:
            is_fire = 0
    else:
        bias_x = -1000
        bias_y = -1000
        is_fire = 0
    return bias_x, bias_y, is_fire


# 串口发送自瞄数据
def serial_send_aiming_data(x_bias, y_bias, is_fire, main_verb_wepon, is_open_scope):
    x_bias = np.int16(x_bias)
    y_bias = np.int16(y_bias)
    is_fire = np.int16(is_fire)
    main_verb_wepon = np.int16(main_verb_wepon)
    is_open_scope = np.int16(is_open_scope)
    
    data = f"A{x_bias},{y_bias},{is_fire},{main_verb_wepon},{is_open_scope}\n"
    encoded_data = data.encode('utf-8')
    ser.write(encoded_data)


# 串口发送参数数据
def serial_send_parameter(kp, kd):
    kp = np.int16(kp)
    kd = np.int16(kd)

    data = f"P{kp},{kd}\n"
    encoded_data = data.encode('utf-8')
    ser.write(encoded_data)


if __name__ == '__main__':
    # 连接串口
    port_num = chose_serial()
    ser = connect_serial(port_num)
    if ser is None:
        print("无法连接到串口，请检查端口号：")
        sys.exit(1)
    # 选择截图工具
    capture_fun = chose_capture_fun()
    # 设定帧率
    max_fps = set_capture_fps()
    # 设置游戏内灵敏度
    game_mouse_speed = set_mouse_speed(game_mouse_speed)
    serial_send_parameter(kp*game_mouse_speed, kd)
    # 初始化截图工具
    print("初始化截图工具...")
    screen_width, screen_height = get_screen_width_height()
    camera, monitor = capture_tool_init(screen_width, screen_height)
    time.sleep(0.1) # 等待初始化完成
    print("截图工具初始化完成，超级瞄准已部署")

    last_time = time.time()
    show_fps_msg_counter = 0    # 计数器，每几个数打印一次信息
    keyboard_press_time = 0     # 储存按键按下的时间，防止短时间多次触发
    while True:
        # 按键退出程序
        if keyboard.is_pressed('ctrl+shift+]'):
            break
        if keyboard.is_pressed(72):
            # 方向键上按下
            if time.time() - keyboard_press_time >= 0.4:
                keyboard_press_time = time.time()
                kp += 1
                print("kp:", kp, "\tkd:", kd)
                serial_send_parameter(kp*game_mouse_speed, kd)
        elif keyboard.is_pressed(80):
            # 方向键下按下
            if time.time() - keyboard_press_time >= 0.4:
                keyboard_press_time = time.time()
                kp -= 1
                print("kp:", kp, "\tkd:", kd)
                serial_send_parameter(kp*game_mouse_speed, kd)

        # 获取截图
        full_screen_img = get_full_screen(camera, monitor)
        aim_area_img = get_aim_area_screen(screen_width, screen_height, full_screen_img)
        # 提取敌人信息
        x_bias, y_bias, is_fire = get_bias_with_enemy(aim_area_img)
        # 发送信息
        serial_send_aiming_data(x_bias, y_bias, is_fire, 0, 0)

        # 限制帧率以及显示帧率
        now_time = time.time()
        spend_time = now_time - last_time
        if(spend_time < 1 / max_fps):
            time.sleep(1/max_fps - spend_time)
            now_time = time.time()
            spend_time = now_time - last_time
        last_time = now_time
        show_fps_msg_counter += 1
        if show_fps_msg_counter >= 1/spend_time:  # 一秒执行1次
            print("spend time: %.3f s, frequency %d Hz" % (spend_time, 1/spend_time))
            show_fps_msg_counter = 0
    
    # 程序结束
    print("程序退出")
    if capture_fun == "dxcam":
        camera.stop()
        camera.is_capturing  # False

