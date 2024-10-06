#include "PD_controller.h"
#include "stdint.h"

/*pid参数*/
uint8_t kp = 20;
#define kp_resolution 100
uint8_t kd = 0;
#define kd_resolution 100
/*限幅参数*/
#define max_speed 127            // 像素每次
#define max_acceleration  300    // 像素增量每次
/*低通滤波参数*/
#define tau 0 //0.0159

/* 限制速度和加速度
 */
static void limit_speed_acceleration(int16_t * move_x, int16_t * move_y){
    static int16_t current_speed_x = 0;
    static int16_t current_speed_y = 0;
    // 计算当前的加速度
    int16_t current_acceleration_x = *move_x - current_speed_x;
    int16_t current_acceleration_y = *move_y - current_speed_y;
    // 限制加速度
    current_acceleration_x = current_acceleration_x > max_acceleration ? max_acceleration : current_acceleration_x;
    current_acceleration_x = current_acceleration_x < -max_acceleration ? -max_acceleration : current_acceleration_x;
    current_acceleration_y = current_acceleration_y > max_acceleration ? max_acceleration : current_acceleration_y;
    current_acceleration_y = current_acceleration_y < -max_acceleration ? -max_acceleration : current_acceleration_y;
    // 更新速度
    current_speed_x += current_acceleration_x;
    current_speed_y += current_acceleration_y;
    // 限制速度
    current_speed_x = current_speed_x > max_speed ? max_speed : current_speed_x;
    current_speed_x = current_speed_x < -max_speed ? -max_speed : current_speed_x;
    current_speed_y = current_speed_y > max_speed ? max_speed : current_speed_y;
    current_speed_y = current_speed_y < -max_speed ? -max_speed : current_speed_y;
    // 更新移动距离
    *move_x = current_speed_x;
    *move_y = current_speed_y;
}

/* 低通滤波器
 */
void low_pass_filter(int16_t * move_x, int16_t * move_y){
    if(tau != 0){
        static int16_t last_move_x = 0;
        static int16_t last_move_y = 0;
        static float last_out_x = 0;
        static float last_out_y = 0;

        float out_x = (last_out_x*(200*tau - 1) + *move_x + last_move_x) / (200*tau + 1);
        float out_y = (last_out_y*(200*tau - 1) + *move_y + last_move_y) / (200*tau + 1);

        last_move_x = *move_x;
        last_move_y = *move_y;
        last_out_x = out_x;
        last_out_y = out_y;

        *move_x = out_x;
        *move_y = out_y; 
    }
}

/* PD控制器
 */
void PD_controller(int16_t input_x, int16_t input_y, int8_t * output_x, int8_t * output_y){
    static int16_t e_x_last = 0;
    static int16_t e_y_last = 0;
    // 计算偏差
    int16_t e_x = input_x;
    int16_t e_y = input_y;
    // PD
    int16_t result_x = kp * e_x / kp_resolution + kd * (e_x - e_x_last) / kd_resolution;
    int16_t result_y = kp * e_y / kp_resolution + kd * (e_y - e_y_last) / kd_resolution;
    // 限幅
    limit_speed_acceleration(&result_x, &result_y);
    // 滤波
    // low_pass_filter(&result_x, &result_y);
    // 输出
    *output_x = result_x;
    *output_y = result_y;
    // 更新last值
    e_x_last = e_x;
    e_y_last = e_y;
}

