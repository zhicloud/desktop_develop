#ifndef MENU_BOX_H
#define MENU_BOX_H

#include <QWidget>
#include <QDesktopWidget>
#include <QPalette>
#include <QRect>
#include <QLabel>
#include <QPushButton>
#include "mylabel.h"
#include "cusercard.h"

class CUserCard;

class CSiglePic2 : public QWidget
{
	Q_OBJECT

public:
	CSiglePic2(QWidget *parent = 0);
	~CSiglePic2();
	void setPixmap(QPixmap pix);
	void setPixIndex(int i);
	const int getPixIndex(){ return PicIndex; };
	QPixmap* getPix(){ return &curpix; };

	static void SetCurSelectPic(CSiglePic2* pic);
	static CSiglePic2* GetCurSelectPic();
	
	
private:
	MyLabel* piclabel;
	int ismidselected;
	static CSiglePic2* curpic;
	QPixmap curpix;
	int PicIndex;
public slots:
	void clickfunc();
};



#include <QDialog>
class ZCMenuBox : public QDialog
{
	Q_OBJECT

public:
	ZCMenuBox(QString message = "", QWidget *parent = 0, CUserCard* card = 0,int whichop = -1);
	~ZCMenuBox();
	int isOk();
	void setWinMain(CMainWindow* mainw);

private:	
	QPushButton *okButton;
	QPushButton *cancelButton;
	QLabel      *dialogLabel;
	int isokbutton;
	CMainWindow* main_win;
	int op;
signals:
	private slots:
	void closeDialogSlot();
	void okDialogSlot();
};

#endif // CUSERPICPANEL_H
