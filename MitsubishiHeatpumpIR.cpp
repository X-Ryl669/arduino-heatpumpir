#include <Arduino.h>
#include <MitsubishiHeatpumpIR.h>

// This is a protected method, i.e. generic Mitsubishi instances cannot be created
MitsubishiHeatpumpIR::MitsubishiHeatpumpIR()
{
}

// The different models just set the model accordingly
MitsubishiFDHeatpumpIR::MitsubishiFDHeatpumpIR()
{
  _mitsubishiModel = MITSUBISHI_FD;
}

MitsubishiFEHeatpumpIR::MitsubishiFEHeatpumpIR()
{
  _mitsubishiModel = MITSUBISHI_FE;
}

void MitsubishiHeatpumpIR::send(IRSender& IR, byte powerModeCmd, byte operatingModeCmd, byte fanSpeedCmd, byte temperatureCmd, byte swingVCmd, byte swingHCmd)
{
  // Sensible defaults for the heat pump mode

  byte powerMode = MITSUBISHI_AIRCON1_MODE_ON;
  byte operatingMode = MITSUBISHI_AIRCON1_MODE_HEAT;
  byte fanSpeed = MITSUBISHI_AIRCON1_FAN_AUTO;
  byte temperature = 23;

  if (powerModeCmd == 0)
  {
    powerMode = MITSUBISHI_AIRCON1_MODE_OFF;
  }

  switch (operatingModeCmd)
  {
    case MODE_AUTO:
      operatingMode = MITSUBISHI_AIRCON1_MODE_AUTO;
      break;
    case MODE_HEAT:
      operatingMode = MITSUBISHI_AIRCON1_MODE_HEAT;
      break;
    case MODE_COOL:
      operatingMode = MITSUBISHI_AIRCON1_MODE_COOL;
      break;
    case MODE_DRY:
      operatingMode = MITSUBISHI_AIRCON1_MODE_DRY;
      break;
    case MODE_FAN:
      operatingMode = MITSUBISHI_AIRCON1_MODE_COOL;
      // Temperature needs to be set to 31 degrees for 'simulated' FAN mode
      temperatureCmd = 31;
      break;
    case MODE_MAINT: // Maintenance mode is just the heat mode at +10, FAN5
      if (_mitsubishiModel == MITSUBISHI_FE) {
        operatingMode |= MITSUBISHI_AIRCON1_MODE_HEAT;
        temperature = 10; // Default to +10 degrees
        fanSpeedCmd = FAN_AUTO;
      }
      break;
  }

  switch (fanSpeedCmd)
  {
    case FAN_AUTO:
      fanSpeed = MITSUBISHI_AIRCON1_FAN_AUTO;
      break;
    case FAN_1:
      fanSpeed = MITSUBISHI_AIRCON1_FAN1;
      break;
    case FAN_2:
      fanSpeed = MITSUBISHI_AIRCON1_FAN2;
      break;
    case FAN_3:
      fanSpeed = MITSUBISHI_AIRCON1_FAN3;
      break;
    case FAN_4:
      fanSpeed = MITSUBISHI_AIRCON1_FAN4;
      break;
  }

  if ( temperatureCmd > 16 && temperatureCmd < 32)
  {
    temperature = temperatureCmd;
  }

  sendMitsubishi(IR, powerMode, operatingMode, fanSpeed, temperature, 0, 0);
}

void MitsubishiHeatpumpIR::sendMitsubishi(IRSender& IR, byte powerMode, byte operatingMode, byte fanSpeed, byte temperature, byte swingVCmd, byte swingHCmd)
{
  byte MitsubishiTemplate[] = { 0x23, 0xCB, 0x26, 0x01, 0x00, 0x20, 0x48, 0x00, 0xC0, 0x7A, 0x61, 0x00, 0x00, 0x00, 0x10, 0x40, 0x00, 0x00 };
  //                            0     1     2     3     4     5     6     7     8     9     10    11    12    13    14    15    16    17

  byte checksum = 0x00;

  // Set the operatingmode on the template message
  MitsubishiTemplate[5] = powerMode;
  MitsubishiTemplate[6] = operatingMode;

  // Set the temperature on the template message
  if (temperature == 10) {
    MitsubishiTemplate[7] = 0x00; // Maintenance mode
    MitsubishiTemplate[15] = 0x20; // This seems to be set to 0x20 on maintenance mode
  } else {
    MitsubishiTemplate[7] = temperature - 16;
  }

  // Set the operatingmode on the template message
  MitsubishiTemplate[9] = fanSpeed;

  // Calculate the checksum
  for (int i=0; i<17; i++) {
    checksum += MitsubishiTemplate[i];
  }

  MitsubishiTemplate[17] = checksum;

  // 40 kHz PWM frequency
  IR.setFrequency(40);

  // The Mitsubishi data is repeated twice
  for (int j=0; j<2; j++) {
    // Header
    IR.mark(MITSUBISHI_AIRCON1_HDR_MARK);
    IR.space(MITSUBISHI_AIRCON1_HDR_SPACE);

    // Data
    for (int i=0; i<sizeof(MitsubishiTemplate); i++) {
      IR.sendIRByte(MitsubishiTemplate[i], MITSUBISHI_AIRCON1_BIT_MARK, MITSUBISHI_AIRCON1_ZERO_SPACE, MITSUBISHI_AIRCON1_ONE_SPACE);
    }

    // Pause between the first and the second data burst
    if (j == 0) {
      IR.mark(MITSUBISHI_AIRCON1_BIT_MARK);
      IR.space(MITSUBISHI_AIRCON1_MSG_SPACE);
    }
  }

  // End mark
  IR.mark(MITSUBISHI_AIRCON1_BIT_MARK);
  IR.space(0);
}