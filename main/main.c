/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"

#include <math.h>
#include <stdlib.h>

#define abs(x) ((x < 0) ? -(x) : x)

const int X_PIN = 26;
const int X_ADC = 0;
const int Y_PIN = 27;
const int Y_ADC = 1;
const int BTN_PIN = 22;

QueueHandle_t QueueAdc;

typedef struct {
    int pin;
    int adc;
} adc_task_arg;

typedef struct {
    int axis;
    int val;
} adc_t;

void write_package(adc_t data) {
    int msb = data.val >> 8;
    int lsb = data.val & 0xFF ;

    uart_putc_raw(uart0, data.axis);
    uart_putc_raw(uart0, lsb);
    uart_putc_raw(uart0, msb);
    uart_putc_raw(uart0, -1);
}

void uart_task(void *p) {
    adc_t data;

    while (true) {
        if (xQueueReceive(QueueAdc, &data, portMAX_DELAY) == pdTRUE) write_package(data);
    }
}

void adc_xy_task(void *p) {
    adc_task_arg *arg = (adc_task_arg *)p;

    adc_init();
    adc_gpio_init(arg->pin);

    uint16_t results[5];
    adc_t data;
    data.axis = arg->adc;
    int i = 0;

    while (true) {
        adc_select_input(arg->adc); 
        results[(i++)%5] = adc_read();
        int value = (((results[0] + results[1] + results[2] + results[3] + results[4])/5) - 2047)/5;

        if (abs(value) >= 30) {
            data.val = value;
            xQueueSend(QueueAdc, &data, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main() {
    stdio_init_all();

    QueueAdc = xQueueCreate(32, sizeof(adc_t));
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 2, NULL);

    adc_task_arg argX = {X_PIN, X_ADC};
    xTaskCreate(adc_xy_task, "x_task", 4096, &argX, 1, NULL);

    adc_task_arg argY = {Y_PIN, Y_ADC};
    xTaskCreate(adc_xy_task, "x_task", 4096, &argY, 1, NULL);

    vTaskStartScheduler();

    while (true);
}
