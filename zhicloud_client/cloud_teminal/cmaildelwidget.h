#ifndef CMAILDELWIDGET_H
#define CMAILDELWIDGET_H

#include <QWidget>
class CMailBoxWidget;
class CMailDelWidget : public QWidget
{
	Q_OBJECT

public:
	CMailDelWidget(QWidget *parent);
	~CMailDelWidget();
public slots:
	void commitfunc();
	void cancelfunc();
private:
	CMailBoxWidget* par;
};

#endif // CMAILDELWIDGET_H
