#include <QApplication>
#include <QThread>
#include <QMutex>
#include <QMessageBox>

#include <QColor>
#include <QLabel>
#include <QtDebug>
#include <QString>
#include <QPushButton>

#include "LeptonThread.h"
#include "MyLabel.h"
#include <iostream>

#include "optionparser.h"

enum  optionIndex { UNKNOWN, HELP, MQTT, LOG, DISPLAY};
const option::Descriptor usage[] =
{
 {UNKNOWN, 0,"" , ""    ,option::Arg::None, "USAGE: example [options]\n\n"
                                            "Options:" },
 {HELP,    0,"" , "help",option::Arg::None, "  --help  \tPrint usage and exit." },
 {MQTT,    0,"m", "mqtt",option::Arg::None, "  --mqtt, -m  \tPublish to MQTT." },
 {LOG,    0,"l", "log",option::Arg::None, "  --log, -l  \tLog frames." },
 {DISPLAY,    0,"d", "display",option::Arg::None, "  --display, -d  \tDisplay frames on screen." },
 {0,0,0,0,0,0}
};


int main( int argc, char **argv )
{
	//create the app
	QApplication a( argc, argv );

  argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
  option::Stats  stats(usage, argc, argv);
  option::Option options[stats.options_max], buffer[stats.buffer_max];
  option::Parser parse(usage, argc, argv, options, buffer);
  if (parse.error())
    return 1;

  if (options[HELP] || argc == 0) {
    option::printUsage(std::cout, usage);
    return 0;
  }

  if (parse.nonOptionsCount() == 0)
    printf("No BaseID argument. Using %d\n", LeptonThread::baseID);
  else if (parse.nonOptionsCount() >= 1) {
    LeptonThread::baseID = atoi(parse.nonOption(0));
    printf("BaseID is %d\n", LeptonThread::baseID);
  }

  QWidget *myWidget = NULL;
  MyLabel *myLabel = NULL;
  QPushButton *button1 = NULL;

  if (options[DISPLAY]) {
    myWidget = new QWidget;
    myWidget->setGeometry(400, 300, 340, 290);

    //create an image placeholder for myLabel
    //fill the top left corner with red, just bcuz
    QImage myImage;
    myImage = QImage(320, 240, QImage::Format_RGB888);
    QRgb red = qRgb(255,0,0);
    for(int i=0;i<80;i++) {
      for(int j=0;j<60;j++) {
        myImage.setPixel(i, j, red);
      }
    }

    //create a label, and set it's image to the placeholder
    myLabel = new MyLabel(myWidget);
    myLabel->setGeometry(10, 10, 320, 240);
    myLabel->setPixmap(QPixmap::fromImage(myImage));

    //create a FFC button
    button1 = new QPushButton("Perform FFC", myWidget);
    button1->setGeometry(320/2-50, 290-35, 100, 30);
  }

	//create a thread to gather SPI data
	//when the thread emits updateImage, the label should update its image accordingly
	LeptonThread *thread = new LeptonThread(options[DISPLAY], options[MQTT], options[LOG]);

  if (options[DISPLAY]){
    QObject::connect(thread, SIGNAL(updateImage(QImage)), myLabel, SLOT(setImage(QImage)));
	
    //connect ffc button to the thread's ffc action
    QObject::connect(button1, SIGNAL(clicked()), thread, SLOT(performFFC()));
  }
  
	thread->start();
	
	if (options[DISPLAY])
    myWidget->show();

	return a.exec();
}

