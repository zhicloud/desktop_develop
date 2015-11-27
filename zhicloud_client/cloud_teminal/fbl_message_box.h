#ifndef FBL_MESSAGE_BOX_H
#define FBL_MESSAGE_BOX_H

#include <QWidget>
#include <QDesktopWidget>
#include <QPalette>
#include <QRect>
#include <QLabel>
#include <QFile>
#include <QDialog>
#include <QPushButton>
#include "mylabel.h"

class fbl_message_box : public QDialog
{
	Q_OBJECT

public:
	fbl_message_box(QString message = "", QWidget *parent = 0);
	~fbl_message_box();

public:
	bool acceptBox();
private:
	QPushButton *okButton;
	QPushButton *cancelButton;
	QLabel      *dialogLabel;
	bool isAccept;

private slots :
	void closeDialogSlot();
	void okBtnClickSlot();
};

#endif // FBL_MESSAGE_BOX_H