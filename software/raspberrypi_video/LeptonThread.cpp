#include "LeptonThread.h"

#include "Palettes.h"
#include "SPI.h"
#include "Lepton_I2C.h"
#include <MQTTAsync.h>
#include <assert.h>
#include <time.h>
#include <QByteArray>

//#include <OsWrapper.h>

// LeptonThred 
#define PACKET_SIZE 164
#define PACKET_SIZE_UINT16 (PACKET_SIZE/2)
#define PACKETS_PER_FRAME 60
#define FRAME_SIZE_UINT16 (PACKET_SIZE_UINT16*PACKETS_PER_FRAME)
#define FPS 10

// MQTT
#define ADDRESS     "tcp://144.121.64.136:1883"
#define CLIENTID    "LeptonPi"
#define TOPIC       "lepton"
#define QOS         0
#define TIMEOUT     10000L

extern "C" {
  volatile MQTTAsync_token deliveredtoken;
  static int deliveredMsgs = 0;
  void connlost(void *context, char *cause);
  void onDisconnect(void* context, MQTTAsync_successData* response);
  void onSend(void* context, MQTTAsync_successData* response);
  void onConnectFailure(void* context, MQTTAsync_failureData* response);
  void onConnect(void* context, MQTTAsync_successData* response);
  long long getCurrSecsPlusMillis(void);
}


// START MQTT CALLBACKS ------------------------


void connlost(void *context, char *cause)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc;

	printf("\nConnection lost\n");
	printf("     cause: %s\n", cause);

	printf("Reconnecting\n");
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
	}
}


void onDisconnect(void* context, MQTTAsync_successData* response)
{
  (void) context;
  (void) response;
	printf("Successful disconnection\n");
}


void onSend(void* context, MQTTAsync_successData* response)
{
  (void) response;
  (void) context;
  
  if ((++deliveredMsgs % 10) == 0) {
    printf("%lld: delivered %d messages\n",
           getCurrSecsPlusMillis(), deliveredMsgs);
  }
  //printf("Message with token value %d delivery confirmed\n", response->token);
}


void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
  (void) context;
	printf("Connect failed, rc %d\n", response ? response->code : 0);
}


void onConnect(void* context, MQTTAsync_successData* response)
{
  (void) context;
  (void) response;
	printf("Successful connection\n");
}

// END MQTT CALLBACKS --------------------------

long long getCurrSecsPlusMillis(void) {
  long            ms; // Milliseconds
  time_t          s;  // Seconds
  struct timespec spec;
  
  clock_gettime(CLOCK_REALTIME, &spec);
  
  s  = spec.tv_sec;
  ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds

  return ((long long)s)*1000LL + (long long)ms;
}



//from: https://stackoverflow.com/questions/2056499/transform-an-array-of-integers-into-a-string?rq=1
int LeptonThread::frameBufferToString(char* output, int output_length)
{
  // if not enough space was available, returns -1
  // otherwise returns the number of characters written to
  // output, not counting the additional null character
  
  // precondition: non-null pointers
  assert(output);
  // precondition: output has room for null
  assert(output_length >= 1);
  
  int written = 0;
  
  for(int i=0;i<FRAME_SIZE_UINT16;i++) {
    //skip the first 2 uint16_t's of every packet, they're 4 header bytes
    if(i % PACKET_SIZE_UINT16 < 2) {
      continue;
    }
		
    int length;
    if (i < (FRAME_SIZE_UINT16 -1))
      length = snprintf(output, output_length, "%d,", frameBuffer[i]);
    else
      length = snprintf(output, output_length, "%d", frameBuffer[i]);
      
    if (length >= output_length) {
      // not enough space
      return -1;
    }
    written += length;
    output += length;
    output_length -= length;
  }
  return written;
}

void LeptonThread::setupMQTT(void)
{
  conn_opts = MQTTAsync_connectOptions_initializer;
	
  // Set up the MQTT Connection and Connect asynchronously
	MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

  MQTTAsync_setCallbacks(client, NULL, connlost, NULL, NULL);

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = client;
}

