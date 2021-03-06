#include <TinyGPS++.h> // includes TinyGPS++ lib
#include <SoftwareSerial.h> // includes inbuilt SoftwareSerial lib  
#include <SparkFunMPU9250-DMP.h>
#include <CurieIMU.h>
#include <MadgwickAHRS.h>
#include <PID_v1.h>
#include <Servo.h>

TinyGPSPlus gps; // object gps for class TinyGPS++
MPU9250_DMP imu; //object imu for mpu9250 class

int imupin = 0;
int imupin1 = 1;
int imupin2 = 2;

#define SerialPort SerialUSB
#define ACCELEROMETER_SENSITIVITY 8192.0
#define GYROSCOPE_SENSITIVITY 65.536
#define M_PI 3.14159265359     
#define dt 0.01             // 10 ms sample rate!    

SoftwareSerial GPS(PA10,PA9);
// Pins on Bees Shield:
SoftwareSerial xbee(2, 3);  // TX, RX

///////////////////////////////IMU Stuff///////////////////////////////////
Madgwick filter;
unsigned long microsPerReading, microsPrevious;
float accelScale, gyroScale;
float roll, pitch, heading;
///////////////////////////////IMU Stuff///////////////////////////////////

/////////////////////////////////PID Stuff////////////////////////////////////////
double Kp=1, Ki=1, Kd=0.07;  //PID Tuning Parameters (NOTE: idk if I tuned it properly)
//     Roll PID stuff
double rollSetpoint, rollInput, rollOutput;
PID rollPID(&rollInput, &rollOutput, &rollSetpoint, Kp, Ki, Kd, DIRECT);


//   PitchPID Stuff
double pitchSetpoint, pitchInput, pitchOutput;
PID pitchPID(&pitchInput, &pitchOutput, &pitchSetpoint, Kp, Ki, Kd, DIRECT);


//Specify the links and initial tuning parameters
/////////////////////////////////PID Stuff////////////////////////////////////////

Servo leftServo, rightServo; //Servos for flaps
int leftOutput, rightOutput; //Output for flaps




boolean configured;
char c = 'A';
int killswitchPin = 8; //pin no. where kill switch is connected
int val = 0;
    
void setup() {
// put your setup code here, to run once:
Serial.begin(9600); // begins serial communication at Serial port at 9600 bauds
GPS.begin(9600); // begins serial communication with gps device at 9600 bauds
mpuinit();
configured = configureRadio();

digitalWrite(13,HIGH);
delay(1000);
Xrest=analogRead(imupin);
Serial.print(Xrest);
Yrest=analogRead(imupin1);
Serial.print(Yrest);
Zrest=analogRead(imupin2);
Serial.print(Zrest);
digitalWrite(13,LOW);

    ///////////////PID Setup/////////////
      rollInput = roll; //Sets the PID inputs to the gyro's roll and pitch axes
      pitchInput = pitch;
    
      rollSetpoint = 0; 
      pitchSetpoint = 0;
    
      rollPID.SetOutputLimits(-90, 90); //limits the PID loop to output numbers in between -90 and 90. I used values from -90 to 90 in order to better visualize the glider position(when flaps are flat, the PID output is 0)
      pitchPID.SetOutputLimits(-90,90);
    
      rollPID.SetMode(AUTOMATIC);  //Turns PID on
      pitchPID.SetMode(AUTOMATIC);
    ///////////////PID Setup/////////////

        ///////////////IMU Setup/////////////
      // start the IMU and filter
      CurieIMU.begin();
      CurieIMU.setGyroRate(25);
      CurieIMU.setAccelerometerRate(25);
      filter.begin(25);
    
      // Set the accelerometer range to 2G
      CurieIMU.setAccelerometerRange(2);
      // Set the gyroscope range to 250 degrees/second
      CurieIMU.setGyroRange(250);
    
      // initialize variables to pace updates to correct rate
      microsPerReading = 1000000 / 25;
      microsPrevious = micros();
    ///////////////IMU Setup/////////////

      leftServo.attach(9);  //servo setup
      rightServo.attach(10); 
      

pinMode(killswitchPin, INPUT); //initialise the pin as input pin


}

