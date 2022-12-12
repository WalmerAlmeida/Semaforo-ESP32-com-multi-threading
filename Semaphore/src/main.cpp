#include <Arduino.h>

typedef enum { kRedLight = 0, kYellowLight = 1, kGreenLight = 2 } tLight;
typedef enum { kRedState = 0, kYellowState = 1, kGreenState = 2 } tState;

#define LED_RED GPIO_NUM_5
#define LED_GREEN GPIO_NUM_2
#define LED_YELLOW GPIO_NUM_4

struct led_task_parameters_t
{
  gpio_num_t led_gpio;
  bool led_value;
};

struct sStateTableEntry {
    tLight light; // all states have associated lights
    tState goEvent; // state to enter when go event occurs
    tState stopEvent; // ... when stop event occurs
    tState timeoutEvent;// ... when timeout occurs
};

static led_task_parameters_t led_gpio[] = {
  {LED_RED, LOW}, {LED_YELLOW, LOW}, {LED_GREEN, LOW}
};
tState currentState = kGreenState;
bool auto_semaphore = false;

String light_to_string(tLight light, bool turnOn){
    String s = "";
    switch (light)
    {
      case kRedLight:
          s = "Red";
          led_gpio[0].led_value = turnOn;
          break;
      case kYellowLight:
          s = "Yellow";
          led_gpio[1].led_value = turnOn;
          break;
      case kGreenLight:
          s = "Green";
          led_gpio[2].led_value = turnOn;
          break;
      default:
          break;
    }

    return s;
}

void LightOff(tLight light) {
    Serial.print(light_to_string(light, false));
    Serial.println("light is off.");
}

void LightOn(tLight light){
    Serial.print(light_to_string(light, true));
    Serial.println("light is on.");
}

struct sStateTableEntry stateTable[] = {
    { kRedLight,    kGreenState,    kRedState,      kRedState },    // Red
    { kYellowLight, kYellowState,   kYellowState,   kRedState },    // Yellow
    { kGreenLight,  kGreenState,    kYellowState,   kGreenState }   // Green
};

// Go event handler
tState HandleEventGo(tState currentState)
{
    Serial.println("Handling Go event.\n");
    LightOff(stateTable[currentState].light);
    tState nextState = stateTable[currentState].goEvent;
    LightOn(stateTable[nextState].light);
    return nextState;
}

// Stop event handler
tState HandleEventStop(tState currentState)
{
    Serial.println("Handling Stop event.\n");
    LightOff(stateTable[currentState].light);
    tState nextState = stateTable[currentState].stopEvent;
    LightOn(stateTable[nextState].light);
    return nextState;
}

// Timeout event handler
tState HandleEventTimeout(tState currentState)
{
    Serial.println("Handling Timeout event.\n");
    LightOff(stateTable[currentState].light);
    tState nextState = stateTable[currentState].timeoutEvent;
    LightOn(stateTable[nextState].light);
    return nextState;
}

void read_serial(void *pvParameter)
{  
  while (1) {
    Serial.println("Type an event (Go, Stop, Timeout) > ");
    while (Serial.available() == 0) {}     //wait for data available
    String teststr = Serial.readStringUntil('\n'); // Serial.readString();  //read until timeout
    teststr.trim();                        // remove any \r \n whitespace at the end of the String
    if (teststr == "Go") {
      currentState = HandleEventGo(currentState);
    } else if (teststr == "Stop") {
      currentState = HandleEventStop(currentState);
    } else if (teststr == "Timeout") {
      currentState = HandleEventTimeout(currentState);
    } else if (teststr == "Auto") {
      auto_semaphore = !auto_semaphore;
      if (auto_semaphore) {
        Serial.println("Auto mode entered!");
      } else {
        Serial.println("Auto mode exit!");
      }
    }
  }
  vTaskDelete( NULL );
}

void set_semaphore(led_task_parameters_t * led_semaphore){
    gpio_set_level(led_semaphore[0].led_gpio, led_semaphore[0].led_value);
    gpio_set_level(led_semaphore[1].led_gpio, led_semaphore[1].led_value);
    gpio_set_level(led_semaphore[2].led_gpio, led_semaphore[2].led_value);
}

void led_task(void *pvParameter)
{
  led_task_parameters_t * led_semaphore = ((led_task_parameters_t *)pvParameter);

  gpio_reset_pin(LED_RED);
  gpio_reset_pin(LED_YELLOW);
  gpio_reset_pin(LED_GREEN);

  gpio_set_direction(LED_YELLOW, GPIO_MODE_OUTPUT);
  gpio_set_direction(LED_RED, GPIO_MODE_OUTPUT);
  gpio_set_direction(LED_GREEN, GPIO_MODE_OUTPUT);

  while (1) {
    if (auto_semaphore) {
      currentState = HandleEventGo(currentState);
      set_semaphore(led_semaphore);
      delay(1000);
      currentState = HandleEventStop(currentState);
      set_semaphore(led_semaphore);
      delay(1000);
      currentState = HandleEventTimeout(currentState);
      set_semaphore(led_semaphore);
      delay(1000);
    } else {
      set_semaphore(led_semaphore);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); 
  }
  vTaskDelete( NULL );
}

void setup ()
{
  Serial.begin(9600);
  Serial.setTimeout(5000);

  LightOn(kGreenLight);
  LightOff(kRedLight);
  LightOff(kYellowLight);

  xTaskCreate(
    &read_serial, // task function
    "read_input_string", // task name
    2048, // stack size in words
    NULL, // pointer to parameters
    5, // priority
    NULL
  ); // out pointer to task handle

  xTaskCreate(
    &led_task, // task function
    "red_led_task", // task name
    2048, // stack size in words
    led_gpio, //&red_led_gpio, // pointer to parameters
    5, // priority
    NULL); // out pointer to task handle
}

void loop () { }
