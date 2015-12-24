#ifndef CUSERPICPANEL_H
#define CUSERPICPANEL_H

#include <QWidget>
#include "mylabel.h"

class CUserCard;

class CSiglePic : public QWidget
{
	Q_OBJECT

public:
	CSiglePic(QWidget *parent = 0);
	~CSiglePic();
	void setPixmap(QPixmap pix);
	void setPixIndex(int i);
	const int getPixIndex(){ return PicIndex; };
	QPixmap* getPix(){ return &curpix; };

	static void SetCurSelectPic(CSiglePic* pic);
	static CSiglePic* GetCurSelectPic();
	
	
private:
	MyLabel* piclabel;
	int ismidselected;
	static CSiglePic* curpic;
	QPixmap curpix;
	int PicIndex;
public slots:
	void clickfunc();
};




class CUserPicPanel : public QWidget
{
	Q_OBJECT

public:
	CUserPicPanel(QWidget *parent = 0,CUserCard* card = 0);
	~CUserPicPanel();

private:	
};

#endif // CUSERPICPANEL_H
