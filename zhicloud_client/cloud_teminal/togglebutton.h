#ifndef TOGGLEBUTTON_H
#define TOGGLEBUTTON_H

#include <QPushButton>
#include <QPainter>
#include <QtCore/qmath.h>
#include <QStaticText>
#include <QString>
#include <QPoint>
#include <QLineEdit>
#include <QIntValidator>
//#include <qt_windows.h>
#include <QSettings>

enum BtnType
{
	SaveUserNameType
};

class ToggleButton : public QPushButton
{
	Q_OBJECT

public:
	explicit ToggleButton(int type, int wid, int hei, QString str="", QWidget *parent=0);
	~ToggleButton();

public:
	void switchCheck();
	void setBtnChecked();
	void setBtnUnchecked();
	bool isBtnChecked();
	void adjustStartPoint(int width, int height, int fontSize, int fontNum);

protected:
	void paintEvent ( QPaintEvent * );

private:
	bool			isChecked;
	int				btnType;
	QString			btnStr;
	QPoint			strStartPoint;
	int				width;
	int				height;
};

#endif // TOGGLEBUTTON_H
