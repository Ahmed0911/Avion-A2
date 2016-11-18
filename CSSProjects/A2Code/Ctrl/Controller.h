//
// File: Controller.h
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
#ifndef RTW_HEADER_Controller_h_
#define RTW_HEADER_Controller_h_
#include <math.h>
#ifndef Controller_COMMON_INCLUDES_
# define Controller_COMMON_INCLUDES_
#include "rtwtypes.h"
#endif                                 // Controller_COMMON_INCLUDES_

// Macros for accessing real-time model data structure
#ifndef rtmGetErrorStatus
# define rtmGetErrorStatus(rtm)        ((rtm)->errorStatus)
#endif

#ifndef rtmSetErrorStatus
# define rtmSetErrorStatus(rtm, val)   ((rtm)->errorStatus = (val))
#endif

// Forward declaration for rtModel
typedef struct tag_RTM RT_MODEL;

// Block signals and states (auto storage) for system '<Root>'
typedef struct {
  real32_T Delay_DSTATE[100];          // '<S2>/Delay'
  real32_T DiscreteFIRFilter_states[99];// '<S2>/Discrete FIR Filter'
  real32_T PWM1;                       // '<S1>/Merge'
  real32_T PWM2;                       // '<S1>/Merge'
  real32_T PWM3;                       // '<S1>/Merge'
  real32_T PWM4;                       // '<S1>/Merge'
  real32_T Integrator_DSTATE;          // '<S17>/Integrator'
  real32_T Filter_DSTATE;              // '<S17>/Filter'
  real32_T Integrator_DSTATE_n;        // '<S19>/Integrator'
  real32_T Filter_DSTATE_f;            // '<S19>/Filter'
  int32_T DiscreteFIRFilter_circBuf;   // '<S2>/Discrete FIR Filter'
  uint8_T is_active_c3_Controller;     // '<S1>/Chart'
  uint8_T is_SystemLogic;              // '<S1>/Chart'
  uint8_T is_LEDs;                     // '<S1>/Chart'
  uint8_T ModeOld;                     // '<S1>/Chart'
  uint8_T temporalCounter_i1;          // '<S1>/Chart'
  boolean_T Off;                       // '<S1>/Chart'
  boolean_T ManualMode;                // '<S1>/Chart'
  boolean_T AttitudeMode;              // '<S1>/Chart'
  boolean_T ArcadeMode;                // '<S1>/Chart'
  boolean_T AttitudeMode_MODE;         // '<S1>/AttitudeMode'
} DW;

// External inputs (root inport signals with auto storage)
typedef struct {
  uint32_T Mode;                       // '<Root>/Mode'
  real32_T Thr;                        // '<Root>/Thr'
  real32_T Aileron;                    // '<Root>/Aileron'
  real32_T Elevator;                   // '<Root>/Elevator'
  real32_T Rudder;                     // '<Root>/Rudder'
  real32_T Pressure0;                  // '<Root>/Pressure0'
  real32_T RPY[3];                     // '<Root>/RPY'
  real32_T dRPY[3];                    // '<Root>/dRPY'
  real32_T PressureAbs;                // '<Root>/PressureAbs'
  real32_T FlatXe[2];                  // '<Root>/FlatXe'
  real32_T FlatVe[3];                  // '<Root>/FlatVe'
} ExtU;

// External outputs (root outports fed by signals with auto storage)
typedef struct {
  real32_T PWM1;                       // '<Root>/PWM1'
  real32_T PWM2;                       // '<Root>/PWM2'
  real32_T PWM3;                       // '<Root>/PWM3'
  real32_T PWM4;                       // '<Root>/PWM4'
  boolean_T GreenLED;                  // '<Root>/GreenLED'
  uint8_T ActualMode;                  // '<Root>/ActualMode'
  real32_T Altitude;                   // '<Root>/Altitude'
  real32_T VertSpeed;                  // '<Root>/VertSpeed'
} ExtY;

// Real-time Model Data Structure
struct tag_RTM {
  const char_T * volatile errorStatus;
};

#ifdef __cplusplus

extern "C" {

#endif

#ifdef __cplusplus

}
#endif

//
//  Exported Global Parameters
//
//  Note: Exported global parameters are tunable parameters with an exported
//  global storage class designation.  Code generation will declare the memory for
//  these parameters and exports their symbols.
//

extern real32_T PitchKd;               // Variable: PitchKd
                                       //  Referenced by: '<S17>/Derivative Gain'

extern real32_T PitchKi;               // Variable: PitchKi
                                       //  Referenced by: '<S17>/Integral Gain'

extern real32_T PitchKp;               // Variable: PitchKp
                                       //  Referenced by: '<S17>/Proportional Gain'

extern real32_T PitchMax;              // Variable: PitchMax
                                       //  Referenced by: '<S11>/Scale'

extern real32_T RollKd;                // Variable: RollKd
                                       //  Referenced by: '<S19>/Derivative Gain'

extern real32_T RollKi;                // Variable: RollKi
                                       //  Referenced by: '<S19>/Integral Gain'

extern real32_T RollKp;                // Variable: RollKp
                                       //  Referenced by: '<S19>/Proportional Gain'

extern real32_T RollMax;               // Variable: RollMax
                                       //  Referenced by: '<S12>/Scale'


// Class declaration for model Controller
class ControllerModelClass {
  // public data and function members
 public:
  // External inputs
  ExtU rtU;

