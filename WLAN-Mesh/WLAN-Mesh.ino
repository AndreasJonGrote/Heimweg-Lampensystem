#include <painlessMesh.h> // benötigt AsyncTCP

#define   MESH_PREFIX     "MeshNetwork"
#define   MESH_PASSWORD   "password123"
#define   MESH_PORT       5555
#define   MESH_MASTER     true  // true für Master-Node, false für Slave-Node
#define   DEVICE_ID       "LAMP_001" // Eindeutige ID für jedes Gerät

Scheduler userScheduler;
painlessMesh  mesh;

void sendMessage(); // Prototype

// Deklariere taskSendMessage vor der setup-Funktion
Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

void setup() {
  Serial.begin(115200);
  mesh.setDebugMsgTypes( ERROR | STARTUP );

  // Basis-Mesh-Initialisierung
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  
  if (MESH_MASTER) {
    // Master-spezifische Initialisierungen
    Serial.println("This is a Master Node");
    mesh.onReceive(&masterReceiveCallback);
    mesh.onNewConnection(&masterNewConnectionCallback);
    mesh.onChangedConnections(&masterChangedConnectionCallback);
    mesh.onNodeTimeAdjusted(&masterNodeTimeAdjustedCallback);
    
    // Master-spezifische Tasks
    userScheduler.addTask(taskSendMessage);
    taskSendMessage.enable();
  } else {
    // Slave-spezifische Initialisierungen
    Serial.println("This is a Slave Node");
    mesh.onReceive(&slaveReceiveCallback);
    mesh.onNewConnection(&slaveNewConnectionCallback);
    mesh.onChangedConnections(&slaveChangedConnectionCallback);
    mesh.onNodeTimeAdjusted(&slaveNodeTimeAdjustedCallback);
  }
}

void loop() {
  mesh.update();
}

// Master-spezifische Callbacks
void masterReceiveCallback(uint32_t from, String &msg) {
  Serial.printf("Master received from %u msg=%s\n", from, msg.c_str());
}

void masterNewConnectionCallback(uint32_t nodeId) {
  Serial.printf("Master: New Connection from nodeId = %u\n", nodeId);
}

void masterChangedConnectionCallback() {
  Serial.printf("Master: Network topology changed\n");
}

void masterNodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Master: Time adjusted. Offset = %d\n", offset);
}

// Slave-spezifische Callbacks
void slaveReceiveCallback(uint32_t from, String &msg) {
  Serial.printf("Slave received from %u msg=%s\n", from, msg.c_str());
}

void slaveNewConnectionCallback(uint32_t nodeId) {
  Serial.printf("Slave: Connected to nodeId = %u\n", nodeId);
}

void slaveChangedConnectionCallback() {
  Serial.printf("Slave: Connection changed\n");
}

void slaveNodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Slave: Time adjusted. Offset = %d\n", offset);
}

void sendMessage() {
  String msg = "Hello from " DEVICE_ID " (Node ";
  msg += mesh.getNodeId();
  msg += ")";
  mesh.sendBroadcast(msg);
  Serial.printf("Master broadcasting: %s\n", msg.c_str());
}
