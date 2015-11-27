#ifndef CUSERCARD_H
#define CUSERCARD_H

#include <QWidget>
#include "clabel.h"
#include <QPushButton>
#include "mylabel.h"
#include <QString>
#include "cuserpicpanel.h"

class CMainWindow;

class CUserCard : public QWidget
{
	Q_OBJECT

public:
	explicit CUserCard(int noclickstate = 0, QWidget *parent = 0, int noedit = 0, int nodel = 0,int hoverstate = 0,int borderstate = 0,int isaddusercard = 0,int islogin = 0);
	~CUserCard();
	void SetMainW(CMainWindow* main_w);
	void setPixmap(QString pix,int height = 70,int width = 70);
	void setText(QString str);
	QString GetPixPath();
	QString GetUsername(){ return struname;}
	void SetUname(QString name){ struname = name; setText(struname); }
	int isshow;
public slots:
	void clspressfunc();
	void clsreleasefunc();
	void editpressfunc();
	void editreleasefunc();
	void midclickfunc();

	void enterEvent(QEvent *);
	void leaveEvent(QEvent *);

public:
	MyLabel* midpic;
	CMainWindow* main_win;
private:
	QString struname;
	QPushButton* closebtn;
	QPushButton* editbtn;
	QLabel*		username;
	int ismidselected;
	int noclickstate;
	QWidget* parentwidget;
	CUserPicPanel* panel;
	
	int borderstate;
	QString strpixpath;

	int isaddusercard;
	int islogincard;
	

};

#endif // CUSERCARD_H
