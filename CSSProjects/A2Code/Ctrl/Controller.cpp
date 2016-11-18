//
// File: Controller.cpp
//
// Code generated for Simulink model 'Controller'.
//
// Model version                  : 1.115
// Simulink Coder version         : 8.9 (R2015b) 13-Aug-2015
// C/C++ source code generated on : Thu Oct 27 20:13:30 2016
//
// Target selection: ert.tlc
// Embedded hardware selection: ARM Compatible->ARM Cortex
// Code generation objectives:
//    1. Execution efficiency
//    2. RAM efficiency
// Validation result: Not run
//
#include "Controller.h"

// Named constants for Chart: '<S1>/Chart'
#define IN_ActiveModes                 ((uint8_T)1U)
#define IN_GreenOff                    ((uint8_T)1U)
#define IN_GreenOn                     ((uint8_T)2U)
#define IN_Off                         ((uint8_T)2U)

// Exported block parameters
real32_T PitchKd = 0.0F;               // Variable: PitchKd
                                       //  Referenced by: '<S17>/Derivative Gain'

real32_T PitchKi = 0.0F;               // Variable: PitchKi
                                       //  Referenced by: '<S17>/Integral Gain'

real32_T PitchKp = 0.1F;               // Variable: PitchKp
                                       //  Referenced by: '<S17>/Proportional Gain'

real32_T PitchMax = 30.0F;             // Variable: PitchMax
                                       //  Referenced by: '<S11>/Scale'

real32_T RollKd = 0.0F;                // Variable: RollKd
                                       //  Referenced by: '<S19>/Derivative Gain'

real32_T RollKi = 0.0F;                // Variable: RollKi
                                       //  Referenced by: '<S19>/Integral Gain'

real32_T RollKp = 0.1F;                // Variable: RollKp
                                       //  Referenced by: '<S19>/Proportional Gain'

real32_T RollMax = 45.0F;              // Variable: RollMax
                                       //  Referenced by: '<S12>/Scale'


// Function for Chart: '<S1>/Chart'
void ControllerModelClass::SelectMode(uint8_T *Mode)
{
  // Inport: '<Root>/Mode'
  // MATLAB Function 'SelectMode': '<S4>:14'
  // Graphical Function 'SelectMode': '<S4>:14'
  // '<S4>:37:1'
  if (rtU.Mode < 600U) {
    // '<S4>:38:1'
    *Mode = 1U;
  } else {
    // '<S4>:41:1'
    if (rtU.Mode < 1300U) {
      // '<S4>:42:1'
      *Mode = 2U;
    } else {
      // '<S4>:43:1'
      *Mode = 3U;
    }
  }

  // End of Inport: '<Root>/Mode'
}