  // External outputs
  ExtY rtY;

  // Model entry point functions

  // model initialize function
  void initialize();

  // model step function
  void step();

  // Constructor
  ControllerModelClass();

  // Destructor
  ~ControllerModelClass();

  // Real-Time Model get method
  RT_MODEL * getRTM();

  // private data and function members
 private:
  // Block signals and states
  DW rtDW;

  // Real-Time Model
  RT_MODEL rtM;

  // private member function(s) for subsystem '<Root>'
  void SelectMode(uint8_T *Mode);
};

//-
//  These blocks were eliminated from the model due to optimizations:
//
//  Block '<S3>/Gain' : Eliminated nontunable gain of 1
//  Block '<S3>/Gain3' : Eliminated nontunable gain of 1
//  Block '<S6>/Gain' : Eliminated nontunable gain of 1
//  Block '<S6>/Gain1' : Eliminated nontunable gain of 1
//  Block '<S6>/Gain2' : Eliminated nontunable gain of 1
//  Block '<S6>/Gain3' : Eliminated nontunable gain of 1


//-
//  The generated code includes comments that allow you to trace directly
//  back to the appropriate location in the model.  The basic format
//  is <system>/block_name, where system is the system number (uniquely
//  assigned by Simulink) and block_name is the name of the block.
//
//  Note that this particular code originates from a subsystem build,
//  and has its own system numbers different from the parent model.
//  Refer to the system hierarchy for this subsystem below, and use the
//  MATLAB hilite_system command to trace the generated code back
//  to the parent model.  For example,
//
//  hilite_system('A2Model/Controller')    - opens subsystem A2Model/Controller
//  hilite_system('A2Model/Controller/Kp') - opens and selects block Kp
//
//  Here is the system hierarchy for this model
//
//  '<Root>' : 'A2Model'
//  '<S1>'   : 'A2Model/Controller'
//  '<S2>'   : 'A2Model/Controller/AltCalculator'
//  '<S3>'   : 'A2Model/Controller/AttitudeMode'
//  '<S4>'   : 'A2Model/Controller/Chart'
//  '<S5>'   : 'A2Model/Controller/Input Scaler'
//  '<S6>'   : 'A2Model/Controller/ManualCtrl'
//  '<S7>'   : 'A2Model/Controller/NextMode2'
//  '<S8>'   : 'A2Model/Controller/Off'
//  '<S9>'   : 'A2Model/Controller/AltCalculator/MATLAB Function'
//  '<S10>'  : 'A2Model/Controller/AttitudeMode/Mapper'
//  '<S11>'  : 'A2Model/Controller/AttitudeMode/PitchController'
//  '<S12>'  : 'A2Model/Controller/AttitudeMode/RollController'
//  '<S13>'  : 'A2Model/Controller/AttitudeMode/Mapper/LimitAndRescale'
//  '<S14>'  : 'A2Model/Controller/AttitudeMode/Mapper/LimitAndRescale1'
//  '<S15>'  : 'A2Model/Controller/AttitudeMode/Mapper/LimitAndRescale2'
//  '<S16>'  : 'A2Model/Controller/AttitudeMode/Mapper/LimitAndRescale3'
//  '<S17>'  : 'A2Model/Controller/AttitudeMode/PitchController/Discrete PID Controller'
//  '<S18>'  : 'A2Model/Controller/AttitudeMode/PitchController/Discrete PID Controller/Clamping circuit'
//  '<S19>'  : 'A2Model/Controller/AttitudeMode/RollController/Discrete PID Controller'
//  '<S20>'  : 'A2Model/Controller/AttitudeMode/RollController/Discrete PID Controller/Clamping circuit'
//  '<S21>'  : 'A2Model/Controller/ManualCtrl/Mapper'
//  '<S22>'  : 'A2Model/Controller/ManualCtrl/Mapper/LimitAndRescale'
//  '<S23>'  : 'A2Model/Controller/ManualCtrl/Mapper/LimitAndRescale1'
//  '<S24>'  : 'A2Model/Controller/ManualCtrl/Mapper/LimitAndRescale2'
//  '<S25>'  : 'A2Model/Controller/ManualCtrl/Mapper/LimitAndRescale3'
//  '<S26>'  : 'A2Model/Controller/NextMode2/Mapper'
//  '<S27>'  : 'A2Model/Controller/NextMode2/Mapper/LimitAndRescale'
//  '<S28>'  : 'A2Model/Controller/NextMode2/Mapper/LimitAndRescale1'
//  '<S29>'  : 'A2Model/Controller/NextMode2/Mapper/LimitAndRescale2'
//  '<S30>'  : 'A2Model/Controller/NextMode2/Mapper/LimitAndRescale3'
//  '<S31>'  : 'A2Model/Controller/Off/Mapper'
//  '<S32>'  : 'A2Model/Controller/Off/Mapper/LimitAndRescale'
//  '<S33>'  : 'A2Model/Controller/Off/Mapper/LimitAndRescale1'
//  '<S34>'  : 'A2Model/Controller/Off/Mapper/LimitAndRescale2'
//  '<S35>'  : 'A2Model/Controller/Off/Mapper/LimitAndRescale3'

#endif                                 // RTW_HEADER_Controller_h_

//
// File trailer for generated code.
//
// [EOF]
//
