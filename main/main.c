
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

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define DISABLE 0
#define ENABLE 1



//Queue Handler/event queue;
static xQueueHandle gpio_event_queue = NULL; // setting up a Queue.

static xQueueHandle new_event_queue = NULL; // setting up a  new Queue

static xQueueHandle ten_values_queue = NULL;

static TaskHandle_t gpio_handler1 = NULL;


//aka Queue_handle_t

//task B

//ISR Handler A

/**
 * @brief 
 *     static to make it always be there.
    void because it has no expected return time.
    gpio_isr_handler because its and interupt service routine handler.
    void * argument because its passed via a pointer arg.
 * 
 * @param argument this value is the gpio number in this context
 */
static void IRAM_ATTR gpio_isr_handler_A(void * argument)
{ 

    uint32_t gpio_number = (uint32_t) argument; 
    //the arg is the pin the gpio interrupt occurs on.
    //This is being cast to 32 bit and saved as gpio number.
    xQueueSendFromISR(gpio_event_queue, &gpio_number,NULL);
    //replace null if you want to trigger another task from this.
    //sends to back of queue and is safe to use withing a ISR.
    //queued by copy not reference so only good for small items.
    //better to only store a pointer to items being queued.
}

//ISR Handler B
static void IRAM_ATTR gpio_isr_handler_B(void * argument)
{ 

    uint32_t gpio_number = (uint32_t) argument; 
    //the arg is the pin the gpio interrupt occurs on.
    //This is being cast to 32 bit and saved as gpio number.
    xQueueSendFromISR(new_event_queue, &gpio_number,NULL);
    //replace null if you want to trigger another task from this.
    //sends to back of queue and is safe to use withing a ISR.
    //queued by copy not reference so only good for small items.
    //better to only store a pointer to items being queued.
}


//task A
/**
 * @brief task just waits and loops, looking for items on the event queue, every 50 ticks.
 * Once recieved it then saves it to a buffer.
 * If its recieved then prints the message saying which pin created interrupt and what level.
 * 
 * @param argument pointer to task
 */
static void gpio_task_1(void * argument)
{
    uint32_t io_number_buffer; //creates a buffer for events?

    for(;;){
        if(xQueueReceive(gpio_event_queue, &io_number_buffer,portMAX_DELAY))
        {
            printf("GPIO %d interrupt, value: %d \n",io_number_buffer,gpio_get_level(io_number_buffer));
            //prints the pin number and then pulls second value from buffer which is the value of pin.

        }
    }
}

static void gpio_task_2(void * argument)
{
    uint32_t io_number_buffer; //creates a buffer for events?

    for(;;){
        if(xQueueReceive(new_event_queue, &io_number_buffer,portMAX_DELAY))
        {
            printf("GPIO %d interrupt, value: %d \n",io_number_buffer,gpio_get_level(io_number_buffer));
            //prints the pin number and then pulls second value from buffer which is the value of pin.

        }
    }
}

static void gpio_task_3(void * argument)
{
    int counter=0;
    for (;;)
    {
        counter = counter +1;
        if(ten_values_queue !=0){

            xQueueSend(ten_values_queue,(void *) &counter,portMAX_DELAY);
        }


    }
}

static void gpio_task_4(void * argument)
{
    uint32_t buffer_FIFO=0;
    for(;;)
    {
        if(ten_values_queue !=0)
        {
            if(xQueuePeek(ten_values_queue,(void *)buffer_FIFO ,(TickType_t)10))
            {
                printf("%d\n",buffer_FIFO);
                vTaskDelay(500);
            }
        }
    }

}





void app_main(void)
{
    //configuration of registers.
    
    //esp port can do registers via their own structures.
    //to configure the peripherals we use a gpio_config_t structure.

    gpio_config_t gpio_configuration_object;
    //creating config object

    //********************
    //Configuring 

    gpio_configuration_object.intr_type=DISABLE;
    //setting whether interupt type or not.

    //gpio_configuration_object.mode = OUTPUT;
    gpio_configuration_object.mode = GPIO_MODE_OUTPUT;
    //sets the perpheral register to output.
    
    gpio_configuration_object.pin_bit_mask = ((1ULL<<18)|(1ULL<<19));
    //bit mask is LSL with the number of pin. can be either 18 or 19.
    //effectively setting bits 18 and 19 to true.

    gpio_configuration_object.pull_down_en = DISABLE;
    //disables pull down

    gpio_configuration_object.pull_up_en = DISABLE;
    //disables pull up mode.

    gpio_config(&gpio_configuration_object);
    //saves structure as a peripherals setting. (Register setup)

    //*************************
    //configuring inputs

    gpio_configuration_object.intr_type =1;
    //setting as interupt type.

    gpio_configuration_object.pin_bit_mask = ((1ULL<<5)|(1ULL<<4));
    //bit mask for pins 4 and 5.

    //gpio_configuration_object.mode = INPUT;
    gpio_configuration_object.mode = GPIO_MODE_DEF_INPUT;
    //setting to input bits.

    gpio_configuration_object.pull_up_en= HIGH;
    //setting pull up mode on.
    gpio_config(&gpio_configuration_object);

    //******************

    gpio_set_intr_type(4,GPIO_INTR_ANYEDGE);

    //GPIO_INTR_ANYEDGE = enum of 3, setting rise/
    //GPIO POS EDGE = 1
    //GPIO NEG EDGE = 2
    //GPIO Disable = 0
    //GPIO INTR HIGH_ Level = 5

    //static uint8_t ucParameterToPass;//added empty parameter

    //create a freeRtos queue
    gpio_event_queue = xQueueCreate(10,sizeof(uint32_t));
    //creates queue of length 10 and max size 32bit

     //create a freeRtos queue
    new_event_queue = xQueueCreate(10,sizeof(uint32_t));
    //creates queue of length 10 and max size 32bit

    ten_values_queue = xQueueCreate(10,sizeof(uint32_t));

    //gpio task.
    xTaskCreate(gpio_task_1,"gpio_task_1",2048,NULL,10,NULL);
    //not got a task handler.

    //gpio task.
    xTaskCreate(gpio_task_2,"gpio_task_2",2048,NULL,9,&gpio_handler1);
    //not got a task handler.

    xTaskCreate(gpio_task_3,"gpio_task_3",2048,NULL,2,NULL);
    xTaskCreate(gpio_task_4,"gpio_task_4",2048,NULL,4,NULL);

    //gpio isr.
    /**
     * @brief creates ISR looking for interupt allocation flag.
     * setting flag to 0.
     * 
     */
    gpio_install_isr_service(0);

    /**
     * @brief creates a link for the interupt handler.
     * If someting on pin 4, 
     * ISR handler called with parameter typecast void * pin 4
     * 
     */
    gpio_isr_handler_add(4,gpio_isr_handler_A,(void*)4);

     /**
     * @brief creates a link for the interupt handler.
     * If someting on pin 4, 
     * ISR handler called with parameter typecast void * pin 4
     * 
     */
    gpio_isr_handler_add(5,gpio_isr_handler_B,(void*)5);

    int cnt = 0;
    while(1) {
        printf("cnt: %d\n", cnt++);
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(18, cnt % 2);
        gpio_set_level(19, cnt % 2);

    }
 
}
