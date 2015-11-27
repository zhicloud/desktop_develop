#ifndef ZC_MESSAGE_H
#define ZC_MESSAGE_H

#include <QWidget>
#include <QDesktopWidget>
#include <QPalette>
#include <QRect>
#include <QLabel>
#include <QDialog>
#include <QPushButton>
#include "mylabel.h"
#include "cusercard.h"

class CUserCard;

class CSiglePic1 : public QWidget
{
	Q_OBJECT

public:
	CSiglePic1(QWidget *parent = 0);
	~CSiglePic1();
	void setPixmap(QPixmap pix);
	void setPixIndex(int i);
	const int getPixIndex(){ return PicIndex; };
	QPixmap* getPix(){ return &curpix; };

	static void SetCurSelectPic(CSiglePic1* pic);
	static CSiglePic1* GetCurSelectPic();
	
	
private:
	MyLabel* piclabel;
	int ismidselected;
	static CSiglePic1* curpic;
	QPixmap curpix;
	int PicIndex;
public slots:
	void clickfunc();
};




class ZCMessageBox : public QDialog
{
	Q_OBJECT

public:
	ZCMessageBox(QString message="", QWidget *parent = 0,CUserCard* card = 0);
	~ZCMessageBox();

private:	
	QPushButton *dialogButton;
	QLabel      *dialogLabel;

	CMainWindow* m_win;

signals:
	private slots:
	void closeDialogSlot();
};

#endif // CUSERPICPANEL_H
