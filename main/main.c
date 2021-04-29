
/* GPIO Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/timer.h"

#define TIMER_DIVIDER 16
#define TIMER_SCALE (TIMER_BASE_CLK/TIMER_DIVIDER)


typedef struct 
{   
    int timer_group;
    int timer_idx;
    int alarm_interval;
    bool auto_reload;
}example_timer_info_t;

/**
 * @brief Nested Structure for timer Events.
 * 
 */
typedef struct 
{
    example_timer_info_t info;
    uint64_t timer_counter_value;
}example_timer_event_t;


static xQueueHandle s_timer_queue;

/**
 * @brief Timer Counter - Print Look
 * 
 * @param counter_value 
 */
static void inline print_timer_counter(uint64_t counter_value)
{
    printf("Counter: 0x%08x%08x\r\n",(uint32_t)  (counter_value>>32),
    (uint32_t)(counter_value));
    printf("Time : %.8f s\r\n",(double)counter_value / TIMER_SCALE);
}

//Timer Group CallBack

static bool IRAM_ATTR timer_group_isr_callback(void *argument)
{
    BaseType_t high_task_awoken = (BaseType_t) false;
    //Setting Base Type as Zero;
    example_timer_info_t *info = (example_timer_info_t *) argument;
    //Creating a pointer - Cast the argument to be example_timer_t.

    uint64_t timer_counter_value = timer_group_get_counter_value_in_isr(
        (info->timer_group),(info->timer_idx));
    //Pulling via the passed arg assuming its structure of example_timer_info.

    example_timer_event_t sEventTime_obj=
    {
        .info.timer_group   = info->timer_group,
        .info.timer_idx     = info->timer_idx,
        .info.alarm_interval= info->alarm_interval,
        .info.auto_reload   = info->auto_reload,
        .timer_counter_value= timer_counter_value

    };
    //Create and Allocate Structure

    if(!(info->auto_reload))
    {
        timer_counter_value += (info->alarm_interval*TIMER_SCALE);
        timer_group_set_alarm_value_in_isr(
                                            info->timer_group,
                                            info->timer_idx,
                                            timer_counter_value);
    }

    xQueueSendFromISR(s_timer_queue,&sEventTime_obj,&high_task_awoken);
    //Sends Event Data back via queue

    return high_task_awoken == pdTRUE;
    //Decides whether to yield at end to a higher Interupt

}
//Create Timers || Timer Group.


static void timer_group_timer_init(int16_t group, int16_t timer,
                    bool auto_reload_param , int16_t timer_interval_sec)
{
    //Setting up a default config via timer_types.h
    timer_config_t config = 
    {
        .divider = TIMER_DIVIDER,
        .alarm_en = TIMER_ALARM_EN,
        .counter_en = TIMER_PAUSE,
        .auto_reload = auto_reload_param,
        .counter_dir = TIMER_COUNT_UP ,
    };

    timer_init(group,timer,&config);
    // creates a timer with group assigned , timer passed and default config.

    timer_set_counter_value(group , timer, 0);
    //sets value at the timer being passed in.
    //this will be the value it initialises from.


    timer_set_alarm_value(group,timer,(timer_interval_sec)*TIMER_SCALE);
    //Sets the time when the interupt Alarm hits
    timer_enable_intr(group,timer);
    //Turns the interupt on... This is all ready to go.


    example_timer_info_t *timer_info = calloc(1,sizeof(example_timer_info_t));

    timer_info->timer_group = group;
    timer_info->timer_idx   = timer;
    timer_info->auto_reload = auto_reload_param;
    timer_info->alarm_interval = timer_interval_sec;
    timer_isr_callback_add(group,timer,timer_group_isr_callback,timer_info,0);
    //Adds the timers to the callback.
    
    timer_start(group,timer);
    
}



void app_main(void)
{
    s_timer_queue = xQueueCreate(10,sizeof(example_timer_event_t));

    timer_group_timer_init(TIMER_GROUP_0,TIMER_0,pdTRUE,3);
    timer_group_timer_init(TIMER_GROUP_1,TIMER_0,pdFALSE,5);


    for(;;)
    {
        example_timer_event_t sEventTime_obj;
        xQueueReceive(s_timer_queue,&sEventTime_obj,portMAX_DELAY);

        if(sEventTime_obj.info.auto_reload)
        {
            printf("Timer Group - Auto Reload ON\n");
        }
        else
        {
            printf("Timer Group - Auto Reload OFF\n");
        }

        printf("Group [%d],timer[%d],alarm\n",sEventTime_obj.info.timer_group,
                                              sEventTime_obj.info.timer_idx);
        
        printf("---------EVENT_TIME-------");
        print_timer_counter(sEventTime_obj.timer_counter_value);

        printf("---------TASK_TIME-------");
        uint64_t task_counter_value;
        timer_get_counter_value(sEventTime_obj.info.timer_group,
                                sEventTime_obj.info.timer_idx,
                                &task_counter_value);
        //puts the task counter value into this func.

        print_timer_counter(task_counter_value);
  
    }

}