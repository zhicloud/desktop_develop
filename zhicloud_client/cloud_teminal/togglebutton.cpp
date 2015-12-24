#include "togglebutton.h"
QString g_zcSysFontFamily = QStringLiteral("fontname");
#define TRUE 1
#define FALSE 0
ToggleButton::ToggleButton(int type, int wid, int hei, QString str, QWidget *parent)
	: QPushButton(parent)
{
	btnType = type;
	btnStr = str;
	width = wid;
	height = hei;
	isChecked = false;
	strStartPoint.setX(0);
	strStartPoint.setY(0);
}

ToggleButton::~ToggleButton()
{

}

void ToggleButton::setBtnChecked()
{
	isChecked = TRUE;
	this->update();
}

void ToggleButton::setBtnUnchecked()
{
	isChecked = FALSE;
	this->update();
}

bool ToggleButton::isBtnChecked()
{
	return isChecked;
}

void ToggleButton::paintEvent ( QPaintEvent * )
{
	QString fontFamily = g_zcSysFontFamily;
	int fontSize = 12;
	if (btnType == SaveUserNameType)
	{
		if (isChecked)
		{
			QPainter painter(this);  
			painter.setRenderHint(QPainter::Antialiasing, true);  
			painter.drawPixmap(0, 0, width, height, QPixmap(":/first_login_res/save_user_check"));
			if (btnStr != "" && btnStr != NULL)
			{
				QStaticText *text = new QStaticText(btnStr);
				text->setTextFormat(Qt::AutoText);
				text->setTextWidth(width);
				QFont font;
				font.setFamily(fontFamily);
				font.setPointSize(fontSize);
				painter.setFont(font);
				painter.setPen(QColor(98, 98, 98));
				adjustStartPoint(width, height, fontSize, btnStr.size());
				painter.drawStaticText(QPoint(strStartPoint.x(), strStartPoint.y()), *text);
			}
		}
		else
		{
			QPainter painter(this);  
			painter.setRenderHint(QPainter::Antialiasing, true);  
			painter.drawPixmap(0, 0, width, height, QPixmap(":/first_login_res/save_user_uncheck"));
			if (btnStr != "" && btnStr != NULL)
			{
				QStaticText *text = new QStaticText(btnStr);
				text->setTextFormat(Qt::AutoText);
				text->setTextWidth(width);
				QFont font;
				font.setFamily(fontFamily);
				font.setPointSize(fontSize);
				painter.setFont(font);
				painter.setPen(QColor(98, 98, 98));
				adjustStartPoint(width, height, fontSize, btnStr.size());
				painter.drawStaticText(QPoint(strStartPoint.x(), strStartPoint.y()), *text);
			}
		}
	}
}

void ToggleButton::switchCheck()
{
	isChecked = !isChecked;
}

void ToggleButton::adjustStartPoint(int width, int height, int fontSize, int fontNum)
{
	strStartPoint.setX((width - fontSize * fontNum) / 2 + 2);
	strStartPoint.setY((height - fontSize) / 2 - 5); 
}