// Model step function
void ControllerModelClass::step()
{
  int32_T j;
  real32_T rtb_Thr;
  real32_T rtb_Aileron;
  real32_T rtb_Elevator;
  real32_T rtb_Rudder;
  real32_T rtb_IntegralGain;
  real32_T rtb_SignDeltaU;
  real32_T rtb_FilterCoefficient;
  real32_T rtb_IntegralGain_k;
  real32_T rtb_SignDeltaU_b;
  real32_T rtb_FilterCoefficient_p;
  boolean_T rtb_NotEqual;
  boolean_T rtb_NotEqual_c;
  uint8_T Mode;
  real32_T rtb_SignDeltaU_j;
  int8_T rtb_SignDeltaU_1;

  // Outputs for Atomic SubSystem: '<Root>/Controller'
  // Chart: '<S1>/Chart'
  // Gateway: Controller/Chart
  if (rtDW.temporalCounter_i1 < 127U) {
    rtDW.temporalCounter_i1++;
  }

  // During: Controller/Chart
  if (rtDW.is_active_c3_Controller == 0U) {
    // Entry: Controller/Chart
    rtDW.is_active_c3_Controller = 1U;

    // Entry Internal: Controller/Chart
    // Entry Internal 'LEDs': '<S4>:1'
    // Transition: '<S4>:7'
    rtDW.is_LEDs = IN_GreenOn;
    rtDW.temporalCounter_i1 = 0U;

    // Outport: '<Root>/GreenLED'
    // Entry 'GreenOn': '<S4>:2'
    rtY.GreenLED = true;

    // Entry 'SystemLogic': '<S4>:85'
    // Entry Internal 'SystemLogic': '<S4>:85'
    // Transition: '<S4>:13'
    rtDW.is_SystemLogic = IN_Off;
    rtDW.Off = true;

    // Outport: '<Root>/ActualMode'
    // Entry 'Off': '<S4>:51'
    rtY.ActualMode = 0U;
  } else {
    // During 'LEDs': '<S4>:1'
    if (rtDW.is_LEDs == IN_GreenOff) {
      // During 'GreenOff': '<S4>:6'
      if (rtDW.temporalCounter_i1 >= 100) {
        // Transition: '<S4>:5'
        rtDW.is_LEDs = IN_GreenOn;
        rtDW.temporalCounter_i1 = 0U;

        // Outport: '<Root>/GreenLED'
        // Entry 'GreenOn': '<S4>:2'
        rtY.GreenLED = true;
      }
    } else {
      // During 'GreenOn': '<S4>:2'
      if (rtDW.temporalCounter_i1 >= 100) {
        // Transition: '<S4>:4'
        rtDW.is_LEDs = IN_GreenOff;
        rtDW.temporalCounter_i1 = 0U;

        // Outport: '<Root>/GreenLED'
        // Entry 'GreenOff': '<S4>:6'
        rtY.GreenLED = false;
      }
    }

    // During 'SystemLogic': '<S4>:85'
    SelectMode(&Mode);
    if (rtDW.is_SystemLogic == IN_ActiveModes) {
      // During 'ActiveModes': '<S4>:53'
      if (Mode != rtDW.ModeOld) {
        // Transition: '<S4>:50'
        // Exit Internal 'ActiveModes': '<S4>:53'
        rtDW.ArcadeMode = false;
        rtDW.AttitudeMode = false;
        rtDW.ManualMode = false;
        rtDW.is_SystemLogic = IN_Off;
        rtDW.Off = true;

        // Outport: '<Root>/ActualMode'
        // Entry 'Off': '<S4>:51'
        rtY.ActualMode = 0U;
      }
    } else {
      // During 'Off': '<S4>:51'
      if (Mode == 1) {
        // Transition: '<S4>:12'
        rtDW.Off = false;
        rtDW.is_SystemLogic = IN_ActiveModes;

        // Entry 'ActiveModes': '<S4>:53'
        rtDW.ModeOld = 1U;
        rtDW.ManualMode = true;

        // Outport: '<Root>/ActualMode'
        // Entry 'ManualMode': '<S4>:11'
        rtY.ActualMode = 1U;
      } else if (Mode == 2) {
        // Transition: '<S4>:87'
        rtDW.Off = false;
        rtDW.is_SystemLogic = IN_ActiveModes;

        // Entry 'ActiveModes': '<S4>:53'
        rtDW.ModeOld = 2U;
        rtDW.AttitudeMode = true;

        // Outport: '<Root>/ActualMode'
        // Entry 'AttitudeMode': '<S4>:47'
        rtY.ActualMode = 2U;
      } else {
        if (Mode == 3) {
          // Transition: '<S4>:246'
          rtDW.Off = false;
          rtDW.is_SystemLogic = IN_ActiveModes;

          // Entry 'ActiveModes': '<S4>:53'
          rtDW.ModeOld = 3U;
          rtDW.ArcadeMode = true;

          // Outport: '<Root>/ActualMode'
          // Entry 'ArcadeMode': '<S4>:245'
          rtY.ActualMode = 3U;
        }
      }
    }
  }

  // End of Chart: '<S1>/Chart'

  // Gain: '<S5>/Gain' incorporates:
  //   Constant: '<S5>/Constant'
  //   Inport: '<Root>/Thr'
  //   Sum: '<S5>/Subtract'

  rtb_Thr = (rtU.Thr - 350.0F) * 0.000740740739F;

  // Saturate: '<S5>/Saturation'
  if (rtb_Thr > 1.0F) {
    rtb_Thr = 1.0F;
  } else {
    if (rtb_Thr < 0.0F) {
      rtb_Thr = 0.0F;
    }
  }

  // End of Saturate: '<S5>/Saturation'

  // Gain: '<S5>/Gain1' incorporates:
  //   Constant: '<S5>/Constant1'
  //   Inport: '<Root>/Aileron'
  //   Sum: '<S5>/Subtract1'

  rtb_Aileron = (rtU.Aileron - 1024.0F) * 0.00147929F;

  // Saturate: '<S5>/Saturation1'
  if (rtb_Aileron > 1.0F) {
    rtb_Aileron = 1.0F;
  } else {
    if (rtb_Aileron < -1.0F) {
      rtb_Aileron = -1.0F;
    }
  }

  // End of Saturate: '<S5>/Saturation1'

  // Gain: '<S5>/Gain2' incorporates:
  //   Constant: '<S5>/Constant2'
  //   Inport: '<Root>/Elevator'
  //   Sum: '<S5>/Subtract2'

  rtb_Elevator = (rtU.Elevator - 1024.0F) * -0.00147929F;

  // Saturate: '<S5>/Saturation2'
  if (rtb_Elevator > 1.0F) {
    rtb_Elevator = 1.0F;
  } else {
    if (rtb_Elevator < -1.0F) {
      rtb_Elevator = -1.0F;
    }
  }

  // End of Saturate: '<S5>/Saturation2'

  // Gain: '<S5>/Gain3' incorporates:
  //   Constant: '<S5>/Constant3'
  //   Inport: '<Root>/Rudder'
  //   Sum: '<S5>/Subtract3'

  rtb_Rudder = (rtU.Rudder - 1024.0F) * 0.00147929F;

  // Saturate: '<S5>/Saturation3'
  if (rtb_Rudder > 1.0F) {
    rtb_Rudder = 1.0F;
  } else {
    if (rtb_Rudder < -1.0F) {
      rtb_Rudder = -1.0F;
    }
  }

  // End of Saturate: '<S5>/Saturation3'

  // Outputs for Enabled SubSystem: '<S1>/AttitudeMode' incorporates:
  //   EnablePort: '<S3>/Enable'

  if (rtDW.AttitudeMode) {
    if (!rtDW.AttitudeMode_MODE) {
      // InitializeConditions for DiscreteIntegrator: '<S17>/Integrator'
      rtDW.Integrator_DSTATE = 0.0F;

      // InitializeConditions for DiscreteIntegrator: '<S17>/Filter'
      rtDW.Filter_DSTATE = 0.0F;

      // InitializeConditions for DiscreteIntegrator: '<S19>/Integrator'
      rtDW.Integrator_DSTATE_n = 0.0F;

      // InitializeConditions for DiscreteIntegrator: '<S19>/Filter'
      rtDW.Filter_DSTATE_f = 0.0F;
      rtDW.AttitudeMode_MODE = true;
    }

    // Sum: '<S11>/Sum' incorporates:
    //   Gain: '<S11>/Scale'
    //   Inport: '<Root>/RPY'

    rtb_IntegralGain = PitchMax * rtb_Elevator - rtU.RPY[1];

    // Gain: '<S17>/Filter Coefficient' incorporates:
    //   DiscreteIntegrator: '<S17>/Filter'
    //   Gain: '<S17>/Derivative Gain'
    //   Sum: '<S17>/SumD'

    rtb_FilterCoefficient = (PitchKd * rtb_IntegralGain - rtDW.Filter_DSTATE) *
      50.0F;

    // Sum: '<S17>/Sum' incorporates:
    //   DiscreteIntegrator: '<S17>/Integrator'
    //   Gain: '<S17>/Proportional Gain'

    rtb_SignDeltaU = (PitchKp * rtb_IntegralGain + rtDW.Integrator_DSTATE) +
      rtb_FilterCoefficient;

    // Sum: '<S13>/Subtract' incorporates:
    //   Constant: '<S13>/Constant'
    //   Gain: '<S13>/Gain'
    //   Saturate: '<S13>/Limit3'

    rtDW.PWM1 = 1000.0F * rtb_Thr + 1000.0F;

    // Sum: '<S12>/Sum' incorporates:
    //   Gain: '<S12>/Scale'
    //   Inport: '<Root>/RPY'

    rtb_IntegralGain_k = RollMax * rtb_Aileron - rtU.RPY[0];

    // Gain: '<S19>/Filter Coefficient' incorporates:
    //   DiscreteIntegrator: '<S19>/Filter'
    //   Gain: '<S19>/Derivative Gain'
    //   Sum: '<S19>/SumD'

    rtb_FilterCoefficient_p = (RollKd * rtb_IntegralGain_k -
      rtDW.Filter_DSTATE_f) * 50.0F;

    // Sum: '<S19>/Sum' incorporates:
    //   DiscreteIntegrator: '<S19>/Integrator'
    //   Gain: '<S19>/Proportional Gain'

    rtb_SignDeltaU_b = (RollKp * rtb_IntegralGain_k + rtDW.Integrator_DSTATE_n)
      + rtb_FilterCoefficient_p;

    // Saturate: '<S19>/Saturate'
    if (rtb_SignDeltaU_b > 1.0F) {
      rtb_SignDeltaU_j = 1.0F;
    } else if (rtb_SignDeltaU_b < -1.0F) {
      rtb_SignDeltaU_j = -1.0F;
    } else {
      rtb_SignDeltaU_j = rtb_SignDeltaU_b;
    }

    // Sum: '<S14>/Subtract' incorporates:
    //   Constant: '<S14>/Constant'
    //   Gain: '<S14>/Gain'
    //   Saturate: '<S14>/Limit3'
    //   Saturate: '<S19>/Saturate'

    rtDW.PWM2 = 500.0F * rtb_SignDeltaU_j + 1500.0F;

    // Saturate: '<S17>/Saturate' incorporates:
    //   DeadZone: '<S18>/DeadZone'

    if (rtb_SignDeltaU > 1.0F) {
      rtb_SignDeltaU_j = 1.0F;
      rtb_SignDeltaU--;
    } else {
      if (rtb_SignDeltaU < -1.0F) {
        rtb_SignDeltaU_j = -1.0F;
      } else {
        rtb_SignDeltaU_j = rtb_SignDeltaU;
      }

      if (rtb_SignDeltaU >= -1.0F) {
        rtb_SignDeltaU = 0.0F;
      } else {
        rtb_SignDeltaU -= -1.0F;
      }
    }

    // Sum: '<S15>/Subtract' incorporates:
    //   Constant: '<S15>/Constant'
    //   Gain: '<S10>/Gain2'
    //   Gain: '<S15>/Gain'
    //   Saturate: '<S15>/Limit3'
    //   Saturate: '<S17>/Saturate'

    rtDW.PWM3 = 500.0F * -rtb_SignDeltaU_j + 1500.0F;

    // Sum: '<S16>/Subtract' incorporates:
    //   Constant: '<S16>/Constant'
    //   Gain: '<S16>/Gain'
    //   Saturate: '<S16>/Limit3'

    rtDW.PWM4 = 500.0F * rtb_Rudder + 1500.0F;

    // RelationalOperator: '<S18>/NotEqual'
    rtb_NotEqual = (0.0F != rtb_SignDeltaU);

    // Signum: '<S18>/SignDeltaU'
    if (rtb_SignDeltaU < 0.0F) {
      rtb_SignDeltaU = -1.0F;
    } else {
      if (rtb_SignDeltaU > 0.0F) {
        rtb_SignDeltaU = 1.0F;
      }
    }

    // End of Signum: '<S18>/SignDeltaU'

    // Gain: '<S17>/Integral Gain'
    rtb_IntegralGain *= PitchKi;

    // DeadZone: '<S20>/DeadZone'
    if (rtb_SignDeltaU_b > 1.0F) {
      rtb_SignDeltaU_b--;
    } else if (rtb_SignDeltaU_b >= -1.0F) {
      rtb_SignDeltaU_b = 0.0F;
    } else {
      rtb_SignDeltaU_b -= -1.0F;
    }

    // End of DeadZone: '<S20>/DeadZone'

    // RelationalOperator: '<S20>/NotEqual'
    rtb_NotEqual_c = (0.0F != rtb_SignDeltaU_b);

    // Signum: '<S20>/SignDeltaU'
    if (rtb_SignDeltaU_b < 0.0F) {
      rtb_SignDeltaU_b = -1.0F;
    } else {
      if (rtb_SignDeltaU_b > 0.0F) {
        rtb_SignDeltaU_b = 1.0F;
      }
    }

    // End of Signum: '<S20>/SignDeltaU'

    // Gain: '<S19>/Integral Gain'
    rtb_IntegralGain_k *= RollKi;

    // DataTypeConversion: '<S18>/DataTypeConv1'
    if (rtb_SignDeltaU < 128.0F) {
      rtb_SignDeltaU_1 = (int8_T)rtb_SignDeltaU;
    } else {
      rtb_SignDeltaU_1 = MAX_int8_T;
    }

    // End of DataTypeConversion: '<S18>/DataTypeConv1'

    // Signum: '<S18>/SignPreIntegrator'
    if (rtb_IntegralGain < 0.0F) {
      rtb_SignDeltaU = -1.0F;
    } else if (rtb_IntegralGain > 0.0F) {
      rtb_SignDeltaU = 1.0F;
    } else {
      rtb_SignDeltaU = rtb_IntegralGain;
    }

    // Switch: '<S17>/Switch' incorporates:
    //   Constant: '<S17>/Constant'
    //   DataTypeConversion: '<S18>/DataTypeConv2'
    //   Logic: '<S18>/AND'
    //   RelationalOperator: '<S18>/Equal'
    //   Signum: '<S18>/SignPreIntegrator'

    if (rtb_NotEqual && (rtb_SignDeltaU_1 == (int8_T)rtb_SignDeltaU)) {
      rtb_IntegralGain = 0.0F;
    }

    // End of Switch: '<S17>/Switch'

    // Update for DiscreteIntegrator: '<S17>/Integrator'
    rtDW.Integrator_DSTATE += 0.0025F * rtb_IntegralGain;

    // Update for DiscreteIntegrator: '<S17>/Filter'
    rtDW.Filter_DSTATE += 0.0025F * rtb_FilterCoefficient;

    // DataTypeConversion: '<S20>/DataTypeConv1'
    if (rtb_SignDeltaU_b < 128.0F) {
      rtb_SignDeltaU_1 = (int8_T)rtb_SignDeltaU_b;
    } else {
      rtb_SignDeltaU_1 = MAX_int8_T;
    }

    // End of DataTypeConversion: '<S20>/DataTypeConv1'

    // Signum: '<S20>/SignPreIntegrator'
    if (rtb_IntegralGain_k < 0.0F) {
      rtb_IntegralGain = -1.0F;
    } else if (rtb_IntegralGain_k > 0.0F) {
      rtb_IntegralGain = 1.0F;
    } else {
      rtb_IntegralGain = rtb_IntegralGain_k;
    }

    // Switch: '<S19>/Switch' incorporates:
    //   Constant: '<S19>/Constant'
    //   DataTypeConversion: '<S20>/DataTypeConv2'
    //   Logic: '<S20>/AND'
    //   RelationalOperator: '<S20>/Equal'
    //   Signum: '<S20>/SignPreIntegrator'

    if (rtb_NotEqual_c && (rtb_SignDeltaU_1 == (int8_T)rtb_IntegralGain)) {
      rtb_IntegralGain_k = 0.0F;
    }

    // End of Switch: '<S19>/Switch'

    // Update for DiscreteIntegrator: '<S19>/Integrator'
    rtDW.Integrator_DSTATE_n += 0.0025F * rtb_IntegralGain_k;

    // Update for DiscreteIntegrator: '<S19>/Filter'
    rtDW.Filter_DSTATE_f += 0.0025F * rtb_FilterCoefficient_p;
  } else {
    if (rtDW.AttitudeMode_MODE) {
      rtDW.AttitudeMode_MODE = false;
    }
  }

  // End of Outputs for SubSystem: '<S1>/AttitudeMode'

  // Outputs for Enabled SubSystem: '<S1>/ManualCtrl' incorporates:
  //   EnablePort: '<S6>/Enable'

  if (rtDW.ManualMode) {
    // Sum: '<S22>/Subtract' incorporates:
    //   Constant: '<S22>/Constant'
    //   Gain: '<S22>/Gain'
    //   Saturate: '<S22>/Limit3'

    rtDW.PWM1 = 1000.0F * rtb_Thr + 1000.0F;

    // Sum: '<S23>/Subtract' incorporates:
    //   Constant: '<S23>/Constant'
    //   Gain: '<S23>/Gain'
    //   Saturate: '<S23>/Limit3'

    rtDW.PWM2 = 500.0F * rtb_Aileron + 1500.0F;

    // Sum: '<S24>/Subtract' incorporates:
    //   Constant: '<S24>/Constant'
    //   Gain: '<S21>/Gain2'
    //   Gain: '<S24>/Gain'
    //   Saturate: '<S24>/Limit3'

    rtDW.PWM3 = 500.0F * -rtb_Elevator + 1500.0F;

    // Sum: '<S25>/Subtract' incorporates:
    //   Constant: '<S25>/Constant'
    //   Gain: '<S25>/Gain'
    //   Saturate: '<S25>/Limit3'

    rtDW.PWM4 = 500.0F * rtb_Rudder + 1500.0F;
  }

  // End of Outputs for SubSystem: '<S1>/ManualCtrl'

  // Outputs for Enabled SubSystem: '<S1>/Off' incorporates:
  //   EnablePort: '<S8>/Enable'

  if (rtDW.Off) {
    // Sum: '<S32>/Subtract' incorporates:
    //   Constant: '<S32>/Constant'

    rtDW.PWM1 = 1000.0F;

    // Sum: '<S33>/Subtract' incorporates:
    //   Constant: '<S33>/Constant'

    rtDW.PWM2 = 1500.0F;

    // Sum: '<S34>/Subtract' incorporates:
    //   Constant: '<S34>/Constant'

    rtDW.PWM3 = 1500.0F;

    // Sum: '<S35>/Subtract' incorporates:
    //   Constant: '<S35>/Constant'

    rtDW.PWM4 = 1500.0F;
  }

  // End of Outputs for SubSystem: '<S1>/Off'

  // MATLAB Function: '<S2>/MATLAB Function' incorporates:
  //   Inport: '<Root>/Pressure0'
  //   Inport: '<Root>/PressureAbs'

  // MATLAB Function 'Controller/AltCalculator/MATLAB Function': '<S9>:1'
  if ((rtU.PressureAbs < 10000.0F) || (rtU.PressureAbs > 150000.0F)) {
    // '<S9>:1:3'
    //  pressure not valid?
    // '<S9>:1:4'
    rtb_Thr = 0.0F;
  } else {
    // '<S9>:1:6'
    rtb_Thr = (1.0F - (real32_T)pow((real_T)(rtU.PressureAbs / rtU.Pressure0),
                (real_T)0.190294951F)) * 44330.0F;
  }

  // End of MATLAB Function: '<S2>/MATLAB Function'

  // DiscreteFir: '<S2>/Discrete FIR Filter'
  rtb_Elevator = rtb_Thr * 0.01F;
  for (j = rtDW.DiscreteFIRFilter_circBuf; j < 99; j++) {
    rtb_Elevator += rtDW.DiscreteFIRFilter_states[j] * 0.01F;
  }

  for (j = 0; j < rtDW.DiscreteFIRFilter_circBuf; j++) {
    rtb_Elevator += rtDW.DiscreteFIRFilter_states[j] * 0.01F;
  }

  // End of DiscreteFir: '<S2>/Discrete FIR Filter'

  // Sum: '<S2>/Sum' incorporates:
  //   Delay: '<S2>/Delay'

  rtb_Aileron = rtb_Elevator - rtDW.Delay_DSTATE[0];

  // Outputs for Enabled SubSystem: '<S1>/NextMode2' incorporates:
  //   EnablePort: '<S7>/Enable'

  if (rtDW.ArcadeMode) {
    // Sum: '<S27>/Subtract' incorporates:
    //   Constant: '<S27>/Constant'

    rtDW.PWM1 = 1020.0F;

    // Sum: '<S28>/Subtract' incorporates:
    //   Constant: '<S28>/Constant'

    rtDW.PWM2 = 1500.0F;

    // Sum: '<S29>/Subtract' incorporates:
    //   Constant: '<S29>/Constant'

    rtDW.PWM3 = 1500.0F;

    // Sum: '<S30>/Subtract' incorporates:
    //   Constant: '<S30>/Constant'

    rtDW.PWM4 = 1500.0F;
  }

  // End of Outputs for SubSystem: '<S1>/NextMode2'

  // Update for Delay: '<S2>/Delay'
  for (j = 0; j < 99; j++) {
    rtDW.Delay_DSTATE[j] = rtDW.Delay_DSTATE[j + 1];
  }

  rtDW.Delay_DSTATE[99] = rtb_Elevator;

  // End of Update for Delay: '<S2>/Delay'

  // Update for DiscreteFir: '<S2>/Discrete FIR Filter'
  // Update circular buffer index
  rtDW.DiscreteFIRFilter_circBuf--;
  if (rtDW.DiscreteFIRFilter_circBuf < 0) {
    rtDW.DiscreteFIRFilter_circBuf = 98;
  }

  // Update circular buffer
  rtDW.DiscreteFIRFilter_states[rtDW.DiscreteFIRFilter_circBuf] = rtb_Thr;

  // End of Update for DiscreteFir: '<S2>/Discrete FIR Filter'
  // End of Outputs for SubSystem: '<Root>/Controller'

  // Outport: '<Root>/PWM1'
  rtY.PWM1 = rtDW.PWM1;

  // Outport: '<Root>/PWM2'
  rtY.PWM2 = rtDW.PWM2;

  // Outport: '<Root>/PWM3'
  rtY.PWM3 = rtDW.PWM3;

  // Outport: '<Root>/PWM4'
  rtY.PWM4 = rtDW.PWM4;

  // Outport: '<Root>/Altitude'
  rtY.Altitude = rtb_Elevator;

  // Outputs for Atomic SubSystem: '<Root>/Controller'
  // Outport: '<Root>/VertSpeed' incorporates:
  //   Gain: '<S2>/Gain'

  rtY.VertSpeed = 4.0F * rtb_Aileron;

  // End of Outputs for SubSystem: '<Root>/Controller'
}

// Model initialize function
void ControllerModelClass::initialize()
{
  // (no initialization code required)
}

// Constructor
ControllerModelClass::ControllerModelClass()
{
}

// Destructor
ControllerModelClass::~ControllerModelClass()
{
  // Currently there is no destructor body generated.
}

// Real-Time Model get method
RT_MODEL * ControllerModelClass::getRTM()
{
  return (&rtM);
}

//
// File trailer for generated code.
//
// [EOF]
//
