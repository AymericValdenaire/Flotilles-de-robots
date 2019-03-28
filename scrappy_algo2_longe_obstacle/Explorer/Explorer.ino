#include <math.h>
#include <Adafruit_MotorShield.h>

#define FORWARD_ 0
#define BACKWARD_ 2
#define LEFT_ -1
#define RIGHT_ 1

/* Ultrasonic sensors */      
const uint8_t echoPin_RIGHT = 12;  // echo signal (receives)
const uint8_t echoPin_right = 10;
const uint8_t echoPin_front_right = 8; 
const uint8_t echoPin_front_left = 6; 
const uint8_t echoPin_left = 4;
const uint8_t echoPin_LEFT = 2; 

const uint8_t trigPin_RIGHT = 13;  // trigger signal (sends)
const uint8_t trigPin_right = 11;
const uint8_t trigPin_front_right = 9;
const uint8_t trigPin_front_left = 7; 
const uint8_t trigPin_left = 5;
const uint8_t trigPin_LEFT = 3;


/* Measurements */
const float safetyDistance = 30; // according with the speed. Expressed in cm
const float robotWidth = 20; // expressed in cm
const float marge = safetyDistance / 3; // margin of movement. It should move between the marging and the safetyDistance


/* LEDs
 * long side : pin
 * short side : ground
 * resistor : 100 Ohm
 */
const uint8_t ledPin_left = 15;
const uint8_t ledPin_back = 16;
const uint8_t ledPin_right = 17;


/*
 * Determines where to move
 */
int objectDetected = 0; // the side where the object is detected
boolean searchingObject = false;
int tick = 0;
int randomDir = FORWARD_;
boolean stop_ = false;


/* Movement */
const int motorSpeed = 100; // from 0 (off) to 255 (max speed)

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61); 

// Motor 4 -> left / Motor 2 -> right
Adafruit_DCMotor *motorLeft = AFMS.getMotor(4);
Adafruit_DCMotor *motorRight = AFMS.getMotor(2);

class Explorer {

  public:
    /*********************************
     * GENERAL SETTINGS
     *********************************/
    int SENSOR_NUMBER = 6;
    // Index of the positions
    int FAR_LEFT = 0, LEFT = 1, UP_LEFT = 2, UP_RIGHT = 3, RIGHT = 4, FAR_RIGHT = 5;
    // Angles of orientation of the sensors
    double angles[6] = {-90, -45, 20, -20, 45, 90};
    // Safety distance in mm
    int SAFETY_DISTANCE = 100;
    // Max distance at which we add a point to the map (in mm)
    int DRAW_DISTANCE = 400;
    // Error margin when moving along an object (in mm)
    int ERROR_MARGIN = 30;
    // Distance at which we will move along an object (in mm)
    int FOLLOW_DISTANCE = 160;
    // Speed of the wheels
    int DEFAULT_SPEED = 100;

    /*********************************
     * DEPENDS ON THE ROBOT'S SPEED
     *********************************/
    // Tick needed per angle
    double TICK_PER_ANGLE;
    double ANGLE_PER_TICK;
    // Tick per mm
    double TICK_PER_MM;
    double MM_PER_TICK;
    /*********************************/



    /*********************************
     * SPECIFIC TO THE ROBOT
     *********************************/
    // Position
    double x, y;
    double angle;

    // Distances from the robot to the closest objects
    double* distances;

    // If the robot will enter in contact with an object
    boolean* contact;

    // Previous informations
    double* prev_distances;
    boolean* prev_contact;

    // Which side do we follow ? (-1 is none)
    int FOLLOWING_SIDE = -1;
    /*********************************/

    // Constructor
    Explorer(double TICK_PER_ANGLE, double TICK_PER_MM){
        distances = new double[6];
        contact = new boolean[6];

        prev_distances = new double[6];
        prev_contact = new boolean[6];

        this->TICK_PER_ANGLE = TICK_PER_ANGLE;
        this->TICK_PER_MM = TICK_PER_MM;

        this->ANGLE_PER_TICK = 1 / TICK_PER_ANGLE;
        this->MM_PER_TICK = 1 / TICK_PER_MM;

        // Starting position
        x = 0; y = 0;
        angle = 0;
    }

    void setPos(int x, int y){
        this->x = x;
        this->y = y;
    }

    void setAngle(int angle){
        this->angle = angle;
    }

    double getX(){
        return x;
    }

    double getY(){
        return y;
    }