void loop() {
  // put your main code here, to run repeatedly:
gpsinput();

}
//----------------gps input---------------------------//
void gpsinput() {
  while (GPS.available()>0){ // check for gps data
    gps.encode(GPS.read()); // reads the gps data
    if (gps.location.isUpdated()){ // check for update in gps location data
      Serial.print("Latitude="); // Prints latitude
      Serial.print(gps.location.lat(),6); // prints latitude data
      Serial.print("Longitude=");// Prints longitude
      Serial.println(gps.location.lng(),6); // prints longitude data 
    }
    if (gps.altitude.isUpdated()){ // check for update in gps location data
      Serial.print("Altitude="); // Prints altitude
      //Serial.println(gps.location.meters(),6); // prints altitude data
      altitude = String((int)gps.location.meters());
      Serial.println(altitude + "m");
    }
  }    
}
//----------------mpu initialisation------------------------//
void mpuinit(){
// Call imu.begin() to verify communication with and
  // initialize the MPU-9250 to it's default values.
  // Most functions return an error code - INV_SUCCESS (0)
  // indicates the IMU was present and successfully set up
  if (imu.begin() != INV_SUCCESS)
  {
    while (1)
    {
      SerialPort.println("Unable to communicate with MPU-9250");
      SerialPort.println("Check connections, and try again.");
      SerialPort.println();
      delay(5000);
    }
  }
   // Use setSensors to turn on or off MPU-9250 sensors.
  // Any of the following defines can be combined:
  // INV_XYZ_GYRO, INV_XYZ_ACCEL, INV_XYZ_COMPASS,
  // INV_X_GYRO, INV_Y_GYRO, or INV_Z_GYRO
  // Enable all sensors:
  
   imu.setSensors(INV_XYZ_GYRO | INV_XYZ_ACCEL | INV_XYZ_COMPASS);
  
  // Use setGyroFSR() and setAccelFSR() to configure the
  // gyroscope and accelerometer full scale ranges.
  // Gyro options are +/- 250, 500, 1000, or 2000 dps
  imu.setGyroFSR(2000); // Set gyro to 2000 dps
  
  // Accel options are +/- 2, 4, 8, or 16 g
  
  imu.setAccelFSR(2); // Set accel to +/-2g
  
  // Note: the MPU-9250's magnetometer FSR is set at 
  // +/- 4912 uT (micro-tesla's)

  // setLPF() can be used to set the digital low-pass filter
  // of the accelerometer and gyroscope.
  // Can be any of the following: 188, 98, 42, 20, 10, 5
  // (values are in Hz).
  imu.setLPF(5); // Set LPF corner frequency to 5Hz

  // The sample rate of the accel/gyro can be set using
  // setSampleRate. Acceptable values range from 4Hz to 1kHz
  imu.setSampleRate(10); // Set sample rate to 10Hz

  // Likewise, the compass (magnetometer) sample rate can be
  // set using the setCompassSampleRate() function.
  // This value can range between: 1-100Hz
  imu.setCompassSampleRate(10); // Set mag rate to 10Hz
}

//----------------mpu9250 input----------------------//
void mpuinput()
{
  // dataReady() checks to see if new accel/gyro data
  // is available. It will return a boolean true or false
  // (New magnetometer data cannot be checked, as the library
  //  runs that sensor in single-conversion mode.)
  if ( imu.dataReady() )
  {
    // Call update() to update the imu objects sensor data.
    // You can specify which sensors to update by combining
    // UPDATE_ACCEL, UPDATE_GYRO, UPDATE_COMPASS, and/or
    // UPDATE_TEMPERATURE.
    // (The update function defaults to accel, gyro, compass,
    //  so you don't have to specify these values.)
    imu.update(UPDATE_ACCEL | UPDATE_GYRO | UPDATE_COMPASS);
    printIMUData();
  }
}

//----------------printing imu data-------------------------//
void printIMUData(void)
{  
  // After calling update() the ax, ay, az, gx, gy, gz, mx,
  // my, mz, time, and/or temerature class variables are all
  // updated. Access them by placing the object. in front:

  // Use the calcAccel, calcGyro, and calcMag functions to
  // convert the raw sensor readings (signed 16-bit values)
  // to their respective units.
  float accelX = imu.calcAccel(imu.ax);
  float accelY = imu.calcAccel(imu.ay);
  float accelZ = imu.calcAccel(imu.az);
  float gyroX = imu.calcGyro(imu.gx);
  float gyroY = imu.calcGyro(imu.gy);
  float gyroZ = imu.calcGyro(imu.gz);
  float magX = imu.calcMag(imu.mx);
  float magY = imu.calcMag(imu.my);
  float magZ = imu.calcMag(imu.mz);
  
  SerialPort.println("Accel: " + String(accelX) + ", " +
              String(accelY) + ", " + String(accelZ) + " g");
  SerialPort.println("Gyro: " + String(gyroX) + ", " +
              String(gyroY) + ", " + String(gyroZ) + " dps");
  SerialPort.println("Mag: " + String(magX) + ", " +
              String(magY) + ", " + String(magZ) + " uT");
  SerialPort.println("Time: " + String(imu.time) + " ms");
  SerialPort.println();
}

