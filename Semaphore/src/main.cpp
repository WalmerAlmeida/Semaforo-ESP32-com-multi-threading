#include <Arduino.h>

typedef enum { kRedLight = 0, kYellowLight = 1, kGreenLight = 2 } tLight;
typedef enum { kRedState = 0, kYellowState = 1, kGreenState = 2 } tState;

#define LED_RED GPIO_NUM_5
#define LED_GREEN GPIO_NUM_2
#define LED_YELLOW GPIO_NUM_4

struct led_task_parameters_t
{
  gpio_num_t led_gpio;
  // TickType_t blink_time;
};

static led_task_parameters_t red_led_gpio = {LED_RED}; // {LED_RED, 500};
static led_task_parameters_t yellow_led_gpio = {LED_YELLOW}; // {LED_BLUE, 500};
static led_task_parameters_t green_led_gpio = {LED_GREEN}; // {LED_GREEN, 500};

struct sStateTableEntry {
    tLight light; // all states have associated lights
    tState goEvent; // state to enter when go event occurs
    tState stopEvent; // ... when stop event occurs
    tState timeoutEvent;// ... when timeout occurs
};

tState currentState = kGreenState;
bool auto_semaphore = false;

String light_to_string(tLight light, bool turnOn){
    String s = "";
    switch (light)
    {
      case kRedLight:
          s = "Red";
          if (turnOn) {
            gpio_set_level(LED_RED, HIGH);
          } else {
            gpio_set_level(LED_RED, LOW);
          }
          break;
      case kGreenLight:
          s = "Green";
          if (turnOn) {
            gpio_set_level(LED_GREEN, HIGH);
          } else {
            gpio_set_level(LED_GREEN, LOW);
          }
          break;
      case kYellowLight:
          s = "Yellow";
          if (turnOn) {
            gpio_set_level(LED_YELLOW, HIGH);
          } else {
            gpio_set_level(LED_YELLOW, LOW);
          }
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
  Serial.println("Enter a line finishing with Enter:\n");
  
  while (1) {
    // Serial.setTimeout(5000);
    Serial.println("Type an event (Go, Stop, Timeout) > ");
    while (Serial.available() == 0) {}     //wait for data available
    String teststr = Serial.readString();  //read until timeout
    teststr.trim();                        // remove any \r \n whitespace at the end of the String
    if (teststr == "Go") {
      currentState = HandleEventGo(currentState);
    } else if (teststr == "Stop") {
      currentState = HandleEventStop(currentState);
    } else if (teststr == "Timeout") {
      currentState = HandleEventTimeout(currentState);
    } else if (teststr == "Auto") {
      auto_semaphore = !auto_semaphore;
      Serial.println("Auto mode entered!");
    }
    // vTaskDelay(10 / portTICK_PERIOD_MS); 
  }
  vTaskDelete( NULL );
}

void led_task(void *pvParameter)
{
  while (1) {
    if (auto_semaphore) {
      currentState = HandleEventGo(currentState);
      delay(1000);
      currentState = HandleEventStop(currentState);
      delay(1000);
      currentState = HandleEventTimeout(currentState);
      delay(1000);
    }
    // led_value = !led_value;
    // vTaskDelay(blink_time / portTICK_PERIOD_MS);
    vTaskDelay(10 / portTICK_PERIOD_MS); 
    // vTaskDelete( NULL );
  }
}

void setup ()
{
  Serial.begin(9600);
  // Serial.println("Hello! I'm using Zephyr %s on %s, a %s board. \n\n", KERNEL_VERSION_STRING, CONFIG_BOARD, CONFIG_ARCH);

  LightOn(kGreenLight);
  LightOff(kRedLight);
  LightOff(kYellowLight);

  gpio_set_direction(LED_GREEN, GPIO_MODE_OUTPUT);
  gpio_set_direction(LED_YELLOW, GPIO_MODE_OUTPUT);
  gpio_set_direction(LED_RED, GPIO_MODE_OUTPUT);
  
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
    NULL, //&red_led_gpio, // pointer to parameters
    5, // priority
    NULL); // out pointer to task handle
}

void loop () { }
