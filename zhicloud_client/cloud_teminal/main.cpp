#include "cloud_terminal.h"
#include <QTranslator>
#include "mainwindow.h"
#include <QtWidgets/QApplication>
#include <QTextCodec>

int main(int argc, char *argv[])
{

	QApplication a(argc, argv);

//	QFont font;

//	font.setFamily(("SIMSUNB"));
//	a.setFont(font);
 //	QTextCodec::setCodecForTr(QTextCodec::codecForName("GB2312"));
// 	QTextCodec::setCodecForTr(QTextCodec::codecForName("utf8"));
// 	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("GB2312"));
	QTextCodec::setCodecForLocale(QTextCodec::codecForName("gb2312"));
	QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
	QTranslator translator;
	bool ret = translator.load("./cloud_terminal_zh.qm");
	a.installTranslator(&translator);
	//cloud_terminal w;
	//w.show();
	CMainWindow w;
	w.show();
	return a.exec();
}
