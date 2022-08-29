#include <iostream>
#include <sstream>
#include <thread>

#include "fre_table.h"
#include "midifile/MidiFile.h"
#include "music.h"
#include "res/SuperMario.mid.h"
#include "res/quanyecha.mid.h"
#include "res/twinkle.mid.h"

using namespace smf;
using namespace std;

#include "driver/ledc.h"
#include "esp_err.h"

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO (5)  // Define the output GPIO
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT  // Set duty resolution to 13 bits
#define LEDC_DUTY (4095)                 // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY (5000)            // Frequency in Hertz. Set frequency at 5 kHz

static void example_ledc_init(void) {
  // Prepare and then apply the LEDC PWM timer configuration
  ledc_timer_config_t ledc_timer = {.speed_mode = LEDC_MODE,
                                    .duty_resolution = LEDC_DUTY_RES,
                                    .timer_num = LEDC_TIMER,
                                    .freq_hz = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
                                    .clk_cfg = LEDC_AUTO_CLK};
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  // Prepare and then apply the LEDC PWM channel configuration
  ledc_channel_config_t ledc_channel = {.gpio_num = LEDC_OUTPUT_IO,
                                        .speed_mode = LEDC_MODE,
                                        .channel = LEDC_CHANNEL,
                                        .intr_type = LEDC_INTR_DISABLE,
                                        .timer_sel = LEDC_TIMER,
                                        .duty = 0,  // Set duty to 0%
                                        .hpoint = 0};
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

extern "C" void app_main() {
  // Set the LEDC peripheral configuration
  example_ledc_init();
  // Set duty to 50%
  ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY));
  // Update duty to apply the new value
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));

  for (;;) {
    playMusic(QuanYeCha, sizeof QuanYeCha, [&](NoteEvent e) {
      if (e.isOn) {
        ledc_set_freq(LEDC_MODE, LEDC_TIMER, fre_table[e.key - 21]);
        ledc_timer_resume(LEDC_MODE, LEDC_TIMER);
      } else {
        ledc_timer_pause(LEDC_MODE, LEDC_TIMER);
      }
      float speed = 1;
      this_thread::sleep_for(std::chrono::milliseconds((uint16_t)((float)e.tickMs / speed)));
    });
  }
}
