/**
 * ------------------------------------------------------------
 *  Arquivo: main.c
 *  Projeto: TempCycleDMA (com sincronização via timer)
 * ------------------------------------------------------------
 *  Descrição:
 *      Ciclo principal com 4 tarefas sincronizadas a partir da
 *      leitura de temperatura (T1), usando repeating_timer_ms.
 *
 *      Tarefa 1 - Leitura da temperatura via DMA (500ms)
 *      Tarefa 2 - Exibição da temperatura no OLED
 *      Tarefa 3 - Análise da tendência da temperatura
 *      Tarefa 4 - Controle de cor da matriz NeoPixel
 *
 *      Tarefa 5 - Alerta extra caso a temperatura seja < 1 °C
 *
 *  Data: 25/05/2025
 * ------------------------------------------------------------
 */

 #include <stdio.h>
 #include "pico/stdlib.h"
 #include "hardware/watchdog.h"
 #include "pico/time.h"
 
 #include "setup.h"
 #include "tarefa1_temp.h"
 #include "tarefa2_display.h"
 #include "tarefa3_tendencia.h"
 #include "tarefa4_controla_neopixel.h"
 #include "neopixel_driver.h"
 #include "testes_cores.h"
 
 float media;
 tendencia_t t;
 
 absolute_time_t ini_tarefa1, fim_tarefa1;
 absolute_time_t ini_tarefa2, fim_tarefa2;
 absolute_time_t ini_tarefa3, fim_tarefa3;
 absolute_time_t ini_tarefa4, fim_tarefa4;
 
 repeating_timer_t timer_t1;
 
 // --- Prototipação dos callbacks ---
 bool tarefa1_callback(repeating_timer_t *rt);
 int64_t tarefa2_callback(alarm_id_t id, void *user_data);
 int64_t tarefa3_callback(alarm_id_t id, void *user_data);
 int64_t tarefa4_callback(alarm_id_t id, void *user_data);
 void tarefa_5();  // Mantida como no loop principal
 
 int main() {
     setup();
 
     // Inicia o timer periódico da Tarefa 1 (a cada 500ms)
     add_repeating_timer_ms(500, tarefa1_callback, NULL, &timer_t1);
 
     while (true) {
         // Alerta de temperatura baixa (extra)
         tarefa_5();
 
         // Exibe dados no terminal a cada 1 segundo
         printf("Temperatura: %.2f °C | T1: %.3fs | T2: %.3fs | T3: %.3fs | T4: %.3fs | Tendência: %s\n",
                media,
                absolute_time_diff_us(ini_tarefa1, fim_tarefa1) / 1e6,
                absolute_time_diff_us(ini_tarefa2, fim_tarefa2) / 1e6,
                absolute_time_diff_us(ini_tarefa3, fim_tarefa3) / 1e6,
                absolute_time_diff_us(ini_tarefa4, fim_tarefa4) / 1e6,
                tendencia_para_texto(t));
 
         sleep_ms(1000);
     }
 
     return 0;
 }
 
 // --- Tarefa 1: Leitura de temperatura via DMA (base do ciclo) ---
 bool tarefa1_callback(repeating_timer_t *rt) {
     ini_tarefa1 = get_absolute_time();
     media = tarefa1_obter_media_temp(&cfg_temp, DMA_TEMP_CHANNEL);
     fim_tarefa1 = get_absolute_time();
 
     // Agenda as próximas tarefas com pequenos deslocamentos
     add_alarm_in_ms(10, tarefa3_callback, NULL, false);
     add_alarm_in_ms(20, tarefa2_callback, NULL, false);
     add_alarm_in_ms(30, tarefa4_callback, NULL, false);
 
     return true;  // Continua repetindo
 }
 
 // --- Tarefa 3: Análise da tendência térmica ---
 int64_t tarefa3_callback(alarm_id_t id, void *user_data) {
     ini_tarefa3 = get_absolute_time();
     t = tarefa3_analisa_tendencia(media);
     fim_tarefa3 = get_absolute_time();
     return 0;
 }
 
 // --- Tarefa 2: Exibição no display OLED ---
 int64_t tarefa2_callback(alarm_id_t id, void *user_data) {
     ini_tarefa2 = get_absolute_time();
     tarefa2_exibir_oled(media, t);
     fim_tarefa2 = get_absolute_time();
     return 0;
 }
 
 // --- Tarefa 4: Controle da matriz NeoPixel ---
 int64_t tarefa4_callback(alarm_id_t id, void *user_data) {
     ini_tarefa4 = get_absolute_time();
     tarefa4_matriz_cor_por_tendencia(t);
     fim_tarefa4 = get_absolute_time();
     return 0;
 }
 
 // --- Tarefa 5: Pisca branco se temperatura for baixa (< 1 °C) ---
 void tarefa_5() {
     if (media < 1.0f) {
         npSetAll(COR_BRANCA);
         npWrite();
         sleep_ms(500);
         npClear();
         npWrite();
         sleep_ms(500);
     }
 }
 