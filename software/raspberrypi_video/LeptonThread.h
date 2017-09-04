#ifndef TEXTTHREAD
#define TEXTTHREAD

#include <ctime>
#include <stdint.h>

#include <QThread>
#include <QtCore>
#include <QPixmap>
#include <QImage>

#include <MQTTAsync.h>

#define DISPLAY_IMAGE 0
#define PUBLISH_MQTT 1
#define LOG_FRAMES 0


#define PACKET_SIZE 164
#define PACKET_SIZE_UINT16 (PACKET_SIZE/2)
#define PACKETS_PER_FRAME 60
#define FRAME_SIZE_UINT16 (PACKET_SIZE_UINT16*PACKETS_PER_FRAME)

// MQTT Packet Defs
#define CHAR_PER_PIXEL 6
#define MQTT_PAYLOAD_SIZE (FRAME_SIZE_UINT16 * CHAR_PER_PIXEL * 2)

class LeptonThread : public QThread
{
  Q_OBJECT;

public:
  LeptonThread(int aBaseID, bool aDisplayImage,
               bool aPublishMQTT, bool aLogFrames, bool aTest);
  ~LeptonThread();

  void run();

public slots:
  void performFFC();

signals:
  void updateText(QString);
  void updateImage(QImage);

private:
  int mBaseID;

  bool mDisplayImage;
  bool mPublishMQTT;
  bool mLogFrames;
  bool mTest;

  QImage myImage;

  uint8_t result[PACKET_SIZE*PACKETS_PER_FRAME];
  uint16_t *frameBuffer;
  long long currFrameTime;
  long long lastFrameTime;
  long long frameDelay;

  char frameStr[MQTT_PAYLOAD_SIZE];
  int frameBufferToString(char* output, int output_length);

  MQTTAsync client;
  int token;
  MQTTAsync_connectOptions conn_opts;
  void setupMQTT(void);
  void publishFrameToMQTT(void);

  char logFileName[1024];
  FILE *logFile;
  void writeFrameToLog(void);
};

#endif