//----------------RF initialsation------------------------//
boolean configureRadio() {
      
// Set the data rate for the SoftwareSerial port:
xbee.begin(9600);
       
// Put the radio in command mode:
Serial.write("Entering command mode\r");
delay(1000);
while(xbee.available()>0) {xbee.read();}
xbee.write("+++");
while(xbee.available()>0) {xbee.read();}
//delay(1000);
//while(xbee.available() > 0) {Serial.write(xbee.read());}
String ok_response = "OK\r"; // The response we expect
      
// Read the text of the response into the response variable
// This satisfies the guard time by waiting for the OK message
String response = String("");
while (response.length() < ok_response.length()) {
  if (xbee.available() > 0) {
      response += (char) xbee.read();
   }
}
Serial.println("response1: " + response);
      
// If we got received OK, configure the XBee and return true:
if (response.equals(ok_response)) {
Serial.println("Enter command mode successful");
        
// Restore to default values:
Serial.println("Restoring default values before making changes");
xbee.write("ATRE\r");
Serial.println("Setting addr high");
xbee.write("ATDH0\r");  // Destination high
//while(xbee.available() > 0) {Serial.write(xbee.read());}
Serial.println("Setting addr low");
xbee.write("ATDL1\r");  // Destination low-REPLACE THIS
//while(xbee.available() > 0) {Serial.write(xbee.read());}
Serial.println("Setting MY address");
xbee.write("ATMYFFFF\r");
        
// Apply changes:
Serial.println("Applying changes");
xbee.write("ATAC\r");
/*    
 ///////////////////////////////////////////////
// Write to non-volatile memory:
// Use similar technique as above to satisfy guard time
Serial.write("Saving\r");
xbee.write("ATWR\r");
String response2 = String("");
//while (xbee.available() > 0) {Serial.write(xbee.read());}
while (response2.length() < ok_response.length()) {
  if (xbee.available() > 0) {
    response2 += (char) xbee.read();
   }
}
Serial.println("response2: " + response2);
       
if (response2.equals(ok_response)) {
Serial.println("Save successful");
}
else { Serial.println("Save not successful");
   return false;
}
        
// And reset module:
Serial.println("Resetting");
xbee.write("ATFR\r");
///////////////////////////////////////////////
       
*/
Serial.write("Exit command mode\r");
xbee.write("ATCN\r");  // Exit command mode
//while(xbee.available() > 0) {Serial.write(xbee.read());}
Serial.write("Finished\r");
return true;
} else {
return false; // This indicates the response was incorrect
   }
}

//-------------------killswitch---------------------//
void killswitch() 
{ 
  while (Serial.available()>0)
  {
    val = digitalRead(killswitchPin); //take input from killswitch
    
    if(val == HIGH)
    Serial.println("KillSwitch if OFF");

    else
    Serial.println("KillSwitch is ON");
  }
}

//---------------velocity from acc---------------//
void calculateVel()
{
int Xread;
int Xrest;
double Gx;
int Yread;
int Yrest;
double Gy;
int Zread;
int Zrest;
double Gz;
int t1;

Serial.print("Time ");
t1=millis();
Serial.println(t1*0.001);
Xread = analogRead(imupin)-Xrest;
Gx=Xread/67.584;
Serial.print("\tAcceleration  X :");
Serial.print(Gx);
Serial.print("\t \tVelocity  X :");
Serial.print(Gx*t1*0.001);
Serial.print("\t \tDistance  X :");
Serial.print(Gx*(t1*t1*0.0000005));
Serial.print("\n \n");
Yread=analogRead(imupin1)-Yrest;
Gy=Yread/67.584;
Serial.print("\tAcceration  Y :");
Serial.print(Gy);
Serial.print("\t\tVelocity  Y :");
Serial.print(Gy*t1*0.001);
Serial.print("\t\tDistance  Y :");
Serial.print(Gy*t1*t1*0.0000005);
Serial.print("\n \n");
Zread=analogRead(imupin2)-Zrest;
Gz=Zread/67.584;
Serial.print("\tAcceration  Z :");
Serial.print(Gz);
Serial.print("\t \t Velocity Z : ");
Serial.print(Gz*t1*0.001);
Serial.print("\t\tDistance  Z :");
Serial.print(Gz*t1*t1*0.0000005);
Serial.print("\n \n \n \n");
delay(700);
}