    double getAngle(){
        return angle;
    }

    double* getDistances(){
        return distances;
    }

    void setDistances(double* distances){
        for(int i = 0; i < SENSOR_NUMBER; i++) this->distances[i] = distances[i];
    }

    /*
 * Calculates the distance with the information obtained from the sensors  
 */
float calculDistance(uint8_t trigPin, uint8_t echoPin){
  uint32_t duration; // duration of the round trip
  float cm;  // distance of the obstacle

  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(3);

  // Start trigger signal

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
 
  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.

  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);
 
  // convert the time into a distance
  cm = (float)((duration<<4)+duration)/1000.0; // cm = 17 * duration/1000
  return cm;
}

    /**
     * Update the distance using the ultrasounds
     */
    void updateDistance(){

        // Copy the previous informations
        for(int i = 0; i < SENSOR_NUMBER; i++){
            prev_distances[i] = distances[i];
            prev_contact[i] = contact[i];
        }

        // Récupérer les informations des capteurs
        double cm_front_left = calculDistance(trigPin_front_left, echoPin_front_left);
        double cm_front_right = calculDistance(trigPin_front_right, echoPin_front_right);
        double cm_left = calculDistance(trigPin_left, echoPin_left);
        double cm_LEFT = calculDistance(trigPin_LEFT, echoPin_LEFT);
        double cm_right = calculDistance(trigPin_right, echoPin_right);
        double cm_RIGHT = calculDistance(trigPin_RIGHT, echoPin_RIGHT);
        //int FAR_LEFT = 0, LEFT = 1, UP_LEFT = 2, UP_RIGHT = 3, RIGHT = 4, FAR_RIGHT = 5;

        //conversion cm -> mm
        distances[FAR_LEFT] = cm_LEFT * 10;
        distances[FAR_RIGHT] = cm_RIGHT * 10;
        distances[LEFT] = cm_left * 10;
        distances[RIGHT] = cm_right * 10;
        distances[UP_LEFT] = cm_front_left * 10;
        distances[UP_RIGHT] = cm_front_right * 10;

        for(int i = 0; i < SENSOR_NUMBER; i++){
            if(distances[i] <= SAFETY_DISTANCE) contact[i] = true;
            else contact[i] = false;
        }

    }

    void speedLeft(int motorSpeed){
      motorLeft->setSpeed(motorSpeed);
    }

    void speedRight(int motorSpeed){
      motorRight->setSpeed(motorSpeed);
    }

    /**
     * Move the robot
     */
    void explore(){
        updateDistance();
        if(FOLLOWING_SIDE == -1) find_();
        else follow();
    }

    /**
     * Move the robot "randomly" until finding an object to move along
     */
    void find_(){
        // Nothing ahead, go forward
        if(!contact[UP_RIGHT] && !contact[UP_LEFT]) forward(50);
        // If there is something ahead, on the right, then we turn the robot to follow it
        else if(contact[UP_RIGHT]) left(90);
        // Same on the left
        else if(contact[UP_LEFT]) right(90);

        // If we're alongside an object on our left or right
        if(distances[FAR_LEFT] <= FOLLOW_DISTANCE + ERROR_MARGIN) FOLLOWING_SIDE = FAR_LEFT;
        else if (distances[FAR_RIGHT] <= FOLLOW_DISTANCE + ERROR_MARGIN) FOLLOWING_SIDE = FAR_RIGHT;
    }

    /**
     * Move the robot to follow an object on a certain side
     */
    void follow(){
        int distance = 7;
        // If there is something on the side at a good distance, we continue to go forward
        if(isThereSomething(FOLLOWING_SIDE, FOLLOW_DISTANCE + ERROR_MARGIN)) {
            // If we're too close, we try to turn a bit
            if(distances[FOLLOWING_SIDE] <= FOLLOW_DISTANCE - ERROR_MARGIN) {
                if(FOLLOWING_SIDE == FAR_RIGHT) left(10);
                if(FOLLOWING_SIDE == FAR_LEFT) right(10);
                
                forward(4);
            }
            if(!contact[UP_RIGHT] && !contact[UP_LEFT]) forward(7);
            else backward(7);
        }
        else{
            // If there is no more something at the side, we try to turn and go forward
            int cpt = 0;
            while(!isThereSomething(FOLLOWING_SIDE, FOLLOW_DISTANCE + ERROR_MARGIN) && cpt < 9){
              if(FOLLOWING_SIDE == FAR_RIGHT) right(10);
              if(FOLLOWING_SIDE == FAR_LEFT) left(10);
              forward(8);
              cpt++;
              updateDistance();
            }

            // Still nothing ? We stop to follow and try to explore something else
            if(!isThereSomething(FOLLOWING_SIDE, FOLLOW_DISTANCE + ERROR_MARGIN)) FOLLOWING_SIDE = -1;
        }
    }

    boolean isThereSomething(int direction, int distance){
        return distances[direction] <= distance;
    }

    // Direction
    void forward(){
        // Wheels go forward
        motorRight->run(FORWARD);
        motorLeft->run(FORWARD);
    }
    void left(){
        // Wheels go left
        motorLeft->run(BACKWARD);
        motorRight->run(FORWARD);
    }
    void right(){
        // Wheels go right
        motorLeft->run(FORWARD);
        motorRight->run(BACKWARD);
    }
    void backward(){
        // Wheels go backward
        motorRight->run(BACKWARD);
        motorLeft->run(BACKWARD);
    }

    void stopmamene(){
        motorLeft->run(RELEASE);
        motorRight->run(RELEASE);
        for(int i = 0; i < 30000; i++){}
    }

    /**
     * Update the position of the robot given the distance travelled
     * @param distance
     */
    void updatePos(double distance){
        double angle = 3.141592654 * this->angle / 180;
        x += cos(angle) * distance;
        y += sin(angle) * distance;
    }

    /**
     * Move forward for the specified distance (in mm)
     * @param distance
     */
    void forward(int distance){
        for(int i = 0; i < distance*TICK_PER_MM; i++){
            updatePos(MM_PER_TICK);
            forward();
        }
        stopmamene();
    }

    /**
     * Move left for the specified angle (in degree)
     * @param angle
     */
    void left(int angle){
        for(int i = 0; i < angle*TICK_PER_ANGLE; i++){
            addAngle(-ANGLE_PER_TICK);
            left();
        }
        stopmamene();
    }

    /**
     * Move right for the specified angle (in degree)
     * @param angle
     */
    void right(int angle){
        for(int i = 0; i < angle*TICK_PER_ANGLE; i++){
            addAngle(ANGLE_PER_TICK);
            right();
        }
        stopmamene();
    }

    /**
     * Move backward for the specified distance (in mm)
     * @param distance
     */
    void backward(int distance){
        for(int i = 0; i < distance*TICK_PER_MM; i++){
            updatePos(-MM_PER_TICK);
            backward();
        }
        stopmamene();
    }

    /**
     * Add the @param angle to the general angle. It will ensure that the general angle belong to [0, 360[ (in degree)
     * @param angle
     */
    void addAngle(double angle){
        this->angle += angle;
        if(this->angle >= 360) this->angle -= 360;
        if(this->angle < 0) this->angle += 360;
    }


    /**
     * Transform the robot's information into a JSON
     * @return
     
    String toJSON(){
        StringBuilder builder = new StringBuilder();
        builder.append("{\n");
        builder.append("\"x\":" + x + ",\n");
        builder.append("\"y\":" + y + ",\n");
        builder.append("\"angle\":" + angle + ",\n");

        builder.append("\"distances\": [");
        for (int i = 0; i < distances.length-1; i++) builder.append(distances[i] + ", ");
        builder.append(distances[distances.length-1] + "],");

        builder.append("\n}");
        return builder.toString();
    }
    */
    /**
     * USELESS FOR THE ROBOT ---- ONLY FOR TESTING PURPOSES
     * @param map
     
    void addToTheMap(Map map){
        for(int i = 0; i < distances.length; i++){
            if(distances[i] <= DRAW_DISTANCE && distances[i] > 0) map.add(x, y, angle, angles[i], distances[i], Map.DEGREE);
        }
    }*/
};

    Explorer* robot;
    boolean test;
    
    void setup(){
      Serial.begin(9600);
      AFMS.begin();

      robot = new Explorer(5.5, 1.55);
      
      // Left wheel
      motorLeft->setSpeed(motorSpeed);
      //motorLeft->run(FORWARD);
      //motorLeft->run(RELEASE);
      
      // Right wheel
      motorRight->setSpeed(motorSpeed);
      //motorRight->run(FORWARD);
      //motorRight->run(RELEASE);

      
    }

    void loop(){
      /*
      if(!test){
        test = true;
        robot->forward(500);
      }*/
      robot->explore();
    }
    
