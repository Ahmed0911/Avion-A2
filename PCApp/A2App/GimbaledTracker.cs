using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WinEthApp
{
    // Base set params
    public enum eBaseSetState { NONE, GOZERO, ZEROWAIT, GOPOS, SET };

    public struct TrackerAngles
    {
        public int TiltCNT;                         /* '<Root>/Tilt' */
        public int PanCNT;                          /* '<Root>/Pan' */
    };

    class GimbaledTracker
    {
        struct ExtU
        {
            public double BaseRoll;                   /* '<Root>/BaseRPY' */
            public double BasePitch;                   /* '<Root>/BaseRPY' */
            public double BaseYaw;                   /* '<Root>/BaseRPY' */
            public double BaseX;
            public double BaseY;
            public double BaseZ;
            public double TargetX;
            public double TargetY;
            public double TargetZ;
        };

         struct ExtY
        {
            public double Tilt;                         /* '<Root>/Tilt' */
            public double Pan;                          /* '<Root>/Pan' */
        };

        // Target
        public double TargetN;
        public double TargetE;
        public double TargetAltitude;

        private int lastPanForWrap = 0;

        // Base
        public eBaseSetState BaseSet = eBaseSetState.NONE;
        private int BaseSetCounter;
        private int BaseSetPanRef;
        private int BaseSetTiltRef;
        private int BaseMotionSpeed = 20;
        private int BaseAntennaOffset = 135;
        public double BaseRoll;
        public double BasePitch;
        public double BaseYaw;

        public TrackerAngles Update(double baseRoll, double basePitch, double baseYaw)
        {
            TrackerAngles angles = new TrackerAngles();

            if (BaseSet == eBaseSetState.SET)
            {
                // Update Tracker Gimbal Data
                ExtU rtU = new ExtU();
                rtU.BaseX = 0; // base is at zero position (will change when GPS starts to work)
                rtU.BaseY = 0;
                rtU.BaseZ = 0;

                rtU.BaseRoll = BaseRoll;
                rtU.BasePitch = BasePitch;
                rtU.BaseYaw = BaseYaw;

                rtU.TargetX = TargetN;
                rtU.TargetY = TargetE;
                rtU.TargetZ = -TargetAltitude; // Z is down, invert altitude

                ExtY gimbaledAngles = TrackerGimbalCalculate(rtU);


                // convert to pan/tilt
                angles.PanCNT = (int)((gimbaledAngles.Pan + BaseAntennaOffset) / 360.0 * 2000); // 135 deg is antenna/yaw offset!
                angles.TiltCNT = (int)(gimbaledAngles.Tilt / 360 * 4818 + 1020); // TODO, convert

                // ADD wrap!
                while( (angles.PanCNT - lastPanForWrap) > 1000 ) angles.PanCNT-=2000;
                while( (angles.PanCNT - lastPanForWrap) < -1000 ) angles.PanCNT+=2000;
                lastPanForWrap = angles.PanCNT;
            }
            else if( BaseSet == eBaseSetState.GOZERO)
            {
                BaseSetPanRef = TrackCNT(BaseSetPanRef, 0, BaseMotionSpeed);
                BaseSetTiltRef = TrackCNT(BaseSetTiltRef, 1000, BaseMotionSpeed);
                angles.PanCNT = BaseSetPanRef;
                angles.TiltCNT = BaseSetTiltRef;

                if (BaseSetPanRef == 0 && BaseSetTiltRef == 1000)
                {
                    BaseSet = eBaseSetState.ZEROWAIT;
                    BaseSetCounter = 50;
                }
            }
            else if (BaseSet == eBaseSetState.ZEROWAIT)
            {
                if( --BaseSetCounter <= 0 )
                {
                    // lock Base RPY
                    BaseRoll = baseRoll;
                    BasePitch = basePitch;
                    BaseYaw = baseYaw;
                    BaseSet = eBaseSetState.GOPOS;
                }
                angles.PanCNT = BaseSetPanRef;
                angles.TiltCNT = BaseSetTiltRef;
            }
            else if (BaseSet == eBaseSetState.GOPOS)
            {
                int target = (int)(BaseAntennaOffset / 360.0 * 2000);
                BaseSetPanRef = TrackCNT(BaseSetPanRef, target, BaseMotionSpeed);
                angles.PanCNT = BaseSetPanRef;
                angles.TiltCNT = BaseSetTiltRef;
                lastPanForWrap = BaseSetPanRef;
                if (BaseSetPanRef == target) BaseSet = eBaseSetState.SET;
            }

            // limit
            if (angles.TiltCNT < 1000) angles.TiltCNT = 1000;
            if (angles.TiltCNT > 3500) angles.TiltCNT = 3500;

            return angles;
        }

        private int TrackCNT(int current, int target, int velocity)
        {
            int delta = current - target;
            if (delta > 0) current = Math.Max(current - velocity, target);
            else current = Math.Min(current + BaseMotionSpeed, target);

            return current;
        }

        public void StartBaseSetProcedure(int startingPan, int startingTilt)
        {
            BaseSetPanRef = startingPan;
            BaseSetTiltRef = startingTilt;
            BaseSet = eBaseSetState.GOZERO;
        }

        // Tracker Calculations             
        private ExtY TrackerGimbalCalculate(ExtU rtU)
        {
            double cosOut;
            double[] rtb_MatrixMultiply = new double[3];
            double[] rtb_VectorConcatenate = new double[9];
            Int32 i;
            double rtb_Subtract_idx_0;
            double rtb_Subtract_idx_1;
            double rtb_Subtract_idx_2;
            ExtY rtY = new ExtY();

            /* SignalConversion: '<S6>/TmpSignal ConversionAtsincosInport1' incorporates:
             *  Gain: '<S5>/Gain1'
             *  Inport: '<Root>/BaseRPY'
             */
            rtb_Subtract_idx_0 = 0.017453292519943295 * rtU.BaseYaw;
            rtb_Subtract_idx_1 = 0.017453292519943295 * rtU.BasePitch;
            rtb_Subtract_idx_2 = 0.017453292519943295 * rtU.BaseRoll;

            /* Trigonometry: '<S6>/sincos' */
            rtb_MatrixMultiply[0] = Math.Cos(rtb_Subtract_idx_0);
            rtb_Subtract_idx_0 = Math.Sin(rtb_Subtract_idx_0);
            rtb_MatrixMultiply[1] = Math.Cos(rtb_Subtract_idx_1);
            rtb_Subtract_idx_1 = Math.Sin(rtb_Subtract_idx_1);
            cosOut = Math.Cos(rtb_Subtract_idx_2);
            rtb_Subtract_idx_2 = Math.Sin(rtb_Subtract_idx_2);

            /* Fcn: '<S6>/Fcn11' */
            rtb_VectorConcatenate[0] = rtb_MatrixMultiply[1] * rtb_MatrixMultiply[0];

            /* Fcn: '<S6>/Fcn21' incorporates:
             *  Trigonometry: '<S6>/sincos'
             */
            rtb_VectorConcatenate[1] = rtb_Subtract_idx_2 * rtb_Subtract_idx_1 *
              rtb_MatrixMultiply[0] - cosOut * rtb_Subtract_idx_0;

            /* Fcn: '<S6>/Fcn31' incorporates:
             *  Trigonometry: '<S6>/sincos'
             */
            rtb_VectorConcatenate[2] = cosOut * rtb_Subtract_idx_1 * rtb_MatrixMultiply[0]
              + rtb_Subtract_idx_2 * rtb_Subtract_idx_0;

            /* Fcn: '<S6>/Fcn12' */
            rtb_VectorConcatenate[3] = rtb_MatrixMultiply[1] * rtb_Subtract_idx_0;

            /* Fcn: '<S6>/Fcn22' incorporates:
             *  Trigonometry: '<S6>/sincos'
             */
            rtb_VectorConcatenate[4] = rtb_Subtract_idx_2 * rtb_Subtract_idx_1 *
              rtb_Subtract_idx_0 + cosOut * rtb_MatrixMultiply[0];

            /* Fcn: '<S6>/Fcn32' incorporates:
             *  Trigonometry: '<S6>/sincos'
             */
            rtb_VectorConcatenate[5] = cosOut * rtb_Subtract_idx_1 * rtb_Subtract_idx_0 -
              rtb_Subtract_idx_2 * rtb_MatrixMultiply[0];

            /* Fcn: '<S6>/Fcn13' */
            rtb_VectorConcatenate[6] = -rtb_Subtract_idx_1;

            /* Fcn: '<S6>/Fcn23' */
            rtb_VectorConcatenate[7] = rtb_Subtract_idx_2 * rtb_MatrixMultiply[1];

            /* Fcn: '<S6>/Fcn33' incorporates:
             *  Trigonometry: '<S6>/sincos'
             */
            rtb_VectorConcatenate[8] = cosOut * rtb_MatrixMultiply[1];

            /* Sum: '<S1>/Subtract' incorporates:
             *  Inport: '<Root>/BaseXYZ'
             *  Inport: '<Root>/TargeXYZ'
             *  Product: '<S1>/Matrix Multiply'
             */
            rtb_Subtract_idx_0 = rtU.TargetX - rtU.BaseX;
            rtb_Subtract_idx_1 = rtU.TargetY - rtU.BaseY;
            cosOut = rtU.TargetZ - rtU.BaseZ;

            /* Product: '<S1>/Matrix Multiply' */
            for (i = 0; i < 3; i++)
            {
                rtb_MatrixMultiply[i] = rtb_VectorConcatenate[i + 6] * cosOut +
                  (rtb_VectorConcatenate[i + 3] * rtb_Subtract_idx_1 +
                   rtb_VectorConcatenate[i] * rtb_Subtract_idx_0);
            }

            /* Outport: '<Root>/Tilt' incorporates:
             *  Gain: '<S1>/Gain'
             *  Gain: '<S3>/Gain'
             *  Math: '<S1>/Math Function'
             *  Math: '<S1>/Math Function1'
             *  Sqrt: '<S1>/Sqrt'
             *  Sum: '<S1>/Sum'
             *  Trigonometry: '<S1>/Trigonometric Function'
             */
            rtY.Tilt = Math.Atan2(-rtb_MatrixMultiply[2], Math.Sqrt(rtb_MatrixMultiply[0] *
              rtb_MatrixMultiply[0] + rtb_MatrixMultiply[1] * rtb_MatrixMultiply[1])) *
              57.295779513082323;

            /* Outport: '<Root>/Pan' incorporates:
             *  Gain: '<S4>/Gain'
             *  Trigonometry: '<S1>/Trigonometric Function1'
             */
            rtY.Pan = 57.295779513082323 * Math.Atan2(rtb_MatrixMultiply[1],
              rtb_MatrixMultiply[0]);

            return rtY;
        }

        public void UpdateTargetPosition(float altitude, double N, double E)
        {
            TargetAltitude = altitude;
            TargetN = N;
            TargetE = E;
        }
    }
}