void ComplementaryFilter(short accData[3], short gyrData[3], float *pitch, float *roll)
{
    float pitchAcc, rollAcc;               
 
    // Integrate the gyroscope data -> int(angularSpeed) = angle
    *pitch += ((float)gyrData[0] / GYROSCOPE_SENSITIVITY) * dt; // Angle around the X-axis
    *roll -= ((float)gyrData[1] / GYROSCOPE_SENSITIVITY) * dt;    // Angle around the Y-axis
 
    // Compensate for drift with accelerometer data if !bullshit
    // Sensitivity = -2 to 2 G at 16Bit -> 2G = 32768 && 0.5G = 8192
    int forceMagnitudeApprox = abs(accData[0]) + abs(accData[1]) + abs(accData[2]);
    if (forceMagnitudeApprox > 8192 && forceMagnitudeApprox < 32768)
    {
  // Turning around the X axis results in a vector on the Y-axis
        pitchAcc = atan2f((float)accData[1], (float)accData[2]) * 180 / M_PI;
        *pitch = *pitch * 0.98 + pitchAcc * 0.02;
 
  // Turning around the Y axis results in a vector on the X-axis
        rollAcc = atan2f((float)accData[0], (float)accData[2]) * 180 / M_PI;
        *roll = *roll * 0.98 + rollAcc * 0.02;
    }
} 

//----------------PID Stabilizer-------------------//
void PIDStabilizer() {
  rollInput = roll; //sets PID inputs to Roll and Pitch
  pitchInput = pitch;
  rollPID.SetTunings(Kp, Ki, Kd); //PID Tunings
  pitchPID.SetTunings(Kp, Ki, Kd);

  int aix, aiy, aiz;  //IMU Stuff
  int gix, giy, giz;
  float ax, ay, az;
  float gx, gy, gz;

  unsigned long microsNow;

  // check if it's time to read data and update the filter
  microsNow = micros();
  if (microsNow - microsPrevious >= microsPerReading) {

    // read raw data from CurieIMU
    CurieIMU.readMotionSensor(aix, aiy, aiz, gix, giy, giz);

    // convert from raw data to gravity and degrees/second units
    ax = convertRawAcceleration(aix);
    ay = convertRawAcceleration(aiy);
    az = convertRawAcceleration(aiz);
    gx = convertRawGyro(gix);
    gy = convertRawGyro(giy);
    gz = convertRawGyro(giz);

    // update the filter, which computes orientation
    filter.updateIMU(gx, gy, gz, ax, ay, az);

    // print the heading, pitch and roll
    roll = filter.getRoll();
    pitch = filter.getPitch();
    heading = filter.getYaw();



    //Calculates PID
    rollPID.Compute();
    pitchPID.Compute();

    //Combines the outputs and adds 90 to make it work with the servos
    leftOutput = ((rollOutput + pitchOutput) / 2) + 90;
    rightOutput = ((rollOutput - pitchOutput) / 2) + 90;

    

     *  One issue with the code is that the way I mix the Roll and Pitch PID loops
     * is not the best way to mix PID loops. For instance, if the pitchPID was 90, and 
     * rollPID was 0, both flaps would not trim up, rather, rightServo would be at -45,
     * while leftServo trims at 135.  
     * 
  
     */

     
    //writes to the servos
    leftServo.write(leftOutput);
    rightServo.write(rightOutput);
    Serial.println(pitch);
    // increment previous time, so we keep proper pace
    microsPrevious = microsPrevious + microsPerReading;
  } //if
}

float convertRawAcceleration(int aRaw) {
  // since we are using 2G range
  // -2g maps to a raw value of -32768
  // +2g maps to a raw value of 32767
  
  float a = (aRaw * 2.0) / 32768.0;
  return a;
}

float convertRawGyro(int gRaw) {
  // since we are using 250 degrees/seconds range
  // -250 maps to a raw value of -32768
  // +250 maps to a raw value of 32767
  
  float g = (gRaw * 250.0) / 32768.0;
  return g;
}