//from: https://stackoverflow.com/questions/3756323/getting-the-current-time-in-milliseconds
void LeptonThread::publishFrame(void)
{
  int strIdx = sprintf(frameStr, "base_id|001|timestamp|%lld|pixels|",
                          currFrameTime);

  int frameStrLen = frameBufferToString(frameStr + strIdx,  MQTT_PAYLOAD_SIZE - strIdx);
  QByteArray frameStrBytes(frameStr + strIdx, frameStrLen);
  QByteArray compressedBytes = qCompress(frameStrBytes).toBase64();
  qstrncpy(frameStr + strIdx, compressedBytes, compressedBytes.size());
  
  //  printf("About to send frame with %d compressed (%d uncompressed) bytes\n",
  //     compressedBytes.size(), frameStrLen);

  //    printf("About to send frame with %d compressed / %d uncompressed bytes: %s\n",
  //     compressedBytes.size(), frameStrLen, frameStr);

 
  // Prepare to get the reponse and then send
  MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	int rc;
  
	opts.onSuccess = onSend;
	opts.context = client;

	pubmsg.payload = frameStr;
	pubmsg.payloadlen = strlen(frameStr);
	pubmsg.qos = QOS;
	pubmsg.retained = 0;

	if ((rc = MQTTAsync_sendMessage(client, TOPIC, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start sendMessage, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}
}

LeptonThread::LeptonThread() : QThread()
{
  setupMQTT();
}

LeptonThread::~LeptonThread() {
}

void LeptonThread::run()
{
  int rc;
  
  if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

	//create the initial image
	myImage = QImage(80, 60, QImage::Format_RGB888);

	//open spi port
	SpiOpenPort(0);

	while(true) {    
		//read data packets from lepton over SPI
		int resets = 0;
		for(int j=0;j<PACKETS_PER_FRAME;j++) {
			//if it's a drop packet, reset j to 0, set to -1 so he'll be at 0 again loop
			read(spi_cs0_fd, result+sizeof(uint8_t)*PACKET_SIZE*j, sizeof(uint8_t)*PACKET_SIZE);
			int packetNumber = result[j*PACKET_SIZE+1];
			if(packetNumber != j) {
				j = -1;
				resets += 1;
				usleep(1000);
				//Note: we've selected 750 resets as an arbitrary limit, since there should never
        //be 750 "null" packets between two valid transmissions at the current poll rate
				//By polling faster, developers may easily exceed this count, and the down period
        //between frames may then be flagged as a loss of sync
				if(resets == 750) {
					SpiClosePort(0);
					usleep(750000);
					SpiOpenPort(0);
				}
			}
      else if(packetNumber == 0)
        currFrameTime = getCurrSecsPlusMillis();
		}
		if(resets >= 30) {
			qDebug() << "done reading, resets: " << resets;
		}

    
    if(currFrameTime < (lastFrameTime + 1000/FPS))
      continue;

       
		frameBuffer = (uint16_t *)result;
		uint16_t value;
		uint16_t minValue = 65535;
		uint16_t maxValue = 0;

		
		for(int i=0;i<FRAME_SIZE_UINT16;i++) {
			//skip the first 2 uint16_t's of every packet, they're 4 header bytes
			if(i % PACKET_SIZE_UINT16 < 2) {
				continue;
			}
			
			//flip the MSB and LSB at the last second
			int temp = result[i*2];
			result[i*2] = result[i*2+1];
			result[i*2+1] = temp;

      
			value = frameBuffer[i];
			if(value > maxValue) {
				maxValue = value;
			}
			if(value < minValue) {
				minValue = value;
			}
		}

    // Publish this frame asyncrhonously over MQTT. The buffer is copied
    // by the MQTT library.
    if (MQTTAsync_isConnected(client)) {
        publishFrame();
      }
    
    lastFrameTime = currFrameTime;


    /*
		float diff = maxValue - minValue;
		float scale = 255/diff;
		QRgb color;
		for(int i=0;i<FRAME_SIZE_UINT16;i++) {
			if(i % PACKET_SIZE_UINT16 < 2) {
				continue;
			}
			value = (frameBuffer[i] - minValue) * scale;
			const int *colormap = colormap_ironblack;
			color = qRgb(colormap[3*value], colormap[3*value+1], colormap[3*value+2]);
			column = (i % PACKET_SIZE_UINT16 ) - 2;
			row = i / PACKET_SIZE_UINT16;
			myImage.setPixel(column, row, color);
		}

		//lets emit the signal for update
		emit updateImage(myImage);
    */
	}
	
	//finally, close SPI port just bcuz
	SpiClosePort(0);

  MQTTAsync_destroy(&client);
}

void LeptonThread::performFFC() {
	//perform FFC
	lepton_perform_ffc();
}
