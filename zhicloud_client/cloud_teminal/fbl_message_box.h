#ifndef FBL_MESSAGE_BOX_H
#define FBL_MESSAGE_BOX_H

#include <QWidget>
#include <QDesktopWidget>
#include <QPalette>
#include <QRect>
#include <QLabel>
#include <QFile>
#include <QDialog>
#include <QDebug>
#include <QPushButton>
#include <QTimer>
#include "mylabel.h"

enum FBLMSGFONTCOLOR
{
	FBLMSGFONTRED = 0,
	FBLMSGFONTGREEN,
	FBLMSGFONTWHITE,
	FBLMSGFONTGRAY,
	FBLMSGFONTBLACK
};

class fbl_message_box : public QDialog
{
	Q_OBJECT

public:
	fbl_message_box(QString message = "", QWidget *parent = 0);
	~fbl_message_box();
	void setFontColor(FBLMSGFONTCOLOR color);
	void setText(QString text);
	void setCmd(QString str);
   void setCmd(int nCurSelIndex);
public:
	bool acceptBox();
private:
	QPushButton *okButton;
	QPushButton *cancelButton;
	QLabel      *dialogLabel;
	bool isAccept;
	QString m_strCmd;
	QTimer *m_timer;
   int m_nSelIndex;

private slots :
	void closeDialogSlot();
	void okBtnClickSlot();
	void rebootSlot();
};

#endif // FBL_MESSAGE_BOX_H
