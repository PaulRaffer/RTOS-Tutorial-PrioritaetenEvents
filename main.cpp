// Copyright (c) 2021 Stefan Grubmüller, Paul Raffer.

#include <cmsis_os2.h>
extern "C" {
#include "armv10_std.h"
}
#include <string.h>


namespace fsst { // interface

[[noreturn]] auto led_Thread1(void * argument) -> void;
[[noreturn]] auto led_Thread2(void * argument) -> void;
[[noreturn]] auto led_Thread3(void * argument) -> void;

class radio_t {
public:
	using frequency_type = double; // MHz

	constexpr explicit radio_t(/*in*/ bool on = false, /*in*/ frequency_type frequency = 100);

	[[nodiscard]] constexpr auto is_on() const -> bool;
	
	/*constexpr*/ auto turn_on(/*in*/ bool on = true) & -> void;
	/*constexpr*/ auto turn_off() & -> void;

	[[nodiscard]] constexpr auto frequency() const -> frequency_type;

	/*constexpr*/ auto set_frequency(/*in*/ frequency_type frequency) & -> void;

private:
	bool on_;
	frequency_type frequency_;
};
	
class car_t {
	public: using speed_type = double; // km/h
	public: using temperature_type = double; // °C

	public: constexpr explicit car_t(
		/*in*/ speed_type max_speed = 200, /*in*/ speed_type speed = 0,
		/*in*/ temperature_type temperature = 20, /*in*/ radio_t radio = radio_t{});
	public: [[nodiscard]] constexpr auto speed() const -> speed_type;

	public: /*constexpr*/ auto set_speed(/*in*/ speed_type speed) & -> void;
	public: /*constexpr*/ auto accelerate(/*in*/ speed_type speed) & -> void;
	public: /*constexpr*/ auto decelerate(/*in*/ speed_type speed) & -> void;

	public: [[nodiscard]] constexpr auto temperature() const -> temperature_type;
	
	public: /*constexpr*/ auto set_temperature(/*in*/ temperature_type temperature) & -> void;
	public: /*constexpr*/ auto heat(/*in*/ temperature_type temperature) & -> void;
	public: /*constexpr*/ auto cool(/*in*/ temperature_type temperature) & -> void;

	private: speed_type const max_speed_;
	private: speed_type speed_;
	private: temperature_type temperature_;
	public: radio_t radio;
};

} // namespace fsst


namespace fsst { // implementation
	
	constexpr radio_t::radio_t(bool const on, frequency_type const frequency)
	:
		on_{on}, frequency_{frequency}
	{}

	constexpr auto radio_t::is_on() const -> bool
	{
		return this->on_;
	}
	
	/*constexpr*/ auto radio_t::turn_on(bool const on) & -> void
	{
		this->on_ = on;
	}
	
	/*constexpr*/ auto radio_t::turn_off() & -> void
	{
		this->turn_on(false);
	}
	

	constexpr auto radio_t::frequency() const -> frequency_type
	{
		return this->frequency_;
	}

	/*constexpr*/ auto radio_t::set_frequency(frequency_type const frequency) & -> void
	{
		this->frequency_ = frequency;
	}
	
	
	
	
	
	constexpr car_t::car_t(
		speed_type const max_speed, speed_type const speed,
		temperature_type const temperature, radio_t const radio)
	:
		max_speed_{max_speed}, speed_{speed},
		temperature_{temperature}, radio{radio}
	{}
	
	constexpr auto car_t::speed() const -> speed_type
	{
		return this->speed_;
	}

	/*constexpr*/ auto car_t::set_speed(speed_type const speed) & -> void
	{
		this->speed_ = speed;//std::clamp(speed, 0, 200);
	}
	
	/*constexpr*/ auto car_t::accelerate(speed_type const speed) & -> void
	{
		this->set_speed(this->speed() + speed);
	}
	/*constexpr*/ auto car_t::decelerate(speed_type const speed) & -> void
	{
		this->accelerate(-speed);
	}

	constexpr auto car_t::temperature() const -> temperature_type
	{
		return this->temperature_;
	}
	
	/*constexpr*/ auto car_t::set_temperature(temperature_type const temperature) & -> void
	{
		this->temperature_ = temperature;
	}
	/*constexpr*/ auto car_t::heat(temperature_type const temperature) & -> void
	{
		this->set_temperature(this->temperature() + temperature);
	}
	/*constexpr*/ auto car_t::cool(temperature_type const temperature) & -> void
	{
		this->heat(-temperature);
	}
	
}; // namespace fsst

/******************************************************************************/
/*                         Global Varibales and Definitions                   */
/******************************************************************************/
osEventFlagsId_t too_hot_flag;
osEventFlagsId_t break_flag;


auto set_led(int led, bool value = 1) -> void
{
	*((volatile unsigned long *)(BITBAND_PERI(GPIOB_ODR, (led + 8)))) = value;
}

auto reset_led(int led) -> void
{
	set_led(led, 0);
}





[[noreturn]] auto radio_func(void * argument) -> void
{
	while (true) {
		set_led(1);
	}
}

[[noreturn]] auto temperature_sensor_func(void * argument) -> void
{
	while (true) {
		// temperatur einlesen
		set_led(2);
	}
}

[[noreturn]] auto air_conditioner_func(void * argument) -> void
{
	while (true) {
		osEventFlagsWait(too_hot_flag, 0x01, osFlagsWaitAny, osWaitForever);
		set_led(3);
	}
}

[[noreturn]] auto break_func(void * argument) -> void
{
	while (true) {
		osEventFlagsWait(break_flag, 0x01, osFlagsWaitAny, osWaitForever);
		set_led(4);
	}
}

struct thread_info_t {
	osThreadFunc_t func;
	void * argument;
	osThreadAttr_t attr;
};

auto new_thread(thread_info_t const * thread_info) -> osThreadId_t
{
	return osThreadNew(thread_info->func, thread_info->argument, &thread_info->attr);
}

auto create_thread(osThreadFunc_t const func, void * argument, char const * name, osPriority_t const priority) -> osThreadId_t
{
	thread_info_t thread;
	memset(&thread, 0, sizeof thread);
	thread.func = func;
	thread.argument = argument;
	thread.attr.name = name;
	thread.attr.priority = priority;
	return new_thread(&thread);
}

auto create_event_flag(char const * name) -> osEventFlagsId_t
{
	osEventFlagsAttr_t event_flag;
	memset(&event_flag, 0, sizeof event_flag);
	event_flag.name = name;
	return osEventFlagsNew(&event_flag);
}

auto create_threads() -> void
{
	auto too_hot_flag = create_event_flag("too hot");
	auto break_flag = create_event_flag("break");
	
	create_thread(radio_func, NULL, "radio", osPriorityNormal);
	create_thread(temperature_sensor_func, NULL, "temperature sensor", osPriorityNormal);
	create_thread(air_conditioner_func, NULL, "air conditioner", osPriorityAboveNormal);
	create_thread(break_func, NULL, "break", osPriorityHigh);
}

auto init() -> void
{
	SystemCoreClockUpdate();
  osKernelInitialize();
	
	init_leds_switches();         // Initialize LED`s and Switches of ARM Minimalsystem
	set_leds(0);                  // Switch off all LEDs
}


auto main() -> int
{
	init();
	create_threads();
  if (osKernelGetState() == osKernelReady) {
    osKernelStart();                    						// Start thread execution
  }
  while(1);
}
