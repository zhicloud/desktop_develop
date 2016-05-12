#ifndef __LOGWIDGET__
#define __LOGWIDGET__

#include <QWidget>
#include <QTextEidt>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

class LogWidget : public QWidget
{
	Q_OBJECT
public:
	LogWidget(QWidget *parent = 0);
	~LogWidget();
private:
	QTextEdit *m_textEdit;
	QLineEdit *m_lineEdit;
	QPushButton *m_lastPageBtn;
	QPushButton *m_nextPageBtn;
	QPushButton *m_homePageBtn;	
	QPushButton *m_endPageBtn;
};
#endif
