#include "esphome.h"
#include "esphome/components/rego1000/rego.h"

#include <vector>

#define INDOOR_THERMOSTAT_TEMP_CAN_ID 0x10000060
#define INDOOR_THERMOSTAT_DIAL_CAN_ID 0x10004060
#define INDOOR_THERMOSTAT_DIAL_MIDPOINT 0x200

using namespace esphome;
using namespace esphome::climate;

namespace rego {

class RegoClimate : public Climate, public RegoBase {
public:
  RegoClimate() {
    this->preset = CLIMATE_PRESET_HOME;
    this->mode = CLIMATE_MODE_HEAT;
    this->action = CLIMATE_ACTION_HEATING;
  }
  ClimateTraits traits() override {
    // The capabilities of the climate device
    auto traits = climate::ClimateTraits();
    traits.set_supports_current_temperature(true);
    traits.set_supported_modes({climate::CLIMATE_MODE_HEAT});
    traits.set_visual_min_temperature(12.0);
    traits.set_visual_max_temperature(25.0);
    traits.set_visual_temperature_step(0.1);
    return traits;
  }

  virtual void publish(float value) override {
    this->target_temperature = value;
    this->publish_state();
  }
  void set_indoor_sensor(sensor::Sensor *indoor_sensor) {
    this->indoor_sensor = indoor_sensor;
    this->set_interval("update_indoor_temperature", 5000, [this]() { this->update_indoor_temperature(); });
  }
  void update_indoor_temperature() {
    if (this->indoor_sensor->has_state()) {
      this->current_temperature = this->indoor_sensor->state;
      this->publish_state();
      int16_t indoor_temp = static_cast<uint16_t>(this->current_temperature / this->value_factor);
      this->canbus->send_data(INDOOR_THERMOSTAT_DIAL_CAN_ID, true, false, value_to_can_data(INDOOR_THERMOSTAT_DIAL_MIDPOINT));
      this->canbus->send_data(INDOOR_THERMOSTAT_TEMP_CAN_ID, true, false, value_to_can_data(indoor_temp));
    }
  }
  void control(const ClimateCall &call) override {
    if (call.get_target_temperature().has_value()) {
      this->target_temperature = *call.get_target_temperature();
      this->publish_state();
      int16_t indoor_setpoint = static_cast<uint16_t>(this->target_temperature / this->value_factor);
      this->canbus->send_data(this->can_poll_id, true, false, value_to_can_data(indoor_setpoint));
    }
  }
protected:
  sensor::Sensor *indoor_sensor;
};

}
